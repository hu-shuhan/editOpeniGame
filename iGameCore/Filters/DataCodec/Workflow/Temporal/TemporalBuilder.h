#ifndef DATACODEC_WORKFLOW_TEMPORAL_TEMPORALBUILDER_H
#define DATACODEC_WORKFLOW_TEMPORAL_TEMPORALBUILDER_H

#include "DataCodec/API/Adapter/IBlockTreeAdapter.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"
#include "DataCodec/Codec/Topology/TopologyFingerprint.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Workflow/Temporal/Temporal.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
namespace datacodec {

class TemporalBuilder {
public:
    struct LastOwnedState {
        TopologyFingerprint fingerprint;
        PointSpatialFingerprint pointSpatialFingerprint;
        std::uint32_t ownerFrameIndex{0};
    };

    struct TemporalHistoryState {
        std::unordered_map<BlockPath, LastOwnedState> previousStates;
        std::vector<BlockPath> previousPaths;
    };

    static bool BuildFrame(
        const IBlockTreeAdapter& frame,
        const std::uint32_t frameCount,
        const std::uint32_t frameIndex,
        const AttrReferenceControlParams& referenceParams,
        const GeometryReferenceControlParams& geometryReferenceParams,
        const TopologyReferenceControlParams& topologyReferenceParams,
        TemporalHistoryState& history,
        TemporalFrame& temporalFrame,
        std::string* error = nullptr) {
        std::vector<BlockPath> currentPaths;
        frame.EnumerateLeafPaths([&currentPaths](const BlockPath& path) {
            currentPaths.push_back(path);
        });
        std::sort(currentPaths.begin(), currentPaths.end());

        const bool samePathSet = frameIndex > 0 && currentPaths == history.previousPaths;
        std::unordered_map<BlockPath, LastOwnedState> currentStates;
        temporalFrame = {};
        temporalFrame.frameIndex = frameIndex;
        if (!BuildTopologyForFrame(
                frame,
                frameIndex,
                samePathSet,
                history.previousStates,
                currentStates,
                currentPaths,
                frameCount > 1u && topologyReferenceParams.enabled,
                temporalFrame.topologyLeaves,
                error)) {
            temporalFrame = {};
            return false;
        }
        temporalFrame.attribute = ResolveTemporalFieldState(frameCount, frameIndex, referenceParams);
        temporalFrame.geometry = ResolveTemporalFieldState(frameCount, frameIndex, geometryReferenceParams);
        history.previousPaths = std::move(currentPaths);
        history.previousStates = std::move(currentStates);
        return true;
    }

    template <typename ReferenceControlParams>
    static TemporalFieldState ResolveTemporalFieldState(
        const std::uint32_t frameCount,
        const std::uint32_t frameIndex,
        const ReferenceControlParams& referenceParams) {
        TemporalFieldState fieldState;
        if (frameCount <= 1u ||
            !referenceParams.enabled ||
            referenceParams.temporalField.codec == TemporalFieldReferenceCodec::Disabled) {
            fieldState.temporalRole = TemporalFieldRole::SingleFrame;
            fieldState.keyFrameIndex = frameIndex;
            return fieldState;
        }

        const auto interval = referenceParams.temporalField.keyFrameInterval;
        const bool forcePredFrames = referenceParams.temporalField.forcePredFrames;

        std::uint32_t keyFrameIndex = 0u;
        bool isKeyFrame = frameIndex == 0u;
        if (forcePredFrames || interval == 0u) {
            keyFrameIndex = 0u;
            isKeyFrame = frameIndex == 0u;
        } else if (interval == 1u) {
            keyFrameIndex = frameIndex;
            isKeyFrame = true;
        } else {
            keyFrameIndex = (frameIndex / interval) * interval;
            isKeyFrame = keyFrameIndex == frameIndex;
        }

        fieldState.temporalRole = isKeyFrame
            ? TemporalFieldRole::KeyFrame
            : TemporalFieldRole::PredFrame;
        fieldState.keyFrameIndex = keyFrameIndex;
        return fieldState;
    }

private:
    static bool BuildTopologyForFrame(
        const IBlockTreeAdapter& frame,
        const std::uint32_t frameIndex,
        const bool samePathSet,
        const std::unordered_map<BlockPath, LastOwnedState>& previousStates,
        std::unordered_map<BlockPath, LastOwnedState>& currentStates,
        const std::vector<BlockPath>& currentPaths,
        const bool referenceEnabled,
        std::vector<TemporalTopologyEntry>& topologyLeaves,
        std::string* error) {
        topologyLeaves.clear();
        topologyLeaves.reserve(currentPaths.size());

        for (const auto& path : currentPaths) {
            auto leaf = frame.GetLeaf(path);
            if (leaf == nullptr) {
                topologyLeaves.clear();
                currentStates.clear();
                return validation::AssignError(error, "frame returned a null leaf adapter for path " + path);
            }

            TemporalTopologyEntry topologyEntry{
                .path = path,
                .ownershipMode = TopologyOwnershipMode::Owned,
                .ownerFrameIndex = frameIndex,
            };

            if (!referenceEnabled) {
                topologyLeaves.push_back(std::move(topologyEntry));
                continue;
            }

            TopologyFingerprint fingerprint;
            if (!TopologyFingerprintBuilder::Build(*leaf, fingerprint, error)) {
                topologyLeaves.clear();
                currentStates.clear();
                return false;
            }
            const auto pointSpatialFingerprint = PointSpatialFingerprintBuilder::Build(*leaf);
            if (samePathSet) {
                const auto previousIt = previousStates.find(path);
                if (previousIt != previousStates.end() &&
                    previousIt->second.fingerprint == fingerprint &&
                    previousIt->second.pointSpatialFingerprint == pointSpatialFingerprint) {
                    topologyEntry.ownershipMode = TopologyOwnershipMode::Reused;
                    topologyEntry.ownerFrameIndex = previousIt->second.ownerFrameIndex;
                }
            }

            currentStates[path] = LastOwnedState{
                .fingerprint = fingerprint,
                .pointSpatialFingerprint = pointSpatialFingerprint,
                .ownerFrameIndex = topologyEntry.ownershipMode == TopologyOwnershipMode::Owned
                    ? frameIndex
                    : topologyEntry.ownerFrameIndex,
            };
            topologyLeaves.push_back(std::move(topologyEntry));
        }

        return true;
    }
};

} // namespace datacodec

#endif

