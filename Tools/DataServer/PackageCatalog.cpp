#include "PackageCatalog.h"

#include "Sha256.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace igame::data_server {
namespace {

constexpr std::array<unsigned char, 8> kCacheMagic{
    'I', 'G', 'P', 'K', 'S', 'H', 'A', '3'
};
constexpr std::uintmax_t kMaxCacheBytes = 64u * 1024u * 1024u;
constexpr std::uint32_t kMaxCacheEntries = 1000000u;
constexpr const char* kArchiveSuffix = ".tar.zst";

std::string pathToUtf8(const fs::path& path) {
#if defined(__cpp_lib_char8_t)
    const std::u8string value = path.generic_u8string();
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
#else
    return path.generic_u8string();
#endif
}

bool endsWithArchiveSuffix(const std::string& value) {
    constexpr std::size_t suffixSize = 8u;
    return value.size() > suffixSize &&
           value.compare(value.size() - suffixSize, suffixSize, kArchiveSuffix) == 0;
}

bool validPackageId(const std::string& value) {
    return !value.empty() && value.size() <= std::numeric_limits<std::uint16_t>::max() &&
           value != "." && value != ".." &&
           value.find('/') == std::string::npos && value.find('\\') == std::string::npos &&
           value.find('\0') == std::string::npos;
}

bool isAsciiWhitespace(unsigned char value) {
    return value == ' ' || value == '\t' || value == '\n' || value == '\r' ||
           value == '\f' || value == '\v';
}

bool hasOuterAsciiWhitespace(const std::string& value) {
    return !value.empty() &&
           (isAsciiWhitespace(static_cast<unsigned char>(value.front())) ||
            isAsciiWhitespace(static_cast<unsigned char>(value.back())));
}

bool hasReparsePoint(const fs::path& path, std::string& error) {
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        error = "GetFileAttributesW failed with error " + std::to_string(GetLastError());
        return true;
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u;
#else
    std::error_code ec;
    const fs::file_status status = fs::symlink_status(path, ec);
    if (ec) {
        error = ec.message();
        return true;
    }
    return fs::is_symlink(status);
#endif
}

bool isSafeRegularPath(const fs::path& path, std::string& error) {
    std::error_code ec;
    const fs::file_status status = fs::symlink_status(path, ec);
    if (ec) {
        error = "failed to inspect path: " + ec.message();
        return false;
    }
    if (fs::is_symlink(status)) {
        error = "path is a symbolic link";
        return false;
    }
    if (!fs::is_regular_file(status)) {
        error = "path is not a regular file";
        return false;
    }
    std::string reparseError;
    if (hasReparsePoint(path, reparseError)) {
        error = reparseError.empty() ? "path is a reparse point"
                                     : "path is unsafe: " + reparseError;
        return false;
    }
    return true;
}

void writeU16(std::ostream& output, std::uint16_t value) {
    const std::array<char, 2> bytes{
        static_cast<char>((value >> 8u) & 0xffu), static_cast<char>(value & 0xffu)
    };
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void writeU32(std::ostream& output, std::uint32_t value) {
    std::array<char, 4> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<char>((value >> ((3u - i) * 8u)) & 0xffu);
    }
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void writeU64(std::ostream& output, std::uint64_t value) {
    std::array<char, 8> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<char>((value >> ((7u - i) * 8u)) & 0xffu);
    }
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

bool readU16(std::istream& input, std::uint16_t& value) {
    std::array<unsigned char, 2> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) { return false; }
    value = static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[0]) << 8u) | bytes[1]);
    return true;
}

bool readU32(std::istream& input, std::uint32_t& value) {
    std::array<unsigned char, 4> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) { return false; }
    value = 0;
    for (unsigned char byte : bytes) {
        value = static_cast<std::uint32_t>((value << 8u) | byte);
    }
    return true;
}

bool readU64(std::istream& input, std::uint64_t& value) {
    std::array<unsigned char, 8> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) { return false; }
    value = 0;
    for (unsigned char byte : bytes) {
        value = (value << 8u) | static_cast<std::uint64_t>(byte);
    }
    return true;
}

std::uint64_t processId() {
#ifdef _WIN32
    return static_cast<std::uint64_t>(GetCurrentProcessId());
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}

bool replaceFileAtomically(const fs::path& temporary,
                           const fs::path& destination,
                           std::string& error) {
#ifdef _WIN32
    if (MoveFileExW(temporary.c_str(), destination.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
        error = "MoveFileExW failed with error " + std::to_string(GetLastError());
        return false;
    }
    return true;
#else
    std::error_code ec;
    fs::rename(temporary, destination, ec);
    if (ec) {
        error = ec.message();
        return false;
    }
    return true;
#endif
}

void hashU64(igpk::Sha256& hash, std::uint64_t value) {
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>((value >> ((7u - i) * 8u)) & 0xffu);
    }
    hash.update(bytes.data(), bytes.size());
}

void hashString(igpk::Sha256& hash, const std::string& value) {
    hashU64(hash, static_cast<std::uint64_t>(value.size()));
    if (!value.empty()) {
        hash.update(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
    }
}

} // namespace

bool samePackageFileState(const PackageFileState& left,
                          const PackageFileState& right) {
    return left.fileSize == right.fileSize &&
           left.mtimeTicks == right.mtimeTicks &&
           left.identityDevice == right.identityDevice &&
           left.identityFile == right.identityFile &&
           left.identityCreation == right.identityCreation &&
           left.identityChange == right.identityChange;
}

bool querySafeRegularFile(const fs::path& path,
                          PackageFileState& state,
                          std::string& error) {
    if (!isSafeRegularPath(path, error)) { return false; }
    std::error_code ec;
    const std::uintmax_t size = fs::file_size(path, ec);
    if (ec) {
        error = "failed to query file size: " + ec.message();
        return false;
    }
    if (size > std::numeric_limits<std::uint64_t>::max()) {
        error = "file size exceeds UInt64";
        return false;
    }
    const auto modified = fs::last_write_time(path, ec);
    if (ec) {
        error = "failed to query modification time: " + ec.message();
        return false;
    }
    state.fileSize = static_cast<std::uint64_t>(size);
    state.mtimeTicks = static_cast<std::int64_t>(modified.time_since_epoch().count());
#ifdef _WIN32
    HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                                nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        error = "CreateFileW(FILE_READ_ATTRIBUTES) failed with error " +
                std::to_string(GetLastError());
        return false;
    }
    BY_HANDLE_FILE_INFORMATION information{};
    FILE_BASIC_INFO basic{};
    const BOOL informationOk = GetFileInformationByHandle(handle, &information);
    const BOOL basicOk = GetFileInformationByHandleEx(handle, FileBasicInfo,
                                                       &basic, sizeof(basic));
    const DWORD metadataError = (informationOk != 0 && basicOk != 0) ? ERROR_SUCCESS
                                                                     : GetLastError();
    CloseHandle(handle);
    if (metadataError != ERROR_SUCCESS) {
        error = "failed to query stable file identity, Windows error " +
                std::to_string(metadataError);
        return false;
    }
    if ((information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u ||
        (information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u) {
        error = "opened path is not a safe regular file";
        return false;
    }
    const std::uint64_t handleSize =
        (static_cast<std::uint64_t>(information.nFileSizeHigh) << 32u) |
        static_cast<std::uint64_t>(information.nFileSizeLow);
    if (handleSize != state.fileSize) {
        error = "file changed while querying metadata";
        return false;
    }
    state.identityDevice = static_cast<std::uint64_t>(information.dwVolumeSerialNumber);
    state.identityFile =
        (static_cast<std::uint64_t>(information.nFileIndexHigh) << 32u) |
        static_cast<std::uint64_t>(information.nFileIndexLow);
    state.identityCreation = basic.CreationTime.QuadPart;
    state.identityChange = basic.ChangeTime.QuadPart;
#else
    struct stat information {};
    if (lstat(path.c_str(), &information) != 0) {
        error = "lstat failed while querying stable file identity";
        return false;
    }
    if (!S_ISREG(information.st_mode)) {
        error = "opened path is not a safe regular file";
        return false;
    }
    if (information.st_size < 0 ||
        static_cast<std::uint64_t>(information.st_size) != state.fileSize) {
        error = "file changed while querying metadata";
        return false;
    }
    state.identityDevice = static_cast<std::uint64_t>(information.st_dev);
    state.identityFile = static_cast<std::uint64_t>(information.st_ino);
#if defined(__APPLE__)
    state.identityCreation = static_cast<std::int64_t>(information.st_ctimespec.tv_sec);
    state.identityChange = static_cast<std::int64_t>(information.st_ctimespec.tv_nsec);
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    state.identityCreation = static_cast<std::int64_t>(information.st_ctim.tv_sec);
    state.identityChange = static_cast<std::int64_t>(information.st_ctim.tv_nsec);
#else
    state.identityCreation = static_cast<std::int64_t>(information.st_ctime);
    state.identityChange = 0;
#endif
#endif
    return true;
}

std::string sha256ToHex(const std::string& digest) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2u);
    for (unsigned char byte : digest) {
        result.push_back(digits[byte >> 4u]);
        result.push_back(digits[byte & 0x0fu]);
    }
    return result;
}

PackageCatalog::PackageCatalog(fs::path root,
                               fs::path cachePath,
                               bool cachePathWasExplicit,
                               CatalogLog log,
                               StopRequested stopRequested)
    : m_root(std::move(root)),
      m_cachePath(std::move(cachePath)),
      m_cachePathWasExplicit(cachePathWasExplicit),
      m_log(std::move(log)),
      m_stopRequested(std::move(stopRequested)) {}

bool PackageCatalog::validateCacheLocation(std::string& error) const {
    const fs::path parent = m_cachePath.parent_path();
    std::error_code ec;
    if (parent.empty() || !fs::is_directory(parent, ec) || ec) {
        error = "catalog cache parent must be an existing directory";
        return false;
    }
    std::string parentError;
    if (hasReparsePoint(parent, parentError)) {
        error = "catalog cache parent must not be a symlink or reparse point";
        return false;
    }
    if (fs::exists(m_cachePath, ec)) {
        if (ec) {
            error = "failed to inspect catalog cache: " + ec.message();
            return false;
        }
        std::string pathError;
        if (!isSafeRegularPath(m_cachePath, pathError)) {
            error = "catalog cache is unsafe: " + pathError;
            return false;
        }
    } else if (ec) {
        error = "failed to inspect catalog cache: " + ec.message();
        return false;
    }
    return true;
}

bool PackageCatalog::initialize(std::string& error) {
    std::error_code ec;
    const fs::file_status rootStatus = fs::symlink_status(m_root, ec);
    if (ec || !fs::is_directory(rootStatus) || fs::is_symlink(rootStatus)) {
        error = "--root must be an existing non-symlink directory";
        return false;
    }
    std::string reparseError;
    if (hasReparsePoint(m_root, reparseError)) {
        error = "--root must not be a reparse point";
        return false;
    }
    const fs::path canonicalRoot = fs::canonical(m_root, ec);
    if (ec) {
        error = "failed to canonicalize --root: " + ec.message();
        return false;
    }
    m_root = canonicalRoot;
    if (!m_cachePathWasExplicit) {
        m_cachePath = (m_root / ".igamevis-catalog-v2.idx").lexically_normal();
    }

    std::string cacheError;
    if (!validateCacheLocation(cacheError)) {
        if (m_cachePathWasExplicit) {
            error = cacheError;
            return false;
        }
        if (m_log) { m_log("WARN", cacheError + "; continuing without a usable disk cache"); }
    } else {
        loadCache();
    }
    return refresh(error);
}

void PackageCatalog::loadCache() {
    std::error_code ec;
    if (!fs::exists(m_cachePath, ec) || ec) { return; }
    const std::uintmax_t fileSize = fs::file_size(m_cachePath, ec);
    if (ec || fileSize > kMaxCacheBytes) {
        if (m_log) { m_log("WARN", "ignoring oversized or unreadable catalog cache"); }
        return;
    }

    std::ifstream input(m_cachePath, std::ios::binary);
    if (!input) {
        if (m_log) { m_log("WARN", "failed to open catalog cache; packages will be rehashed"); }
        return;
    }
    std::array<unsigned char, kCacheMagic.size()> magic{};
    input.read(reinterpret_cast<char*>(magic.data()), static_cast<std::streamsize>(magic.size()));
    std::uint32_t rootLength = 0;
    std::uint32_t count = 0;
    if (!input || magic != kCacheMagic || !readU32(input, rootLength) ||
        !readU32(input, count) || rootLength > 1024u * 1024u || count > kMaxCacheEntries) {
        if (m_log) { m_log("WARN", "ignoring invalid catalog cache header"); }
        return;
    }
    std::string cachedRoot(rootLength, '\0');
    if (rootLength != 0u) {
        input.read(cachedRoot.data(), static_cast<std::streamsize>(cachedRoot.size()));
    }
    if (!input || cachedRoot != pathToUtf8(m_root)) {
        if (m_log) { m_log("WARN", "catalog cache belongs to a different canonical root; packages will be rehashed"); }
        return;
    }

    std::map<std::string, CachedDigest> loaded;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint16_t nameLength = 0;
        std::uint16_t reserved = 0;
        std::uint64_t size = 0;
        std::uint64_t mtimeBits = 0;
        std::uint64_t identityDevice = 0;
        std::uint64_t identityFile = 0;
        std::uint64_t identityCreation = 0;
        std::uint64_t identityChange = 0;
        if (!readU16(input, nameLength) || !readU16(input, reserved) || reserved != 0u ||
            !readU64(input, size) || !readU64(input, mtimeBits) ||
            !readU64(input, identityDevice) || !readU64(input, identityFile) ||
            !readU64(input, identityCreation) || !readU64(input, identityChange)) {
            loaded.clear();
            break;
        }
        CachedDigest entry;
        entry.state.fileSize = size;
        entry.state.mtimeTicks = static_cast<std::int64_t>(mtimeBits);
        entry.state.identityDevice = identityDevice;
        entry.state.identityFile = identityFile;
        entry.state.identityCreation = static_cast<std::int64_t>(identityCreation);
        entry.state.identityChange = static_cast<std::int64_t>(identityChange);
        entry.sha256.resize(32u);
        input.read(entry.sha256.data(), static_cast<std::streamsize>(entry.sha256.size()));
        std::string name(nameLength, '\0');
        if (nameLength != 0u) {
            input.read(name.data(), static_cast<std::streamsize>(name.size()));
        }
        if (!input || !validPackageId(name) || loaded.find(name) != loaded.end()) {
            loaded.clear();
            break;
        }
        loaded.emplace(std::move(name), std::move(entry));
    }
    if (loaded.size() != count || input.peek() != std::char_traits<char>::eof()) {
        if (m_log) { m_log("WARN", "ignoring corrupt catalog cache; packages will be rehashed"); }
        return;
    }
    m_cache = std::move(loaded);
    if (m_log) {
        m_log("INFO", "loaded " + std::to_string(m_cache.size()) + " SHA-256 entries from " +
                          pathToUtf8(m_cachePath));
    }
}

void PackageCatalog::saveCache(const std::map<std::string, CachedDigest>& entries) const {
    std::string locationError;
    if (!validateCacheLocation(locationError)) {
        if (m_log) { m_log("WARN", "catalog cache not written: " + locationError); }
        return;
    }

    const auto nonce = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    fs::path temporary = m_cachePath;
    temporary += ".tmp." + std::to_string(processId()) + "." + std::to_string(nonce);
    std::error_code ec;
    if (fs::exists(temporary, ec)) {
        if (m_log) { m_log("WARN", "catalog cache temporary path unexpectedly exists"); }
        return;
    }

    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        if (m_log) { m_log("WARN", "failed to create catalog cache temporary file"); }
        return;
    }
    output.write(reinterpret_cast<const char*>(kCacheMagic.data()),
                 static_cast<std::streamsize>(kCacheMagic.size()));
    const std::string canonicalRoot = pathToUtf8(m_root);
    if (canonicalRoot.size() > std::numeric_limits<std::uint32_t>::max()) {
        output.close();
        fs::remove(temporary, ec);
        if (m_log) { m_log("WARN", "canonical root is too long for the catalog cache"); }
        return;
    }
    writeU32(output, static_cast<std::uint32_t>(canonicalRoot.size()));
    writeU32(output, static_cast<std::uint32_t>(entries.size()));
    output.write(canonicalRoot.data(), static_cast<std::streamsize>(canonicalRoot.size()));
    for (const auto& item : entries) {
        writeU16(output, static_cast<std::uint16_t>(item.first.size()));
        writeU16(output, 0u);
        writeU64(output, item.second.state.fileSize);
        writeU64(output, static_cast<std::uint64_t>(item.second.state.mtimeTicks));
        writeU64(output, item.second.state.identityDevice);
        writeU64(output, item.second.state.identityFile);
        writeU64(output, static_cast<std::uint64_t>(item.second.state.identityCreation));
        writeU64(output, static_cast<std::uint64_t>(item.second.state.identityChange));
        output.write(item.second.sha256.data(), static_cast<std::streamsize>(item.second.sha256.size()));
        output.write(item.first.data(), static_cast<std::streamsize>(item.first.size()));
    }
    output.flush();
    const bool writeSucceeded = static_cast<bool>(output);
    output.close();
    if (!writeSucceeded || !output) {
        fs::remove(temporary, ec);
        if (m_log) { m_log("WARN", "failed while writing catalog cache temporary file"); }
        return;
    }

    if (fs::exists(m_cachePath, ec)) {
        std::string pathError;
        if (ec || !isSafeRegularPath(m_cachePath, pathError)) {
            fs::remove(temporary, ec);
            if (m_log) { m_log("WARN", "catalog cache destination became unsafe"); }
            return;
        }
    }
    std::string replaceError;
    if (!replaceFileAtomically(temporary, m_cachePath, replaceError)) {
        fs::remove(temporary, ec);
        if (m_log) { m_log("WARN", "failed to publish catalog cache atomically: " + replaceError); }
        return;
    }
    if (m_log) {
        m_log("INFO", "published SHA-256 index atomically at " + pathToUtf8(m_cachePath));
    }
}

bool PackageCatalog::hashStableFile(const fs::path& path,
                                    const PackageFileState& before,
                                    std::string& digest,
                                    std::string& error) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "failed to open archive for SHA-256";
        return false;
    }
    igpk::Sha256 hash;
    std::vector<std::uint8_t> buffer(4u * 1024u * 1024u);
    std::uint64_t processed = 0;
    while (input) {
        if (m_stopRequested && m_stopRequested()) {
            error = "catalog SHA-256 calculation was cancelled";
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
            error = "I/O error while hashing archive";
            return false;
        }
    }
    if (processed != before.fileSize) {
        error = "archive size changed while hashing";
        return false;
    }
    PackageFileState after;
    if (!querySafeRegularFile(path, after, error)) { return false; }
    if (!samePackageFileState(after, before)) {
        error = "archive changed while hashing";
        return false;
    }
    const auto bytes = hash.finalize();
    digest.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

std::uint64_t PackageCatalog::computeRevision(
    const std::map<std::string, PackageRecord>& packages) const {
    igpk::Sha256 hash;
    hashU64(hash, static_cast<std::uint64_t>(packages.size()));
    for (const auto& item : packages) {
        const PackageRecord& package = item.second;
        hashString(hash, package.packageId);
        hashString(hash, package.displayName);
        hashString(hash, package.fileName);
        hashU64(hash, package.state.fileSize);
        hashU64(hash, static_cast<std::uint64_t>(package.state.mtimeTicks));
        hashString(hash, package.sha256);
    }
    const auto digest = hash.finalize();
    std::uint64_t revision = 0;
    for (std::size_t i = 0; i < 8u; ++i) {
        revision = (revision << 8u) | static_cast<std::uint64_t>(digest[i]);
    }
    return revision == 0u ? 1u : revision;
}

bool PackageCatalog::refresh(std::string& error) {
    if (m_stopRequested && m_stopRequested()) {
        error = "catalog refresh was cancelled";
        return false;
    }
    if (m_log) { m_log("INFO", "refreshing direct-child package catalog synchronously"); }

    std::map<std::string, PackageRecord> refreshed;
    std::map<std::string, CachedDigest> refreshedCache;
    std::error_code iteratorError;
    fs::directory_iterator iterator(m_root, fs::directory_options::skip_permission_denied,
                                    iteratorError);
    const fs::directory_iterator end;
    if (iteratorError) {
        error = "failed to enumerate --root: " + iteratorError.message();
        return false;
    }
    while (iterator != end) {
        if (m_stopRequested && m_stopRequested()) {
            error = "catalog refresh was cancelled";
            return false;
        }
        const fs::path candidate = iterator->path();
        iterator.increment(iteratorError);
        if (iteratorError) {
            error = "failed while enumerating --root: " + iteratorError.message();
            return false;
        }

        const std::string fileName = pathToUtf8(candidate.filename());
        if (hasOuterAsciiWhitespace(fileName)) {
            if (m_log) {
                m_log("WARN", "skipping archive whose package ID has leading or trailing "
                                  "ASCII whitespace: " + fileName);
            }
            continue;
        }
        if (!endsWithArchiveSuffix(fileName)) { continue; }
        std::error_code absoluteError;
        const fs::path absoluteCandidate = fs::absolute(candidate, absoluteError).lexically_normal();
        std::error_code equivalentError;
        const bool isCachePath = (!absoluteError && absoluteCandidate == m_cachePath) ||
                                 fs::equivalent(candidate, m_cachePath, equivalentError);
        if (isCachePath) {
            if (m_log) { m_log("WARN", "catalog cache path is never exposed as a package"); }
            continue;
        }
        if (!validPackageId(fileName)) {
            if (m_log) { m_log("WARN", "skipping archive with invalid package ID"); }
            continue;
        }

        std::string stateError;
        PackageFileState state;
        if (!querySafeRegularFile(candidate, state, stateError)) {
            if (m_log) {
                m_log("WARN", "rejecting unsafe catalog candidate " + fileName + ": " + stateError);
            }
            continue;
        }
        fs::path canonical = fs::canonical(candidate, iteratorError);
        if (iteratorError) {
            if (m_log) {
                m_log("WARN", "skipping archive that could not be canonicalized: " + fileName);
            }
            iteratorError.clear();
            continue;
        }
        if (canonical.parent_path() != m_root) {
            if (m_log) { m_log("WARN", "rejecting archive outside the canonical --root: " + fileName); }
            continue;
        }

        std::string digest;
        const auto cached = m_cache.find(fileName);
        if (cached != m_cache.end() &&
            samePackageFileState(cached->second.state, state) &&
            cached->second.sha256.size() == 32u) {
            digest = cached->second.sha256;
            if (m_log) { m_log("INFO", "reused cached SHA-256 for " + fileName); }
        } else {
            if (m_log) { m_log("INFO", "computing SHA-256 before publishing " + fileName); }
            if (!hashStableFile(canonical, state, digest, stateError)) {
                if (m_stopRequested && m_stopRequested()) {
                    error = stateError;
                    return false;
                }
                if (m_log) {
                    m_log("WARN", "package is not catalog-ready and was skipped: " + fileName +
                                      ": " + stateError);
                }
                continue;
            }
        }

        PackageRecord record;
        record.path = std::move(canonical);
        record.packageId = fileName;
        record.fileName = fileName;
        record.displayName = fileName.substr(0u, fileName.size() - 8u);
        record.state = state;
        record.sha256 = digest;
        record.versionToken = sha256ToHex(digest);
        refreshed.emplace(record.packageId, record);
        refreshedCache.emplace(record.packageId, CachedDigest{ state, std::move(digest) });
    }

    const std::uint64_t refreshedRevision = computeRevision(refreshed);
    m_packages = std::move(refreshed);
    m_cache = refreshedCache;
    m_revision = refreshedRevision;
    saveCache(refreshedCache);
    if (m_log) {
        m_log("INFO", "catalog ready: " + std::to_string(m_packages.size()) +
                          " package(s), revision " + std::to_string(m_revision));
    }
    return true;
}

const PackageRecord* PackageCatalog::find(const std::string& packageId) const {
    const auto found = m_packages.find(packageId);
    return found == m_packages.end() ? nullptr : &found->second;
}

} // namespace igame::data_server
