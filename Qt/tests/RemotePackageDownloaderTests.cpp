#include <IQCore/igQtPackageDownloader.h>

#include <DataTransfer/iGamePackageProtocol.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using TestSocket = SOCKET;
constexpr TestSocket InvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using TestSocket = int;
constexpr TestSocket InvalidSocket = -1;
#endif

namespace
{
void closeSocket(TestSocket socket)
{
    if (socket == InvalidSocket) { return; }
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

class SocketRuntime
{
public:
    SocketRuntime()
    {
#ifdef _WIN32
        WSADATA data{};
        const int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
        }
#endif
    }

    ~SocketRuntime()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

void setSocketTimeout(TestSocket socket)
{
#ifdef _WIN32
    const DWORD milliseconds = 5000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&milliseconds), sizeof(milliseconds));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
               reinterpret_cast<const char*>(&milliseconds), sizeof(milliseconds));
#else
    const timeval timeout{5, 0};
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
}

void receiveAll(TestSocket socket, std::uint8_t* output, std::size_t size)
{
    std::size_t received = 0;
    while (received < size) {
        const int count = recv(socket,
                               reinterpret_cast<char*>(output + received),
                               static_cast<int>(size - received), 0);
        if (count <= 0) { throw std::runtime_error("fake server receive failed"); }
        received += static_cast<std::size_t>(count);
    }
}

void sendAll(TestSocket socket, const std::uint8_t* input, std::size_t size)
{
    std::size_t sent = 0;
    while (sent < size) {
        const int count = send(socket,
                               reinterpret_cast<const char*>(input + sent),
                               static_cast<int>(size - sent), 0);
        if (count <= 0) { throw std::runtime_error("fake server send failed"); }
        sent += static_cast<std::size_t>(count);
    }
}

struct ReceivedFrame
{
    igpk::FrameHeader header;
    std::vector<std::uint8_t> payload;
};

ReceivedFrame receiveFrame(TestSocket socket)
{
    std::array<std::uint8_t, igpk::kHeaderSize> encodedHeader{};
    receiveAll(socket, encodedHeader.data(), encodedHeader.size());
    ReceivedFrame frame;
    std::string error;
    if (!igpk::decodeHeader(encodedHeader, frame.header, error)) {
        throw std::runtime_error("invalid client frame: " + error);
    }
    frame.payload.resize(static_cast<std::size_t>(frame.header.payloadSize));
    if (!frame.payload.empty()) {
        receiveAll(socket, frame.payload.data(), frame.payload.size());
    }
    return frame;
}

void sendFrame(TestSocket socket,
               igpk::MessageType type,
               std::uint64_t requestId,
               const std::vector<std::uint8_t>& payload)
{
    igpk::FrameHeader header;
    header.type = type;
    header.requestId = requestId;
    header.payloadSize = payload.size();
    const auto encodedHeader = igpk::encodeHeader(header);
    sendAll(socket, encodedHeader.data(), encodedHeader.size());
    if (!payload.empty()) { sendAll(socket, payload.data(), payload.size()); }
}

enum class InfoFault
{
    None,
    WrongPackageId,
    UnsafeFileName,
    NullFileName,
    EmptyVersion,
    FrameTooSmall,
    FrameTooLarge,
    ChunkZero,
    ChunkTooLarge,
    FrameChunkMismatch,
    FileTooLarge,
    WrongRequestId,
    TruncatedDigest,
    EmptyLegacyDigest
};

class FakePackageServer final
{
public:
    FakePackageServer(QByteArray contents, bool expectGet, bool stallAfterGet = false,
                      InfoFault infoFault = InfoFault::None)
        : m_Contents(std::move(contents))
        , m_Digest(QCryptographicHash::hash(m_Contents, QCryptographicHash::Sha256))
        , m_ExpectGet(expectGet)
        , m_StallAfterGet(stallAfterGet)
        , m_InfoFault(infoFault)
    {
        m_Listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_Listener == InvalidSocket) { throw std::runtime_error("socket failed"); }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if (bind(m_Listener, reinterpret_cast<const sockaddr*>(&address),
                 sizeof(address)) != 0 || listen(m_Listener, 1) != 0) {
            closeSocket(m_Listener);
            m_Listener = InvalidSocket;
            throw std::runtime_error("bind/listen failed");
        }
        sockaddr_in bound{};
#ifdef _WIN32
        int boundLength = sizeof(bound);
#else
        socklen_t boundLength = sizeof(bound);
#endif
        if (getsockname(m_Listener, reinterpret_cast<sockaddr*>(&bound),
                        &boundLength) != 0) {
            closeSocket(m_Listener);
            m_Listener = InvalidSocket;
            throw std::runtime_error("getsockname failed");
        }
        m_Port = ntohs(bound.sin_port);
    }

    ~FakePackageServer()
    {
        closeSocket(m_Listener);
        if (m_Thread.joinable()) { m_Thread.join(); }
    }

    std::uint16_t port() const { return m_Port; }
    void start() { m_Thread = std::thread([this]() { run(); }); }
    void join()
    {
        if (m_Thread.joinable()) { m_Thread.join(); }
    }
    const std::string& failure() const { return m_Failure; }
    bool sawGet() const { return m_SawGet.load(); }
    bool sawGoodbye() const { return m_SawGoodbye.load(); }
    bool sawClientClose() const { return m_SawClientClose.load(); }
    bool waitForGet()
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        return m_Condition.wait_for(lock, std::chrono::seconds(5),
                                    [this]() { return m_SawGet.load(); });
    }

private:
    void run()
    {
        TestSocket client = InvalidSocket;
        try {
            client = accept(m_Listener, nullptr, nullptr);
            if (client == InvalidSocket) { throw std::runtime_error("accept failed"); }
            setSocketTimeout(client);

            const ReceivedFrame infoFrame = receiveFrame(client);
            if (infoFrame.header.type != igpk::MessageType::InfoRequest) {
                throw std::runtime_error("expected INFO_REQUEST");
            }
            igpk::InfoRequest infoRequest;
            std::string error;
            if (!igpk::decodeInfoRequest(infoFrame.payload, infoRequest, error) ||
                infoRequest.packageId != " sample.tar.zst") {
                throw std::runtime_error("invalid INFO_REQUEST: " + error);
            }

            igpk::InfoResponse info;
            info.fileSize = static_cast<std::uint64_t>(m_Contents.size());
            info.mtimeTicks = 1700000000;
            info.packageId = " sample.tar.zst";
            info.fileName = " sample.tar.zst";
            info.versionToken = m_Digest.toHex().toStdString();
            info.sha256.assign(m_Digest.constData(),
                               static_cast<std::size_t>(m_Digest.size()));
            switch (m_InfoFault) {
                case InfoFault::WrongPackageId: info.packageId = "another.tar.zst"; break;
                case InfoFault::UnsafeFileName: info.fileName = "../escape.tar.zst"; break;
                case InfoFault::NullFileName: info.fileName.push_back('\0'); break;
                case InfoFault::EmptyVersion: info.versionToken.clear(); break;
                case InfoFault::FrameTooSmall: info.maxFrameSize = 1; break;
                case InfoFault::FrameTooLarge: info.maxFrameSize = igpk::kMaxFrameSize + 1; break;
                case InfoFault::ChunkZero: info.maxChunkSize = 0; break;
                case InfoFault::ChunkTooLarge: info.maxChunkSize = igpk::kMaxChunkSize + 1; break;
                case InfoFault::FrameChunkMismatch: info.maxFrameSize = 64; break;
                case InfoFault::FileTooLarge:
                    info.fileSize = std::numeric_limits<std::uint64_t>::max(); break;
                case InfoFault::EmptyLegacyDigest: info.sha256.clear(); break;
                default: break;
            }
            auto infoPayload = igpk::encodeInfoResponse(info);
            if (m_InfoFault == InfoFault::TruncatedDigest) { infoPayload.pop_back(); }
            sendFrame(client, igpk::MessageType::InfoResponse,
                      m_InfoFault == InfoFault::WrongRequestId ? 99 : infoFrame.header.requestId,
                      infoPayload);

            ReceivedFrame next = receiveFrame(client);
            if (m_ExpectGet) {
                if (next.header.type != igpk::MessageType::GetRequest) {
                    throw std::runtime_error("expected GET_REQUEST");
                }
                igpk::GetRequest get;
                error.clear();
                if (!igpk::decodeGetRequest(next.payload, get, error) ||
                    get.packageId != info.packageId ||
                    get.versionToken != info.versionToken || get.offset != 0u ||
                    get.requestedLength != static_cast<std::uint32_t>(m_Contents.size())) {
                    throw std::runtime_error("invalid GET_REQUEST: " + error);
                }
                m_SawGet.store(true);
                m_Condition.notify_all();
                if (m_StallAfterGet) {
                    char trailing = 0;
                    const int count = recv(client, &trailing, 1, 0);
                    if (count > 0) {
                        throw std::runtime_error(
                                "expected downloader shutdown to close the socket");
                    }
                    m_SawClientClose.store(true);
                    closeSocket(client);
                    client = InvalidSocket;
                    closeSocket(m_Listener);
                    m_Listener = InvalidSocket;
                    return;
                }
                sendFrame(client, igpk::MessageType::DataChunk,
                          next.header.requestId,
                          igpk::encodeDataChunk(
                                  0u,
                                  reinterpret_cast<const std::uint8_t*>(m_Contents.constData()),
                                  static_cast<std::uint32_t>(m_Contents.size())));
                next = receiveFrame(client);
            }

            if (next.header.type != igpk::MessageType::Goodbye ||
                !next.payload.empty()) {
                throw std::runtime_error("expected GOODBYE");
            }
            m_SawGoodbye.store(true);
        } catch (const std::exception& error) {
            m_Failure = error.what();
        }
        closeSocket(client);
        closeSocket(m_Listener);
        m_Listener = InvalidSocket;
    }

    TestSocket m_Listener{InvalidSocket};
    std::uint16_t m_Port{0};
    QByteArray m_Contents;
    QByteArray m_Digest;
    bool m_ExpectGet{false};
    bool m_StallAfterGet{false};
    InfoFault m_InfoFault{InfoFault::None};
    std::thread m_Thread;
    std::string m_Failure;
    std::mutex m_Mutex;
    std::condition_variable m_Condition;
    std::atomic_bool m_SawGet{false};
    std::atomic_bool m_SawGoodbye{false};
    std::atomic_bool m_SawClientClose{false};
};

bool require(bool condition, const char* message)
{
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; }
    return condition;
}

struct DownloadOutcome
{
    QString readyPath;
    QString failure;
    bool started{false};
    bool cancelled{false};
    bool timedOut{false};
    int validatedMetadataCount{0};
    int legacyMetadataCount{0};
};

DownloadOutcome runDownload(igQtPackageDownloader& downloader,
                            std::uint16_t port,
                            const QString& cacheDirectory)
{
    DownloadOutcome outcome;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(10000);
    QObject connectionScope;
    bool terminalSignal = false;

    const auto quitWhenStopped = [&]() {
        if (terminalSignal && !downloader.IsRunning()) { loop.quit(); }
    };
    QObject::connect(&downloader, &igQtPackageDownloader::ValidatedPackageInfoReceived,
                     &connectionScope, [&](const QString&, quint16, const QString&,
                                           const QString&, const QString&, const QByteArray&, quint64) {
                         ++outcome.validatedMetadataCount;
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::PackageInfoReceived,
                     &connectionScope, [&](const QString&, const QString&, const QString&, quint64) {
                         ++outcome.legacyMetadataCount;
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::PackageReady,
                     &connectionScope, [&](const QString& path) {
                         outcome.readyPath = path;
                         terminalSignal = true;
                         quitWhenStopped();
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::DownloadFailed,
                     &connectionScope, [&](const QString& message) {
                         outcome.failure = message;
                         terminalSignal = true;
                         quitWhenStopped();
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::DownloadCancelled,
                     &connectionScope, [&]() {
                         outcome.cancelled = true;
                         terminalSignal = true;
                         quitWhenStopped();
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::RunningChanged,
                     &connectionScope, [&](bool running) {
                         if (!running) { quitWhenStopped(); }
                     });
    QObject::connect(&timeout, &QTimer::timeout, &connectionScope, [&]() {
        outcome.timedOut = true;
        terminalSignal = true;
        downloader.Cancel();
        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        quitWhenStopped();
    });

    if (!downloader.SetServerEndpoint(QStringLiteral("localhost"), port)) {
        outcome.failure = QStringLiteral("SetServerEndpoint failed");
        return outcome;
    }
    outcome.started = downloader.StartDownload(
            QStringLiteral(" sample.tar.zst"), cacheDirectory);
    if (!outcome.started) {
        outcome.failure = QStringLiteral("StartDownload failed");
        return outcome;
    }
    timeout.start();
    loop.exec();
    timeout.stop();
    if (outcome.timedOut && downloader.IsRunning()) { downloader.Cancel(); }
    return outcome;
}

bool testHostnameAndRepeatedDownloaderLifecycle()
{
    QTemporaryDir cache;
    bool ok = require(cache.isValid(), "temporary cache directory");
    if (!cache.isValid()) { return false; }

    const QByteArray contents("iGameVis package downloader hostname test\n");
    igQtPackageDownloader downloader;

    FakePackageServer firstServer(contents, true);
    firstServer.start();
    const DownloadOutcome first = runDownload(downloader, firstServer.port(), cache.path());
    firstServer.join();
    ok &= require(first.started, "hostname download starts");
    ok &= require(!first.timedOut && !first.cancelled && first.failure.isEmpty(),
                  "hostname download completes without failure");
    ok &= require(!first.readyPath.isEmpty() && !downloader.IsRunning(),
                  "first download releases worker lifecycle");
    ok &= require(firstServer.failure().empty() && firstServer.sawGet() &&
                          firstServer.sawGoodbye(),
                  "hostname download preserves package id and completes INFO/GET/GOODBYE");
    ok &= require(first.validatedMetadataCount == 1 && first.legacyMetadataCount == 1,
                  "normal download emits validated and legacy metadata once");
    QFile downloaded(first.readyPath);
    ok &= require(downloaded.open(QIODevice::ReadOnly) &&
                          downloaded.readAll() == contents,
                  "hostname download writes the expected package");

    FakePackageServer cacheHitServer(contents, false);
    cacheHitServer.start();
    const DownloadOutcome cacheHit = runDownload(
            downloader, cacheHitServer.port(), cache.path());
    cacheHitServer.join();
    ok &= require(cacheHit.started, "same downloader starts a second request");
    ok &= require(!cacheHit.timedOut && !cacheHit.cancelled &&
                          cacheHit.failure.isEmpty(),
                  "second request completes without lifecycle failure");
    ok &= require(cacheHit.readyPath == first.readyPath && !downloader.IsRunning(),
                  "second request returns the verified cached package");
    ok &= require(cacheHitServer.failure().empty() &&
                          !cacheHitServer.sawGet() && cacheHitServer.sawGoodbye(),
                  "cache hit performs INFO/GOODBYE without retransferring data");
    ok &= require(cacheHit.validatedMetadataCount == 1 && cacheHit.legacyMetadataCount == 1,
                  "cached download preserves both validated and legacy metadata");
    return ok;
}

struct InfoOutcome
{
    QString serverAddress;
    quint16 serverPort{0};
    QString packageId;
    QString fileName;
    QString versionToken;
    QByteArray sha256;
    quint64 fileSize{0};
    QString failure;
    int validatedCount{0};
    int legacyCount{0};
    bool started{false};
    bool concurrentRejected{false};
    bool cancelled{false};
    bool timedOut{false};
    bool transferSignal{false};
    bool hashStatus{false};
};

InfoOutcome runInfoQuery(igQtPackageDownloader& downloader, std::uint16_t port)
{
    InfoOutcome outcome;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(10000);
    QObject connectionScope;
    const auto quitWhenStopped = [&]() {
        if (!downloader.IsRunning() &&
            (outcome.validatedCount || !outcome.failure.isEmpty() || outcome.cancelled || outcome.timedOut)) {
            loop.quit();
        }
    };
    QObject::connect(&downloader, &igQtPackageDownloader::ValidatedPackageInfoReceived,
                     &connectionScope, [&](const QString& address, quint16 serverPort,
                                           const QString& packageId, const QString& fileName,
                                           const QString& token, const QByteArray& sha, quint64 size) {
                         outcome.serverAddress = address;
                         outcome.serverPort = serverPort;
                         outcome.packageId = packageId;
                         outcome.fileName = fileName;
                         outcome.versionToken = token;
                         outcome.sha256 = sha;
                         outcome.fileSize = size;
                         ++outcome.validatedCount;
                         quitWhenStopped();
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::PackageInfoReceived,
                     &connectionScope, [&](const QString&, const QString&, const QString&, quint64) {
                         ++outcome.legacyCount;
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::PackageReady,
                     &connectionScope, [&](const QString&) { outcome.transferSignal = true; });
    QObject::connect(&downloader, &igQtPackageDownloader::DownloadStarted,
                     &connectionScope, [&](quint64, quint64) { outcome.transferSignal = true; });
    QObject::connect(&downloader, &igQtPackageDownloader::DownloadProgress,
                     &connectionScope, [&](quint64, quint64) { outcome.transferSignal = true; });
    QObject::connect(&downloader, &igQtPackageDownloader::StatusChanged,
                     &connectionScope, [&](const QString& status) {
                         if (status.contains(QStringLiteral("Verifying")) ||
                             status.contains(QStringLiteral("Using the complete cached"))) {
                             outcome.hashStatus = true;
                         }
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::DownloadFailed,
                     &connectionScope, [&](const QString& message) {
                         outcome.failure = message;
                         quitWhenStopped();
                     });
    QObject::connect(&downloader, &igQtPackageDownloader::DownloadCancelled,
                     &connectionScope, [&]() { outcome.cancelled = true; quitWhenStopped(); });
    QObject::connect(&downloader, &igQtPackageDownloader::RunningChanged,
                     &connectionScope, [&](bool running) { if (!running) quitWhenStopped(); });
    QObject::connect(&timeout, &QTimer::timeout, &connectionScope, [&]() {
        outcome.timedOut = true;
        downloader.Cancel();
        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        quitWhenStopped();
    });
    if (!downloader.SetServerEndpoint(QStringLiteral("127.0.0.1"), port)) {
        outcome.failure = QStringLiteral("Cannot set query endpoint");
        return outcome;
    }
    outcome.started = downloader.QueryPackageInfo(QStringLiteral(" sample.tar.zst"));
    if (!outcome.started) {
        outcome.failure = QStringLiteral("Cannot start INFO query");
        return outcome;
    }
    outcome.concurrentRejected = !downloader.QueryPackageInfo(QStringLiteral(" sample.tar.zst")) &&
            !downloader.StartDownload(QStringLiteral(" sample.tar.zst"), QStringLiteral("must-not-create")) &&
            !downloader.SetServerEndpoint(QStringLiteral("127.0.0.1"), port);
    timeout.start();
    loop.exec();
    timeout.stop();
    if (outcome.timedOut && downloader.IsRunning()) { downloader.Shutdown(); }
    return outcome;
}

class CurrentDirectoryGuard final
{
public:
    explicit CurrentDirectoryGuard(const QString& directory) : m_Previous(QDir::currentPath())
    {
        if (!QDir::setCurrent(directory)) { throw std::runtime_error("cannot set test working directory"); }
    }
    ~CurrentDirectoryGuard() { QDir::setCurrent(m_Previous); }
private:
    QString m_Previous;
};

bool testInfoOnlyHasNoTransferOrCacheSideEffects()
{
    QTemporaryDir directory;
    if (!require(directory.isValid(), "INFO-only temporary directory")) { return false; }
    CurrentDirectoryGuard currentDirectory(directory.path());
    QFile sentinel(QStringLiteral(" sample.tar.zst"));
    const QByteArray oldContents("existing cache must not be examined or replaced");
    if (!require(sentinel.open(QIODevice::WriteOnly) && sentinel.write(oldContents) == oldContents.size(),
                 "create INFO-only cache sentinel")) { return false; }
    sentinel.close();
    const QStringList before = QDir(directory.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    const QByteArray contents("query content differs from local sentinel");
    FakePackageServer server(contents, false);
    server.start();
    igQtPackageDownloader downloader;
    const InfoOutcome result = runInfoQuery(downloader, server.port());
    server.join();
    bool ok = require(result.started && result.concurrentRejected && !result.timedOut &&
                              !result.cancelled && result.failure.isEmpty() && !downloader.IsRunning(),
                      "INFO-only succeeds and releases its worker; concurrent requests are rejected");
    const QByteArray digest = QCryptographicHash::hash(contents, QCryptographicHash::Sha256);
    ok &= require(result.validatedCount == 1 && result.legacyCount == 1 &&
                          result.serverAddress == QStringLiteral("127.0.0.1") &&
                          result.serverPort == server.port() &&
                          result.packageId == QStringLiteral(" sample.tar.zst") &&
                          result.fileName == QStringLiteral(" sample.tar.zst") &&
                          result.versionToken == QString::fromLatin1(digest.toHex()) &&
                          result.sha256 == digest && result.fileSize == static_cast<quint64>(contents.size()),
                  "INFO-only emits exact validated identity including endpoint and raw SHA-256");
    ok &= require(server.failure().empty() && server.sawGoodbye() && !server.sawGet() &&
                          !result.transferSignal && !result.hashStatus,
                  "INFO-only performs INFO/GOODBYE without GET, ready/progress or hash work");
    ok &= require(QDir(directory.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot) == before &&
                          sentinel.open(QIODevice::ReadOnly) && sentinel.readAll() == oldContents,
                  "INFO-only preserves working-directory contents and cached bytes");
    sentinel.close();

    // Query completion must leave the same public object usable for a normal
    // download; no Cancel race or partially-running worker may be required.
    FakePackageServer downloadServer(contents, true);
    downloadServer.start();
    const DownloadOutcome download = runDownload(downloader, downloadServer.port(), directory.path());
    downloadServer.join();
    ok &= require(download.started && !download.timedOut && download.failure.isEmpty() &&
                          !download.readyPath.isEmpty() && download.validatedMetadataCount == 1 &&
                          downloadServer.failure().empty() && downloadServer.sawGet(),
                  "normal download works on the same downloader after INFO-only completes");
    return ok;
}

bool testInfoOnlyRejectsUnvalidatedMetadata()
{
    bool ok = true;
    igQtPackageDownloader downloader;
    const std::array faults{InfoFault::WrongPackageId, InfoFault::UnsafeFileName,
            InfoFault::NullFileName, InfoFault::EmptyVersion, InfoFault::FrameTooSmall,
            InfoFault::FrameTooLarge, InfoFault::ChunkZero, InfoFault::ChunkTooLarge,
            InfoFault::FrameChunkMismatch, InfoFault::FileTooLarge,
            InfoFault::WrongRequestId, InfoFault::TruncatedDigest};
    for (const auto fault : faults) {
        FakePackageServer server(QByteArray("invalid INFO must not be published"), false, false, fault);
        server.start();
        const InfoOutcome result = runInfoQuery(downloader, server.port());
        server.join();
        const bool rejected = result.started && !result.timedOut && !result.cancelled &&
                !result.failure.isEmpty() && result.validatedCount == 0 && result.legacyCount == 0 &&
                !result.transferSignal && !result.hashStatus && !downloader.IsRunning() &&
                server.failure().empty() && server.sawGoodbye() && !server.sawGet();
        if (!rejected) { std::cerr << "INFO fault case " << static_cast<int>(fault) << '\n'; }
        ok &= require(rejected, "invalid INFO fails before publishing metadata or touching transfer/cache");
    }
    FakePackageServer legacy(QByteArray("legacy server without digest"), false, false,
                             InfoFault::EmptyLegacyDigest);
    legacy.start();
    const InfoOutcome result = runInfoQuery(downloader, legacy.port());
    legacy.join();
    ok &= require(result.validatedCount == 1 && result.sha256.isEmpty() && result.failure.isEmpty() &&
                          !result.transferSignal && legacy.failure().empty() && legacy.sawGoodbye(),
                  "legacy empty digest is preserved explicitly, never manufactured as a verified digest");
    return ok;
}

bool testSynchronousShutdownWaitsForWorker()
{
    QTemporaryDir cache;
    bool ok = require(cache.isValid(), "shutdown test cache directory");
    if (!cache.isValid()) { return false; }

    const QByteArray contents("server intentionally withholds this chunk");
    FakePackageServer server(contents, true, true);
    server.start();

    igQtPackageDownloader downloader;
    ok &= require(downloader.SetServerEndpoint(
                          QStringLiteral("127.0.0.1"), server.port()),
                  "set shutdown-test endpoint");
    ok &= require(downloader.StartDownload(
                          QStringLiteral(" sample.tar.zst"), cache.path()),
                  "start shutdown-test transfer");
    ok &= require(server.waitForGet(),
                  "shutdown-test worker reaches a blocking receive");

    const auto start = std::chrono::steady_clock::now();
    downloader.Shutdown();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    server.join();

    ok &= require(!downloader.IsRunning(),
                  "Shutdown synchronously clears the running state");
    ok &= require(elapsed < std::chrono::seconds(5),
                  "Shutdown promptly unblocks the worker socket");
    ok &= require(server.failure().empty() && server.sawGet() &&
                          server.sawClientClose(),
                  "Shutdown closes the socket before returning");
    return ok;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try {
        SocketRuntime socketRuntime;
        bool ok = testHostnameAndRepeatedDownloaderLifecycle();
        ok &= testInfoOnlyHasNoTransferOrCacheSideEffects();
        ok &= testInfoOnlyRejectsUnvalidatedMetadata();
        ok &= testSynchronousShutdownWaitsForWorker();
        if (!ok) { return 1; }
        std::cout << "Remote package downloader tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
