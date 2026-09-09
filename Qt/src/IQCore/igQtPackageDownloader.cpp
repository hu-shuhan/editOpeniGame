#include <IQCore/igQtPackageDownloader.h>

#include <DataTransfer/iGamePackageProtocol.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
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
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")
#endif
using NativeSocket = SOCKET;
static constexpr NativeSocket InvalidSocket = INVALID_SOCKET;
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using NativeSocket = int;
static constexpr NativeSocket InvalidSocket = -1;
#endif

namespace
{
constexpr int SocketTimeoutMilliseconds = 30000;
constexpr int MaximumVersionRestarts = 3;
constexpr int MaximumTransportRetries = 5;
constexpr unsigned long TransportRetryDelayMilliseconds = 500;

enum class DownloadAttemptResult
{
    Complete,
    StaleVersion,
    RetryableTransportFailure,
    Cancelled,
    Failed
};

struct PackageInfo
{
    std::uint64_t fileSize{0};
    std::uint64_t mtimeTicks{0};
    std::uint32_t maximumFrameSize{0};
    std::uint32_t maximumChunkSize{0};
    QString packageId;
    QString fileName;
    QString versionToken;
    QByteArray sha256;
};

struct CacheMetadata
{
    QString packageId;
    QString fileName;
    QString versionToken;
    std::uint64_t fileSize{0};
    std::uint64_t mtimeTicks{0};
    QByteArray sha256;
    bool verified{false};
};

void CloseSocket(NativeSocket socket)
{
    if (socket == InvalidSocket) { return; }
#if defined(_WIN32) || defined(_WIN64)
    closesocket(socket);
#else
    close(socket);
#endif
}

void ShutdownSocket(NativeSocket socket)
{
    if (socket == InvalidSocket) { return; }
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
    return QStringLiteral("%1 failed: %2").arg(operation, QString::fromLocal8Bit(std::strerror(errno)));
#endif
}

bool IsSafeFileName(const QString& fileName)
{
    if (fileName.isEmpty() || fileName == QStringLiteral(".") || fileName == QStringLiteral("..")) {
        return false;
    }
    if (fileName.contains(QLatin1Char('/')) || fileName.contains(QLatin1Char('\\')) ||
        fileName.contains(QLatin1Char(':'))) {
        return false;
    }
    return QFileInfo(fileName).fileName() == fileName;
}

bool MetadataMatches(const CacheMetadata& metadata, const PackageInfo& info)
{
    return metadata.packageId == info.packageId && metadata.fileName == info.fileName &&
           metadata.versionToken == info.versionToken && metadata.fileSize == info.fileSize &&
           metadata.mtimeTicks == info.mtimeTicks && metadata.sha256 == info.sha256;
}

bool ReadMetadata(const QString& path, CacheMetadata& metadata)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { return false; }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) { return false; }
    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("schema")).toInt() != 1) { return false; }

    bool sizeOk = false;
    bool ticksOk = false;
    const auto fileSize = object.value(QStringLiteral("fileSize")).toString().toULongLong(&sizeOk);
    const auto mtimeTicks = object.value(QStringLiteral("mtimeTicks")).toString().toULongLong(&ticksOk);
    if (!sizeOk || !ticksOk) { return false; }

    metadata.packageId = object.value(QStringLiteral("packageId")).toString();
    metadata.fileName = object.value(QStringLiteral("fileName")).toString();
    metadata.versionToken = object.value(QStringLiteral("versionToken")).toString();
    metadata.fileSize = fileSize;
    metadata.mtimeTicks = mtimeTicks;
    metadata.sha256 = QByteArray::fromHex(object.value(QStringLiteral("sha256")).toString().toLatin1());
    metadata.verified = object.value(QStringLiteral("verified")).toBool(false);
    return !metadata.packageId.isEmpty() && !metadata.fileName.isEmpty() &&
           !metadata.versionToken.isEmpty();
}

bool WriteMetadata(const QString& path,
                   const PackageInfo& info,
                   bool verified,
                   QString& errorMessage)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema"), 1);
    object.insert(QStringLiteral("packageId"), info.packageId);
    object.insert(QStringLiteral("fileName"), info.fileName);
    object.insert(QStringLiteral("versionToken"), info.versionToken);
    object.insert(QStringLiteral("fileSize"), QString::number(info.fileSize));
    object.insert(QStringLiteral("mtimeTicks"), QString::number(info.mtimeTicks));
    object.insert(QStringLiteral("sha256"), QString::fromLatin1(info.sha256.toHex()));
    object.insert(QStringLiteral("verified"), verified);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        errorMessage = QStringLiteral("Cannot write metadata %1: %2").arg(path, file.errorString());
        return false;
    }
    const QByteArray json = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (file.write(json) != json.size()) {
        errorMessage = QStringLiteral("Cannot write complete metadata %1: %2").arg(path, file.errorString());
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        errorMessage = QStringLiteral("Cannot commit metadata %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

bool AtomicReplace(const QString& source, const QString& destination, QString& errorMessage)
{
#if defined(_WIN32) || defined(_WIN64)
    const std::wstring sourcePath = QDir::toNativeSeparators(source).toStdWString();
    const std::wstring destinationPath = QDir::toNativeSeparators(destination).toStdWString();
    if (MoveFileExW(sourcePath.c_str(), destinationPath.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
        errorMessage = QStringLiteral("Cannot replace %1 with %2 (Windows error %3)")
                               .arg(destination, source)
                               .arg(GetLastError());
        return false;
    }
    return true;
#else
    const QByteArray sourceName = QFile::encodeName(source);
    const QByteArray destinationName = QFile::encodeName(destination);
    if (::rename(sourceName.constData(), destinationName.constData()) != 0) {
        errorMessage = QStringLiteral("Cannot replace %1 with %2: %3")
                               .arg(destination, source, QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }
    return true;
#endif
}
} // namespace

struct igQtPackageDownloadCancellationState
{
    std::atomic_bool cancelled{false};
    std::mutex socketMutex;
    NativeSocket activeSocket{InvalidSocket};

    void RequestCancel()
    {
        cancelled.store(true, std::memory_order_release);
        std::lock_guard<std::mutex> lock(socketMutex);
        ShutdownSocket(activeSocket);
    }

    void SetSocket(NativeSocket socket)
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        activeSocket = socket;
        if (cancelled.load(std::memory_order_acquire)) { ShutdownSocket(activeSocket); }
    }

    void ClearSocket(NativeSocket socket)
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        if (activeSocket == socket) { activeSocket = InvalidSocket; }
    }
};

namespace
{
class SocketSession
{
public:
    explicit SocketSession(std::shared_ptr<igQtPackageDownloadCancellationState> cancelState)
        : m_CancelState(std::move(cancelState))
    {
    }

    ~SocketSession() { Close(); }

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
            NativeSocket socketHandle = socket(candidate->ai_family,
                                               candidate->ai_socktype,
                                               candidate->ai_protocol);
            if (socketHandle == InvalidSocket) {
#if defined(_WIN32) || defined(_WIN64)
                lastConnectError = WSAGetLastError();
#else
                lastConnectError = errno;
#endif
                continue;
            }
            ConfigureTimeouts(socketHandle);
            m_CancelState->SetSocket(socketHandle);
#if defined(_WIN32) || defined(_WIN64)
            const int connectResult = connect(
                    socketHandle, candidate->ai_addr,
                    static_cast<int>(candidate->ai_addrlen));
            if (connectResult != 0) { lastConnectError = WSAGetLastError(); }
#else
            const int connectResult = connect(
                    socketHandle, candidate->ai_addr, candidate->ai_addrlen);
            if (connectResult != 0) { lastConnectError = errno; }
#endif
            if (connectResult == 0 && !IsCancelled()) {
                m_Socket = socketHandle;
                break;
            }
            m_CancelState->ClearSocket(socketHandle);
            CloseSocket(socketHandle);
        }
        freeaddrinfo(addresses);

        if (m_Socket == InvalidSocket) {
            if (IsCancelled()) {
                errorMessage = QStringLiteral("Transfer cancelled");
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
        if (m_Socket != InvalidSocket) {
            m_CancelState->ClearSocket(m_Socket);
            CloseSocket(m_Socket);
            m_Socket = InvalidSocket;
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
            return SendAll(reinterpret_cast<const char*>(header.data()),
                           header.size(), errorMessage) &&
                   SendAll(reinterpret_cast<const char*>(payload.data()),
                           payload.size(), errorMessage);
        } catch (const std::exception& error) {
            errorMessage = QStringLiteral("Outgoing protocol payload is invalid: %1")
                                   .arg(QString::fromUtf8(error.what()));
            return false;
        }
    }

    bool ReceiveFrame(igpk::FrameHeader& header,
                      std::vector<std::uint8_t>& payload,
                      QString& errorMessage)
    {
        std::array<std::uint8_t, igpk::kHeaderSize> bytes{};
        if (!ReceiveAll(reinterpret_cast<char*>(bytes.data()), bytes.size(), errorMessage)) {
            return false;
        }
        std::string decodeError;
        if (!igpk::decodeHeader(bytes, header, decodeError) ||
            header.version != igpk::kProtocolVersion) {
            errorMessage = QStringLiteral("Unsupported or malformed package protocol header");
            return false;
        }
        payload.resize(static_cast<std::size_t>(header.payloadSize));
        return ReceiveAll(reinterpret_cast<char*>(payload.data()), payload.size(), errorMessage);
    }

private:
    static void ConfigureTimeouts(NativeSocket socketHandle)
    {
#if defined(_WIN32) || defined(_WIN64)
        DWORD timeout = SocketTimeoutMilliseconds;
        setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
        timeval timeout{};
        timeout.tv_sec = SocketTimeoutMilliseconds / 1000;
        timeout.tv_usec = (SocketTimeoutMilliseconds % 1000) * 1000;
        setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
    }

    bool IsCancelled() const
    {
        return m_CancelState->cancelled.load(std::memory_order_acquire);
    }

    bool SendAll(const char* data, std::size_t size, QString& errorMessage)
    {
        std::size_t sent = 0;
        while (sent < size) {
            if (m_CancelState->cancelled.load(std::memory_order_acquire)) {
                errorMessage = QStringLiteral("Transfer cancelled");
                return false;
            }
            const int count = static_cast<int>(std::min<std::size_t>(size - sent,
                                                                     std::numeric_limits<int>::max()));
#if defined(_WIN32) || defined(_WIN64)
            const int result = send(m_Socket, data + sent, count, 0);
            if (result == SOCKET_ERROR) {
                const int code = WSAGetLastError();
                if (code == WSAEINTR) { continue; }
                errorMessage = QStringLiteral("send failed (WinSock error %1)").arg(code);
                return false;
            }
#else
            const int result = static_cast<int>(send(m_Socket, data + sent, count, 0));
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

    bool ReceiveAll(char* data, std::size_t size, QString& errorMessage)
    {
        std::size_t received = 0;
        while (received < size) {
            if (m_CancelState->cancelled.load(std::memory_order_acquire)) {
                errorMessage = QStringLiteral("Transfer cancelled");
                return false;
            }
            const int count = static_cast<int>(std::min<std::size_t>(size - received,
                                                                     std::numeric_limits<int>::max()));
#if defined(_WIN32) || defined(_WIN64)
            const int result = recv(m_Socket, data + received, count, 0);
            if (result == SOCKET_ERROR) {
                const int code = WSAGetLastError();
                if (code == WSAEINTR) { continue; }
                errorMessage = QStringLiteral("recv failed (WinSock error %1)").arg(code);
                return false;
            }
#else
            const int result = static_cast<int>(recv(m_Socket, data + received, count, 0));
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

    std::shared_ptr<igQtPackageDownloadCancellationState> m_CancelState;
    NativeSocket m_Socket{InvalidSocket};
#if defined(_WIN32) || defined(_WIN64)
    bool m_WinsockStarted{false};
#endif
};

class PackageDownloadWorker final : public QObject
{
    Q_OBJECT

public:
    PackageDownloadWorker(QString address,
                          quint16 port,
                          QString packageId,
                          QString cacheDirectory,
                          std::shared_ptr<igQtPackageDownloadCancellationState> cancelState)
        : m_Address(std::move(address))
        , m_Port(port)
        , m_PackageId(std::move(packageId))
        , m_CacheDirectory(std::move(cacheDirectory))
        , m_CancelState(std::move(cancelState))
    {
    }

public slots:
    void Run()
    {
        QString failure;
        int versionRestarts = 0;
        int transportRetries = 0;
        while (versionRestarts < MaximumVersionRestarts &&
               transportRetries < MaximumTransportRetries) {
            if (m_CancelState->cancelled.load(std::memory_order_acquire)) {
                emit Cancelled();
                emit Done();
                return;
            }

            const DownloadAttemptResult result = DownloadOnce(failure);
            if (result == DownloadAttemptResult::Complete) {
                emit Done();
                return;
            }
            if (result == DownloadAttemptResult::Cancelled) {
                emit Cancelled();
                emit Done();
                return;
            }
            if (result == DownloadAttemptResult::StaleVersion) {
                emit Status(QStringLiteral("Server package changed during transfer; restarting from byte 0"));
                ++versionRestarts;
                transportRetries = 0;
                continue;
            }
            if (result == DownloadAttemptResult::RetryableTransportFailure) {
                ++transportRetries;
                emit Status(QStringLiteral("Connection interrupted (%1/%2); reconnecting and resuming")
                                    .arg(transportRetries)
                                    .arg(MaximumTransportRetries));
                for (unsigned long waited = 0;
                     waited < TransportRetryDelayMilliseconds && !IsCancelled();
                     waited += 50) {
                    QThread::msleep(50);
                }
                continue;
            }

            emit Failed(failure);
            emit Done();
            return;
        }

        if (versionRestarts >= MaximumVersionRestarts) {
            emit Failed(QStringLiteral("Package changed repeatedly during transfer; retry later"));
        } else {
            emit Failed(QStringLiteral("Connection could not be restored after %1 attempts: %2")
                                .arg(MaximumTransportRetries)
                                .arg(failure));
        }
        emit Done();
    }

signals:
    void Status(const QString& message);
    void PackageInfoAvailable(const QString& packageId,
                              const QString& fileName,
                              const QString& versionToken,
                              quint64 fileSize);
    void Started(quint64 resumeOffset, quint64 totalBytes);
    void Progress(quint64 receivedBytes, quint64 totalBytes);
    void Ready(const QString& packagePath);
    void Failed(const QString& message);
    void Cancelled();
    void Done();

private:
    DownloadAttemptResult DownloadOnce(QString& failure)
    {
        SocketSession socket(m_CancelState);
        emit Status(QStringLiteral("Connecting to %1:%2").arg(m_Address).arg(m_Port));
        if (!socket.Connect(m_Address, m_Port, failure)) {
            return IsCancelled() ? DownloadAttemptResult::Cancelled
                                 : DownloadAttemptResult::RetryableTransportFailure;
        }

        PackageInfo info;
        const DownloadAttemptResult infoResult = RequestPackageInfo(socket, info, failure);
        if (infoResult != DownloadAttemptResult::Complete) {
            if (infoResult != DownloadAttemptResult::RetryableTransportFailure &&
                infoResult != DownloadAttemptResult::Cancelled) {
                SendGoodbye(socket);
            }
            return infoResult;
        }
        emit PackageInfoAvailable(info.packageId, info.fileName, info.versionToken, info.fileSize);

        if (info.packageId != m_PackageId) {
            failure = QStringLiteral("Server returned package id '%1' for request '%2'")
                              .arg(info.packageId, m_PackageId);
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }
        if (!IsSafeFileName(info.fileName)) {
            failure = QStringLiteral("Server returned an unsafe package filename: %1").arg(info.fileName);
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }
        if (info.maximumFrameSize > igpk::kMaxFrameSize ||
            info.maximumFrameSize < igpk::kHeaderSize ||
            info.maximumChunkSize == 0 || info.maximumChunkSize > igpk::kMaxChunkSize) {
            failure = QStringLiteral("Server announced incompatible frame or chunk limits");
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }
        if (info.fileSize > static_cast<std::uint64_t>(std::numeric_limits<qint64>::max())) {
            failure = QStringLiteral("Package is larger than QFile's signed 64-bit size limit");
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }

        QDir cacheDirectory;
        if (!cacheDirectory.mkpath(m_CacheDirectory)) {
            failure = QStringLiteral("Cannot create cache directory: %1").arg(m_CacheDirectory);
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }
        cacheDirectory.setPath(m_CacheDirectory);
        const QString finalPath = cacheDirectory.filePath(info.fileName);
        const QString partPath = finalPath + QStringLiteral(".part");
        const QString partMetadataPath = partPath + QStringLiteral(".json");
        const QString finalMetadataPath = finalPath + QStringLiteral(".download.json");

        CacheMetadata finalMetadata;
        if (QFileInfo::exists(finalPath) && ReadMetadata(finalMetadataPath, finalMetadata) &&
            MetadataMatches(finalMetadata, info) && finalMetadata.verified &&
            static_cast<std::uint64_t>(QFileInfo(finalPath).size()) == info.fileSize) {
            bool cacheIsValid = true;
            if (!info.sha256.isEmpty()) {
                emit Status(QStringLiteral("Verifying the complete cached package"));
                QString cacheFailure;
                const DownloadAttemptResult verifyResult =
                        VerifySha256(finalPath, info.sha256, cacheFailure);
                if (verifyResult == DownloadAttemptResult::Cancelled) {
                    SendGoodbye(socket);
                    return verifyResult;
                }
                cacheIsValid = verifyResult == DownloadAttemptResult::Complete;
                if (!cacheIsValid) {
                    emit Status(QStringLiteral("Cached package failed SHA-256; downloading it again"));
                    if (!QFile::remove(finalPath) || !QFile::remove(finalMetadataPath)) {
                        failure = QStringLiteral("Cannot remove an invalid cached package: %1")
                                          .arg(cacheFailure);
                        SendGoodbye(socket);
                        return DownloadAttemptResult::Failed;
                    }
                }
            }
            if (cacheIsValid) {
                emit Status(QStringLiteral("Using the complete cached package"));
                emit Started(info.fileSize, info.fileSize);
                emit Progress(info.fileSize, info.fileSize);
                emit Ready(QDir::cleanPath(finalPath));
                SendGoodbye(socket);
                return DownloadAttemptResult::Complete;
            }
        }

        CacheMetadata partMetadata;
        const bool hasMatchingPartMetadata =
                ReadMetadata(partMetadataPath, partMetadata) && MetadataMatches(partMetadata, info);
        if (!hasMatchingPartMetadata) {
            if (QFileInfo::exists(partPath)) {
                emit Status(QStringLiteral("Cached partial package is from another version; restarting"));
                if (!QFile::remove(partPath)) {
                    failure = QStringLiteral("Cannot discard stale partial package: %1").arg(partPath);
                    SendGoodbye(socket);
                    return DownloadAttemptResult::Failed;
                }
            }
            QFile::remove(partMetadataPath);
        }

        QFile package(partPath);
        if (!package.open(QIODevice::ReadWrite)) {
            failure = QStringLiteral("Cannot open partial package %1: %2").arg(partPath, package.errorString());
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }

        std::uint64_t offset = static_cast<std::uint64_t>(package.size());
        if (offset > info.fileSize || (!hasMatchingPartMetadata && offset != 0)) {
            if (!package.resize(0)) {
                failure = QStringLiteral("Cannot reset partial package %1: %2").arg(partPath, package.errorString());
                SendGoodbye(socket);
                return DownloadAttemptResult::Failed;
            }
            offset = 0;
        }
        if (!package.seek(static_cast<qint64>(offset))) {
            failure = QStringLiteral("Cannot seek partial package %1: %2").arg(partPath, package.errorString());
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }

        if (!WriteMetadata(partMetadataPath, info, false, failure)) {
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }

        emit Started(offset, info.fileSize);
        emit Progress(offset, info.fileSize);
        if (offset > 0) {
            emit Status(QStringLiteral("Resuming package at %1 of %2 bytes").arg(offset).arg(info.fileSize));
        } else {
            emit Status(QStringLiteral("Downloading %1 bytes").arg(info.fileSize));
        }

        std::uint64_t requestId = 2;
        while (offset < info.fileSize) {
            if (IsCancelled()) {
                package.flush();
                SendGoodbye(socket);
                return DownloadAttemptResult::Cancelled;
            }

            const std::uint64_t remaining = info.fileSize - offset;
            const auto requested = static_cast<std::uint32_t>(
                    std::min<std::uint64_t>(remaining, info.maximumChunkSize));
            const DownloadAttemptResult chunkResult =
                    RequestChunk(socket, info, requestId++, offset, requested, package, failure);
            if (chunkResult != DownloadAttemptResult::Complete) {
                package.flush();
                if (chunkResult == DownloadAttemptResult::StaleVersion) {
                    package.close();
                    QFile::remove(partPath);
                    QFile::remove(partMetadataPath);
                }
                if (chunkResult != DownloadAttemptResult::RetryableTransportFailure &&
                    chunkResult != DownloadAttemptResult::Cancelled) {
                    SendGoodbye(socket);
                }
                return chunkResult;
            }
            offset += requested;
            emit Progress(offset, info.fileSize);
        }

        if (!package.flush()) {
            failure = QStringLiteral("Cannot flush partial package %1: %2").arg(partPath, package.errorString());
            SendGoodbye(socket);
            return DownloadAttemptResult::Failed;
        }
        package.close();
        SendGoodbye(socket);

        if (!info.sha256.isEmpty()) {
            emit Status(QStringLiteral("Verifying package SHA-256"));
            const DownloadAttemptResult verifyResult = VerifySha256(partPath, info.sha256, failure);
            if (verifyResult != DownloadAttemptResult::Complete) {
                if (verifyResult == DownloadAttemptResult::Failed) {
                    QFile::remove(partPath);
                    QFile::remove(partMetadataPath);
                }
                return verifyResult;
            }
        }

        if (!AtomicReplace(partPath, finalPath, failure)) { return DownloadAttemptResult::Failed; }
        if (!WriteMetadata(finalMetadataPath, info, true, failure)) { return DownloadAttemptResult::Failed; }
        QFile::remove(partMetadataPath);

        emit Status(QStringLiteral("Package download complete"));
        emit Ready(QDir::cleanPath(finalPath));
        return DownloadAttemptResult::Complete;
    }

    DownloadAttemptResult RequestPackageInfo(SocketSession& socket,
                                             PackageInfo& info,
                                             QString& failure)
    {
        const QByteArray packageId = m_PackageId.toUtf8();
        if (packageId.isEmpty() || packageId.size() > std::numeric_limits<std::uint16_t>::max()) {
            failure = QStringLiteral("Package id is empty or too long");
            return DownloadAttemptResult::Failed;
        }
        igpk::InfoRequest infoRequest;
        infoRequest.packageId.assign(packageId.constData(),
                                     static_cast<std::size_t>(packageId.size()));
        std::vector<std::uint8_t> request;
        try {
            request = igpk::encodeInfoRequest(infoRequest);
        } catch (const std::exception&) {
            failure = QStringLiteral("Package id is empty or too long");
            return DownloadAttemptResult::Failed;
        }
        if (!socket.SendFrame(igpk::MessageType::InfoRequest, 1, request, failure)) {
            return IsCancelled() ? DownloadAttemptResult::Cancelled
                                 : DownloadAttemptResult::RetryableTransportFailure;
        }

        igpk::FrameHeader header;
        std::vector<std::uint8_t> payload;
        if (!socket.ReceiveFrame(header, payload, failure)) {
            return IsCancelled() ? DownloadAttemptResult::Cancelled
                                 : DownloadAttemptResult::RetryableTransportFailure;
        }
        if (header.requestId != 1) {
            failure = QStringLiteral("INFO response request id does not match");
            return DownloadAttemptResult::Failed;
        }
        if (header.type == igpk::MessageType::Error) {
            igpk::ErrorResponse error;
            std::string decodeError;
            if (!igpk::decodeErrorResponse(payload, error, decodeError)) {
                failure = QStringLiteral("ERROR payload is invalid: %1")
                                  .arg(QString::fromStdString(decodeError));
                return DownloadAttemptResult::Failed;
            }
            failure = QStringLiteral("Server rejected INFO request (%1): %2")
                              .arg(static_cast<std::uint32_t>(error.code))
                              .arg(QString::fromUtf8(error.message.data(),
                                                    static_cast<int>(error.message.size())));
            return DownloadAttemptResult::Failed;
        }
        if (header.type != igpk::MessageType::InfoResponse) {
            failure = QStringLiteral("Expected INFO_RESPONSE, received message type %1")
                              .arg(static_cast<std::uint16_t>(header.type));
            return DownloadAttemptResult::Failed;
        }
        igpk::InfoResponse response;
        std::string decodeError;
        if (!igpk::decodeInfoResponse(payload, response, decodeError)) {
            failure = QStringLiteral("INFO_RESPONSE payload is invalid: %1")
                              .arg(QString::fromStdString(decodeError));
            return DownloadAttemptResult::Failed;
        }
        info.fileSize = response.fileSize;
        info.mtimeTicks = static_cast<std::uint64_t>(response.mtimeTicks);
        info.maximumFrameSize = response.maxFrameSize;
        info.maximumChunkSize = response.maxChunkSize;
        info.packageId = QString::fromUtf8(response.packageId.data(),
                                           static_cast<int>(response.packageId.size()));
        info.fileName = QString::fromUtf8(response.fileName.data(),
                                          static_cast<int>(response.fileName.size()));
        info.versionToken = QString::fromUtf8(response.versionToken.data(),
                                              static_cast<int>(response.versionToken.size()));
        info.sha256 = QByteArray(response.sha256.data(),
                                 static_cast<int>(response.sha256.size()));
        if (info.packageId.isEmpty() || info.fileName.isEmpty() || info.versionToken.isEmpty()) {
            failure = QStringLiteral("INFO_RESPONSE contains an empty required field");
            return DownloadAttemptResult::Failed;
        }
        return DownloadAttemptResult::Complete;
    }

    DownloadAttemptResult RequestChunk(SocketSession& socket,
                                       const PackageInfo& info,
                                       std::uint64_t requestId,
                                       std::uint64_t offset,
                                       std::uint32_t requested,
                                       QFile& package,
                                       QString& failure)
    {
        const QByteArray packageId = info.packageId.toUtf8();
        const QByteArray token = info.versionToken.toUtf8();
        if (packageId.size() > std::numeric_limits<std::uint16_t>::max() ||
            token.size() > std::numeric_limits<std::uint16_t>::max()) {
            failure = QStringLiteral("Package id or version token exceeds protocol limit");
            return DownloadAttemptResult::Failed;
        }

        igpk::GetRequest getRequest;
        getRequest.packageId.assign(packageId.constData(),
                                    static_cast<std::size_t>(packageId.size()));
        getRequest.versionToken.assign(token.constData(),
                                       static_cast<std::size_t>(token.size()));
        getRequest.offset = offset;
        getRequest.requestedLength = requested;
        std::vector<std::uint8_t> request;
        try {
            request = igpk::encodeGetRequest(getRequest);
        } catch (const std::exception&) {
            failure = QStringLiteral("Package id or version token exceeds protocol limit");
            return DownloadAttemptResult::Failed;
        }
        if (!socket.SendFrame(igpk::MessageType::GetRequest,
                              requestId, request, failure)) {
            return IsCancelled() ? DownloadAttemptResult::Cancelled
                                 : DownloadAttemptResult::RetryableTransportFailure;
        }

        igpk::FrameHeader header;
        std::vector<std::uint8_t> payload;
        if (!socket.ReceiveFrame(header, payload, failure)) {
            return IsCancelled() ? DownloadAttemptResult::Cancelled
                                 : DownloadAttemptResult::RetryableTransportFailure;
        }
        if (header.requestId != requestId) {
            failure = QStringLiteral("DATA response request id does not match");
            return DownloadAttemptResult::Failed;
        }
        if (header.type == igpk::MessageType::Error) {
            igpk::ErrorResponse error;
            std::string decodeError;
            if (!igpk::decodeErrorResponse(payload, error, decodeError)) {
                failure = QStringLiteral("ERROR payload is invalid: %1")
                                  .arg(QString::fromStdString(decodeError));
                return DownloadAttemptResult::Failed;
            }
            if (error.code == igpk::ErrorCode::StaleVersion) {
                return DownloadAttemptResult::StaleVersion;
            }
            failure = QStringLiteral("Server rejected GET request (%1): %2")
                              .arg(static_cast<std::uint32_t>(error.code))
                              .arg(QString::fromUtf8(error.message.data(),
                                                    static_cast<int>(error.message.size())));
            return DownloadAttemptResult::Failed;
        }
        if (header.type != igpk::MessageType::DataChunk) {
            failure = QStringLiteral("Expected a valid DATA_CHUNK response");
            return DownloadAttemptResult::Failed;
        }

        igpk::DataChunkView chunk;
        std::string decodeError;
        if (!igpk::decodeDataChunk(payload, chunk, decodeError)) {
            failure = QStringLiteral("Expected a valid DATA_CHUNK response");
            return DownloadAttemptResult::Failed;
        }
        if (chunk.offset != offset || chunk.dataLength != requested) {
            failure = QStringLiteral("DATA_CHUNK offset or length does not match the request");
            return DownloadAttemptResult::Failed;
        }
        if (igpk::crc32(chunk.data, chunk.dataLength) != chunk.crc32) {
            failure = QStringLiteral("CRC32 mismatch at package offset %1").arg(offset);
            return DownloadAttemptResult::Failed;
        }
        if (package.write(reinterpret_cast<const char*>(chunk.data), chunk.dataLength) !=
            static_cast<qint64>(chunk.dataLength)) {
            failure = QStringLiteral("Cannot write package data at offset %1: %2")
                              .arg(offset)
                              .arg(package.errorString());
            return DownloadAttemptResult::Failed;
        }
        return DownloadAttemptResult::Complete;
    }

    DownloadAttemptResult VerifySha256(const QString& path,
                                       const QByteArray& expected,
                                       QString& failure) const
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            failure = QStringLiteral("Cannot verify package %1: %2").arg(path, file.errorString());
            return DownloadAttemptResult::Failed;
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        constexpr qint64 bufferSize = 4 * 1024 * 1024;
        while (!file.atEnd()) {
            if (IsCancelled()) { return DownloadAttemptResult::Cancelled; }
            const QByteArray bytes = file.read(bufferSize);
            if (bytes.isEmpty() && file.error() != QFile::NoError) {
                failure = QStringLiteral("Cannot read package for verification: %1").arg(file.errorString());
                return DownloadAttemptResult::Failed;
            }
            hash.addData(bytes);
        }
        if (hash.result() != expected) {
            failure = QStringLiteral("Downloaded package SHA-256 does not match server metadata");
            return DownloadAttemptResult::Failed;
        }
        return DownloadAttemptResult::Complete;
    }

    void SendGoodbye(SocketSession& socket)
    {
        if (IsCancelled()) { return; }
        QString ignored;
        socket.SendFrame(igpk::MessageType::Goodbye,
                         std::numeric_limits<std::uint64_t>::max(), {}, ignored);
    }

    bool IsCancelled() const
    {
        return m_CancelState->cancelled.load(std::memory_order_acquire);
    }

    QString m_Address;
    quint16 m_Port;
    QString m_PackageId;
    QString m_CacheDirectory;
    std::shared_ptr<igQtPackageDownloadCancellationState> m_CancelState;
};
} // namespace

igQtPackageDownloader::igQtPackageDownloader(QObject* parent)
    : QObject(parent)
{
}

igQtPackageDownloader::~igQtPackageDownloader()
{
    Shutdown();
}

void igQtPackageDownloader::Shutdown()
{
    Cancel();
    if (m_Thread != nullptr) {
        QThread* const thread = m_Thread;
        // Do not call quit() here.  PackageDownloadWorker::Done schedules the
        // worker's DeferredDelete, and destroying the worker is what stops the
        // event loop.  Cancellation shuts down the active socket so Run()
        // returns and that orderly sequence can complete.
        if (thread->isRunning()) { thread->wait(); }
        disconnect(thread, &QThread::finished,
                   this, &igQtPackageDownloader::OnThreadFinished);
        m_Thread = nullptr;
        delete thread;
    }
    m_CancelState.reset();
    if (m_Running) {
        m_Running = false;
        emit RunningChanged(false);
    }
}

bool igQtPackageDownloader::SetServerEndpoint(const QString& address, quint16 port)
{
    if (m_Running || address.trimmed().isEmpty() || port == 0) { return false; }
    m_ServerAddress = address.trimmed();
    m_ServerPort = port;
    return true;
}

bool igQtPackageDownloader::StartDownload(const QString& packageId, const QString& cacheDirectory)
{
    if (m_Running || packageId.trimmed().isEmpty() || cacheDirectory.trimmed().isEmpty()) { return false; }

    if (m_Thread != nullptr) {
        if (m_Thread->isRunning()) { return false; }
        delete m_Thread;
        m_Thread = nullptr;
    }

    m_CancelState = std::make_shared<igQtPackageDownloadCancellationState>();
    m_Thread = new QThread(this);
    m_Thread->setObjectName(QStringLiteral("iGameVis package download"));
    auto* worker = new PackageDownloadWorker(m_ServerAddress,
                                             m_ServerPort,
                                             packageId,
                                             QDir::cleanPath(cacheDirectory.trimmed()),
                                             m_CancelState);
    worker->moveToThread(m_Thread);

    connect(m_Thread, &QThread::started, worker, &PackageDownloadWorker::Run);
    connect(worker, &PackageDownloadWorker::Status, this, &igQtPackageDownloader::StatusChanged);
    connect(worker,
            &PackageDownloadWorker::PackageInfoAvailable,
            this,
            &igQtPackageDownloader::PackageInfoReceived);
    connect(worker, &PackageDownloadWorker::Started, this, &igQtPackageDownloader::DownloadStarted);
    connect(worker, &PackageDownloadWorker::Progress, this, &igQtPackageDownloader::DownloadProgress);
    connect(worker, &PackageDownloadWorker::Ready, this, &igQtPackageDownloader::PackageReady);
    connect(worker, &PackageDownloadWorker::Failed, this, &igQtPackageDownloader::DownloadFailed);
    connect(worker, &PackageDownloadWorker::Cancelled, this, &igQtPackageDownloader::DownloadCancelled);
    // Process DeferredDelete in the worker event loop before stopping it.  A
    // finished->deleteLater connection queues deletion after the event loop
    // has already stopped and leaks one worker for every completed download.
    connect(worker, &PackageDownloadWorker::Done,
            worker, &QObject::deleteLater);
    connect(worker, &QObject::destroyed,
            m_Thread, &QThread::quit, Qt::DirectConnection);
    connect(m_Thread, &QThread::finished, this, &igQtPackageDownloader::OnThreadFinished);

    m_Running = true;
    emit RunningChanged(true);
    m_Thread->start();
    return true;
}

void igQtPackageDownloader::Cancel()
{
    if (m_CancelState) { m_CancelState->RequestCancel(); }
}

void igQtPackageDownloader::OnThreadFinished()
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

#include "igQtPackageDownloader.moc"
