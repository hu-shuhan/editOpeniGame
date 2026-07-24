#ifndef DATACODEC_WORKFLOW_ENCODE_STAGES_PARAMSENCODESTAGE_H
#define DATACODEC_WORKFLOW_ENCODE_STAGES_PARAMSENCODESTAGE_H

#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"
#include "DataCodec/Runtime/Failure/EncodeFailureManagement.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"

#include <cstdint>
#include <memory>
#include <string>
namespace datacodec {

class ParamsEncodeStage final : public EncodeStage {
public:
    static constexpr std::string_view kTypeName = "ParamsEncodeStage";

    const char* Name() const override { return "ParamsEncodeStage"; }

    void PrepareOutputs(EncodeContext&, EncodeLeafWorkspace& workspace) override {
        workspace.MarkTransferCachePending(workspace.TransferCacheLayout().params);
    }

    // 汇总所有编码 stage 写入的元数据，并生成最终 params transfer cache
    EncodeStageExecutionStatus Execute(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace) override {
        if (workspace.TransferCacheLayout().attributes.has_value() && !ValidateAttribute(context, workspace)) {
            return EncodeStageExecutionStatus::Failed;
        }
        if (workspace.TransferCacheLayout().attributes.has_value()) {
            workspace.MarkTransferCacheReady(*workspace.TransferCacheLayout().attributes);
        }
        if (workspace.TransferCacheLayout().params == kInvalidTransferCacheIndex) {
            FailEncodeStage(
                context,
                workspace,
                kTypeName,
                CodecErrorCode::PipelineFailure,
                "params transfer cache layout was not initialized");
            return EncodeStageExecutionStatus::Failed;
        }
        std::vector<std::uint8_t> paramsBytes;
        std::string error;
        RefreshAttributeDecodeScheduleHints(workspace.StorageParams());
        if (!SerializeCodecStorageParams(workspace.StorageParams(), paramsBytes, &error)) {
            FailEncodeStage(
                context,
                workspace,
                kTypeName,
                CodecErrorCode::InvalidInput,
                "failed to serialize params: " + error);
            return EncodeStageExecutionStatus::Failed;
        }
        workspace.PublishTransferCacheBytes(
            workspace.TransferCacheLayout().params,
            std::move(paramsBytes),
            EncodedFieldCodecType::Params);
        return EncodeStageExecutionStatus::Completed;
    }

private:
    static bool ValidateAttribute(EncodeContext& context, EncodeLeafWorkspace& workspace) {
        const auto attributeOutput = workspace.AttributeOutput();
        if (attributeOutput == nullptr) {
            FailEncodeStage(
                context,
                workspace,
                kTypeName,
                CodecErrorCode::PipelineFailure,
                "attribute output was not initialized");
            return false;
        }
        if (const auto missingRecords = attributeOutput->MissingRecordCount(); missingRecords > 0u) {
            FailEncodeStage(
                context,
                workspace,
                kTypeName,
                CodecErrorCode::EncodeFailure,
                "attribute source is missing " + std::to_string(missingRecords) + " records");
            return false;
        }
        return true;
    }
};

} // namespace datacodec

#endif
