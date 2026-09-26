#include <IQCore/igQtRemoteCatalogClient.h>

#include <DataTransfer/iGamePackageProtocol.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QSet>
#include <QThread>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using NativeCatalogSocket = SOCKET;
static constexpr NativeCatalogSocket InvalidCatalogSocket = INVALID_SOCKET;
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using NativeCatalogSocket = int;
static constexpr NativeCatalogSocket InvalidCatalogSocket = -1;
#endif

namespace
{
constexpr int SendSocketTimeoutMilliseconds = 30000;
constexpr int ReceiveSocketTimeoutMilliseconds = 30 * 60 * 1000;
constexpr int MaximumCatalogEntries = 100000;
constexpr int MaximumCatalogPages =
        (MaximumCatalogEntries + igpk::kMaxCatalogPageSize - 1) /
        igpk::kMaxCatalogPageSize;

void CloseSocket(NativeCatalogSocket socket)
{
    if (socket == InvalidCatalogSocket) { return; }
#if defined(_WIN32) || defined(_WIN64)
    closesocket(socket);
#else
    close(socket);
#endif
}

void ShutdownSocket(NativeCatalogSocket socket)
{
    if (socket == InvalidCatalogSocket) { return; }
#if defined(_WIN32) || defined(_WIN64)
    shutdown(socket, SD_BOTH);
#else
    shutdown(socket, SHUT_RDWR);
#endif
}

QString LastSocketError(const QString& operation)
{
#if defined(_WIN32) || defined(_WIN64)
    return QStringLiteral("%1 failed (WinSock error %2)").arg(operation).arg(WSAGetLastError());
#else
    return QStringLiteral("%1 failed: %2")
            .arg(operation, QString::fromLocal8Bit(std::strerror(errno)));
#endif
}

bool HasValidRequiredFields(const igpk::CatalogEntry& entry)
{
    return !entry.packageId.empty() && !entry.displayName.empty() &&
           !entry.fileName.empty() && !entry.versionToken.empty() &&
           entry.sha256.size() == 32u;
}
} // namespace

struct igQtRemoteCatalogCancellationState
{
    std::atomic_bool cancelled{false};
    std::mutex socketMutex;
    NativeCatalogSocket activeSocket{InvalidCatalogSocket};

    void RequestCancel()
    {
        cancelled.store(true, std::memory_order_release);
        std::lock_guard<std::mutex> lock(socketMutex);
        ShutdownSocket(activeSocket);
    }

    void SetSocket(NativeCatalogSocket socket)
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        activeSocket = socket;
        if (cancelled.load(std::memory_order_acquire)) { ShutdownSocket(activeSocket); }
    }

    void ClearSocket(NativeCatalogSocket socket)
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        if (activeSocket == socket) { activeSocket = InvalidCatalogSocket; }
    }
};

namespace
{
class CatalogSocketSession final
{
public:
    explicit CatalogSocketSession(
            std::shared_ptr<igQtRemoteCatalogCancellationState> cancelState)
        : m_CancelState(std::move(cancelState))
    {
    }

    ~CatalogSocketSession() { Close(); }

    bool Connect(const QString& address, quint16 port, QString& errorMessage)
    {
#if defined(_WIN32) || defined(_WIN64)
        WSADATA data{};
        const int startupResult = WSAStartup(MAKEWORD(2, 2), &data);
        if (startupResult != 0) {
            errorMessage = QStringLiteral("WSAStartup failed (WinSock error %1)")
                                   .arg(startupResult);
            return false;
        }
        m_WinsockStarted = true;
#endif

        const QByteArray host = address.toUtf8();
        const QByteArray service = QByteArray::number(port);
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        addrinfo* addresses = nullptr;
        const int lookupResult = getaddrinfo(
                host.constData(), service.constData(), &hints, &addresses);
        if (lookupResult != 0) {
            errorMessage = QStringLiteral("Cannot resolve server host '%1' (error %2)")
                                   .arg(address)
                                   .arg(lookupResult);
            return false;
        }

        int lastConnectError = 0;
        for (const addrinfo* candidate = addresses;
             candidate != nullptr && !IsCancelled(); candidate = candidate->ai_next) {
            NativeCatalogSocket socketHandle = socket(candidate->ai_family,
                                                       candidate->ai_socktype,
                                                       candidate->ai_protocol);
            if (socketHandle == InvalidCatalogSocket) { continue; }
            ConfigureTimeouts(socketHandle);
            m_CancelState->SetSocket(socketHandle);
#if defined(_WIN32) || defined(_WIN64)
            const int connectResult = connect(socketHandle,
                                              candidate->ai_addr,
                                              static_cast<int>(candidate->ai_addrlen));
            if (connectResult == 0) {
                m_Socket = socketHandle;
                break;
            }
            lastConnectError = WSAGetLastError();
#else
            const int connectResult = connect(socketHandle,
                                              candidate->ai_addr,
                                              candidate->ai_addrlen);
            if (connectResult == 0) {
                m_Socket = socketHandle;
                break;
            }
            lastConnectError = errno;
#endif
            m_CancelState->ClearSocket(socketHandle);
            CloseSocket(socketHandle);
        }
        freeaddrinfo(addresses);

        if (m_Socket == InvalidCatalogSocket) {
            if (IsCancelled()) {
                errorMessage = QStringLiteral("Catalog fetch cancelled");
            } else {
                errorMessage = QStringLiteral("Cannot connect to %1:%2 (socket error %3)")
                                       .arg(address)
                                       .arg(port)
                                       .arg(lastConnectError);
            }
            return false;
        }
        return true;
    }

    void Close()
    {
        if (m_Socket != InvalidCatalogSocket) {
            m_CancelState->ClearSocket(m_Socket);
            ShutdownSocket(m_Socket);
            CloseSocket(m_Socket);
            m_Socket = InvalidCatalogSocket;
        }
#if defined(_WIN32) || defined(_WIN64)
        if (m_WinsockStarted) {
            WSACleanup();
            m_WinsockStarted = false;
        }
#endif
    }

    bool SendFrame(igpk::MessageType type,
                   std::uint64_t requestId,
                   const std::vector<std::uint8_t>& payload,
                   QString& errorMessage)
    {
        try {
            igpk::FrameHeader frame;
            frame.type = type;
            frame.requestId = requestId;
            frame.payloadSize = payload.size();
            const auto header = igpk::encodeHeader(frame);
            return SendAll(header.data(), header.size(), errorMessage) &&
                   SendAll(payload.data(), payload.size(), errorMessage);
        } catch (const std::exception& error) {
            errorMessage = QStringLiteral("Cannot encode protocol frame: %1")
                                   .arg(QString::fromUtf8(error.what()));
            return false;
        }
    }

    bool ReceiveFrame(igpk::FrameHeader& frame,
                      std::vector<std::uint8_t>& payload,
                      QString& errorMessage)
    {
        std::array<std::uint8_t, igpk::kHeaderSize> bytes{};
        if (!ReceiveAll(bytes.data(), bytes.size(), errorMessage)) { return false; }
        std::string decodeError;
        if (!igpk::decodeHeader(bytes, frame, decodeError)) {
            errorMessage = QStringLiteral("Invalid catalog response header: %1")
                                   .arg(QString::fromStdString(decodeError));
            return false;
        }
        if (frame.version != igpk::kProtocolVersion) {
            errorMessage = QStringLiteral("Unsupported package protocol version %1")
                                   .arg(frame.version);
            return false;
        }
        payload.resize(static_cast<std::size_t>(frame.payloadSize));
        return ReceiveAll(payload.data(), payload.size(), errorMessage);
    }

    void SendGoodbye()
    {
        if (m_Socket == InvalidCatalogSocket || IsCancelled()) { return; }
        QString ignored;
        SendFrame(igpk::MessageType::Goodbye,
                  std::numeric_limits<std::uint64_t>::max(), {}, ignored);
    }

private:
    static void ConfigureTimeouts(NativeCatalogSocket socketHandle)
    {
#if defined(_WIN32) || defined(_WIN64)
        DWORD receiveTimeout = ReceiveSocketTimeoutMilliseconds;
        DWORD sendTimeout = SendSocketTimeoutMilliseconds;
        setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&receiveTimeout), sizeof(receiveTimeout));
        setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&sendTimeout), sizeof(sendTimeout));
#else
        timeval receiveTimeout{};
        receiveTimeout.tv_sec = ReceiveSocketTimeoutMilliseconds / 1000;
        receiveTimeout.tv_usec = (ReceiveSocketTimeoutMilliseconds % 1000) * 1000;
        timeval sendTimeout{};
        sendTimeout.tv_sec = SendSocketTimeoutMilliseconds / 1000;
        sendTimeout.tv_usec = (SendSocketTimeoutMilliseconds % 1000) * 1000;
        setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
                   &receiveTimeout, sizeof(receiveTimeout));
        setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO,
                   &sendTimeout, sizeof(sendTimeout));
#endif
    }

    bool SendAll(const std::uint8_t* data, std::size_t size, QString& errorMessage)
    {
        std::size_t sent = 0;
        while (sent < size) {
            if (IsCancelled()) {
                errorMessage = QStringLiteral("Catalog fetch cancelled");
                return false;
            }
            const int count = static_cast<int>(std::min<std::size_t>(
                    size - sent, static_cast<std::size_t>(std::numeric_limits<int>::max())));
#if defined(_WIN32) || defined(_WIN64)
            const int result = send(m_Socket,
                                    reinterpret_cast<const char*>(data + sent), count, 0);
            if (result == SOCKET_ERROR) {
                const int code = WSAGetLastError();
                if (code == WSAEINTR) { continue; }
                errorMessage = QStringLiteral("send failed (WinSock error %1)").arg(code);
                return false;
            }
#else
#if defined(MSG_NOSIGNAL)
            constexpr int sendFlags = MSG_NOSIGNAL;
#else
            constexpr int sendFlags = 0;
#endif
            const int result = static_cast<int>(send(
                    m_Socket, data + sent, static_cast<std::size_t>(count), sendFlags));
            if (result < 0) {
                if (errno == EINTR) { continue; }
                errorMessage = LastSocketError(QStringLiteral("send"));
                return false;
            }
#endif
            if (result == 0) {
                errorMessage = QStringLiteral("Server closed the connection while sending");
                return false;
            }
            sent += static_cast<std::size_t>(result);
        }
        return true;
    }

    bool ReceiveAll(std::uint8_t* data, std::size_t size, QString& errorMessage)
    {
        std::size_t received = 0;
        while (received < size) {
            if (IsCancelled()) {
                errorMessage = QStringLiteral("Catalog fetch cancelled");
                return false;
            }
            const int count = static_cast<int>(std::min<std::size_t>(
                    size - received,
                    static_cast<std::size_t>(std::numeric_limits<int>::max())));
#if defined(_WIN32) || defined(_WIN64)
            const int result = recv(m_Socket,
                                    reinterpret_cast<char*>(data + received), count, 0);
            if (result == SOCKET_ERROR) {
                const int code = WSAGetLastError();
                if (code == WSAEINTR) { continue; }
                errorMessage = QStringLiteral("recv failed (WinSock error %1)").arg(code);
                return false;
            }
#else
            const int result = static_cast<int>(recv(
                    m_Socket, data + received, static_cast<std::size_t>(count), 0));
            if (result < 0) {
                if (errno == EINTR) { continue; }
                errorMessage = LastSocketError(QStringLiteral("recv"));
                return false;
            }
#endif
            if (result == 0) {
                errorMessage = QStringLiteral("Server closed the connection while receiving");
                return false;
            }
            received += static_cast<std::size_t>(result);
        }
        return true;
    }

    bool IsCancelled() const
    {
        return m_CancelState->cancelled.load(std::memory_order_acquire);
    }

    std::shared_ptr<igQtRemoteCatalogCancellationState> m_CancelState;
    NativeCatalogSocket m_Socket{InvalidCatalogSocket};
#if defined(_WIN32) || defined(_WIN64)
    bool m_WinsockStarted{false};
#endif
};

class RemoteCatalogWorker final : public QObject
{
    Q_OBJECT

public:
    RemoteCatalogWorker(QString address,
                        quint16 port,
                        std::shared_ptr<igQtRemoteCatalogCancellationState> cancelState)
        : m_Address(std::move(address))
        , m_Port(port)
        , m_CancelState(std::move(cancelState))
    {
    }

public slots:
    void Run()
    {
        CatalogSocketSession session(m_CancelState);
        QString failure;
        emit Status(QStringLiteral("Connecting to %1:%2").arg(m_Address).arg(m_Port));
        if (!session.Connect(m_Address, m_Port, failure)) {
            FinishFailure(session, failure);
            return;
        }

        QVector<igQtRemoteCatalogEntry> entries;
        QSet<QString> packageIds;
        QSet<quint32> visitedCursors;
        std::uint64_t revision = 0;
        std::uint32_t cursor = 0;
        std::uint64_t requestId = 1;
        int pageNumber = 1;

        for (;;) {
            if (IsCancelled()) {
                FinishFailure(session, QStringLiteral("Catalog fetch cancelled"));
                return;
            }
            if (visitedCursors.contains(cursor)) {
                FinishFailure(session, QStringLiteral("Server returned a repeating catalog cursor"));
                return;
            }
            if (pageNumber > MaximumCatalogPages) {
                FinishFailure(session,
                              QStringLiteral("Server catalog has too many pages"));
                return;
            }
            visitedCursors.insert(cursor);
            emit Status(QStringLiteral("Fetching catalog page %1").arg(pageNumber));

            igpk::CatalogRequest request;
            request.revision = revision;
            request.cursor = cursor;
            request.pageSize = igpk::kMaxCatalogPageSize;
            request.flags = pageNumber == 1 ? igpk::kCatalogFlagForceRefresh : 0u;

            std::vector<std::uint8_t> requestPayload;
            try {
                requestPayload = igpk::encodeCatalogRequest(request);
            } catch (const std::exception& error) {
                FinishFailure(session,
                              QStringLiteral("Cannot encode catalog request: %1")
                                      .arg(QString::fromUtf8(error.what())));
                return;
            }
            if (!session.SendFrame(igpk::MessageType::CatalogRequest,
                                   requestId, requestPayload, failure)) {
                FinishFailure(session, failure);
                return;
            }
            if (pageNumber == 1) {
                emit Status(QStringLiteral(
                        "Server is scanning and verifying packages; this can take several minutes..."));
            }

            igpk::FrameHeader responseHeader;
            std::vector<std::uint8_t> responsePayload;
            if (!session.ReceiveFrame(responseHeader, responsePayload, failure)) {
                FinishFailure(session, failure);
                return;
            }
            if (responseHeader.requestId != requestId) {
                FinishFailure(session,
                              QStringLiteral("CATALOG response request id does not match"));
                return;
            }
            if (responseHeader.type == igpk::MessageType::Error) {
                igpk::ErrorResponse errorResponse;
                std::string decodeError;
                if (!igpk::decodeErrorResponse(responsePayload,
                                               errorResponse, decodeError)) {
                    FinishFailure(session,
                                  QStringLiteral("Invalid server error response: %1")
                                          .arg(QString::fromStdString(decodeError)));
                } else {
                    FinishFailure(
                            session,
                            QStringLiteral("Server rejected catalog request (%1): %2")
                                    .arg(static_cast<std::uint32_t>(errorResponse.code))
                                    .arg(QString::fromUtf8(errorResponse.message.data(),
                                                          static_cast<int>(errorResponse.message.size()))));
                }
                return;
            }
            if (responseHeader.type != igpk::MessageType::CatalogResponse) {
                FinishFailure(
                        session,
                        QStringLiteral("Expected CATALOG_RESPONSE, received message type %1")
                                .arg(static_cast<std::uint16_t>(responseHeader.type)));
                return;
            }

            igpk::CatalogResponse response;
            std::string decodeError;
            if (!igpk::decodeCatalogResponse(responsePayload, response, decodeError)) {
                FinishFailure(session,
                              QStringLiteral("Invalid catalog response: %1")
                                      .arg(QString::fromStdString(decodeError)));
                return;
            }
            if (response.entries.size() > request.pageSize) {
                FinishFailure(session,
                              QStringLiteral("Catalog page exceeds the requested page size"));
                return;
            }
            if (pageNumber == 1) {
                revision = response.revision;
            } else if (response.revision != revision) {
                FinishFailure(session,
                              QStringLiteral("Server catalog changed while it was being fetched"));
                return;
            }
            if (entries.size() > MaximumCatalogEntries -
                                 static_cast<int>(response.entries.size())) {
                FinishFailure(session,
                              QStringLiteral("Server catalog exceeds the %1-entry client limit")
                                      .arg(MaximumCatalogEntries));
                return;
            }
            if (response.entries.empty() &&
                response.nextCursor != igpk::kCatalogEndCursor) {
                FinishFailure(session,
                              QStringLiteral("Server returned an empty non-final catalog page"));
                return;
            }

            for (const auto& entry : response.entries) {
                if (!HasValidRequiredFields(entry)) {
                    FinishFailure(session,
                                  QStringLiteral("Catalog contains an entry with invalid metadata"));
                    return;
                }
                const QString packageId = QString::fromUtf8(
                        entry.packageId.data(), static_cast<int>(entry.packageId.size()));
                if (packageIds.contains(packageId)) {
                    FinishFailure(session,
                                  QStringLiteral("Catalog contains duplicate package id '%1'")
                                          .arg(packageId));
                    return;
                }
                packageIds.insert(packageId);

                igQtRemoteCatalogEntry converted;
                converted.packageId = packageId;
                converted.displayName = QString::fromUtf8(
                        entry.displayName.data(), static_cast<int>(entry.displayName.size()));
                converted.fileName = QString::fromUtf8(
                        entry.fileName.data(), static_cast<int>(entry.fileName.size()));
                converted.versionToken = QString::fromUtf8(
                        entry.versionToken.data(), static_cast<int>(entry.versionToken.size()));
                converted.fileSize = entry.fileSize;
                converted.mtimeTicks = entry.mtimeTicks;
                entries.push_back(std::move(converted));
            }

            if (response.nextCursor == igpk::kCatalogEndCursor) { break; }
            cursor = response.nextCursor;
            ++pageNumber;
            ++requestId;
        }

        // The data server is intentionally single-client.  Finish the catalog
        // protocol and close this socket before making the result actionable;
        // the selected package can then use OpenRemotePackage on a new socket.
        session.SendGoodbye();
        session.Close();
        if (IsCancelled()) {
            emit Cancelled();
        } else {
            emit Catalog(entries);
        }
        emit Done();
    }

signals:
    void Status(const QString& message);
    void Catalog(const QVector<igQtRemoteCatalogEntry>& entries);
    void Failed(const QString& message);
    void Cancelled();
    void Done();

private:
    void FinishFailure(CatalogSocketSession& session, const QString& message)
    {
        const bool cancelled = IsCancelled();
        if (!cancelled) { session.SendGoodbye(); }
        session.Close();
        if (cancelled) {
            emit Cancelled();
        } else {
            emit Failed(message);
        }
        emit Done();
    }

    bool IsCancelled() const
    {
        return m_CancelState->cancelled.load(std::memory_order_acquire);
    }

    QString m_Address;
    quint16 m_Port{0};
    std::shared_ptr<igQtRemoteCatalogCancellationState> m_CancelState;
};
} // namespace

QString igQtRemotePackageCacheDirectory(const QString& cacheRoot,
                                        const QString& host,
                                        quint16 port,
                                        const QString& packageId)
{
    const QString cleanRoot = cacheRoot.trimmed();
    const QString cleanHost = host.trimmed().toCaseFolded();
    // Package ids are opaque server values.  Do not trim a valid filename:
    // the exact id is also sent by INFO/GET, and different ids must never
    // share a cache namespace.
    const QString exactPackageId = packageId;
    if (cleanRoot.isEmpty() || cleanHost.isEmpty() || port == 0 ||
        exactPackageId.trimmed().isEmpty()) {
        return {};
    }
    const QByteArray namespaceKey = cleanHost.toUtf8() + ':' + QByteArray::number(port) + '\n' +
                                    exactPackageId.toUtf8();
    const QString digest = QString::fromLatin1(
            QCryptographicHash::hash(namespaceKey, QCryptographicHash::Sha256).toHex());
    return QDir(QDir::cleanPath(cleanRoot))
            .filePath(QStringLiteral("packages/%1").arg(digest));
}

igQtRemoteCatalogClient::igQtRemoteCatalogClient(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<QVector<igQtRemoteCatalogEntry>>(
            "QVector<igQtRemoteCatalogEntry>");
}

igQtRemoteCatalogClient::~igQtRemoteCatalogClient()
{
    Cancel();
    if (m_Thread != nullptr) {
        if (m_Thread->isRunning()) { m_Thread->wait(); }
        delete m_Thread;
        m_Thread = nullptr;
    }
}

bool igQtRemoteCatalogClient::Fetch(const QString& serverAddress, quint16 serverPort)
{
    const QString address = serverAddress.trimmed();
    if (m_Running || address.isEmpty() || serverPort == 0) { return false; }

    if (m_Thread != nullptr) {
        if (m_Thread->isRunning()) { return false; }
        delete m_Thread;
        m_Thread = nullptr;
    }

    m_CancelState = std::make_shared<igQtRemoteCatalogCancellationState>();
    m_Thread = new QThread(this);
    m_Thread->setObjectName(QStringLiteral("iGameVis remote catalog fetch"));
    auto* worker = new RemoteCatalogWorker(
            address, serverPort, m_CancelState);
    worker->moveToThread(m_Thread);

    connect(m_Thread, &QThread::started, worker, &RemoteCatalogWorker::Run);
    connect(worker, &RemoteCatalogWorker::Status,
            this, &igQtRemoteCatalogClient::StatusChanged);
    connect(worker, &RemoteCatalogWorker::Catalog,
            this, &igQtRemoteCatalogClient::CatalogReady);
    connect(worker, &RemoteCatalogWorker::Failed,
            this, &igQtRemoteCatalogClient::Failed);
    connect(worker, &RemoteCatalogWorker::Cancelled,
            this, &igQtRemoteCatalogClient::Cancelled);
    // Let the worker event loop process DeferredDelete, then stop the thread
    // from QObject::destroyed.  This makes repeated Fetch calls release both
    // objects instead of queuing deleteLater after the event loop has stopped.
    connect(worker, &RemoteCatalogWorker::Done,
            worker, &QObject::deleteLater);
    connect(worker, &QObject::destroyed,
            m_Thread, &QThread::quit, Qt::DirectConnection);
    connect(m_Thread, &QThread::finished,
            this, &igQtRemoteCatalogClient::OnThreadFinished);

    m_Running = true;
    emit RunningChanged(true);
    m_Thread->start();
    return true;
}

void igQtRemoteCatalogClient::Cancel()
{
    if (m_CancelState) { m_CancelState->RequestCancel(); }
}

void igQtRemoteCatalogClient::OnThreadFinished()
{
    QThread* const finishedThread = qobject_cast<QThread*>(sender());
    if (finishedThread != nullptr && m_Thread == finishedThread) {
        m_Thread = nullptr;
        finishedThread->deleteLater();
    }
    m_CancelState.reset();
    if (m_Running) {
        m_Running = false;
        emit RunningChanged(false);
    }
}

#include "igQtRemoteCatalogClient.moc"
