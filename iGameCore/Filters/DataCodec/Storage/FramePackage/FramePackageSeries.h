#ifndef DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGESERIES_H
#define DATACODEC_STORAGE_FRAMEPACKAGE_FRAMEPACKAGESERIES_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace datacodec
{

struct FramePackagePathInfo {
    std::filesystem::path fullPath;
    std::filesystem::path directory;
    std::string baseName;
    std::string seriesStem;
    std::uint32_t frameIndex{0u};
    bool hasFrameIndex{false};
};

struct FramePackageSeriesFile {
    std::uint32_t frameIndex{0u};
    std::filesystem::path framePackagePath;
};

struct FramePackageSeries {
    std::filesystem::path entryFrameFilePath;
    std::string seriesStem;
    std::uint32_t entryFrameIndex{0u};
    bool hasExplicitFrameIndex{false};
    std::vector<FramePackageSeriesFile> frames;
};

inline std::string ToLowerFramePackageAscii(std::string value) {
    for (char& ch: value) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
    return value;
}

inline bool ParseFramePackagePath(const std::filesystem::path& path, FramePackagePathInfo& parsed,
                                  std::string* error = nullptr) {
    if (ToLowerFramePackageAscii(path.extension().string()) != ".igc") {
        if (error != nullptr) { *error = "unsupported frame package series extension"; }
        parsed = {};
        return false;
    }

    parsed = {};
    parsed.fullPath = path;
    parsed.directory = path.parent_path();
    parsed.baseName = path.stem().string();
    parsed.seriesStem = parsed.baseName;
    std::size_t digitBegin = parsed.baseName.size();
    while (digitBegin > 0u && std::isdigit(static_cast<unsigned char>(parsed.baseName[digitBegin - 1u]))) {
        --digitBegin;
    }
    if (digitBegin == parsed.baseName.size() || digitBegin == 0u || parsed.baseName[digitBegin - 1u] != '_') {
        return true;
    }

    std::uint64_t frameIndex = 0u;
    for (const char ch: parsed.baseName.substr(digitBegin)) {
        frameIndex = frameIndex * 10u + static_cast<std::uint64_t>(ch - '0');
        if (frameIndex > std::numeric_limits<std::uint32_t>::max()) {
            if (error != nullptr) { *error = "frame package series index exceeds uint32 range"; }
            parsed = {};
            return false;
        }
    }
    parsed.seriesStem = parsed.baseName.substr(0u, digitBegin);
    parsed.frameIndex = static_cast<std::uint32_t>(frameIndex);
    parsed.hasFrameIndex = true;
    return true;
}

inline std::string FormatFramePackageIndex(const std::uint32_t frameIndex, const int width = 4) {
    std::ostringstream output;
    output.width(width);
    output.fill('0');
    output << frameIndex;
    return output.str();
}

inline std::string NormalizeFramePackageSeriesStem(std::string stem) {
    if (!stem.empty() && std::isalnum(static_cast<unsigned char>(stem.back()))) { stem.push_back('_'); }
    return stem;
}

inline std::filesystem::path BuildFramePackagePath(const std::string& seriesStem,
                                                   const std::uint32_t frameIndex,
                                                   const int width = 4) {
    return NormalizeFramePackageSeriesStem(seriesStem) + FormatFramePackageIndex(frameIndex, width) + ".igc";
}

inline bool ResolveFramePackageSeries(const std::filesystem::path& entryPath, FramePackageSeries& sequence,
                                      std::string* error = nullptr) {
    sequence = {};
    FramePackagePathInfo parsedEntry;
    if (!ParseFramePackagePath(entryPath, parsedEntry, error)) { return false; }
    if (!std::filesystem::exists(entryPath)) {
        if (error != nullptr) { *error = "frame package series entry does not exist"; }
        return false;
    }
    sequence.entryFrameFilePath = entryPath;
    sequence.seriesStem = parsedEntry.seriesStem;
    sequence.entryFrameIndex = parsedEntry.frameIndex;
    sequence.hasExplicitFrameIndex = parsedEntry.hasFrameIndex;
    if (!parsedEntry.hasFrameIndex) {
        sequence.frames.push_back({0u, entryPath});
        return true;
    }

    auto directory = parsedEntry.directory;
    if (directory.empty()) { directory = std::filesystem::current_path(); }
    std::map<std::uint32_t, FramePackageSeriesFile> frames;
    std::error_code directoryError;
    for (const auto& entry: std::filesystem::directory_iterator(directory, directoryError)) {
        if (directoryError || !entry.is_regular_file()) { continue; }
        FramePackagePathInfo candidate;
        if (!ParseFramePackagePath(entry.path(), candidate, nullptr) || !candidate.hasFrameIndex ||
            candidate.seriesStem != parsedEntry.seriesStem) {
            continue;
        }
        frames[candidate.frameIndex] = {candidate.frameIndex, entry.path()};
    }
    for (const auto& [frameIndex, frame]: frames) {
        (void) frameIndex;
        sequence.frames.push_back(frame);
    }
    if (sequence.frames.empty() ||
        std::none_of(sequence.frames.begin(), sequence.frames.end(), [&](const auto& frame) {
            return frame.frameIndex == sequence.entryFrameIndex;
        })) {
        if (error != nullptr) { *error = "frame package series entry index was not discovered"; }
        sequence = {};
        return false;
    }
    return true;
}

} // datacodec命名空间

#endif
