#include <IQCore/igQtPackageDownloader.h>

#include <DataTransfer/iGamePackageProtocol.h>

#include <QCoreApplication>
#include <QCryptographicHash>
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

class FakePackageServer final
{
public:
    FakePackageServer(QByteArray contents, bool expectGet, bool stallAfterGet = false)
        : m_Contents(std::move(contents))
        , m_Digest(QCryptographicHash::hash(m_Contents, QCryptographicHash::Sha256))
        , m_ExpectGet(expectGet)
        , m_StallAfterGet(stallAfterGet)
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
            sendFrame(client, igpk::MessageType::InfoResponse,
                      infoFrame.header.requestId, igpk::encodeInfoResponse(info));

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
        ok &= testSynchronousShutdownWaitsForWorker();
        if (!ok) { return 1; }
        std::cout << "Remote package downloader tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
