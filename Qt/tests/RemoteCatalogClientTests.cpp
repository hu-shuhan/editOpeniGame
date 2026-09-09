#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
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

#include <IQCore/igQtRemoteCatalogClient.h>
#include <IQCore/igQtRemotePackageValidation.h>
#include <DataTransfer/iGamePackageProtocol.h>

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

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
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
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
                               static_cast<int>(size - received),
                               0);
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
                               static_cast<int>(size - sent),
                               0);
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
    ReceivedFrame result;
    std::string error;
    if (!igpk::decodeHeader(encodedHeader, result.header, error)) {
        throw std::runtime_error("invalid client frame header: " + error);
    }
    result.payload.resize(static_cast<std::size_t>(result.header.payloadSize));
    if (!result.payload.empty()) {
        receiveAll(socket, result.payload.data(), result.payload.size());
    }
    return result;
}

void sendFrame(TestSocket socket,
               igpk::MessageType messageType,
               std::uint64_t requestId,
               const std::vector<std::uint8_t>& payload)
{
    igpk::FrameHeader header;
    header.type = messageType;
    header.requestId = requestId;
    header.payloadSize = payload.size();
    const auto encodedHeader = igpk::encodeHeader(header);
    sendAll(socket, encodedHeader.data(), encodedHeader.size());
    if (!payload.empty()) { sendAll(socket, payload.data(), payload.size()); }
}

std::string digestToHex(const std::string& digest)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string output;
    output.reserve(digest.size() * 2u);
    for (unsigned char byte : digest) {
        output.push_back(digits[byte >> 4u]);
        output.push_back(digits[byte & 0x0fu]);
    }
    return output;
}

igpk::CatalogEntry makeEntry(unsigned int index)
{
    char stem[32]{};
    std::snprintf(stem, sizeof(stem), "model-%03u", index);
    igpk::CatalogEntry entry;
    entry.fileSize = index == 0u ? 5ull * 1024ull * 1024ull * 1024ull + 7ull
                                 : 1000ull + index;
    entry.mtimeTicks = -1000 + static_cast<std::int64_t>(index);
    entry.packageId = std::string(stem) + ".tar.zst";
    entry.displayName = stem;
    entry.fileName = entry.packageId;
    entry.sha256.resize(32u);
    for (std::size_t byte = 0; byte < entry.sha256.size(); ++byte) {
        entry.sha256[byte] = static_cast<char>((index + byte) & 0xffu);
    }
    entry.versionToken = digestToHex(entry.sha256);
    return entry;
}

enum class FakeScenario
{
    PagedSuccess,
    EmptySuccess,
    RevisionMismatch,
};

class FakeCatalogServer
{
public:
    explicit FakeCatalogServer(FakeScenario scenario)
        : m_scenario(scenario)
    {
        m_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listener == InvalidSocket) { throw std::runtime_error("socket failed"); }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if (bind(m_listener,
                 reinterpret_cast<const sockaddr*>(&address),
                 sizeof(address)) != 0) {
            closeSocket(m_listener);
            m_listener = InvalidSocket;
            throw std::runtime_error("bind failed");
        }
        if (listen(m_listener, 1) != 0) {
            closeSocket(m_listener);
            m_listener = InvalidSocket;
            throw std::runtime_error("listen failed");
        }
        sockaddr_in bound{};
#ifdef _WIN32
        int boundLength = sizeof(bound);
#else
        socklen_t boundLength = sizeof(bound);
#endif
        if (getsockname(m_listener,
                        reinterpret_cast<sockaddr*>(&bound),
                        &boundLength) != 0) {
            closeSocket(m_listener);
            m_listener = InvalidSocket;
            throw std::runtime_error("getsockname failed");
        }
        m_port = ntohs(bound.sin_port);
    }

    ~FakeCatalogServer()
    {
        closeSocket(m_listener);
        if (m_thread.joinable()) { m_thread.join(); }
    }

    std::uint16_t port() const { return m_port; }
    void start() { m_thread = std::thread([this]() { run(); }); }

    void join()
    {
        if (m_thread.joinable()) { m_thread.join(); }
    }

    const std::string& failure() const { return m_failure; }
    bool sawGoodbye() const { return m_sawGoodbye.load(); }
    bool sawClientClose() const { return m_sawClientClose.load(); }

private:
    void run()
    {
        TestSocket client = InvalidSocket;
        try {
            client = accept(m_listener, nullptr, nullptr);
            if (client == InvalidSocket) { throw std::runtime_error("accept failed"); }
            setSocketTimeout(client);

            const ReceivedFrame firstFrame = receiveFrame(client);
            if (firstFrame.header.type != igpk::MessageType::CatalogRequest) {
                throw std::runtime_error("first client frame is not CATALOG_REQUEST");
            }
            igpk::CatalogRequest firstRequest;
            std::string decodeError;
            if (!igpk::decodeCatalogRequest(firstFrame.payload, firstRequest, decodeError)) {
                throw std::runtime_error("bad first CATALOG_REQUEST: " + decodeError);
            }
            if (!firstRequest.forceRefresh() || firstRequest.revision != 0u ||
                firstRequest.cursor != 0u ||
                firstRequest.pageSize != igpk::kMaxCatalogPageSize) {
                throw std::runtime_error("first CATALOG_REQUEST fields are incorrect");
            }

            igpk::CatalogResponse firstResponse;
            firstResponse.revision = m_scenario == FakeScenario::EmptySuccess ? 88u : 77u;
            firstResponse.nextCursor = m_scenario == FakeScenario::EmptySuccess
                    ? igpk::kCatalogEndCursor : 100u;
            if (m_scenario != FakeScenario::EmptySuccess) {
                for (unsigned int index = 0; index < 100u; ++index) {
                    firstResponse.entries.push_back(makeEntry(index));
                }
            }
            sendFrame(client,
                      igpk::MessageType::CatalogResponse,
                      firstFrame.header.requestId,
                      igpk::encodeCatalogResponse(firstResponse));

            if (m_scenario != FakeScenario::EmptySuccess) {
                const ReceivedFrame secondFrame = receiveFrame(client);
                if (secondFrame.header.type != igpk::MessageType::CatalogRequest) {
                    throw std::runtime_error("second client frame is not CATALOG_REQUEST");
                }
                igpk::CatalogRequest secondRequest;
                decodeError.clear();
                if (!igpk::decodeCatalogRequest(secondFrame.payload, secondRequest, decodeError)) {
                    throw std::runtime_error("bad second CATALOG_REQUEST: " + decodeError);
                }
                if (secondRequest.forceRefresh() || secondRequest.revision != 77u ||
                    secondRequest.cursor != 100u ||
                    secondRequest.pageSize != igpk::kMaxCatalogPageSize) {
                    throw std::runtime_error("second CATALOG_REQUEST fields are incorrect");
                }

                igpk::CatalogResponse secondResponse;
                secondResponse.revision = m_scenario == FakeScenario::RevisionMismatch
                        ? 78u : 77u;
                secondResponse.nextCursor = igpk::kCatalogEndCursor;
                secondResponse.entries.push_back(makeEntry(100u));
                sendFrame(client,
                          igpk::MessageType::CatalogResponse,
                          secondFrame.header.requestId,
                          igpk::encodeCatalogResponse(secondResponse));
            }

            const ReceivedFrame goodbye = receiveFrame(client);
            if (goodbye.header.type != igpk::MessageType::Goodbye ||
                !goodbye.payload.empty()) {
                throw std::runtime_error("client did not finish catalog fetch with GOODBYE");
            }
            m_sawGoodbye.store(true);

            char trailing = 0;
            const int count = recv(client, &trailing, 1, 0);
            if (count != 0) {
                throw std::runtime_error("client did not close after GOODBYE");
            }
            m_sawClientClose.store(true);
        } catch (const std::exception& error) {
            m_failure = error.what();
        }
        closeSocket(client);
        closeSocket(m_listener);
        m_listener = InvalidSocket;
    }

    TestSocket m_listener{InvalidSocket};
    FakeScenario m_scenario;
    std::uint16_t m_port{0};
    std::thread m_thread;
    std::string m_failure;
    std::atomic<bool> m_sawGoodbye{false};
    std::atomic<bool> m_sawClientClose{false};
};

bool require(bool condition, const char* message)
{
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; }
    return condition;
}

bool writeTestFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
           file.write(contents) == contents.size();
}

bool testVtmManifestReaderSemantics()
{
    QTemporaryDir temporary;
    bool ok = require(temporary.isValid(), "temporary VTM validation root");
    if (!temporary.isValid()) { return false; }

    const QString packageRoot = QDir(temporary.path()).filePath(QStringLiteral("package"));
    ok &= require(QDir().mkpath(packageRoot), "create VTM package root");
    ok &= require(writeTestFile(QDir(packageRoot).filePath(QStringLiteral("piece.vtu")),
                                QByteArrayLiteral("test-piece")),
                  "write package-local VTM piece");
    ok &= require(writeTestFile(QDir(temporary.path()).filePath(QStringLiteral("outside.vtu")),
                                QByteArrayLiteral("outside-piece")),
                  "write outside VTM piece");

    const auto validate = [&](const QString& name,
                              const QByteArray& body,
                              bool expected) {
        const QString manifestPath = QDir(packageRoot).filePath(name);
        if (!writeTestFile(manifestPath, body)) {
            return require(false, "write VTM manifest");
        }
        QString error;
        const bool valid = igQtValidateRemoteVtmManifest(
                manifestPath, packageRoot, error);
        if (valid != expected) {
            std::cerr << "FAILED: VTM validation result for "
                      << name.toStdString() << ": "
                      << error.toStdString() << '\n';
            return false;
        }
        return true;
    };

    ok &= validate(QStringLiteral("flat.vtm"),
                   QByteArrayLiteral(
                       "<VTKFile><vtkMultiBlockDataSet>"
                       "<DataSet file=\"piece.vtu\"/>"
                       "</vtkMultiBlockDataSet></VTKFile>"),
                   true);
    ok &= validate(QStringLiteral("block-arbitrary-tag.vtm"),
                   QByteArrayLiteral(
                       "<VTKFile><vtkMultiBlockDataSet><Block>"
                       "<Piece file=\"piece.vtu\"/>"
                       "</Block></vtkMultiBlockDataSet></VTKFile>"),
                   true);
    ok &= validate(QStringLiteral("block-traversal.vtm"),
                   QByteArrayLiteral(
                       "<VTKFile><vtkMultiBlockDataSet><Block>"
                       "<Arbitrary file=\"../outside.vtu\"/>"
                       "</Block></vtkMultiBlockDataSet></VTKFile>"),
                   false);
    ok &= validate(QStringLiteral("block-precedence.vtm"),
                   QByteArrayLiteral(
                       "<VTKFile><vtkMultiBlockDataSet>"
                       "<DataSet file=\"piece.vtu\"/>"
                       "<Block><NotDataSet file=\"../outside.vtu\"/></Block>"
                       "</vtkMultiBlockDataSet></VTKFile>"),
                   false);
    return ok;
}

bool testCacheNamespace()
{
    QTemporaryDir temporary;
    bool ok = require(temporary.isValid(), "temporary cache root");
    if (!temporary.isValid()) { return false; }

    const QString alpha = igQtRemotePackageCacheDirectory(
        temporary.path(), QStringLiteral(" LOCALHOST "), 34567,
        QStringLiteral("alpha.tar.zst"));
    const QString alphaCanonical = igQtRemotePackageCacheDirectory(
        temporary.path(), QStringLiteral("localhost"), 34567,
        QStringLiteral("alpha.tar.zst"));
    const QString beta = igQtRemotePackageCacheDirectory(
        temporary.path(), QStringLiteral("localhost"), 34567,
        QStringLiteral("beta.tar.zst"));
    const QString otherPort = igQtRemotePackageCacheDirectory(
        temporary.path(), QStringLiteral("localhost"), 34568,
        QStringLiteral("alpha.tar.zst"));
    const QString unsafeId = igQtRemotePackageCacheDirectory(
        temporary.path(), QStringLiteral("localhost"), 34567,
        QStringLiteral("../outside.tar.zst"));
    const QString leadingSpaceId = igQtRemotePackageCacheDirectory(
        temporary.path(), QStringLiteral("localhost"), 34567,
        QStringLiteral(" alpha.tar.zst"));
    const QString whitespaceOnlyId = igQtRemotePackageCacheDirectory(
        temporary.path(), QStringLiteral("localhost"), 34567,
        QStringLiteral("   "));

    ok &= require(!alpha.isEmpty(), "cache namespace is non-empty");
    ok &= require(alpha == alphaCanonical, "host normalization is stable");
    ok &= require(alpha != beta, "two packages have isolated caches");
    ok &= require(alpha != otherPort, "two endpoints have isolated caches");
    ok &= require(unsafeId != alpha, "full logical package id participates in cache key");
    ok &= require(leadingSpaceId != alpha,
                  "leading whitespace in an opaque package id is preserved");
    ok &= require(whitespaceOnlyId.isEmpty(), "whitespace-only package id is rejected");

    const QString packagesRoot = QDir::cleanPath(
        QDir(temporary.path()).filePath(QStringLiteral("packages")));
    const QRegularExpression shaDirectory(QStringLiteral("^[0-9a-f]{64}$"));
    for (const QString& path : {alpha, beta, otherPort, unsafeId, leadingSpaceId}) {
        const QFileInfo info(path);
        ok &= require(QDir::cleanPath(info.absolutePath()) == packagesRoot,
                      "cache namespace remains under cacheRoot/packages");
        ok &= require(shaDirectory.match(info.fileName()).hasMatch(),
                      "cache namespace leaf is a lowercase SHA-256");
    }
    return ok;
}

struct FetchOutcome
{
    QVector<igQtRemoteCatalogEntry> entries;
    QString failure;
    bool started{false};
    bool ready{false};
    bool cancelled{false};
    bool timedOut{false};
};

FetchOutcome runFetch(igQtRemoteCatalogClient& client, std::uint16_t port)
{
    FetchOutcome outcome;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(10000);
    QObject connectionScope;
    bool terminalSignal = false;

    const auto quitWhenStopped = [&]() {
        if (terminalSignal && !client.IsRunning()) { loop.quit(); }
    };
    QObject::connect(&client, &igQtRemoteCatalogClient::CatalogReady,
                     &connectionScope,
                     [&](const QVector<igQtRemoteCatalogEntry>& entries) {
                         if (terminalSignal) { return; }
                         outcome.entries = entries;
                         outcome.ready = true;
                         terminalSignal = true;
                         quitWhenStopped();
                     });
    QObject::connect(&client, &igQtRemoteCatalogClient::Failed,
                     &connectionScope,
                     [&](const QString& message) {
                         if (terminalSignal) { return; }
                         outcome.failure = message;
                         terminalSignal = true;
                         quitWhenStopped();
                     });
    QObject::connect(&client, &igQtRemoteCatalogClient::Cancelled,
                     &connectionScope,
                     [&]() {
                         if (terminalSignal) { return; }
                         outcome.cancelled = true;
                         terminalSignal = true;
                         quitWhenStopped();
                     });
    QObject::connect(&client, &igQtRemoteCatalogClient::RunningChanged,
                     &connectionScope,
                     [&](bool running) {
                         if (!running) { quitWhenStopped(); }
                     });
    QObject::connect(&timeout, &QTimer::timeout,
                     &connectionScope,
                     [&]() {
                         outcome.timedOut = true;
                         terminalSignal = true;
                         client.Cancel();
                         QTimer::singleShot(2000, &loop, &QEventLoop::quit);
                         quitWhenStopped();
                     });

    outcome.started = client.Fetch(QStringLiteral("127.0.0.1"), port);
    if (!outcome.started) {
        outcome.failure = QStringLiteral("Fetch returned false");
        return outcome;
    }
    timeout.start();
    loop.exec();
    timeout.stop();
    if (client.IsRunning() && outcome.timedOut) { client.Cancel(); }
    return outcome;
}

bool validateClosedServer(const FakeCatalogServer& server, const char* scenario)
{
    bool ok = true;
    if (!server.failure().empty()) {
        std::cerr << "FAILED: " << scenario << " fake server: "
                  << server.failure() << '\n';
        ok = false;
    }
    ok &= require(server.sawGoodbye(), "catalog client sends GOODBYE");
    ok &= require(server.sawClientClose(),
                  "catalog client closes its socket before handoff");
    return ok;
}

bool testPagedCatalogClientLifecycle()
{
    igQtRemoteCatalogClient client;
    bool ok = true;

    FakeCatalogServer pagedServer(FakeScenario::PagedSuccess);
    pagedServer.start();
    const FetchOutcome paged = runFetch(client, pagedServer.port());
    pagedServer.join();

    ok &= require(paged.started, "first catalog Fetch starts");
    ok &= require(!paged.timedOut, "first catalog Fetch completes before timeout");
    ok &= require(paged.failure.isEmpty(), "first catalog Fetch does not fail");
    ok &= require(paged.ready, "catalog client emits CatalogReady");
    ok &= require(!client.IsRunning(), "catalog client finishes its worker lifecycle");
    ok &= validateClosedServer(pagedServer, "paged");
    ok &= require(paged.entries.size() == 101,
                  "catalog client aggregates all pages");
    if (paged.entries.size() == 101) {
        ok &= require(paged.entries.front().packageId ==
                          QStringLiteral("model-000.tar.zst") &&
                          paged.entries.back().packageId ==
                          QStringLiteral("model-100.tar.zst"),
                      "catalog entry ordering survives pagination");
        ok &= require(paged.entries.front().fileSize ==
                          5ull * 1024ull * 1024ull * 1024ull + 7ull,
                      "catalog client preserves a greater-than-4-GiB size");
        ok &= require(paged.entries.front().displayName ==
                          QStringLiteral("model-000"),
                      "catalog client exposes display names");
    }

    FakeCatalogServer emptyServer(FakeScenario::EmptySuccess);
    emptyServer.start();
    const FetchOutcome empty = runFetch(client, emptyServer.port());
    emptyServer.join();
    ok &= require(empty.started, "second Fetch starts on the same client");
    ok &= require(empty.ready && empty.entries.isEmpty(),
                  "repeated Fetch returns an empty catalog cleanly");
    ok &= require(empty.failure.isEmpty() && !empty.timedOut && !empty.cancelled,
                  "repeated Fetch has no lifecycle error");
    ok &= require(!client.IsRunning(), "repeated Fetch releases its worker thread");
    ok &= validateClosedServer(emptyServer, "empty repeated-fetch");

    FakeCatalogServer mismatchServer(FakeScenario::RevisionMismatch);
    mismatchServer.start();
    const FetchOutcome mismatch = runFetch(client, mismatchServer.port());
    mismatchServer.join();
    ok &= require(mismatch.started, "third Fetch starts on the same client");
    ok &= require(!mismatch.ready && !mismatch.failure.isEmpty(),
                  "catalog client rejects a revision change between pages");
    ok &= require(mismatch.failure.contains(QStringLiteral("changed"),
                                             Qt::CaseInsensitive),
                  "revision mismatch produces an actionable failure");
    ok &= require(!client.IsRunning(), "failed Fetch releases its worker thread");
    ok &= validateClosedServer(mismatchServer, "revision mismatch");
    return ok;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try {
        SocketRuntime socketRuntime;
        bool ok = testVtmManifestReaderSemantics();
        ok &= testCacheNamespace();
        ok &= testPagedCatalogClientLifecycle();
        if (!ok) { return 1; }
        std::cout << "Remote catalog client tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
