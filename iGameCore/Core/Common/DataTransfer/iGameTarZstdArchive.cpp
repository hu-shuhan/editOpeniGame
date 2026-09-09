#include "DataTransfer/iGameTarZstdArchive.h"

#include <zstd.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iGame::data_transfer {
namespace {

namespace fs = std::filesystem;

constexpr std::size_t TarBlockBytes = 512;
constexpr std::size_t TarEndBytes = TarBlockBytes * 2;
constexpr std::size_t MinimumIoBufferBytes = 4096;
constexpr std::size_t MaximumIoBufferBytes = 64U * 1024U * 1024U;

class ArchiveFailure final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ArchiveCancelled final : public std::runtime_error {
public:
    ArchiveCancelled()
        : std::runtime_error("archive operation cancelled") {
    }
};

[[noreturn]] void Fail(const std::string& message) {
    throw ArchiveFailure(message);
}

std::string WithSystemError(const std::string& message, const std::error_code& error) {
    if (!error) {
        return message;
    }
    return message + ": " + error.message();
}

std::size_t NormalizedBufferSize(std::size_t requested) {
    return std::clamp(requested, MinimumIoBufferBytes, MaximumIoBufferBytes);
}

std::string PathToUtf8(const fs::path& path) {
#if defined(__cpp_lib_char8_t)
    const std::u8string value = path.generic_u8string();
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
#else
    return path.generic_u8string();
#endif
}

fs::path Utf8ToPath(const std::string& value) {
#if defined(__cpp_lib_char8_t)
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(value.data()), value.size()));
#else
    return fs::u8path(value);
#endif
}

bool IsSameOrDescendant(const fs::path& candidate, const fs::path& root) {
    const fs::path relative = candidate.lexically_relative(root);
    if (relative.empty()) {
        return candidate == root;
    }
    if (relative.is_absolute()) {
        return false;
    }
    const auto first = relative.begin();
    return first != relative.end() && *first != fs::path("..");
}

std::uint64_t CheckedAdd(std::uint64_t left, std::uint64_t right,
                         const char* description) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        Fail(std::string(description) + " exceeds UInt64");
    }
    return left + right;
}

std::uint64_t TarPaddedSize(std::uint64_t size) {
    const std::uint64_t remainder = size % TarBlockBytes;
    return remainder == 0 ? size : CheckedAdd(size, TarBlockBytes - remainder,
                                               "tar entry size");
}

void CheckProgress(const ArchiveProgressCallback& callback,
                   const ArchiveProgress& progress) {
    if (callback && !callback(progress)) {
        throw ArchiveCancelled();
    }
}

std::string ValidateArchivePath(std::string name, bool directory) {
    if (directory && !name.empty() && name.back() == '/') {
        name.pop_back();
    }
    if (name.empty()) {
        Fail("archive entry has an empty path");
    }
    if (name.front() == '/' || name.find('\\') != std::string::npos) {
        Fail("archive entry path must be relative and use forward slashes: " + name);
    }
    // ':' is rejected on every platform so a package accepted on Linux cannot
    // become a drive-relative path or NTFS alternate data stream on Windows.
    if (name.find(':') != std::string::npos) {
        Fail("archive entry path contains drive/stream syntax: " + name);
    }

    std::size_t begin = 0;
    while (begin <= name.size()) {
        const std::size_t end = name.find('/', begin);
        const std::size_t length =
            (end == std::string::npos ? name.size() : end) - begin;
        const std::string component = name.substr(begin, length);
        if (component.empty() || component == "." || component == "..") {
            Fail("archive entry contains an empty or dot path component: " + name);
        }
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }

    const fs::path parsed = Utf8ToPath(name);
    if (parsed.empty() || parsed.has_root_name() || parsed.has_root_directory() ||
        parsed.is_absolute()) {
        Fail("archive entry is not a relative path: " + name);
    }
    return name;
}

struct TarPathFields {
    std::string name;
    std::string prefix;
};

TarPathFields SplitTarPath(const std::string& archivePath) {
    if (archivePath.size() <= 100) {
        return {archivePath, {}};
    }

    std::size_t slash = archivePath.size();
    while (slash != std::string::npos) {
        slash = archivePath.rfind('/', slash == archivePath.size()
                                               ? archivePath.size() - 1
                                               : slash - 1);
        if (slash == std::string::npos) {
            break;
        }
        const std::size_t prefixLength = slash;
        const std::size_t nameLength = archivePath.size() - slash - 1;
        if (prefixLength <= 155 && nameLength > 0 && nameLength <= 100) {
            return {archivePath.substr(slash + 1), archivePath.substr(0, slash)};
        }
        if (slash == 0) {
            break;
        }
    }
    Fail("archive entry path cannot be represented by a USTAR header: " + archivePath);
}

void WriteTarNumber(char* field, std::size_t length, std::uint64_t value) {
    if (length < 2) {
        Fail("internal tar numeric field is too short");
    }

    std::uint64_t octalLimit = 1;
    bool octalLimitOverflow = false;
    for (std::size_t i = 0; i < length - 1; ++i) {
        if (octalLimit > std::numeric_limits<std::uint64_t>::max() / 8) {
            octalLimitOverflow = true;
            break;
        }
        octalLimit *= 8;
    }

    if (octalLimitOverflow || value < octalLimit) {
        std::fill(field, field + length, '0');
        field[length - 1] = '\0';
        std::size_t cursor = length - 1;
        do {
            if (cursor == 0) {
                Fail("tar numeric value does not fit field");
            }
            field[--cursor] = static_cast<char>('0' + (value & 7U));
            value >>= 3U;
        } while (value != 0);
        return;
    }

    // POSIX/GNU base-256 encoding extends USTAR beyond the 8 GiB octal limit.
    std::fill(field, field + length, '\0');
    for (std::size_t i = length; i > 1; --i) {
        field[i - 1] = static_cast<char>(value & 0xffU);
        value >>= 8U;
    }
    if (value != 0) {
        Fail("tar numeric value does not fit base-256 field");
    }
    field[0] = static_cast<char>(0x80U);
}

std::uint64_t ReadTarNumber(const char* field, std::size_t length,
                            const char* description) {
    if (length == 0) {
        Fail(std::string("empty tar ") + description + " field");
    }
    const auto first = static_cast<unsigned char>(field[0]);
    if ((first & 0x80U) != 0) {
        if ((first & 0x40U) != 0) {
            Fail(std::string("negative tar ") + description + " is unsupported");
        }
        std::uint64_t value = first & 0x3fU;
        for (std::size_t i = 1; i < length; ++i) {
            if (value > (std::numeric_limits<std::uint64_t>::max() >> 8U)) {
                Fail(std::string("tar ") + description + " exceeds UInt64");
            }
            value = (value << 8U) | static_cast<unsigned char>(field[i]);
        }
        return value;
    }

    std::uint64_t value = 0;
    bool sawDigit = false;
    bool sawTerminator = false;
    for (std::size_t i = 0; i < length; ++i) {
        const unsigned char character = static_cast<unsigned char>(field[i]);
        if (character == '\0' || character == ' ') {
            if (sawDigit) {
                sawTerminator = true;
            }
            continue;
        }
        if (sawTerminator || character < '0' || character > '7') {
            Fail(std::string("invalid octal tar ") + description);
        }
        sawDigit = true;
        if (value > (std::numeric_limits<std::uint64_t>::max() >> 3U)) {
            Fail(std::string("tar ") + description + " exceeds UInt64");
        }
        value = (value << 3U) | (character - '0');
    }
    return value;
}

std::string ReadTarString(const char* field, std::size_t length) {
    std::size_t used = 0;
    while (used < length && field[used] != '\0') {
        ++used;
    }
    return std::string(field, used);
}

std::array<char, TarBlockBytes> MakeTarHeader(const std::string& archivePath,
                                               bool directory,
                                               std::uint64_t size) {
    const TarPathFields fields = SplitTarPath(archivePath);
    std::array<char, TarBlockBytes> header{};
    std::memcpy(header.data(), fields.name.data(), fields.name.size());
    std::memcpy(header.data() + 345, fields.prefix.data(), fields.prefix.size());
    WriteTarNumber(header.data() + 100, 8, directory ? 0755U : 0644U);
    WriteTarNumber(header.data() + 108, 8, 0);
    WriteTarNumber(header.data() + 116, 8, 0);
    WriteTarNumber(header.data() + 124, 12, size);
    WriteTarNumber(header.data() + 136, 12, 0);
    std::fill(header.data() + 148, header.data() + 156, ' ');
    header[156] = directory ? '5' : '0';
    std::memcpy(header.data() + 257, "ustar", 5);
    header[262] = '\0';
    header[263] = '0';
    header[264] = '0';

    unsigned int checksum = 0;
    for (const unsigned char character : header) {
        checksum += character;
    }
    char checksumDigits[7]{};
    for (int i = 5; i >= 0; --i) {
        checksumDigits[i] = static_cast<char>('0' + (checksum & 7U));
        checksum >>= 3U;
    }
    std::memcpy(header.data() + 148, checksumDigits, 6);
    header[154] = '\0';
    header[155] = ' ';
    return header;
}

struct SourceEntry {
    fs::path sourcePath;
    std::string archivePath;
    bool directory = false;
    std::uint64_t size = 0;
};

class ZstdOutputStream {
public:
    ZstdOutputStream(const fs::path& outputPath, int compressionLevel,
                     std::size_t bufferBytes, std::uint64_t pledgedSize)
        : m_Output(outputPath, std::ios::binary | std::ios::trunc)
        , m_Context(ZSTD_createCCtx(), &ZSTD_freeCCtx)
        , m_OutputBuffer(std::max(bufferBytes, ZSTD_CStreamOutSize()))
        , m_PledgedSize(pledgedSize) {
        if (!m_Output) {
            Fail("cannot create archive temporary file: " + PathToUtf8(outputPath));
        }
        if (!m_Context) {
            Fail("cannot allocate Zstandard compression context");
        }
        SetParameter(ZSTD_c_compressionLevel, compressionLevel,
                     "compression level");
        SetParameter(ZSTD_c_checksumFlag, 1, "content checksum");
        SetParameter(ZSTD_c_contentSizeFlag, 1, "content size");
        const std::size_t pledged =
            ZSTD_CCtx_setPledgedSrcSize(m_Context.get(), pledgedSize);
        if (ZSTD_isError(pledged)) {
            Fail(std::string("cannot set Zstandard pledged content size: ") +
                 ZSTD_getErrorName(pledged));
        }
    }

    void Write(const void* data, std::size_t size) {
        ZSTD_inBuffer input{data, size, 0};
        while (input.pos < input.size) {
            ZSTD_outBuffer output{m_OutputBuffer.data(), m_OutputBuffer.size(), 0};
            const std::size_t before = input.pos;
            const std::size_t status =
                ZSTD_compressStream2(m_Context.get(), &output, &input, ZSTD_e_continue);
            if (ZSTD_isError(status)) {
                Fail(std::string("Zstandard compression failed: ") +
                     ZSTD_getErrorName(status));
            }
            FlushOutput(output.pos);
            if (input.pos == before && output.pos == 0) {
                Fail("Zstandard compressor made no forward progress");
            }
        }
        m_InputBytes = CheckedAdd(m_InputBytes, size, "compressed input size");
    }

    void Finish() {
        if (m_InputBytes != m_PledgedSize) {
            Fail("tar byte count does not match pledged Zstandard content size");
        }
        std::size_t remaining = 0;
        do {
            ZSTD_inBuffer input{nullptr, 0, 0};
            ZSTD_outBuffer output{m_OutputBuffer.data(), m_OutputBuffer.size(), 0};
            remaining =
                ZSTD_compressStream2(m_Context.get(), &output, &input, ZSTD_e_end);
            if (ZSTD_isError(remaining)) {
                Fail(std::string("cannot finalize Zstandard frame: ") +
                     ZSTD_getErrorName(remaining));
            }
            FlushOutput(output.pos);
            if (remaining != 0 && output.pos == 0) {
                Fail("Zstandard finalizer made no forward progress");
            }
        } while (remaining != 0);
        m_Output.flush();
        if (!m_Output) {
            Fail("cannot flush archive temporary file");
        }
        m_Output.close();
        if (m_Output.fail()) {
            Fail("cannot close archive temporary file");
        }
    }

    std::uint64_t OutputBytes() const noexcept {
        return m_OutputBytes;
    }

private:
    using ContextPointer = std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)>;

    void SetParameter(ZSTD_cParameter parameter, int value,
                      const char* description) {
        const std::size_t status =
            ZSTD_CCtx_setParameter(m_Context.get(), parameter, value);
        if (ZSTD_isError(status)) {
            Fail(std::string("cannot set Zstandard ") + description + ": " +
                 ZSTD_getErrorName(status));
        }
    }

    void FlushOutput(std::size_t size) {
        if (size == 0) {
            return;
        }
        m_Output.write(m_OutputBuffer.data(), static_cast<std::streamsize>(size));
        if (!m_Output) {
            Fail("cannot write archive temporary file");
        }
        m_OutputBytes = CheckedAdd(m_OutputBytes, size, "archive output size");
    }

    std::ofstream m_Output;
    ContextPointer m_Context;
    std::vector<char> m_OutputBuffer;
    std::uint64_t m_PledgedSize = 0;
    std::uint64_t m_InputBytes = 0;
    std::uint64_t m_OutputBytes = 0;
};

class TarExtractor {
public:
    TarExtractor(fs::path root, bool replaceExisting)
        : m_Root(std::move(root))
        , m_ReplaceExisting(replaceExisting) {
    }

    void Consume(const char* data, std::size_t size) {
        std::size_t offset = 0;
        while (offset < size) {
            switch (m_State) {
            case State::Header: {
                const std::size_t amount =
                    std::min(size - offset, TarBlockBytes - m_HeaderUsed);
                std::memcpy(m_Header.data() + m_HeaderUsed, data + offset, amount);
                m_HeaderUsed += amount;
                offset += amount;
                if (m_HeaderUsed == TarBlockBytes) {
                    ProcessHeader();
                    m_HeaderUsed = 0;
                }
                break;
            }
            case State::FileData: {
                const std::size_t amount = static_cast<std::size_t>(
                    std::min<std::uint64_t>(size - offset, m_RemainingFileBytes));
                m_CurrentFile.write(data + offset,
                                    static_cast<std::streamsize>(amount));
                if (!m_CurrentFile) {
                    Fail("cannot write extracted file: " +
                         PathToUtf8(m_CurrentTarget));
                }
                offset += amount;
                m_RemainingFileBytes -= amount;
                m_ContentBytes = CheckedAdd(m_ContentBytes, amount,
                                             "extracted content size");
                if (m_RemainingFileBytes == 0) {
                    CompleteFile();
                }
                break;
            }
            case State::Padding: {
                const std::size_t amount = static_cast<std::size_t>(
                    std::min<std::uint64_t>(size - offset, m_RemainingPadding));
                for (std::size_t i = 0; i < amount; ++i) {
                    if (data[offset + i] != '\0') {
                        Fail("tar file padding contains non-zero data");
                    }
                }
                offset += amount;
                m_RemainingPadding -= amount;
                if (m_RemainingPadding == 0) {
                    m_State = State::Header;
                }
                break;
            }
            case State::Done:
                Fail("tar contains data after its two end-of-archive blocks");
            }
        }
    }

    void Finish() {
        if (m_State != State::Done || m_HeaderUsed != 0 || m_ZeroBlocks != 2) {
            Fail("tar stream is truncated or lacks two end-of-archive blocks");
        }
    }

    std::uint64_t EntryCount() const noexcept {
        return m_EntryCount;
    }

    std::uint64_t ContentBytes() const noexcept {
        return m_ContentBytes;
    }

    std::uint64_t DeclaredContentBytes() const noexcept {
        return m_DeclaredContentBytes;
    }

    const fs::path& CurrentEntry() const noexcept {
        return m_CurrentRelative;
    }

private:
    enum class State { Header, FileData, Padding, Done };

    void ProcessHeader() {
        const bool allZero = std::all_of(m_Header.begin(), m_Header.end(),
                                         [](char value) { return value == '\0'; });
        if (allZero) {
            ++m_ZeroBlocks;
            if (m_ZeroBlocks == 2) {
                m_State = State::Done;
            }
            return;
        }
        if (m_ZeroBlocks != 0) {
            Fail("tar has a non-zero header after an end marker");
        }
        if (std::memcmp(m_Header.data() + 257, "ustar", 5) != 0 ||
            m_Header[262] != '\0' || m_Header[263] != '0' ||
            m_Header[264] != '0') {
            Fail("tar entry is not in the supported USTAR format");
        }

        const std::uint64_t storedChecksum =
            ReadTarNumber(m_Header.data() + 148, 8, "checksum");
        std::uint64_t computedChecksum = 0;
        for (std::size_t i = 0; i < m_Header.size(); ++i) {
            computedChecksum += (i >= 148 && i < 156)
                                    ? static_cast<unsigned char>(' ')
                                    : static_cast<unsigned char>(m_Header[i]);
        }
        if (storedChecksum != computedChecksum) {
            Fail("tar header checksum mismatch");
        }

        const char type = m_Header[156];
        const bool directory = type == '5';
        const bool regularFile = type == '0' || type == '\0';
        if (!directory && !regularFile) {
            Fail("tar contains a link or non-regular entry type");
        }
        if (!ReadTarString(m_Header.data() + 157, 100).empty()) {
            Fail("tar entry contains a link target");
        }

        std::string name = ReadTarString(m_Header.data(), 100);
        const std::string prefix = ReadTarString(m_Header.data() + 345, 155);
        if (!prefix.empty()) {
            name = prefix + "/" + name;
        }
        const std::string key = ValidateArchivePath(name, directory);
        if (!m_SeenPaths.insert(key).second) {
            Fail("tar contains a duplicate entry path: " + key);
        }
        m_CurrentRelative = Utf8ToPath(key);

        const std::uint64_t entrySize =
            ReadTarNumber(m_Header.data() + 124, 12, "entry size");
        if (directory) {
            if (entrySize != 0) {
                Fail("tar directory entry has non-zero size: " + key);
            }
            EnsureDirectory(m_CurrentRelative);
            ++m_EntryCount;
            m_CurrentRelative.clear();
            m_State = State::Header;
            return;
        }

        m_DeclaredContentBytes = CheckedAdd(m_DeclaredContentBytes, entrySize,
                                             "declared archive content size");
        OpenFile(m_CurrentRelative);
        m_RemainingFileBytes = entrySize;
        m_RemainingPadding =
            (TarBlockBytes - (entrySize % TarBlockBytes)) % TarBlockBytes;
        if (m_RemainingFileBytes == 0) {
            CompleteFile();
        } else {
            m_State = State::FileData;
        }
    }

    void EnsureDirectory(const fs::path& relativeDirectory) {
        fs::path current = m_Root;
        for (const fs::path& component : relativeDirectory) {
            current /= component;
            std::error_code error;
            const fs::file_status status = fs::symlink_status(current, error);
            if (error && status.type() != fs::file_type::not_found) {
                Fail(WithSystemError("cannot inspect staging directory " +
                                         PathToUtf8(current),
                                     error));
            }
            if (fs::exists(status)) {
                if (fs::is_symlink(status) || !fs::is_directory(status)) {
                    Fail("staging path component is a symlink or non-directory: " +
                         PathToUtf8(current));
                }
            } else {
                if (!fs::create_directory(current, error) || error) {
                    Fail(WithSystemError("cannot create staging directory " +
                                             PathToUtf8(current),
                                         error));
                }
            }

            const fs::path resolved = fs::weakly_canonical(current, error);
            if (error || !IsSameOrDescendant(resolved, m_Root)) {
                Fail("staging path escapes extraction root: " +
                     PathToUtf8(current));
            }
        }
    }

    void OpenFile(const fs::path& relativeFile) {
        if (!relativeFile.parent_path().empty()) {
            EnsureDirectory(relativeFile.parent_path());
        }
        m_CurrentTarget = m_Root / relativeFile;
        std::error_code error;
        const fs::file_status status = fs::symlink_status(m_CurrentTarget, error);
        if (error && status.type() != fs::file_type::not_found) {
            Fail(WithSystemError("cannot inspect extraction target " +
                                     PathToUtf8(m_CurrentTarget),
                                 error));
        }
        if (fs::exists(status)) {
            if (fs::is_symlink(status) || !fs::is_regular_file(status)) {
                Fail("extraction target is a symlink or non-regular file: " +
                     PathToUtf8(m_CurrentTarget));
            }
            if (!m_ReplaceExisting) {
                Fail("extraction target already exists: " +
                     PathToUtf8(m_CurrentTarget));
            }
        }
        m_CurrentFile.open(m_CurrentTarget,
                           std::ios::binary | std::ios::trunc);
        if (!m_CurrentFile) {
            Fail("cannot create extracted file: " +
                 PathToUtf8(m_CurrentTarget));
        }
    }

    void CompleteFile() {
        m_CurrentFile.flush();
        if (!m_CurrentFile) {
            Fail("cannot flush extracted file: " +
                 PathToUtf8(m_CurrentTarget));
        }
        m_CurrentFile.close();
        if (m_CurrentFile.fail()) {
            Fail("cannot close extracted file: " +
                 PathToUtf8(m_CurrentTarget));
        }
        ++m_EntryCount;
        m_CurrentTarget.clear();
        m_CurrentRelative.clear();
        m_State = m_RemainingPadding == 0 ? State::Header : State::Padding;
    }

    fs::path m_Root;
    bool m_ReplaceExisting = false;
    State m_State = State::Header;
    std::array<char, TarBlockBytes> m_Header{};
    std::size_t m_HeaderUsed = 0;
    unsigned int m_ZeroBlocks = 0;
    std::uint64_t m_RemainingFileBytes = 0;
    std::uint64_t m_RemainingPadding = 0;
    std::uint64_t m_EntryCount = 0;
    std::uint64_t m_ContentBytes = 0;
    std::uint64_t m_DeclaredContentBytes = 0;
    fs::path m_CurrentRelative;
    fs::path m_CurrentTarget;
    std::ofstream m_CurrentFile;
    std::unordered_set<std::string> m_SeenPaths;
};

TarZstdArchiveResult FailedResult(const std::exception& exception,
                                  bool cancelled = false) {
    TarZstdArchiveResult result;
    result.cancelled = cancelled;
    result.error = exception.what();
    return result;
}

} // namespace

TarZstdArchiveResult TarZstdArchive::Create(
    const fs::path& sourceDirectory, const fs::path& archiveFile,
    const TarZstdCreateOptions& options) {
    fs::path temporaryFile;
    bool temporaryCreated = false;
    try {
        if (sourceDirectory.empty() || archiveFile.empty()) {
            Fail("source directory and archive path are required");
        }

        std::error_code error;
        const fs::file_status sourceStatus =
            fs::symlink_status(sourceDirectory, error);
        if (error || fs::is_symlink(sourceStatus) ||
            !fs::is_directory(sourceStatus)) {
            Fail(WithSystemError("source path must be a real directory", error));
        }
        const fs::path sourceRoot = fs::weakly_canonical(sourceDirectory, error);
        if (error) {
            Fail(WithSystemError("cannot resolve source directory", error));
        }

        fs::path archiveAbsolute = fs::absolute(archiveFile, error);
        if (error) {
            Fail(WithSystemError("cannot resolve archive path", error));
        }
        archiveAbsolute = archiveAbsolute.lexically_normal();
        const fs::path outputParent = archiveAbsolute.parent_path();
        const fs::path resolvedParent = fs::weakly_canonical(outputParent, error);
        if (error) {
            Fail(WithSystemError("cannot resolve archive parent directory", error));
        }
        const fs::path resolvedOutput = resolvedParent / archiveAbsolute.filename();
        if (IsSameOrDescendant(resolvedOutput, sourceRoot)) {
            Fail("archive output must not be inside the source directory");
        }

        std::vector<SourceEntry> entries;
        std::uint64_t totalContentBytes = 0;
        ArchiveProgress progress;
        progress.phase = ArchivePhase::Scanning;

        fs::recursive_directory_iterator iterator(
            sourceRoot, fs::directory_options::none, error);
        const fs::recursive_directory_iterator end;
        if (error) {
            Fail(WithSystemError("cannot enumerate source directory", error));
        }
        while (iterator != end) {
            const fs::path path = iterator->path();
            const fs::file_status status = fs::symlink_status(path, error);
            if (error) {
                Fail(WithSystemError("cannot inspect source entry " +
                                         PathToUtf8(path),
                                     error));
            }
            if (fs::is_symlink(status)) {
                Fail("source contains a symbolic link: " + PathToUtf8(path));
            }

            const fs::path relative = path.lexically_relative(sourceRoot);
            std::string archivePath =
                ValidateArchivePath(PathToUtf8(relative), fs::is_directory(status));
            SourceEntry entry;
            entry.sourcePath = path;
            entry.directory = fs::is_directory(status);
            if (entry.directory) {
                archivePath.push_back('/');
            } else if (fs::is_regular_file(status)) {
                const std::uintmax_t fileSize = fs::file_size(path, error);
                if (error || fileSize > std::numeric_limits<std::uint64_t>::max()) {
                    Fail(WithSystemError("cannot get source file size " +
                                             PathToUtf8(path),
                                         error));
                }
                entry.size = static_cast<std::uint64_t>(fileSize);
                totalContentBytes = CheckedAdd(totalContentBytes, entry.size,
                                                "source content size");
            } else {
                Fail("source contains a non-regular entry: " + PathToUtf8(path));
            }
            entry.archivePath = archivePath;
            (void)SplitTarPath(entry.archivePath);
            entries.push_back(std::move(entry));

            progress.currentEntry = relative;
            progress.entriesProcessed = entries.size();
            progress.contentBytesTotal = totalContentBytes;
            CheckProgress(options.progress, progress);

            iterator.increment(error);
            if (error) {
                Fail(WithSystemError("cannot continue source enumeration", error));
            }
        }

        std::sort(entries.begin(), entries.end(),
                  [](const SourceEntry& left, const SourceEntry& right) {
                      return left.archivePath < right.archivePath;
                  });

        std::uint64_t tarSize = TarEndBytes;
        for (const SourceEntry& entry : entries) {
            tarSize = CheckedAdd(tarSize, TarBlockBytes, "tar size");
            if (!entry.directory) {
                tarSize = CheckedAdd(tarSize, TarPaddedSize(entry.size),
                                     "tar size");
            }
        }

        if (!fs::exists(outputParent, error)) {
            if (!fs::create_directories(outputParent, error) || error) {
                Fail(WithSystemError("cannot create archive output directory", error));
            }
        } else if (error || !fs::is_directory(outputParent, error)) {
            Fail(WithSystemError("archive output parent is not a directory", error));
        }

        const fs::file_status outputStatus = fs::symlink_status(archiveAbsolute, error);
        if (error && outputStatus.type() != fs::file_type::not_found) {
            Fail(WithSystemError("cannot inspect existing archive", error));
        }
        if (fs::exists(outputStatus)) {
            if (fs::is_symlink(outputStatus) || !fs::is_regular_file(outputStatus)) {
                Fail("archive target is a symlink or non-regular file");
            }
            if (!options.replaceExistingArchive) {
                Fail("archive target already exists");
            }
        }

        temporaryFile = archiveAbsolute;
        temporaryFile += ".tmp";
        const fs::file_status temporaryStatus =
            fs::symlink_status(temporaryFile, error);
        if (error && temporaryStatus.type() != fs::file_type::not_found) {
            Fail(WithSystemError("cannot inspect archive temporary path", error));
        }
        if (fs::exists(temporaryStatus)) {
            Fail("archive temporary path already exists: " +
                 PathToUtf8(temporaryFile));
        }

        ZstdOutputStream compressed(
            temporaryFile, options.compressionLevel,
            NormalizedBufferSize(options.ioBufferBytes), tarSize);
        temporaryCreated = true;
        std::vector<char> inputBuffer(NormalizedBufferSize(options.ioBufferBytes));
        const std::array<char, TarBlockBytes> zeroBlock{};

        progress = {};
        progress.phase = ArchivePhase::Compressing;
        progress.entriesTotal = entries.size();
        progress.contentBytesTotal = totalContentBytes;
        CheckProgress(options.progress, progress);

        for (const SourceEntry& entry : entries) {
            progress.currentEntry = Utf8ToPath(entry.archivePath);
            const auto header =
                MakeTarHeader(entry.archivePath, entry.directory, entry.size);
            compressed.Write(header.data(), header.size());

            if (!entry.directory) {
                const fs::file_status currentStatus =
                    fs::symlink_status(entry.sourcePath, error);
                if (error || fs::is_symlink(currentStatus) ||
                    !fs::is_regular_file(currentStatus)) {
                    Fail(WithSystemError("source file type changed while packing " +
                                             PathToUtf8(entry.sourcePath),
                                         error));
                }
                const std::uintmax_t currentSize =
                    fs::file_size(entry.sourcePath, error);
                if (error || currentSize != entry.size) {
                    Fail("source file size changed while packing: " +
                         PathToUtf8(entry.sourcePath));
                }

                std::ifstream input(entry.sourcePath, std::ios::binary);
                if (!input) {
                    Fail("cannot open source file: " +
                         PathToUtf8(entry.sourcePath));
                }
                std::uint64_t remaining = entry.size;
                while (remaining != 0) {
                    const std::size_t amount = static_cast<std::size_t>(
                        std::min<std::uint64_t>(remaining, inputBuffer.size()));
                    input.read(inputBuffer.data(),
                               static_cast<std::streamsize>(amount));
                    if (input.gcount() != static_cast<std::streamsize>(amount)) {
                        Fail("source file was truncated while packing: " +
                             PathToUtf8(entry.sourcePath));
                    }
                    compressed.Write(inputBuffer.data(), amount);
                    remaining -= amount;
                    progress.contentBytesProcessed = CheckedAdd(
                        progress.contentBytesProcessed, amount,
                        "packed content progress");
                    progress.archiveBytesProcessed = compressed.OutputBytes();
                    CheckProgress(options.progress, progress);
                }
                if (input.peek() != std::char_traits<char>::eof()) {
                    Fail("source file grew while packing: " +
                         PathToUtf8(entry.sourcePath));
                }
                const std::size_t padding = static_cast<std::size_t>(
                    (TarBlockBytes - (entry.size % TarBlockBytes)) % TarBlockBytes);
                if (padding != 0) {
                    compressed.Write(zeroBlock.data(), padding);
                }
            }
            ++progress.entriesProcessed;
            progress.archiveBytesProcessed = compressed.OutputBytes();
            CheckProgress(options.progress, progress);
        }

        compressed.Write(zeroBlock.data(), zeroBlock.size());
        compressed.Write(zeroBlock.data(), zeroBlock.size());
        progress.phase = ArchivePhase::Finalizing;
        CheckProgress(options.progress, progress);
        compressed.Finish();

        if (fs::exists(outputStatus)) {
            if (!fs::remove(archiveAbsolute, error) || error) {
                Fail(WithSystemError("cannot replace existing archive", error));
            }
        }
        fs::rename(temporaryFile, archiveAbsolute, error);
        if (error) {
            Fail(WithSystemError("cannot publish completed archive", error));
        }
        temporaryCreated = false;

        const std::uintmax_t finalSize = fs::file_size(archiveAbsolute, error);
        if (error || finalSize > std::numeric_limits<std::uint64_t>::max()) {
            Fail(WithSystemError("cannot get completed archive size", error));
        }
        TarZstdArchiveResult result;
        result.success = true;
        result.entryCount = entries.size();
        result.contentBytes = totalContentBytes;
        result.archiveBytes = static_cast<std::uint64_t>(finalSize);
        return result;
    } catch (const ArchiveCancelled& exception) {
        if (temporaryCreated) {
            std::error_code ignored;
            fs::remove(temporaryFile, ignored);
        }
        return FailedResult(exception, true);
    } catch (const std::exception& exception) {
        if (temporaryCreated) {
            std::error_code ignored;
            fs::remove(temporaryFile, ignored);
        }
        return FailedResult(exception);
    }
}

TarZstdArchiveResult TarZstdArchive::Extract(
    const fs::path& archiveFile, const fs::path& stagingDirectory,
    const TarZstdExtractOptions& options) {
    try {
        if (archiveFile.empty() || stagingDirectory.empty()) {
            Fail("archive path and staging directory are required");
        }
        std::error_code error;
        const fs::file_status archiveStatus = fs::symlink_status(archiveFile, error);
        if (error || fs::is_symlink(archiveStatus) ||
            !fs::is_regular_file(archiveStatus)) {
            Fail(WithSystemError("archive path must be a real regular file", error));
        }
        const std::uintmax_t archiveFileSize = fs::file_size(archiveFile, error);
        if (error || archiveFileSize > std::numeric_limits<std::uint64_t>::max()) {
            Fail(WithSystemError("cannot get archive size", error));
        }

        const fs::file_status stagingStatus =
            fs::symlink_status(stagingDirectory, error);
        if (error && stagingStatus.type() != fs::file_type::not_found) {
            Fail(WithSystemError("cannot inspect staging directory", error));
        }
        if (fs::exists(stagingStatus)) {
            if (fs::is_symlink(stagingStatus) ||
                !fs::is_directory(stagingStatus)) {
                Fail("staging path must be a real directory");
            }
            if (!options.replaceExistingFiles) {
                fs::directory_iterator checkEmpty(stagingDirectory, error);
                if (error) {
                    Fail(WithSystemError("cannot inspect staging directory", error));
                }
                if (checkEmpty != fs::directory_iterator{}) {
                    Fail("staging directory is not empty");
                }
            }
        } else if (!fs::create_directories(stagingDirectory, error) || error) {
            Fail(WithSystemError("cannot create staging directory", error));
        }

        const fs::path stagingRoot = fs::weakly_canonical(stagingDirectory, error);
        if (error) {
            Fail(WithSystemError("cannot resolve staging directory", error));
        }
        const fs::path archiveResolved = fs::weakly_canonical(archiveFile, error);
        if (error) {
            Fail(WithSystemError("cannot resolve archive file", error));
        }
        if (IsSameOrDescendant(archiveResolved, stagingRoot)) {
            Fail("archive file must not be inside its staging directory");
        }

        std::ifstream archive(archiveFile, std::ios::binary);
        if (!archive) {
            Fail("cannot open archive: " + PathToUtf8(archiveFile));
        }
        using DecompressionContext =
            std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)>;
        DecompressionContext context(ZSTD_createDCtx(), &ZSTD_freeDCtx);
        if (!context) {
            Fail("cannot allocate Zstandard decompression context");
        }

        const std::size_t bufferSize =
            NormalizedBufferSize(options.ioBufferBytes);
        std::vector<char> inputBuffer(std::max(bufferSize, ZSTD_DStreamInSize()));
        std::vector<char> outputBuffer(std::max(bufferSize, ZSTD_DStreamOutSize()));
        TarExtractor extractor(stagingRoot, options.replaceExistingFiles);

        ArchiveProgress progress;
        progress.phase = ArchivePhase::Extracting;
        progress.archiveBytesTotal = static_cast<std::uint64_t>(archiveFileSize);
        CheckProgress(options.progress, progress);

        bool frameHeaderChecked = false;
        bool frameEnded = false;
        std::uint64_t expectedTarBytes = 0;
        std::uint64_t compressedBytesRead = 0;
        std::uint64_t decompressedBytes = 0;

        while (!frameEnded) {
            archive.read(inputBuffer.data(),
                         static_cast<std::streamsize>(inputBuffer.size()));
            const std::streamsize count = archive.gcount();
            if (count == 0) {
                if (archive.bad()) {
                    Fail("I/O error while reading compressed archive");
                }
                break;
            }
            const std::size_t inputSize = static_cast<std::size_t>(count);
            if (!frameHeaderChecked) {
                const unsigned long long frameSize =
                    ZSTD_getFrameContentSize(inputBuffer.data(), inputSize);
                if (frameSize == ZSTD_CONTENTSIZE_ERROR) {
                    Fail("archive is not a valid Zstandard frame");
                }
                if (frameSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                    Fail("Zstandard frame does not declare its content size");
                }
                expectedTarBytes = static_cast<std::uint64_t>(frameSize);
                frameHeaderChecked = true;
            }

            ZSTD_inBuffer input{inputBuffer.data(), inputSize, 0};
            while (input.pos < input.size) {
                ZSTD_outBuffer output{outputBuffer.data(), outputBuffer.size(), 0};
                const std::size_t before = input.pos;
                const std::size_t remaining =
                    ZSTD_decompressStream(context.get(), &output, &input);
                if (ZSTD_isError(remaining)) {
                    Fail(std::string("Zstandard decompression/checksum failed: ") +
                         ZSTD_getErrorName(remaining));
                }
                if (output.pos != 0) {
                    extractor.Consume(outputBuffer.data(), output.pos);
                    decompressedBytes = CheckedAdd(decompressedBytes, output.pos,
                                                   "decompressed tar size");
                }

                progress.currentEntry = extractor.CurrentEntry();
                progress.entriesProcessed = extractor.EntryCount();
                progress.contentBytesProcessed = extractor.ContentBytes();
                progress.contentBytesTotal = extractor.DeclaredContentBytes();
                progress.archiveBytesProcessed = CheckedAdd(
                    compressedBytesRead, input.pos, "archive read progress");
                CheckProgress(options.progress, progress);

                if (remaining == 0) {
                    frameEnded = true;
                    if (input.pos != input.size) {
                        Fail("archive has trailing bytes or concatenated Zstandard frames");
                    }
                    break;
                }
                if (input.pos == before && output.pos == 0) {
                    Fail("Zstandard decompressor made no forward progress");
                }
            }
            compressedBytesRead = CheckedAdd(compressedBytesRead, inputSize,
                                             "archive bytes read");
        }

        if (!frameEnded) {
            Fail("Zstandard frame is truncated");
        }
        if (archive.peek() != std::char_traits<char>::eof()) {
            Fail("archive has trailing bytes or concatenated Zstandard frames");
        }
        if (decompressedBytes != expectedTarBytes) {
            Fail("Zstandard content size does not match decompressed tar size");
        }
        extractor.Finish();

        progress.phase = ArchivePhase::Finalizing;
        progress.currentEntry.clear();
        progress.entriesProcessed = extractor.EntryCount();
        progress.contentBytesProcessed = extractor.ContentBytes();
        progress.contentBytesTotal = extractor.DeclaredContentBytes();
        progress.archiveBytesProcessed = static_cast<std::uint64_t>(archiveFileSize);
        CheckProgress(options.progress, progress);

        TarZstdArchiveResult result;
        result.success = true;
        result.entryCount = extractor.EntryCount();
        result.contentBytes = extractor.ContentBytes();
        result.archiveBytes = static_cast<std::uint64_t>(archiveFileSize);
        return result;
    } catch (const ArchiveCancelled& exception) {
        return FailedResult(exception, true);
    } catch (const std::exception& exception) {
        return FailedResult(exception);
    }
}

} // namespace iGame::data_transfer
