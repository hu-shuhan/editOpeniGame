#include "PackageProtocol.h"
#include "PackageCatalog.h"
#include "Sha256.h"
#include "DataTransfer/iGameTarZstdArchive.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

namespace fs = std::filesystem;

namespace {

std::string pathToUtf8(const fs::path& path) {
#if defined(__cpp_lib_char8_t)
    const std::u8string value = path.generic_u8string();
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
#else
    return path.generic_u8string();
#endif
}

fs::path utf8ToPath(const std::string& value) {
#if defined(__cpp_lib_char8_t)
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(value.data()), value.size()));
#else
    return fs::u8path(value);
#endif
}

std::atomic<bool> g_stopRequested{ false };
std::chrono::milliseconds g_connectionIdleTimeout{ std::chrono::minutes(5) };

#ifdef _WIN32
BOOL WINAPI consoleControlHandler(DWORD eventType) {
    switch (eventType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        g_stopRequested.store(true);
        return TRUE;
    default:
        return FALSE;
    }
}
#else
void signalHandler(int) { g_stopRequested.store(true); }
#endif

void logLine(const char* level, const std::string& message) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &nowTime);
#else
    localtime_r(&nowTime, &local);
#endif
    std::cout << '[' << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << "] [" << level << "] "
              << message << std::endl;
}

std::string formatBytes(std::uint64_t bytes) {
    static constexpr const char* units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
    long double value = static_cast<long double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0L && unit + 1u < std::size(units)) {
        value /= 1024.0L;
        ++unit;
    }
    std::ostringstream output;
    output << std::fixed << std::setprecision(unit == 0u ? 0 : 2) << value << ' ' << units[unit];
    return output.str();
}

#ifdef _WIN32
int lastSocketError() { return WSAGetLastError(); }
void closeSocket(SocketHandle socket) {
    if (socket != kInvalidSocket) { closesocket(socket); }
}
constexpr int kShutdownBoth = SD_BOTH;
#else
int lastSocketError() { return errno; }
void closeSocket(SocketHandle socket) {
    if (socket != kInvalidSocket) { close(socket); }
}
constexpr int kShutdownBoth = SHUT_RDWR;
#endif

bool isInterruptedError(int error) {
#ifdef _WIN32
    return error == WSAEINTR;
#else
    return error == EINTR;
#endif
}

bool isTimeoutError(int error) {
#ifdef _WIN32
    return error == WSAETIMEDOUT || error == WSAEWOULDBLOCK;
#else
    return error == EAGAIN || error == EWOULDBLOCK;
#endif
}

void configureSocketTimeouts(SocketHandle socket) {
#ifdef _WIN32
    const DWORD timeoutMs = 1000u;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
#else
    const timeval timeout{ 1, 0 };
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
}

enum class IoResult { Ok, Closed, Error, Stopped, IdleTimeout };

IoResult receiveAll(SocketHandle socket, std::uint8_t* destination, std::size_t size) {
    std::size_t received = 0;
    auto lastProgress = std::chrono::steady_clock::now();
    while (received < size) {
        if (g_stopRequested.load()) { return IoResult::Stopped; }
        const std::size_t remaining = size - received;
        const int requestSize = static_cast<int>(std::min<std::size_t>(remaining,
                                                                       std::numeric_limits<int>::max()));
        const int count = recv(socket,
                               reinterpret_cast<char*>(destination + received),
                               requestSize,
                               0);
        if (count > 0) {
            received += static_cast<std::size_t>(count);
            lastProgress = std::chrono::steady_clock::now();
            continue;
        }
        if (count == 0) { return IoResult::Closed; }
        const int error = lastSocketError();
        if (isInterruptedError(error) || isTimeoutError(error)) {
            if (std::chrono::steady_clock::now() - lastProgress >=
                g_connectionIdleTimeout) {
                return IoResult::IdleTimeout;
            }
            continue;
        }
        return IoResult::Error;
    }
    return IoResult::Ok;
}

IoResult sendAll(SocketHandle socket, const std::uint8_t* source, std::size_t size) {
    std::size_t sent = 0;
    auto lastProgress = std::chrono::steady_clock::now();
    while (sent < size) {
        if (g_stopRequested.load()) { return IoResult::Stopped; }
        const std::size_t remaining = size - sent;
        const int requestSize = static_cast<int>(std::min<std::size_t>(remaining,
                                                                       std::numeric_limits<int>::max()));
#ifdef _WIN32
        constexpr int flags = 0;
#else
        constexpr int flags = MSG_NOSIGNAL;
#endif
        const int count = send(socket,
                               reinterpret_cast<const char*>(source + sent),
                               requestSize,
                               flags);
        if (count > 0) {
            sent += static_cast<std::size_t>(count);
            lastProgress = std::chrono::steady_clock::now();
            continue;
        }
        if (count == 0) { return IoResult::Closed; }
        const int error = lastSocketError();
        if (isInterruptedError(error) || isTimeoutError(error)) {
            if (std::chrono::steady_clock::now() - lastProgress >=
                g_connectionIdleTimeout) {
                return IoResult::IdleTimeout;
            }
            continue;
        }
        return IoResult::Error;
    }
    return IoResult::Ok;
}

bool sendFrame(SocketHandle socket,
               igpk::MessageType type,
               std::uint64_t requestId,
               const std::vector<std::uint8_t>& payload) {
    if (payload.size() > igpk::kMaxPayloadSize) { return false; }
    igpk::FrameHeader header;
    header.type = type;
    header.requestId = requestId;
    header.payloadSize = payload.size();
    const auto encodedHeader = igpk::encodeHeader(header);
    if (sendAll(socket, encodedHeader.data(), encodedHeader.size()) != IoResult::Ok) { return false; }
    return payload.empty() || sendAll(socket, payload.data(), payload.size()) == IoResult::Ok;
}

bool sendError(SocketHandle socket,
               std::uint64_t requestId,
               igpk::ErrorCode code,
               const std::string& message,
               const std::string& currentVersionToken = {}) {
    logLine("WARN", "request " + std::to_string(requestId) + ": " + message);
    igpk::ErrorResponse response;
    response.code = code;
    response.message = message;
    response.currentVersionToken = currentVersionToken;
    return sendFrame(socket, igpk::MessageType::Error, requestId,
                     igpk::encodeErrorResponse(response));
}

struct Options {
    fs::path filePath;
    fs::path rootPath;
    fs::path catalogCachePath;
    fs::path packSourceDirectory;
    std::string packageId;
    std::string bindAddress = "127.0.0.1";
    std::uint16_t port = 34567u;
    std::uint32_t idleTimeoutSeconds = 300u;
    int compressionLevel = 3;
    bool replacePackage = false;
    bool catalogCacheWasExplicit = false;
    std::string sha256;
    bool showHelp = false;
};

void printUsage(const char* executable) {
    std::cout
        << "Usage:\n  " << executable
        << " --file <archive.tar.zst> [--id <package-id>] [--bind <IPv4>]\n"
           "      [--port <1-65535>] [--sha256 <64-hex-digits>]\n"
           "  "
        << executable
        << " --pack <source-directory> --file <output.tar.zst> [--replace-package]\n"
           "      [--compression-level <1-19>] [the serving options above]\n\n"
           "  "
        << executable
        << " --root <package-directory> [--catalog-cache <index-file>]\n"
           "      [--bind 127.0.0.1] [--port <1-65535>]\n\n"
           "--file and --root are mutually exclusive. --root scans only direct regular\n"
           "*.tar.zst children and rejects symlinks/reparse points. The network accepts\n"
           "logical package IDs only, never paths. With --pack, --file is created before\n"
           "the listening socket opens.\n"
           "The default endpoint is 127.0.0.1:34567. --idle-timeout-seconds defaults\n"
           "to 300. Press Ctrl+C to stop.\n";
}

bool isHexSha256(const std::string& value) {
    return value.empty() ||
           (value.size() == 64u && std::all_of(value.begin(), value.end(), [](unsigned char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
            }));
}

std::string decodeHexSha256(const std::string& value) {
    if (value.empty()) { return {}; }
    auto nibble = [](unsigned char c) -> unsigned char {
        if (c >= '0' && c <= '9') { return static_cast<unsigned char>(c - '0'); }
        if (c >= 'a' && c <= 'f') { return static_cast<unsigned char>(c - 'a' + 10); }
        return static_cast<unsigned char>(c - 'A' + 10);
    };
    std::string bytes(32u, '\0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<char>((nibble(static_cast<unsigned char>(value[2u * i])) << 4u) |
                                     nibble(static_cast<unsigned char>(value[2u * i + 1u])));
    }
    return bytes;
}

bool validPackageId(const std::string& value) {
    return !value.empty() && value.size() <= 1024u && value != "." && value != ".." &&
           value.find('/') == std::string::npos && value.find('\\') == std::string::npos &&
           value.find('\0') == std::string::npos;
}

bool parsePort(const std::string& value, std::uint16_t& port) {
    try {
        std::size_t consumed = 0;
        const unsigned long parsed = std::stoul(value, &consumed, 10);
        if (consumed != value.size() || parsed == 0u || parsed > 65535u) { return false; }
        port = static_cast<std::uint16_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseCompressionLevel(const std::string& value, int& level) {
    try {
        std::size_t consumed = 0;
        const long parsed = std::stol(value, &consumed, 10);
        if (consumed != value.size() || parsed < 1 || parsed > 19) { return false; }
        level = static_cast<int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseIdleTimeoutSeconds(const std::string& value, std::uint32_t& seconds) {
    try {
        std::size_t consumed = 0;
        const unsigned long parsed = std::stoul(value, &consumed, 10);
        if (consumed != value.size() || parsed == 0u || parsed > 86400u) { return false; }
        seconds = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseArguments(const std::vector<std::string>& arguments,
                    Options& options,
                    std::string& error) {
    std::string fileArgument;
    std::string rootArgument;
    std::string cacheArgument;
    std::string packArgument;
    bool sawFile = false;
    bool sawRoot = false;
    for (std::size_t i = 1; i < arguments.size(); ++i) {
        const std::string& argument = arguments[i];
        if (argument == "--help" || argument == "-h") {
            options.showHelp = true;
            return true;
        }
        auto requireValue = [&](const char* name) -> const std::string* {
            if (i + 1u >= arguments.size()) {
                error = std::string(name) + " requires a value";
                return nullptr;
            }
            return &arguments[++i];
        };
        if (argument == "--file") {
            const auto* value = requireValue("--file");
            if (!value) { return false; }
            if (sawFile) {
                error = "--file may be specified only once";
                return false;
            }
            sawFile = true;
            fileArgument = *value;
        } else if (argument == "--root") {
            const auto* value = requireValue("--root");
            if (!value) { return false; }
            if (sawRoot) {
                error = "--root may be specified only once";
                return false;
            }
            sawRoot = true;
            rootArgument = *value;
        } else if (argument == "--catalog-cache") {
            const auto* value = requireValue("--catalog-cache");
            if (!value) { return false; }
            if (!cacheArgument.empty()) {
                error = "--catalog-cache may be specified only once";
                return false;
            }
            cacheArgument = *value;
            options.catalogCacheWasExplicit = true;
        } else if (argument == "--pack") {
            const auto* value = requireValue("--pack");
            if (!value) { return false; }
            packArgument = *value;
        } else if (argument == "--replace-package") {
            options.replacePackage = true;
        } else if (argument == "--compression-level") {
            const auto* value = requireValue("--compression-level");
            if (!value) { return false; }
            if (!parseCompressionLevel(*value, options.compressionLevel)) {
                error = "--compression-level must be an integer in the range 1..19";
                return false;
            }
        } else if (argument == "--id") {
            const auto* value = requireValue("--id");
            if (!value) { return false; }
            options.packageId = *value;
        } else if (argument == "--bind") {
            const auto* value = requireValue("--bind");
            if (!value) { return false; }
            options.bindAddress = *value;
        } else if (argument == "--port") {
            const auto* value = requireValue("--port");
            if (!value) { return false; }
            if (!parsePort(*value, options.port)) {
                error = "--port must be an integer in the range 1..65535";
                return false;
            }
        } else if (argument == "--idle-timeout-seconds") {
            const auto* value = requireValue("--idle-timeout-seconds");
            if (!value) { return false; }
            if (!parseIdleTimeoutSeconds(*value, options.idleTimeoutSeconds)) {
                error = "--idle-timeout-seconds must be an integer in the range 1..86400";
                return false;
            }
        } else if (argument == "--sha256") {
            const auto* value = requireValue("--sha256");
            if (!value) { return false; }
            options.sha256 = *value;
        } else {
            error = "unknown argument: " + argument;
            return false;
        }
    }

    if (sawFile == sawRoot) {
        error = "exactly one of --file or --root is required";
        return false;
    }
    std::error_code ec;
    if (sawRoot) {
        if (rootArgument.empty()) {
            error = "--root requires a non-empty path";
            return false;
        }
        if (!packArgument.empty() || options.replacePackage || !options.packageId.empty() ||
            !options.sha256.empty()) {
            error = "--pack, --replace-package, --id, and --sha256 are valid only with --file";
            return false;
        }
        options.rootPath = fs::absolute(utf8ToPath(rootArgument), ec);
        if (ec) {
            error = "failed to resolve --root";
            return false;
        }
        options.rootPath = options.rootPath.lexically_normal();
        if (!fs::is_directory(options.rootPath, ec) || ec) {
            error = "--root must name an existing directory";
            return false;
        }
        if (cacheArgument.empty()) {
            options.catalogCachePath = options.rootPath / ".igamevis-catalog-v2.idx";
        } else {
            options.catalogCachePath = fs::absolute(utf8ToPath(cacheArgument), ec);
            if (ec) {
                error = "failed to resolve --catalog-cache";
                return false;
            }
            options.catalogCachePath = options.catalogCachePath.lexically_normal();
        }
    } else {
        if (fileArgument.empty()) {
            error = "--file requires a non-empty path";
            return false;
        }
        if (options.catalogCacheWasExplicit) {
            error = "--catalog-cache is valid only with --root";
            return false;
        }
        options.filePath = fs::absolute(utf8ToPath(fileArgument), ec);
        if (ec) {
            error = "failed to resolve --file";
            return false;
        }
        options.filePath = options.filePath.lexically_normal();

        if (packArgument.empty()) {
            igame::data_server::PackageFileState state;
            if (!igame::data_server::querySafeRegularFile(options.filePath, state, error)) {
                error = "--file must name a safe existing regular file: " + error;
                return false;
            }
            options.filePath = fs::canonical(options.filePath, ec);
            if (ec) {
                error = "failed to canonicalize --file";
                return false;
            }
            if (options.replacePackage) {
                error = "--replace-package is valid only with --pack";
                return false;
            }
        } else {
            options.packSourceDirectory = fs::absolute(utf8ToPath(packArgument), ec);
            if (ec || !fs::is_directory(options.packSourceDirectory, ec) || ec) {
                error = "--pack must name an existing directory";
                return false;
            }
            options.packSourceDirectory = fs::weakly_canonical(options.packSourceDirectory, ec);
            if (ec) {
                error = "failed to canonicalize --pack";
                return false;
            }
        }

        if (options.packageId.empty()) { options.packageId = pathToUtf8(options.filePath.filename()); }
        if (!validPackageId(options.packageId)) {
            error = "--id must be non-empty and must not contain path separators";
            return false;
        }
        if (!isHexSha256(options.sha256)) {
            error = "--sha256 must contain exactly 64 hexadecimal digits";
            return false;
        }
        options.sha256 = decodeHexSha256(options.sha256);
    }
    if (options.bindAddress == "localhost") { options.bindAddress = "127.0.0.1"; }
    if (options.bindAddress != "127.0.0.1") {
        error = "--bind is restricted to the loopback address 127.0.0.1";
        return false;
    }
    return true;
}

bool createPackageIfRequested(const Options& options, std::string& error) {
    if (options.packSourceDirectory.empty()) { return true; }

    using iGame::data_transfer::ArchivePhase;
    using iGame::data_transfer::ArchiveProgress;
    using iGame::data_transfer::TarZstdArchive;
    using iGame::data_transfer::TarZstdCreateOptions;

    std::uint64_t bytesAtLastLog = 0;
    std::uint64_t entriesAtLastScanLog = 0;
    unsigned int percentAtLastLog = std::numeric_limits<unsigned int>::max();
    bool finalizingLogged = false;

    TarZstdCreateOptions createOptions;
    createOptions.compressionLevel = options.compressionLevel;
    createOptions.replaceExistingArchive = options.replacePackage;
    createOptions.progress = [&](const ArchiveProgress& progress) {
        if (g_stopRequested.load()) { return false; }
        if (progress.phase == ArchivePhase::Scanning) {
            if (progress.entriesProcessed == 1u ||
                progress.entriesProcessed - entriesAtLastScanLog >= 128u) {
                logLine("INFO", "packing scan: " + std::to_string(progress.entriesProcessed) +
                                    " entries, " + formatBytes(progress.contentBytesTotal));
                entriesAtLastScanLog = progress.entriesProcessed;
            }
        } else if (progress.phase == ArchivePhase::Compressing) {
            const unsigned int percent = progress.contentBytesTotal == 0u
                                             ? 100u
                                             : static_cast<unsigned int>(
                                                   static_cast<long double>(progress.contentBytesProcessed) *
                                                   100.0L /
                                                   static_cast<long double>(progress.contentBytesTotal));
            const bool first = percentAtLastLog == std::numeric_limits<unsigned int>::max();
            const bool percentAdvanced = !first && percent >= percentAtLastLog + 1u;
            const bool byteInterval =
                progress.contentBytesProcessed - bytesAtLastLog >= 256u * 1024u * 1024u;
            const bool complete = progress.contentBytesProcessed >= progress.contentBytesTotal;
            if (first || percentAdvanced || byteInterval ||
                (complete && percentAtLastLog != 100u)) {
                logLine("INFO", "packing progress: " +
                                    formatBytes(progress.contentBytesProcessed) + " / " +
                                    formatBytes(progress.contentBytesTotal) + " (" +
                                    std::to_string(std::min(percent, 100u)) + "%), archive " +
                                    formatBytes(progress.archiveBytesProcessed));
                percentAtLastLog = percent;
                bytesAtLastLog = progress.contentBytesProcessed;
            }
        } else if (progress.phase == ArchivePhase::Finalizing && !finalizingLogged) {
            logLine("INFO", "packing: finalizing tar.zst archive");
            finalizingLogged = true;
        }
        return !g_stopRequested.load();
    };

    logLine("INFO", "packing source " + pathToUtf8(options.packSourceDirectory) +
                        " -> " + pathToUtf8(options.filePath) +
                        " at zstd level " + std::to_string(options.compressionLevel));
    const auto result = TarZstdArchive::Create(options.packSourceDirectory,
                                               options.filePath,
                                               createOptions);
    if (!result.success) {
        error = result.cancelled ? "package creation was cancelled" : result.error;
        return false;
    }
    logLine("INFO", "package created: " + std::to_string(result.entryCount) +
                        " entries, " + formatBytes(result.contentBytes) + " -> " +
                        formatBytes(result.archiveBytes));
    return true;
}

std::string digestToHex(const std::string& digest) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2u);
    for (unsigned char byte : digest) {
        result.push_back(digits[byte >> 4u]);
        result.push_back(digits[byte & 0x0fu]);
    }
    return result;
}

bool computeAndVerifyPackageSha256(Options& options, std::string& error) {
    igame::data_server::PackageFileState safeBefore;
    if (!igame::data_server::querySafeRegularFile(options.filePath, safeBefore, error)) {
        error = "configured package is unsafe before SHA-256: " + error;
        return false;
    }
    std::error_code systemError;
    const std::uint64_t sizeBefore = fs::file_size(options.filePath, systemError);
    if (systemError) {
        error = "failed to query package before SHA-256: " + systemError.message();
        return false;
    }
    const auto mtimeBefore = fs::last_write_time(options.filePath, systemError);
    if (systemError) {
        error = "failed to query package mtime before SHA-256: " + systemError.message();
        return false;
    }

    std::ifstream input(options.filePath, std::ios::binary);
    if (!input) {
        error = "failed to open package for SHA-256";
        return false;
    }
    igpk::Sha256 hash;
    std::vector<std::uint8_t> buffer(4u * 1024u * 1024u);
    std::uint64_t processed = 0;
    std::uint64_t bytesAtLastLog = 0;
    unsigned int percentAtLastLog = std::numeric_limits<unsigned int>::max();
    logLine("INFO", "computing package SHA-256 over " + formatBytes(sizeBefore));
    while (input) {
        if (g_stopRequested.load()) {
            error = "SHA-256 calculation was cancelled";
            return false;
        }
        input.read(reinterpret_cast<char*>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count > 0) {
            hash.update(buffer.data(), static_cast<std::size_t>(count));
            processed += static_cast<std::uint64_t>(count);
        }
        if (input.bad()) {
            error = "I/O error while computing package SHA-256";
            return false;
        }

        const unsigned int percent = sizeBefore == 0u
                                         ? 100u
                                         : static_cast<unsigned int>(
                                               static_cast<long double>(processed) * 100.0L /
                                               static_cast<long double>(sizeBefore));
        const bool first = percentAtLastLog == std::numeric_limits<unsigned int>::max();
        const bool percentAdvanced = !first && percent >= percentAtLastLog + 5u;
        const bool byteInterval = processed - bytesAtLastLog >= 512u * 1024u * 1024u;
        const bool complete = processed >= sizeBefore;
        if (first || percentAdvanced || byteInterval ||
            (complete && percentAtLastLog != 100u)) {
            logLine("INFO", "SHA-256 progress: " + formatBytes(processed) + " / " +
                                formatBytes(sizeBefore) + " (" +
                                std::to_string(std::min(percent, 100u)) + "%)");
            percentAtLastLog = percent;
            bytesAtLastLog = processed;
        }
    }
    if (processed != sizeBefore) {
        error = "package size changed while computing SHA-256";
        return false;
    }

    const auto digestArray = hash.finalize();
    const std::string computed(reinterpret_cast<const char*>(digestArray.data()),
                               digestArray.size());
    const std::uint64_t sizeAfter = fs::file_size(options.filePath, systemError);
    if (systemError) {
        error = "failed to query package after SHA-256: " + systemError.message();
        return false;
    }
    const auto mtimeAfter = fs::last_write_time(options.filePath, systemError);
    if (systemError || sizeAfter != sizeBefore || mtimeAfter != mtimeBefore) {
        error = "package changed while computing SHA-256";
        return false;
    }
    igame::data_server::PackageFileState safeAfter;
    if (!igame::data_server::querySafeRegularFile(options.filePath, safeAfter, error) ||
        !igame::data_server::samePackageFileState(safeAfter, safeBefore)) {
        if (error.empty()) { error = "package changed while computing SHA-256"; }
        return false;
    }
    if (!options.sha256.empty() && options.sha256 != computed) {
        error = "--sha256 does not match the configured package";
        return false;
    }
    options.sha256 = computed;
    logLine("INFO", "package SHA-256 " + digestToHex(options.sha256));
    return true;
}

using igame::data_server::PackageCatalog;
using igame::data_server::PackageFileState;
using igame::data_server::PackageRecord;

std::string makeLegacyVersionToken(std::uint64_t fileSize, std::int64_t mtimeTicks) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << fileSize << '-'
           << std::setw(16) << static_cast<std::uint64_t>(mtimeTicks);
    return output.str();
}

std::uint64_t revisionFromDigest(const std::string& digest) {
    std::uint64_t revision = 0;
    for (std::size_t i = 0; i < std::min<std::size_t>(8u, digest.size()); ++i) {
        revision = (revision << 8u) | static_cast<unsigned char>(digest[i]);
    }
    return revision == 0u ? 1u : revision;
}

class PackageRegistry {
public:
    explicit PackageRegistry(PackageRecord package)
        : m_revision(revisionFromDigest(package.sha256)) {
        m_legacy.emplace(package.packageId, std::move(package));
    }

    explicit PackageRegistry(std::unique_ptr<PackageCatalog> catalog)
        : m_catalog(std::move(catalog)) {}

    const std::map<std::string, PackageRecord>& packages() const {
        return m_catalog ? m_catalog->packages() : m_legacy;
    }

    const PackageRecord* find(const std::string& packageId) const {
        if (m_catalog) { return m_catalog->find(packageId); }
        const auto found = m_legacy.find(packageId);
        return found == m_legacy.end() ? nullptr : &found->second;
    }

    bool refresh(std::string& error) {
        return !m_catalog || m_catalog->refresh(error);
    }

    std::uint64_t revision() const {
        return m_catalog ? m_catalog->revision() : m_revision;
    }

    bool rootMode() const { return static_cast<bool>(m_catalog); }

private:
    std::map<std::string, PackageRecord> m_legacy;
    std::unique_ptr<PackageCatalog> m_catalog;
    std::uint64_t m_revision = 1u;
};

enum class PackageStateResult { Current, Stale, Error };

PackageStateResult verifyPackageState(const PackageRecord& package, std::string& error) {
    PackageFileState current;
    if (!igame::data_server::querySafeRegularFile(package.path, current, error)) {
        return PackageStateResult::Error;
    }
    if (!igame::data_server::samePackageFileState(current, package.state)) {
        error = "package changed; request a catalog refresh before downloading it";
        return PackageStateResult::Stale;
    }
    return PackageStateResult::Current;
}

struct ConnectionProgress {
    std::uint64_t bytesSent = 0;
    std::uint64_t highWater = 0;
    std::uint64_t bytesAtLastLog = 0;
    unsigned int percentAtLastLog = std::numeric_limits<unsigned int>::max();

    void record(std::uint64_t offset, std::uint64_t length, std::uint64_t total) {
        bytesSent += length;
        highWater = std::max(highWater, offset + length);
        const unsigned int percent = total == 0u
                                         ? 100u
                                         : static_cast<unsigned int>((static_cast<long double>(highWater) * 100.0L) /
                                                                     static_cast<long double>(total));
        const bool first = percentAtLastLog == std::numeric_limits<unsigned int>::max();
        const bool percentAdvanced = !first && percent >= percentAtLastLog + 1u;
        const bool byteInterval = bytesSent - bytesAtLastLog >= 256u * 1024u * 1024u;
        const bool complete = highWater >= total;
        if (first || percentAdvanced || byteInterval || complete) {
            std::ostringstream message;
            message << "transfer progress: high-water " << formatBytes(highWater) << " / "
                    << formatBytes(total) << " (" << std::min(percent, 100u) << "%), sent this connection "
                    << formatBytes(bytesSent);
            logLine("INFO", message.str());
            percentAtLastLog = percent;
            bytesAtLastLog = bytesSent;
        }
    }
};

class PinnedPackageFile {
public:
    PinnedPackageFile() = default;
    PinnedPackageFile(const PinnedPackageFile&) = delete;
    PinnedPackageFile& operator=(const PinnedPackageFile&) = delete;

    PinnedPackageFile(PinnedPackageFile&& other) noexcept { moveFrom(std::move(other)); }
    PinnedPackageFile& operator=(PinnedPackageFile&& other) noexcept {
        if (this != &other) {
            reset();
            moveFrom(std::move(other));
        }
        return *this;
    }

    ~PinnedPackageFile() { reset(); }

    bool open(const fs::path& path, std::string& error) {
        reset();
#ifdef _WIN32
        m_handle = CreateFileW(path.c_str(), GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                               nullptr);
        if (m_handle == INVALID_HANDLE_VALUE) {
            error = "CreateFileW failed with error " + std::to_string(GetLastError());
            return false;
        }
#else
        m_input.open(path, std::ios::binary);
        if (!m_input) {
            error = "failed to open package stream";
            return false;
        }
#endif
        return true;
    }

    bool readAt(std::uint64_t offset,
                std::uint8_t* destination,
                std::uint32_t length,
                std::string& error) {
#ifdef _WIN32
        LARGE_INTEGER position{};
        position.QuadPart = static_cast<LONGLONG>(offset);
        if (SetFilePointerEx(m_handle, position, nullptr, FILE_BEGIN) == 0) {
            error = "SetFilePointerEx failed with error " + std::to_string(GetLastError());
            return false;
        }
        std::uint32_t completed = 0;
        while (completed < length) {
            DWORD count = 0;
            if (ReadFile(m_handle, destination + completed, length - completed,
                         &count, nullptr) == 0) {
                error = "ReadFile failed with error " + std::to_string(GetLastError());
                return false;
            }
            if (count == 0u) {
                error = "short read while reading the package";
                return false;
            }
            completed += count;
        }
#else
        m_input.clear();
        m_input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!m_input) {
            error = "failed to seek to the requested offset";
            return false;
        }
        m_input.read(reinterpret_cast<char*>(destination),
                     static_cast<std::streamsize>(length));
        if (m_input.gcount() != static_cast<std::streamsize>(length)) {
            error = "short read while reading the package";
            return false;
        }
#endif
        return true;
    }

    bool isOpen() const {
#ifdef _WIN32
        return m_handle != INVALID_HANDLE_VALUE;
#else
        return m_input.is_open();
#endif
    }

    void reset() {
#ifdef _WIN32
        if (m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
#else
        m_input = std::ifstream{};
#endif
    }

private:
    void moveFrom(PinnedPackageFile&& other) noexcept {
#ifdef _WIN32
        m_handle = other.m_handle;
        other.m_handle = INVALID_HANDLE_VALUE;
#else
        m_input = std::move(other.m_input);
#endif
    }

#ifdef _WIN32
    HANDLE m_handle = INVALID_HANDLE_VALUE;
#else
    std::ifstream m_input;
#endif
};

struct TransferSession {
    std::string packageId;
    std::string versionToken;
    std::uint64_t fileSize = 0;
    PinnedPackageFile input;

    bool matches(const igpk::GetRequest& request) const {
        return input.isOpen() && packageId == request.packageId &&
               versionToken == request.versionToken;
    }

    void reset() {
        input.reset();
        packageId.clear();
        versionToken.clear();
        fileSize = 0;
    }
};

bool handleInfo(SocketHandle client,
                std::uint64_t requestId,
                const std::vector<std::uint8_t>& payload,
                const PackageRegistry& packages) {
    igpk::InfoRequest request;
    std::string error;
    if (!igpk::decodeInfoRequest(payload, request, error)) {
        return sendError(client, requestId, igpk::ErrorCode::BadRequest, error);
    }
    const PackageRecord* package = packages.find(request.packageId);
    if (!package) {
        return sendError(client, requestId, igpk::ErrorCode::PackageNotFound,
                         "unknown package id");
    }
    igpk::InfoResponse response;
    response.packageId = package->packageId;
    response.fileName = package->fileName;
    if (packages.rootMode()) {
        const PackageStateResult state = verifyPackageState(*package, error);
        if (state == PackageStateResult::Error) {
            return sendError(client, requestId, igpk::ErrorCode::IoError, error);
        }
        if (state == PackageStateResult::Stale) {
            return sendError(client, requestId, igpk::ErrorCode::StaleVersion, error);
        }
        response.fileSize = package->state.fileSize;
        response.mtimeTicks = package->state.mtimeTicks;
        response.versionToken = package->versionToken;
        response.sha256 = package->sha256;
    } else {
        PackageFileState current;
        if (!igame::data_server::querySafeRegularFile(package->path, current, error)) {
            return sendError(client, requestId, igpk::ErrorCode::IoError, error);
        }
        response.fileSize = current.fileSize;
        response.mtimeTicks = current.mtimeTicks;
        response.versionToken = makeLegacyVersionToken(current.fileSize, current.mtimeTicks);
        if (igame::data_server::samePackageFileState(current, package->state)) {
            response.sha256 = package->sha256;
        }
    }
    logLine("INFO", "INFO " + response.packageId + ": " + formatBytes(response.fileSize) +
                        ", content token " + response.versionToken);
    return sendFrame(client, igpk::MessageType::InfoResponse, requestId,
                     igpk::encodeInfoResponse(response));
}

bool handleGet(SocketHandle client,
               std::uint64_t requestId,
               const std::vector<std::uint8_t>& payload,
               const PackageRegistry& packages,
               ConnectionProgress& progress,
               TransferSession& session) {
    igpk::GetRequest request;
    std::string error;
    if (!igpk::decodeGetRequest(payload, request, error)) {
        return sendError(client, requestId, igpk::ErrorCode::BadRequest, error);
    }
    if (request.requestedLength == 0u || request.requestedLength > igpk::kMaxChunkSize) {
        return sendError(client, requestId, igpk::ErrorCode::InvalidRange,
                         "requestedLength must be in the range 1.." +
                             std::to_string(igpk::kMaxChunkSize));
    }

    if (!session.matches(request)) {
        const PackageRecord* package = packages.find(request.packageId);
        if (!package) {
            return sendError(client, requestId, igpk::ErrorCode::PackageNotFound,
                             "unknown package id");
        }
        PackageFileState openedState;
        std::string currentToken;
        if (packages.rootMode()) {
            const PackageStateResult beforeOpen = verifyPackageState(*package, error);
            if (beforeOpen == PackageStateResult::Error) {
                return sendError(client, requestId, igpk::ErrorCode::IoError, error);
            }
            if (beforeOpen == PackageStateResult::Stale) {
                return sendError(client, requestId, igpk::ErrorCode::StaleVersion, error);
            }
            openedState = package->state;
            currentToken = package->versionToken;
        } else {
            if (!igame::data_server::querySafeRegularFile(package->path, openedState, error)) {
                return sendError(client, requestId, igpk::ErrorCode::IoError, error);
            }
            currentToken = makeLegacyVersionToken(openedState.fileSize,
                                                  openedState.mtimeTicks);
        }
        if (request.versionToken != currentToken) {
            return sendError(client, requestId, igpk::ErrorCode::StaleVersion,
                             "package version does not match; request INFO again",
                             currentToken);
        }

        PinnedPackageFile opened;
        if (!opened.open(package->path, error)) {
            return sendError(client, requestId, igpk::ErrorCode::IoError,
                             "failed to open the configured package: " + error);
        }
        PackageFileState afterOpen;
        if (!igame::data_server::querySafeRegularFile(package->path, afterOpen, error)) {
            return sendError(client, requestId, igpk::ErrorCode::IoError, error);
        }
        if (!igame::data_server::samePackageFileState(afterOpen, openedState)) {
            return sendError(client, requestId, igpk::ErrorCode::StaleVersion,
                             "package changed while opening the transfer session");
        }

        if (progress.bytesSent != 0u) {
            logLine("INFO", "switching transfer session after sending " +
                                formatBytes(progress.bytesSent));
        }
        progress = ConnectionProgress{};
        session.reset();
        session.packageId = package->packageId;
        session.versionToken = currentToken;
        session.fileSize = openedState.fileSize;
        session.input = std::move(opened);
        logLine("INFO", "pinned transfer session for " + session.packageId +
                            ", token " + session.versionToken);
    }

    if (request.offset >= session.fileSize) {
        return sendError(client, requestId, igpk::ErrorCode::InvalidRange,
                         "offset is outside the package");
    }
    if (request.offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        return sendError(client, requestId, igpk::ErrorCode::InvalidRange,
                         "offset exceeds the server stream limit");
    }

    const std::uint64_t remaining = session.fileSize - request.offset;
    const auto length = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(remaining, request.requestedLength));
    std::vector<std::uint8_t> data(length);
    if (!session.input.readAt(request.offset, data.data(), length, error)) {
        session.reset();
        return sendError(client, requestId, igpk::ErrorCode::IoError, error);
    }

    const auto response = igpk::encodeDataChunk(request.offset, data.data(), length);
    if (!sendFrame(client, igpk::MessageType::DataChunk, requestId, response)) { return false; }
    progress.record(request.offset, length, session.fileSize);
    return true;
}

bool handleCatalog(SocketHandle client,
                   std::uint64_t requestId,
                   const std::vector<std::uint8_t>& payload,
                   PackageRegistry& packages) {
    igpk::CatalogRequest request;
    std::string error;
    if (!igpk::decodeCatalogRequest(payload, request, error)) {
        return sendError(client, requestId, igpk::ErrorCode::BadRequest, error);
    }
    if (!packages.rootMode()) {
        return sendError(client, requestId, igpk::ErrorCode::BadRequest,
                         "CATALOG_REQUEST requires server --root mode");
    }
    if (request.forceRefresh() && !packages.refresh(error)) {
        return sendError(client, requestId, igpk::ErrorCode::IoError,
                         "catalog refresh failed: " + error);
    }

    const std::uint64_t revision = packages.revision();
    if ((request.cursor != 0u && request.revision == 0u) ||
        (request.revision != 0u && request.revision != revision)) {
        return sendError(client, requestId, igpk::ErrorCode::StaleVersion,
                         "catalog revision changed; restart pagination at cursor zero");
    }
    const auto& available = packages.packages();
    if (available.size() > std::numeric_limits<std::uint32_t>::max()) {
        return sendError(client, requestId, igpk::ErrorCode::InternalError,
                         "catalog contains too many packages for a UInt32 cursor");
    }
    if (request.cursor == igpk::kCatalogEndCursor ||
        request.cursor > static_cast<std::uint32_t>(available.size())) {
        return sendError(client, requestId, igpk::ErrorCode::InvalidRange,
                         "catalog cursor is outside the current catalog");
    }

    igpk::CatalogResponse response;
    response.revision = revision;
    auto current = available.begin();
    std::advance(current, request.cursor);
    const std::size_t remaining = available.size() - request.cursor;
    const std::size_t count = std::min<std::size_t>(remaining, request.pageSize);
    for (std::size_t i = 0; i < count; ++i, ++current) {
        const PackageRecord& package = current->second;
        igpk::CatalogEntry entry;
        entry.fileSize = package.state.fileSize;
        entry.mtimeTicks = package.state.mtimeTicks;
        entry.packageId = package.packageId;
        entry.displayName = package.displayName;
        entry.fileName = package.fileName;
        entry.versionToken = package.versionToken;
        entry.sha256 = package.sha256;
        response.entries.push_back(std::move(entry));
    }
    const std::uint32_t following = request.cursor + static_cast<std::uint32_t>(count);
    response.nextCursor = following < available.size() ? following : igpk::kCatalogEndCursor;
    logLine("INFO", "CATALOG revision " + std::to_string(revision) + ", cursor " +
                        std::to_string(request.cursor) + ", returned " +
                        std::to_string(response.entries.size()));
    return sendFrame(client, igpk::MessageType::CatalogResponse, requestId,
                     igpk::encodeCatalogResponse(response));
}

bool waitForReadable(SocketHandle socket, int timeoutMilliseconds) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(socket, &readSet);
    timeval timeout{};
    timeout.tv_sec = timeoutMilliseconds / 1000;
    timeout.tv_usec = (timeoutMilliseconds % 1000) * 1000;
    const int result = select(static_cast<int>(socket) + 1, &readSet, nullptr, nullptr, &timeout);
    if (result < 0 && isInterruptedError(lastSocketError())) { return false; }
    return result > 0 && FD_ISSET(socket, &readSet);
}

void serveClient(SocketHandle client, PackageRegistry& packages) {
    ConnectionProgress progress;
    TransferSession transferSession;
    auto idleSince = std::chrono::steady_clock::now();
    while (!g_stopRequested.load()) {
        if (!waitForReadable(client, 500)) {
            if (std::chrono::steady_clock::now() - idleSince >=
                g_connectionIdleTimeout) {
                logLine("INFO", "closing idle client connection after " +
                                    std::to_string(
                                        std::chrono::duration_cast<std::chrono::seconds>(
                                            g_connectionIdleTimeout).count()) +
                                    " seconds");
                break;
            }
            continue;
        }

        std::array<std::uint8_t, igpk::kHeaderSize> headerBytes{};
        const IoResult headerResult = receiveAll(client, headerBytes.data(), headerBytes.size());
        if (headerResult != IoResult::Ok) {
            if (headerResult == IoResult::IdleTimeout) {
                logLine("INFO", "closing client that stalled while sending a frame header");
            }
            break;
        }

        igpk::FrameHeader header;
        std::string error;
        if (!igpk::decodeHeader(headerBytes, header, error)) {
            logLine("WARN", "invalid frame header: " + error + "; closing connection");
            break;
        }
        std::vector<std::uint8_t> payload(static_cast<std::size_t>(header.payloadSize));
        if (!payload.empty()) {
            const IoResult payloadResult = receiveAll(client, payload.data(), payload.size());
            if (payloadResult != IoResult::Ok) {
                if (payloadResult == IoResult::IdleTimeout) {
                    logLine("INFO", "closing client that stalled while sending a frame payload");
                }
                break;
            }
        }

        if (header.version != igpk::kProtocolVersion) {
            if (!sendError(client, header.requestId, igpk::ErrorCode::UnsupportedProtocol,
                           "unsupported protocol version " + std::to_string(header.version))) {
                break;
            }
            continue;
        }

        bool keepConnection = true;
        try {
            switch (header.type) {
            case igpk::MessageType::InfoRequest:
                keepConnection = handleInfo(client, header.requestId, payload, packages);
                break;
            case igpk::MessageType::GetRequest:
                keepConnection = handleGet(client, header.requestId, payload, packages,
                                           progress, transferSession);
                break;
            case igpk::MessageType::CatalogRequest:
                keepConnection = handleCatalog(client, header.requestId, payload, packages);
                break;
            case igpk::MessageType::Goodbye:
                if (!payload.empty()) {
                    keepConnection = sendError(client, header.requestId,
                                               igpk::ErrorCode::BadRequest,
                                               "GOODBYE payload must be empty");
                } else {
                    logLine("INFO", "client requested a clean disconnect");
                    return;
                }
                break;
            default:
                keepConnection = sendError(client, header.requestId,
                                           igpk::ErrorCode::BadRequest,
                                           "message type is not valid for a client request");
                break;
            }
        } catch (const std::exception& exception) {
            keepConnection = sendError(client, header.requestId, igpk::ErrorCode::InternalError,
                                       std::string("request failed: ") + exception.what());
        }
        if (!keepConnection) { break; }
        idleSince = std::chrono::steady_clock::now();
    }
    if (progress.bytesSent != 0u) {
        logLine("INFO", "connection transferred " + formatBytes(progress.bytesSent));
    }
}

class SocketRuntime {
public:
    SocketRuntime() {
#ifdef _WIN32
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
        m_initialized = true;
#endif
    }
    ~SocketRuntime() {
#ifdef _WIN32
        if (m_initialized) { WSACleanup(); }
#endif
    }

private:
    bool m_initialized = false;
};

int runServer(Options options) {
    g_connectionIdleTimeout = std::chrono::seconds(options.idleTimeoutSeconds);
    std::string packageError;
    std::unique_ptr<PackageRegistry> packages;
    if (!options.rootPath.empty()) {
        auto catalog = std::make_unique<PackageCatalog>(
            options.rootPath,
            options.catalogCachePath,
            options.catalogCacheWasExplicit,
            [](const char* level, const std::string& message) { logLine(level, message); },
            [] { return g_stopRequested.load(); });
        if (!catalog->initialize(packageError)) {
            if (g_stopRequested.load()) {
                logLine("INFO", packageError);
                return 0;
            }
            throw std::runtime_error("failed to initialize catalog: " + packageError);
        }
        packages = std::make_unique<PackageRegistry>(std::move(catalog));
        logLine("INFO", "serving catalog root " + pathToUtf8(options.rootPath));
    } else {
        if (!createPackageIfRequested(options, packageError)) {
            if (g_stopRequested.load()) {
                logLine("INFO", packageError);
                return 0;
            }
            throw std::runtime_error("failed to create package: " + packageError);
        }
        if (g_stopRequested.load()) {
            logLine("INFO", "stop requested before opening the listening socket");
            return 0;
        }

        if (!computeAndVerifyPackageSha256(options, packageError)) {
            if (g_stopRequested.load()) {
                logLine("INFO", packageError);
                return 0;
            }
            throw std::runtime_error(packageError);
        }
        PackageRecord package;
        package.path = fs::canonical(options.filePath);
        package.packageId = options.packageId;
        package.fileName = pathToUtf8(package.path.filename());
        package.displayName = package.fileName;
        constexpr std::size_t suffixSize = 8u;
        if (package.displayName.size() > suffixSize &&
            package.displayName.compare(package.displayName.size() - suffixSize,
                                        suffixSize, ".tar.zst") == 0) {
            package.displayName.resize(package.displayName.size() - suffixSize);
        }
        if (!igame::data_server::querySafeRegularFile(package.path, package.state, packageError)) {
            throw std::runtime_error(packageError);
        }
        package.sha256 = options.sha256;
        package.versionToken = makeLegacyVersionToken(package.state.fileSize,
                                                      package.state.mtimeTicks);
        logLine("INFO", "serving only " + pathToUtf8(package.path));
        logLine("INFO", "package id " + package.packageId + ", size " +
                            formatBytes(package.state.fileSize) + ", content token " +
                            package.versionToken);
        packages = std::make_unique<PackageRegistry>(std::move(package));
    }

    SocketRuntime socketRuntime;

    SocketHandle listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == kInvalidSocket) {
        throw std::runtime_error("failed to create listening socket, error " +
                                 std::to_string(lastSocketError()));
    }

    const int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(options.port);
    if (inet_pton(AF_INET, options.bindAddress.c_str(), &address.sin_addr) != 1) {
        closeSocket(listener);
        throw std::runtime_error("--bind must be a numeric IPv4 address or localhost");
    }
    if (bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        const int error = lastSocketError();
        closeSocket(listener);
        throw std::runtime_error("bind failed, error " + std::to_string(error));
    }
    if (listen(listener, 8) != 0) {
        const int error = lastSocketError();
        closeSocket(listener);
        throw std::runtime_error("listen failed, error " + std::to_string(error));
    }

    logLine("INFO", "READY listening on " + options.bindAddress + ':' +
                        std::to_string(options.port) + "; packages=" +
                        std::to_string(packages->packages().size()) + "; revision=" +
                        std::to_string(packages->revision()) + "; press Ctrl+C to stop");

    while (!g_stopRequested.load()) {
        if (!waitForReadable(listener, 500)) { continue; }
        sockaddr_in clientAddress{};
#ifdef _WIN32
        int clientAddressSize = sizeof(clientAddress);
#else
        socklen_t clientAddressSize = sizeof(clientAddress);
#endif
        SocketHandle client = accept(listener, reinterpret_cast<sockaddr*>(&clientAddress),
                                     &clientAddressSize);
        if (client == kInvalidSocket) {
            if (!g_stopRequested.load()) {
                logLine("WARN", "accept failed, error " + std::to_string(lastSocketError()));
            }
            continue;
        }
        configureSocketTimeouts(client);
        char clientIp[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIp, sizeof(clientIp));
        logLine("INFO", std::string("client connected from ") + clientIp + ':' +
                            std::to_string(ntohs(clientAddress.sin_port)));
        serveClient(client, *packages);
        shutdown(client, kShutdownBoth);
        closeSocket(client);
        logLine("INFO", "client disconnected");
    }

    shutdown(listener, kShutdownBoth);
    closeSocket(listener);
    logLine("INFO", "server stopped cleanly");
    return 0;
}

#ifdef _WIN32
std::string wideToUtf8(const wchar_t* value) {
    if (!value || *value == L'\0') { return {}; }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) { return {}; }
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}
#endif

int entryPoint(const std::vector<std::string>& arguments) {
    Options options;
    std::string error;
    if (!parseArguments(arguments, options, error)) {
        std::cerr << "Error: " << error << "\n\n";
        printUsage(arguments.empty() ? "iGameVisDataServer" : arguments.front().c_str());
        return 2;
    }
    if (options.showHelp) {
        printUsage(arguments.empty() ? "iGameVisDataServer" : arguments.front().c_str());
        return 0;
    }

#ifdef _WIN32
    SetConsoleCtrlHandler(consoleControlHandler, TRUE);
#else
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#endif
    try {
        return runServer(options);
    } catch (const std::exception& exception) {
        logLine("ERROR", exception.what());
        return 1;
    }
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
    std::vector<std::string> arguments;
    arguments.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) { arguments.push_back(wideToUtf8(argv[i])); }
    return entryPoint(arguments);
}
#else
int main(int argc, char* argv[]) {
    std::vector<std::string> arguments(argv, argv + argc);
    return entryPoint(arguments);
}
#endif
