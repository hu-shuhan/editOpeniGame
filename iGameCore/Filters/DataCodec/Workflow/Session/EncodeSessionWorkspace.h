#ifndef DATACODEC_WORKFLOW_SESSION_ENCODESESSIONWORKSPACE_H
#define DATACODEC_WORKFLOW_SESSION_ENCODESESSIONWORKSPACE_H

#include "DataCodec/API/Adapter/IBlockTreeAdapter.h"
#include "DataCodec/Storage/FramePackage/FramePackageFormat.h"
#include "DataCodec/Workflow/Session/DataCodecReferenceState.h"
#include "DataCodec/Workflow/Temporal/Temporal.h"
#include "DataCodec/API/Params/CodecParamFactories.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace datacodec {

struct DataCodecEncodeFrameInput {
    IBlockTreeAdapter* blockTreeAdapter{nullptr};
    std::string rootName;
    std::uint32_t frameIndex{0u};
    std::uint32_t frameCount{1u};
    float timeValue{0.0f};
    const CodecControlParams* controlParams{nullptr};
    std::span<const AttributeTarget> attributeTargets;
};

struct LeafEncodeRun {
    BlockTreeLeafRecord leaf;
    FramePackageLeafRecord frameLeaf;
    std::unique_ptr<IEncodeAdapter> adapter;
    std::unique_ptr<EncodeContext> context;
};

struct FrameEncodeState {
    CodecControlParams controlParams;
    TemporalFrame temporalFrame;
    std::unordered_map<BlockPath, TemporalTopologyEntry> topologyByPath;
    FramePackage framePackage;
    std::vector<BlockTreeLeafRecord> leaves;
};

class EncodeSessionWorkspace {
public:
    void ResetSession() {
        ResetActiveFrame();
        m_state = {};
        m_referenceCaches.clear();
        m_referenceByteStoreSession.ReleaseAll();
        m_referenceByteStoreSession.Reset();
        m_lastPreparedFrameIndex.reset();
        m_expectedFrameCount.reset();
        m_expectedFrameTreeSignature.reset();
    }

    bool PrepareFrame(
        const DataCodecEncodeFrameInput& input,
        FrameEncodeState& output,
        std::string* error = nullptr) {
        ResetActiveFrame();
        if (input.blockTreeAdapter == nullptr) {
            AssignError(error, "DataCodec encode workspace requires a block tree adapter");
            return false;
        }
        if (!ValidateFrameSequence(input, error)) {
            return false;
        }
        const auto frameTreeSignature = MakeFrameTreeSignature(*input.blockTreeAdapter);
        if (!ValidateFrameTreeSignature(input, frameTreeSignature, error)) {
            return false;
        }

        output = {};
        output.controlParams = input.controlParams != nullptr
            ? *input.controlParams
            : CodecControlParamsFactory::MakeEncodeConfiguration(
                DataCodecEncodeOptions{}).controlParams;
        m_referenceByteStoreSession.ConfigureResidentLimit(
            output.controlParams.resourceBudget.EncodeReferenceResidentLimitBytes());

        output.leaves = input.blockTreeAdapter->GetLeafRecords();
        if (output.leaves.empty()) {
            output = {};
            AssignError(error, "DataCodec frame package input does not contain leaf objects");
            return false;
        }

        if (!TemporalBuilder::BuildFrame(
                *input.blockTreeAdapter,
                input.frameCount,
                input.frameIndex,
                output.controlParams.attrReference,
                output.controlParams.geometryReference,
                output.controlParams.topologyReference,
                m_state.temporalHistory,
                output.temporalFrame,
                error)) {
            output = {};
            return false;
        }

        output.topologyByPath.reserve(output.temporalFrame.topologyLeaves.size());
        for (const auto& entry : output.temporalFrame.topologyLeaves) {
            output.topologyByPath.emplace(entry.path, entry);
        }
        output.framePackage.frameIndex = output.temporalFrame.frameIndex;
        output.framePackage.timeValue = input.timeValue;
        output.framePackage.geometryTemporalRole = output.temporalFrame.geometry.temporalRole;
        output.framePackage.geometryKeyFrameIndex = output.temporalFrame.geometry.keyFrameIndex;
        output.framePackage.attributeTemporalRole = output.temporalFrame.attribute.temporalRole;
        output.framePackage.attributeKeyFrameIndex = output.temporalFrame.attribute.keyFrameIndex;
        output.framePackage.rootName = input.rootName.empty()
            ? input.blockTreeAdapter->GetRootName()
            : input.rootName;
        PruneReferenceCaches(output.framePackage);
        CollectBranchRecords(*input.blockTreeAdapter, output.framePackage.branches);
        m_blockTreeAdapter = input.blockTreeAdapter;
        m_attributeTargets = input.attributeTargets;
        m_lastPreparedFrameIndex = input.frameIndex;
        m_expectedFrameCount = input.frameCount;
        if (!m_expectedFrameTreeSignature.has_value() && input.frameCount > 1u) {
            m_expectedFrameTreeSignature = std::move(frameTreeSignature);
        }
        return true;
    }

    bool PrepareLeaf(
        const FrameEncodeState& framePlan,
        const std::size_t leafIndex,
        LeafEncodeRun& output,
        std::string* error = nullptr) {
        output.leaf = {};
        output.frameLeaf = {};
        output.adapter.reset();
        output.context.reset();

        if (m_blockTreeAdapter == nullptr) {
            AssignError(error, "DataCodec encode workspace has no active block tree adapter");
            return false;
        }
        if (leafIndex >= framePlan.leaves.size()) {
            AssignError(error, "DataCodec encode workspace leaf index is out of range");
            return false;
        }

        const auto& leaf = framePlan.leaves[leafIndex];
        auto adapter = m_blockTreeAdapter->GetLeaf(leaf.path);
        if (adapter == nullptr) {
            AssignError(error, "DataCodec encode workspace failed to create leaf adapter for " + leaf.path);
            return false;
        }

        auto context = std::make_unique<EncodeContext>();
        context->adapter = adapter.get();
        context->objectName = leaf.name;
        context->controlParams = &framePlan.controlParams;
        context->path = leaf.path;
        context->frameIndex = framePlan.framePackage.frameIndex;
        context->attributeTargets = m_attributeTargets;
        context->attributeTemporalRole = framePlan.framePackage.attributeTemporalRole;
        context->attributeKeyFrameIndex = framePlan.framePackage.attributeKeyFrameIndex;
        context->geometryTemporalRole = framePlan.framePackage.geometryTemporalRole;
        context->geometryKeyFrameIndex = framePlan.framePackage.geometryKeyFrameIndex;
        context->referenceByteStoreSession = &m_referenceByteStoreSession;
        if (!PrepareAttributeReferences(framePlan, leaf.path, *context, error) ||
            !PrepareGeometryReferences(framePlan, leaf.path, *context, error)) {
            output = {};
            return false;
        }

        output.leaf = leaf;
        output.frameLeaf = MakeFrameLeafRecord(framePlan, leaf);
        output.adapter = std::move(adapter);
        output.context = std::move(context);
        return true;
    }

    bool CommitEncodedLeaf(
        FrameEncodeState& framePlan,
        const LeafEncodeRun& leafRun,
        const std::uint64_t leafPackageByteSize,
        std::string* error = nullptr) {
        if (leafPackageByteSize == 0u) {
            AssignError(error, "DataCodec leaf package is empty");
            return false;
        }
        auto record = leafRun.frameLeaf;
        record.leafPackageByteSize = leafPackageByteSize;
        framePlan.framePackage.leaves.push_back(std::move(record));
        return true;
    }

    DataCodecReferenceState& ReferenceState() noexcept { return m_state; }
    const DataCodecReferenceState& ReferenceState() const noexcept { return m_state; }

private:
    struct LeafReferenceCaches {
        std::shared_ptr<DecodedAttributeCacheSet> attributes;
        std::shared_ptr<DecodedGeometryReferenceCache> geometry;
    };

    struct FrameTreeSignature {
        std::vector<BlockPath> branchPaths;
        std::vector<BlockPath> leafPaths;
    };

    static FrameTreeSignature MakeFrameTreeSignature(const IBlockTreeAdapter& adapter) {
        FrameTreeSignature signature;
        const auto branches = adapter.GetBranchRecords();
        signature.branchPaths.reserve(branches.size());
        for (const auto& branch : branches) {
            signature.branchPaths.push_back(branch.path);
        }

        const auto leaves = adapter.GetLeafRecords();
        signature.leafPaths.reserve(leaves.size());
        for (const auto& leaf : leaves) {
            signature.leafPaths.push_back(leaf.path);
        }

        std::sort(signature.branchPaths.begin(), signature.branchPaths.end());
        std::sort(signature.leafPaths.begin(), signature.leafPaths.end());
        return signature;
    }

    bool ValidateFrameTreeSignature(
        const DataCodecEncodeFrameInput& input,
        const FrameTreeSignature& signature,
        std::string* error) const {
        if (input.frameCount <= 1u || !m_expectedFrameTreeSignature.has_value()) {
            return true;
        }
        const auto& expected = *m_expectedFrameTreeSignature;
        if (expected.branchPaths == signature.branchPaths &&
            expected.leafPaths == signature.leafPaths) {
            return true;
        }

        return AssignErrorResult(
            error,
            "DataCodec multi-frame block-tree paths differ from frame zero at frame " +
                std::to_string(input.frameIndex) +
                " expected branches=" + std::to_string(expected.branchPaths.size()) +
                " leaves=" + std::to_string(expected.leafPaths.size()) +
                " actual branches=" + std::to_string(signature.branchPaths.size()) +
                " leaves=" + std::to_string(signature.leafPaths.size()));
    }

    bool PrepareAttributeReferences(
        const FrameEncodeState& framePlan,
        const BlockPath& path,
        EncodeContext& context,
        std::string* error) {
        const auto frameIndex = framePlan.framePackage.frameIndex;
        const auto role = framePlan.framePackage.attributeTemporalRole;
        if (role == TemporalFieldRole::SingleFrame) {
            return true;
        }
        if (role == TemporalFieldRole::KeyFrame) {
            auto& caches = m_referenceCaches[frameIndex][path];
            if (caches.attributes == nullptr) {
                caches.attributes = std::make_shared<DecodedAttributeCacheSet>();
            }
            context.currentAttributeReferenceCache = caches.attributes.get();
            return true;
        }
        const auto frameIterator = m_referenceCaches.find(
            framePlan.framePackage.attributeKeyFrameIndex);
        if (frameIterator == m_referenceCaches.end()) {
            return AssignErrorResult(error, "attribute key-frame cache frame is missing");
        }
        const auto leafIterator = frameIterator->second.find(path);
        if (leafIterator == frameIterator->second.end() ||
            leafIterator->second.attributes == nullptr) {
            return AssignErrorResult(error, "attribute key-frame cache leaf is missing");
        }
        context.attributeKeyFrameReference.attrReferenceCache = leafIterator->second.attributes;
        return true;
    }

    bool PrepareGeometryReferences(
        const FrameEncodeState& framePlan,
        const BlockPath& path,
        EncodeContext& context,
        std::string* error) {
        const auto frameIndex = framePlan.framePackage.frameIndex;
        const auto role = framePlan.framePackage.geometryTemporalRole;
        if (role == TemporalFieldRole::SingleFrame) {
            return true;
        }
        if (role == TemporalFieldRole::KeyFrame) {
            auto& caches = m_referenceCaches[frameIndex][path];
            if (caches.geometry == nullptr) {
                caches.geometry = std::make_shared<DecodedGeometryReferenceCache>();
            }
            context.currentGeometryReferenceCache = caches.geometry.get();
            return true;
        }
        const auto frameIterator = m_referenceCaches.find(
            framePlan.framePackage.geometryKeyFrameIndex);
        if (frameIterator == m_referenceCaches.end()) {
            return AssignErrorResult(error, "geometry key-frame cache frame is missing");
        }
        const auto leafIterator = frameIterator->second.find(path);
        if (leafIterator == frameIterator->second.end() ||
            leafIterator->second.geometry == nullptr) {
            return AssignErrorResult(error, "geometry key-frame cache leaf is missing");
        }
        context.geometryKeyFrameReference.geometryReferenceCache = leafIterator->second.geometry;
        return true;
    }

    static bool AssignErrorResult(std::string* error, std::string text) {
        AssignError(error, std::move(text));
        return false;
    }

    bool ValidateFrameSequence(
        const DataCodecEncodeFrameInput& input,
        std::string* error) const {
        if (input.frameCount == 0u ||
            (input.frameCount > 1u && input.frameIndex >= input.frameCount)) {
            return AssignErrorResult(error, "DataCodec encode frame index exceeds frame count");
        }
        if (!m_lastPreparedFrameIndex.has_value()) {
            if (input.frameCount > 1u && input.frameIndex != 0u) {
                return AssignErrorResult(error, "DataCodec encode session must begin from frame zero");
            }
            return true;
        }
        if (m_expectedFrameCount.has_value() && input.frameCount != *m_expectedFrameCount) {
            return AssignErrorResult(error, "DataCodec encode session frame count changed without reset");
        }
        if (input.frameIndex != *m_lastPreparedFrameIndex + 1u) {
            return AssignErrorResult(error, "DataCodec encode session requires strictly sequential frames");
        }
        return true;
    }

    void PruneReferenceCaches(const FramePackage& framePackage) {
        const bool keepAttributeReference =
            framePackage.attributeTemporalRole != TemporalFieldRole::SingleFrame;
        const bool keepGeometryReference =
            framePackage.geometryTemporalRole != TemporalFieldRole::SingleFrame;
        for (auto frameIt = m_referenceCaches.begin(); frameIt != m_referenceCaches.end();) {
            for (auto leafIt = frameIt->second.begin(); leafIt != frameIt->second.end();) {
                if (!keepAttributeReference || frameIt->first != framePackage.attributeKeyFrameIndex) {
                    leafIt->second.attributes.reset();
                }
                if (!keepGeometryReference || frameIt->first != framePackage.geometryKeyFrameIndex) {
                    leafIt->second.geometry.reset();
                }
                if (leafIt->second.attributes == nullptr && leafIt->second.geometry == nullptr) {
                    leafIt = frameIt->second.erase(leafIt);
                } else {
                    ++leafIt;
                }
            }
            if (frameIt->second.empty()) {
                frameIt = m_referenceCaches.erase(frameIt);
            } else {
                ++frameIt;
            }
        }
    }

    static void CollectBranchRecords(
        const IBlockTreeAdapter& adapter,
        std::vector<FramePackageBranchRecord>& branches) {
        for (const auto& branch : adapter.GetBranchRecords()) {
            branches.push_back(FramePackageBranchRecord{
                .path = branch.path,
                .name = branch.name,
            });
        }
    }

    static FramePackageLeafRecord MakeFrameLeafRecord(
        const FrameEncodeState& framePlan,
        const BlockTreeLeafRecord& leaf) {
        const auto topologyIt = framePlan.topologyByPath.find(leaf.path);
        const TemporalTopologyEntry* topologyEntry =
            topologyIt != framePlan.topologyByPath.end() ? &topologyIt->second : nullptr;
        return FramePackageLeafRecord{
            .path = leaf.path,
            .name = leaf.name,
            .ownerFrameIndex = topologyEntry != nullptr
                ? topologyEntry->ownerFrameIndex
                : framePlan.framePackage.frameIndex,
            .topologyMode = topologyEntry != nullptr
                ? topologyEntry->ownershipMode
                : TopologyOwnershipMode::Owned,
        };
    }

    static void AssignError(std::string* error, std::string text) {
        if (error != nullptr) {
            *error = std::move(text);
        }
    }

    void ResetActiveFrame() noexcept {
        m_blockTreeAdapter = nullptr;
        m_attributeTargets = {};
    }

    DataCodecReferenceState m_state;
    std::unordered_map<std::uint32_t, std::unordered_map<BlockPath, LeafReferenceCaches>> m_referenceCaches;
    std::optional<std::uint32_t> m_lastPreparedFrameIndex;
    std::optional<std::uint32_t> m_expectedFrameCount;
    std::optional<FrameTreeSignature> m_expectedFrameTreeSignature;
    bytestore::ByteStoreSession m_referenceByteStoreSession;
    IBlockTreeAdapter* m_blockTreeAdapter{nullptr};
    std::span<const AttributeTarget> m_attributeTargets;
};

} // namespace datacodec

#endif
