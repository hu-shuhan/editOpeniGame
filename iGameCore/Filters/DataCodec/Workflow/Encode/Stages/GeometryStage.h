#ifndef DATACODEC_WORKFLOW_ENCODE_STAGES_GEOMETRYSTAGE_H
#define DATACODEC_WORKFLOW_ENCODE_STAGES_GEOMETRYSTAGE_H

#include "DataCodec/Codec/Geometry/GeometryEncode.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Failure/EncodeFailureManagement.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
namespace datacodec {

inline bool EncodeGeometryTransferCache(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const std::size_t transferCacheIndex) {
    GeometryTransferCacheResult geometryTransferCache;
    std::string error;
    const auto* geometry = workspace.GeometrySourceView();
    if (geometry == nullptr) {
        FailEncodeStage(
            context,
            workspace,
            "GeometryStage",
            CodecErrorCode::MissingInput,
            "failed to prepare remapped geometry cache");
        return false;
    }

    GeometryEncodeRuntime runtime{
        .data = GeometryEncodeData{
            .keyFrameReference = context.geometryKeyFrameReference,
            .temporalRole = context.geometryTemporalRole,
            .keyFrameIndex = context.geometryKeyFrameIndex,
            .spatialBlockElementCount =
                workspace.StorageParams().spatialBlockParams.pointElementCount,
        },
        .cache = GeometryEncodeCache{
            .cacheResources = workspace.CacheResourcesRef(),
            .byteStoreSession = workspace.ByteStoreSessionRef(),
            .currentReferenceCache = context.currentGeometryReferenceCache,
            .useMemoryTransferCache =
                workspace.ResourceBudget().GeometryEncodeTransferCacheStorageMode() == EncodeStorageMode::Memory,
            .useMemoryStaging =
                workspace.ResourceBudget().GeometryEncodeStagingStorageMode() == EncodeStorageMode::Memory,
        },
    };
    const GeometryReferenceControlParams defaultDependency{};
    auto geometrySource = workspace.GeometryNumericArraySource();
    if (!BuildGeometryTransferCache(
            std::move(geometrySource),
            workspace.StorageParams().geomParams,
            runtime,
            context.controlParams != nullptr
                ? &context.controlParams->geomControl
                : nullptr,
            context.controlParams != nullptr
                ? context.controlParams->geometryReference
                : defaultDependency,
            geometryTransferCache,
            &error)) {
        FailEncodeStage(
            context,
            workspace,
            "GeometryStage",
            CodecErrorCode::EncodeFailure,
            "failed to build geometry transfer cache: " + error);
        return false;
    }

    auto publishedPayload = geometryTransferCache.transferCache->TakePayload();
    if (publishedPayload == nullptr) {
        FailEncodeStage(
            context,
            workspace,
            "GeometryStage",
            CodecErrorCode::EncodeFailure,
            "geometry transfer cache payload is missing");
        return false;
    }
    workspace.StorageParams().geomParams = geometryTransferCache.transferCache->Meta();
    workspace.PublishTransferCache(
        transferCacheIndex,
        std::move(publishedPayload),
        workspace.StorageParams().geomParams.codecType);
    return true;
}

class GeometryStage final : public EncodeStage {
public:
    static constexpr std::string_view kTypeName = "GeometryStage";

    // 把该 stage 绑定到 pipeline 预先准备好的 geometry transfer cache
    explicit GeometryStage(const std::size_t transferCacheIndex) : m_transferCacheIndex(transferCacheIndex) {}

    const char* Name() const override { return "GeometryStage"; }

    // 编码 pipeline 已准备好的重排后几何分量缓冲
    EncodeStageExecutionStatus Execute(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace) override {
        return EncodeGeometryTransferCache(context, workspace, m_transferCacheIndex)
            ? EncodeStageExecutionStatus::Completed
            : EncodeStageExecutionStatus::Failed;
    }

private:
    // 当前 stage 实例负责写入的 transfer cache 槽位
    std::size_t m_transferCacheIndex{kInvalidTransferCacheIndex};
};

} // namespace datacodec

#endif
