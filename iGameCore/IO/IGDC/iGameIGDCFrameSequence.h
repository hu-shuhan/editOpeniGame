#ifndef iGameIGDCFrameSequence_h
#define iGameIGDCFrameSequence_h

#include "DataCodec/Workflow/FrameSequence/FrameDecodeSource.h"
#include "DataCodec/Storage/FramePackage/FramePackageIO.h"
#include "DataCodec/Storage/FramePackage/FramePackageSeries.h"
#include "DataCodec/Filter/Adapter/iGameFileByteRangeIO.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

struct IGDCFrameSequence {
    std::filesystem::path entryPath;
    std::uint32_t entryFrameIndex{0u};
    std::vector<std::filesystem::path> selectedFramePaths;
    std::vector<std::uint32_t> selectedFrameIndices;
    std::vector<std::filesystem::path> framePaths;
    std::vector<::datacodec::FrameDecodeSource> decodeSources;
};

inline std::string ResolveIGDCFrameSequenceStem(const std::filesystem::path& outputHint) {
    ::datacodec::FramePackagePathInfo parsed;
    auto normalized = outputHint;
    normalized.replace_extension(".igc");
    if (::datacodec::ParseFramePackagePath(normalized, parsed, nullptr) && parsed.hasFrameIndex) {
        return parsed.seriesStem;
    }
    return normalized.stem().string();
}

inline std::filesystem::path BuildIGDCFrameSequencePath(const std::filesystem::path& outputHint,
                                                        const std::uint32_t frameIndex,
                                                        const int width = 4) {
    return outputHint.parent_path() /
           ::datacodec::BuildFramePackagePath(ResolveIGDCFrameSequenceStem(outputHint), frameIndex, width);
}

inline bool EquivalentIGDCFramePath(const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
    std::error_code equivalentError;
    const auto equivalent = std::filesystem::equivalent(lhs, rhs, equivalentError);
    if (!equivalentError) { return equivalent; }
    return lhs.lexically_normal() == rhs.lexically_normal();
}

inline bool ResolveIGDCFrameSelection(const std::vector<std::filesystem::path>& selectedPaths,
                                      IGDCFrameSequence& sequence, std::string* error = nullptr) {
    sequence = {};
    if (selectedPaths.empty()) {
        if (error != nullptr) { *error = "frame package selection is empty"; }
        return false;
    }
    ::datacodec::FramePackageSeries discovered;
    if (!::datacodec::ResolveFramePackageSeries(selectedPaths.front(), discovered, error)) { return false; }
    sequence.framePaths.reserve(discovered.frames.size());
    sequence.decodeSources.reserve(discovered.frames.size());
    for (const auto& frame: discovered.frames) {
        auto reader = std::make_shared<iGameFileByteRangeReader>(frame.framePackagePath);
        auto metadata = std::make_shared<::datacodec::FramePackage>();
        if (!::datacodec::FramePackageIO::ReadMetadata(*reader, *metadata, error)) {
            sequence = {};
            return false;
        }
        if (metadata->frameIndex != frame.frameIndex) {
            if (error != nullptr) { *error = "frame package file name index does not match its metadata"; }
            sequence = {};
            return false;
        }
        const auto sourceIdentity = ::datacodec::MakePackageDecodeSourceIdentity(
            metadata->identity,
            ::datacodec::framepackagewire::kFramePackageVersion,
            reader->ByteSize());
        sequence.framePaths.push_back(frame.framePackagePath);
        sequence.decodeSources.push_back(::datacodec::FrameDecodeSource{
                .frameIndex = metadata->frameIndex,
                .timeValue = metadata->timeValue,
                .frameReader = std::move(reader),
                .sourceIdentity = sourceIdentity,
                .framePackage = std::move(metadata),
        });
    }
    if (sequence.decodeSources.empty()) {
        if (error != nullptr) { *error = "frame package series contains no frames"; }
        sequence = {};
        return false;
    }

    std::vector<std::pair<std::uint32_t, std::filesystem::path>> selectedFrames;
    selectedFrames.reserve(selectedPaths.size());
    for (const auto& selectedPath: selectedPaths) {
        const auto pathIterator = std::find_if(
                sequence.framePaths.begin(), sequence.framePaths.end(),
                [&selectedPath](const auto& discoveredPath) {
                    return EquivalentIGDCFramePath(discoveredPath, selectedPath);
                });
        if (pathIterator == sequence.framePaths.end()) {
            if (error != nullptr) { *error = "selected frame does not belong to the discovered frame package series"; }
            sequence = {};
            return false;
        }
        const auto ordinal = static_cast<std::size_t>(std::distance(sequence.framePaths.begin(), pathIterator));
        selectedFrames.emplace_back(sequence.decodeSources[ordinal].frameIndex, *pathIterator);
    }
    std::sort(selectedFrames.begin(), selectedFrames.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });
    selectedFrames.erase(std::unique(selectedFrames.begin(), selectedFrames.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first == rhs.first;
    }), selectedFrames.end());
    for (const auto& [frameIndex, framePath]: selectedFrames) {
        sequence.selectedFrameIndices.push_back(frameIndex);
        sequence.selectedFramePaths.push_back(framePath);
    }
    sequence.entryFrameIndex = sequence.selectedFrameIndices.front();
    sequence.entryPath = sequence.selectedFramePaths.front();
    return true;
}

inline bool ResolveIGDCFrameSequence(const std::filesystem::path& entryPath, IGDCFrameSequence& sequence,
                                     std::string* error = nullptr) {
    return ResolveIGDCFrameSelection({entryPath}, sequence, error);
}

IGAME_NAMESPACE_END

#endif
