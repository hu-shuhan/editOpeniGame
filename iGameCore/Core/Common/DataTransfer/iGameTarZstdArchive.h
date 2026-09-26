#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace iGame::data_transfer {

enum class ArchivePhase {
    Scanning,
    Compressing,
    Extracting,
    Finalizing
};

struct ArchiveProgress {
    ArchivePhase phase = ArchivePhase::Scanning;
    std::filesystem::path currentEntry;
    std::uint64_t entriesProcessed = 0;
    std::uint64_t entriesTotal = 0;
    std::uint64_t contentBytesProcessed = 0;
    std::uint64_t contentBytesTotal = 0;
    std::uint64_t archiveBytesProcessed = 0;
    std::uint64_t archiveBytesTotal = 0;
};

// Return false to cancel the operation. The callback is invoked synchronously
// on the thread calling Create() or Extract().
using ArchiveProgressCallback = std::function<bool(const ArchiveProgress&)>;

struct TarZstdCreateOptions {
    int compressionLevel = 3;
    std::size_t ioBufferBytes = 1024U * 1024U;
    bool replaceExistingArchive = false;
    ArchiveProgressCallback progress;
};

struct TarZstdExtractOptions {
    std::size_t ioBufferBytes = 1024U * 1024U;
    bool replaceExistingFiles = false;
    ArchiveProgressCallback progress;
};

struct TarZstdArchiveResult {
    bool success = false;
    bool cancelled = false;
    std::string error;
    std::uint64_t entryCount = 0;
    std::uint64_t contentBytes = 0;
    std::uint64_t archiveBytes = 0;
};

// A small-memory, deterministic tar + Zstandard archive implementation for
// local data transfer. Create() archives the contents of sourceDirectory at
// the archive root (it does not add sourceDirectory's own name).
//
// Security policy:
//  * source symlinks and non-file/non-directory entries are rejected;
//  * extraction accepts only regular files and directories;
//  * absolute paths, backslashes, drive/ADS syntax and dot components are
//    rejected before any archive entry is materialized;
//  * existing symlinks in the staging tree are never followed.
class TarZstdArchive {
public:
    static TarZstdArchiveResult Create(
        const std::filesystem::path& sourceDirectory,
        const std::filesystem::path& archiveFile,
        const TarZstdCreateOptions& options = {});

    static TarZstdArchiveResult Extract(
        const std::filesystem::path& archiveFile,
        const std::filesystem::path& stagingDirectory,
        const TarZstdExtractOptions& options = {});
};

} // namespace iGame::data_transfer
