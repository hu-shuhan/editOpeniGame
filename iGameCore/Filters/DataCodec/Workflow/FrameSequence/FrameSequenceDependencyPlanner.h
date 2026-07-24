#ifndef DATACODEC_WORKFLOW_FRAMESEQUENCE_FRAMESEQUENCEDEPENDENCYPLANNER_H
#define DATACODEC_WORKFLOW_FRAMESEQUENCE_FRAMESEQUENCEDEPENDENCYPLANNER_H

#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/FramePackage/FramePackageIO.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace datacodec {

struct FrameSequenceDependencyPlan {
    std::vector<std::uint32_t> decodeOrder;
    std::vector<std::uint32_t> referenceFrames;
};

class FrameSequenceDependencyPlanner final {
public:
    using FrameReaderMap = std::unordered_map<std::uint32_t, std::shared_ptr<IByteRangeReader>>;
    using FramePackageMap = std::unordered_map<std::uint32_t, std::shared_ptr<const FramePackage>>;

    FrameSequenceDependencyPlanner(
        FrameReaderMap frameReaders,
        FramePackageMap framePackages = {})
        : m_frameReaders(std::move(frameReaders)),
          m_preloadedFramePackages(std::move(framePackages)) {}

    [[nodiscard]] bool BuildPlan(
        const std::uint32_t targetFrameIndex,
        FrameSequenceDependencyPlan& plan,
        std::string* error = nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        plan = {};
        std::unordered_set<std::uint32_t> visiting;
        std::unordered_set<std::uint32_t> visited;
        std::unordered_set<std::uint32_t> references;
        if (!VisitFrame(
                targetFrameIndex,
                targetFrameIndex,
                visiting,
                visited,
                references,
                plan.decodeOrder,
                error)) {
            plan = {};
            return false;
        }
        for (const auto frameIndex : plan.decodeOrder) {
            if (references.contains(frameIndex)) {
                plan.referenceFrames.push_back(frameIndex);
            }
        }
        return true;
    }

private:
    [[nodiscard]] bool VisitFrame(
        const std::uint32_t frameIndex,
        const std::uint32_t targetFrameIndex,
        std::unordered_set<std::uint32_t>& visiting,
        std::unordered_set<std::uint32_t>& visited,
        std::unordered_set<std::uint32_t>& references,
        std::vector<std::uint32_t>& decodeOrder,
        std::string* error) {
        if (visited.contains(frameIndex)) {
            return true;
        }
        if (!visiting.insert(frameIndex).second) {
            return validation::AssignError(error, "frame sequence dependency contains a cycle");
        }

        const FramePackage* framePackage = nullptr;
        if (!LoadFramePackage(frameIndex, framePackage, error)) {
            visiting.erase(frameIndex);
            return false;
        }

        std::vector<std::uint32_t> dependencies;
        AppendTemporalDependency(
            framePackage->attributeTemporalRole,
            framePackage->attributeKeyFrameIndex,
            dependencies,
            references);
        AppendTemporalDependency(
            framePackage->geometryTemporalRole,
            framePackage->geometryKeyFrameIndex,
            dependencies,
            references);
        for (const auto& leaf : framePackage->leaves) {
            if (leaf.topologyMode == TopologyOwnershipMode::Reused) {
                AppendUnique(dependencies, leaf.ownerFrameIndex);
                references.insert(leaf.ownerFrameIndex);
            }
        }

        for (const auto dependencyFrameIndex : dependencies) {
            if (dependencyFrameIndex == frameIndex) {
                continue;
            }
            if (!VisitFrame(
                    dependencyFrameIndex,
                    targetFrameIndex,
                    visiting,
                    visited,
                    references,
                    decodeOrder,
                    error)) {
                visiting.erase(frameIndex);
                return false;
            }
        }

        visiting.erase(frameIndex);
        visited.insert(frameIndex);
        decodeOrder.push_back(frameIndex);
        if (frameIndex != targetFrameIndex) {
            references.insert(frameIndex);
        }
        return true;
    }

    static void AppendTemporalDependency(
        const TemporalFieldRole role,
        const std::uint32_t keyFrameIndex,
        std::vector<std::uint32_t>& dependencies,
        std::unordered_set<std::uint32_t>& references) {
        if (role != TemporalFieldRole::PredFrame) {
            return;
        }
        AppendUnique(dependencies, keyFrameIndex);
        references.insert(keyFrameIndex);
    }

    [[nodiscard]] bool LoadFramePackage(
        const std::uint32_t frameIndex,
        const FramePackage*& framePackage,
        std::string* error) {
        const auto preloaded = m_preloadedFramePackages.find(frameIndex);
        if (preloaded != m_preloadedFramePackages.end() && preloaded->second != nullptr) {
            framePackage = preloaded->second.get();
            return true;
        }
        const auto cached = m_framePackages.find(frameIndex);
        if (cached != m_framePackages.end()) {
            framePackage = &cached->second;
            return true;
        }
        const auto reader = m_frameReaders.find(frameIndex);
        if (reader == m_frameReaders.end() || reader->second == nullptr) {
            return validation::AssignError(error, "frame sequence reader is missing");
        }

        FramePackage loaded;
        if (!FramePackageIO::ReadMetadata(*reader->second, loaded, error)) {
            return false;
        }
        if (loaded.frameIndex != frameIndex) {
            return validation::AssignError(
                error,
                "frame sequence package index does not match its source");
        }
        const auto [iterator, inserted] = m_framePackages.emplace(frameIndex, std::move(loaded));
        (void)inserted;
        framePackage = &iterator->second;
        return true;
    }

    static void AppendUnique(
        std::vector<std::uint32_t>& frames,
        const std::uint32_t frameIndex) {
        if (std::find(frames.begin(), frames.end(), frameIndex) == frames.end()) {
            frames.push_back(frameIndex);
        }
    }

    std::unordered_map<std::uint32_t, FramePackage> m_framePackages;
    FrameReaderMap m_frameReaders;
    FramePackageMap m_preloadedFramePackages;
    std::mutex m_mutex;
};

} // namespace datacodec

#endif
