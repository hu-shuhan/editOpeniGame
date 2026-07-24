#include "DataCodec/Filter/Adapter/iGameFramePresentationBridge.h"

#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "iGameDrawObject.h"

#include <unordered_map>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

constexpr const char* kFrameMarker = "DataCodec.FramePresentation";
constexpr const char* kFrameIndex = "DataCodec.FrameIndex";
constexpr const char* kBlockPath = "DataCodec.BlockPath";
constexpr const char* kTopologyMode = "DataCodec.TopologyMode";
constexpr const char* kTopologyOwnerFrameIndex = "DataCodec.TopologyOwnerFrameIndex";
constexpr const char* kDrawablePrepared = "DataCodec.DrawablePrepared";

struct LeafPresentationInfo {
    DrawObject* object{nullptr};
    std::string path;
    std::uint32_t frameIndex{0u};
    std::uint32_t topologyOwnerFrameIndex{0u};
    ::datacodec::TopologyOwnershipMode topologyMode{
        ::datacodec::TopologyOwnershipMode::Owned};
    bool drawablePrepared{false};
};

[[nodiscard]] bool ReadLeafPresentationInfo(
    DataObject* object,
    LeafPresentationInfo& info) {
    auto drawObject = DynamicCast<DrawObject>(object);
    auto* metadata = object != nullptr ? object->GetMetadata() : nullptr;
    int marker = 0;
    int frameIndex = 0;
    int topologyMode = 0;
    int ownerFrameIndex = 0;
    int drawablePrepared = 0;
    std::string path;
    if (drawObject == nullptr || metadata == nullptr ||
        !metadata->GetInt(kFrameMarker, marker) || marker != 1 ||
        !metadata->GetInt(kFrameIndex, frameIndex) || frameIndex < 0 ||
        !metadata->GetString(kBlockPath, path) ||
        !metadata->GetInt(kTopologyMode, topologyMode) ||
        !metadata->GetInt(kTopologyOwnerFrameIndex, ownerFrameIndex) || ownerFrameIndex < 0 ||
        !metadata->GetInt(kDrawablePrepared, drawablePrepared)) {
        return false;
    }
    if (topologyMode < static_cast<int>(::datacodec::TopologyOwnershipMode::Owned) ||
        topologyMode > static_cast<int>(::datacodec::TopologyOwnershipMode::Reused)) {
        return false;
    }
    info = LeafPresentationInfo{
        .object = drawObject,
        .path = std::move(path),
        .frameIndex = static_cast<std::uint32_t>(frameIndex),
        .topologyOwnerFrameIndex = static_cast<std::uint32_t>(ownerFrameIndex),
        .topologyMode = static_cast<::datacodec::TopologyOwnershipMode>(topologyMode),
        .drawablePrepared = drawablePrepared != 0,
    };
    return true;
}

void CollectLeafPresentationInfo(
    DataObject* object,
    std::vector<LeafPresentationInfo>& leaves) {
    if (object == nullptr) { return; }
    LeafPresentationInfo info;
    if (ReadLeafPresentationInfo(object, info)) {
        leaves.push_back(std::move(info));
        return;
    }
    if (!object->HasSubDataObject()) { return; }
    for (auto iterator = object->SubDataObjectIteratorBegin();
         iterator != object->SubDataObjectIteratorEnd();
         ++iterator) {
        CollectLeafPresentationInfo(iterator->second.get(), leaves);
    }
}

[[nodiscard]] bool CompatibleTopologyOwner(
    const LeafPresentationInfo& current,
    const LeafPresentationInfo& next) {
    return current.object != nullptr && next.object != nullptr &&
        current.object->GetDataObjectType() == next.object->GetDataObjectType() &&
        current.topologyOwnerFrameIndex == next.topologyOwnerFrameIndex;
}

}

bool PrepareDataCodecDecodedLeaf(
    const DataObject::Pointer& output,
    const ::datacodec::FramePackageLeafRecord& leaf,
    const std::uint32_t frameIndex,
    std::string* error) {
    auto drawObject = DynamicCast<DrawObject>(output);
    if (drawObject == nullptr || output->GetMetadata() == nullptr) {
        return ::datacodec::validation::AssignError(
            error,
            "frame leaf output is not drawable");
    }
    const auto ownerFrameIndex = leaf.topologyMode == ::datacodec::TopologyOwnershipMode::Owned
        ? frameIndex
        : leaf.ownerFrameIndex;
    auto* metadata = output->GetMetadata();
    metadata->AddInt(kFrameMarker, 1);
    metadata->AddInt(kFrameIndex, static_cast<int>(frameIndex));
    metadata->AddString(kBlockPath, leaf.path);
    metadata->AddInt(kTopologyMode, static_cast<int>(leaf.topologyMode));
    metadata->AddInt(kTopologyOwnerFrameIndex, static_cast<int>(ownerFrameIndex));

    drawObject->SetShellRenderingOption(false);
    if (leaf.topologyMode == ::datacodec::TopologyOwnershipMode::Owned) {
        metadata->AddInt(kDrawablePrepared, 0);
        return true;
    }

    const auto points = output->GetPoints();
    if (points == nullptr) {
        return ::datacodec::validation::AssignError(
            error,
            "reused topology frame leaf has no geometry points");
    }
    auto renderPoints = points->ConvertToArray();
    if (renderPoints == nullptr) {
        return ::datacodec::validation::AssignError(
            error,
            "failed to prepare reused topology frame geometry");
    }
    renderPoints->Modified();
    drawObject->SetRenderPoints(std::move(renderPoints));
    metadata->AddInt(kDrawablePrepared, 0);
    return true;
}

DataCodecFramePresentationResult PrepareDataCodecFramePresentation(
    DataObject* currentFrame,
    DataObject* nextFrame) {
    DataCodecFramePresentationResult result;
    if (nextFrame == nullptr) { return result; }

    std::vector<LeafPresentationInfo> nextLeaves;
    CollectLeafPresentationInfo(nextFrame, nextLeaves);
    if (nextLeaves.empty()) { return result; }
    result.recognized = true;

    auto currentDrawObject = DynamicCast<DrawObject>(currentFrame);
    auto nextDrawObject = DynamicCast<DrawObject>(nextFrame);
    if (currentDrawObject != nullptr && nextDrawObject != nullptr) {
        nextDrawObject->SetName(currentDrawObject->GetName());
        nextDrawObject->SetTimeFrames(currentDrawObject->PeekTimeFrames());
        nextDrawObject->SetColorMapper(currentDrawObject->GetColorMapper());
        nextDrawObject->SetAttributeIndex(-1);
        nextDrawObject->SetPointSize(currentDrawObject->GetPointSize());
        nextDrawObject->SetLineWidth(currentDrawObject->GetLineWidth());
        nextDrawObject->SetTransparency(currentDrawObject->GetTransparency());
        nextDrawObject->SetViewStyle(currentDrawObject->GetViewStyle());
        nextDrawObject->SetVisibility(currentDrawObject->GetVisibility());
        nextDrawObject->SetDeformationData(currentDrawObject->GetDeformationData());
    }

    std::vector<LeafPresentationInfo> currentLeaves;
    CollectLeafPresentationInfo(currentFrame, currentLeaves);
    std::unordered_map<std::string, LeafPresentationInfo> currentByPath;
    currentByPath.reserve(currentLeaves.size());
    for (auto& leaf : currentLeaves) {
        currentByPath.emplace(leaf.path, std::move(leaf));
    }

    result.drawableReady = true;
    for (auto& nextLeaf : nextLeaves) {
        if (nextLeaf.drawablePrepared) { continue; }
        bool reused = false;
        if (nextLeaf.topologyMode == ::datacodec::TopologyOwnershipMode::Reused) {
            const auto currentIterator = currentByPath.find(nextLeaf.path);
            if (currentIterator != currentByPath.end() &&
                CompatibleTopologyOwner(currentIterator->second, nextLeaf)) {
                if (!currentIterator->second.drawablePrepared) {
                    currentIterator->second.object->ForceReConvertToDrawableData();
                    currentIterator->second.object->ConvertToDrawableData();
                    if (auto* metadata = currentIterator->second.object->GetMetadata(); metadata != nullptr) {
                        metadata->AddInt(kDrawablePrepared, 1);
                    } else {
                        result.drawableReady = false;
                    }
                }
                nextLeaf.object->ReuseTopologyDrawDataFrom(currentIterator->second.object);
                reused = true;
                ++result.reusedTopologyLeafCount;
            }
        }
        if (!reused) {
            nextLeaf.object->ForceReConvertToDrawableData();
            nextLeaf.object->ConvertToDrawableData();
        }
        if (auto* metadata = nextLeaf.object->GetMetadata(); metadata != nullptr) {
            metadata->AddInt(kDrawablePrepared, 1);
        } else {
            result.drawableReady = false;
        }
    }
    return result;
}

bool IsDataCodecPreparedFrame(const DataObject* object) {
    if (object == nullptr) { return false; }
    std::vector<LeafPresentationInfo> leaves;
    CollectLeafPresentationInfo(const_cast<DataObject*>(object), leaves);
    return !leaves.empty();
}

std::optional<std::uint32_t> DataCodecFrameIndex(const DataObject* object) {
    if (object == nullptr) { return std::nullopt; }
    std::vector<LeafPresentationInfo> leaves;
    CollectLeafPresentationInfo(const_cast<DataObject*>(object), leaves);
    return leaves.empty()
        ? std::optional<std::uint32_t>{}
        : std::optional<std::uint32_t>{leaves.front().frameIndex};
}

IGAME_NAMESPACE_END
