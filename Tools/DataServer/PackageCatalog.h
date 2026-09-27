#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>

namespace igame::data_server {

struct PackageFileState {
    std::uint64_t fileSize = 0;
    std::int64_t mtimeTicks = 0;

    // This fingerprint is deliberately not exposed on protocol v1.  It makes
    // the on-disk SHA cache and in-process stale-file checks stronger than a
    // size/mtime tuple, which can be preserved across an atomic replacement.
    // Windows stores volume/file IDs plus creation/change times.  POSIX stores
    // device/inode plus ctime seconds/nanoseconds in the last two fields.
    std::uint64_t identityDevice = 0;
    std::uint64_t identityFile = 0;
    std::int64_t identityCreation = 0;
    std::int64_t identityChange = 0;
};

bool samePackageFileState(const PackageFileState& left,
                          const PackageFileState& right);

struct PackageRecord {
    std::filesystem::path path;
    std::string packageId;
    std::string displayName;
    std::string fileName;
    PackageFileState state;
    std::string sha256;       // Raw 32-byte digest.
    std::string versionToken; // Lowercase SHA-256 hexadecimal text.
};

using CatalogLog = std::function<void(const char* level, const std::string& message)>;
using StopRequested = std::function<bool()>;

// Rejects symlinks and Windows reparse points before returning metadata.
bool querySafeRegularFile(const std::filesystem::path& path,
                          PackageFileState& state,
                          std::string& error);

std::string sha256ToHex(const std::string& digest);

class PackageCatalog {
public:
    PackageCatalog(std::filesystem::path root,
                   std::filesystem::path cachePath,
                   bool cachePathWasExplicit,
                   CatalogLog log,
                   StopRequested stopRequested);

    // Loads the persistent index and builds the first ready catalog. A package
    // is published only after a stable digest is available.
    bool initialize(std::string& error);

    // Synchronously scans direct children and atomically replaces the in-memory
    // sorted map only after the complete refresh has finished.
    bool refresh(std::string& error);

    const std::map<std::string, PackageRecord>& packages() const { return m_packages; }
    const PackageRecord* find(const std::string& packageId) const;
    std::uint64_t revision() const { return m_revision; }
    const std::filesystem::path& root() const { return m_root; }
    const std::filesystem::path& cachePath() const { return m_cachePath; }

private:
    struct CachedDigest {
        PackageFileState state;
        std::string sha256;
    };

    bool validateCacheLocation(std::string& error) const;
    void loadCache();
    void saveCache(const std::map<std::string, CachedDigest>& entries) const;
    bool hashStableFile(const std::filesystem::path& path,
                        const PackageFileState& before,
                        std::string& digest,
                        std::string& error) const;
    std::uint64_t computeRevision(const std::map<std::string, PackageRecord>& packages) const;

    std::filesystem::path m_root;
    std::filesystem::path m_cachePath;
    bool m_cachePathWasExplicit = false;
    CatalogLog m_log;
    StopRequested m_stopRequested;
    std::map<std::string, CachedDigest> m_cache;
    std::map<std::string, PackageRecord> m_packages;
    std::uint64_t m_revision = 0;
};

} // namespace igame::data_server
