#ifndef DATACODEC_WORKFLOW_DECODE_STAGES_PARAMSDECODESTAGE_H
#define DATACODEC_WORKFLOW_DECODE_STAGES_PARAMSDECODESTAGE_H

#include "DataCodec/Runtime/Context/DecodeContext.h"
#include "DataCodec/Runtime/Workspace/DecodeLeafWorkspace.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageFieldDecodeStream.h"
#include "DataCodec/API/Params/ParamsDecodeLimits.h"
#include "DataCodec/Runtime/Failure/DecodeFailureManagement.h"
#include "DataCodec/Workflow/Decode/Stages/FieldDecodeInput.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"

#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

class ParamsDecodeStage final : public DecodeStage {
public:
    static constexpr std::string_view kTypeName = "ParamsDecodeStage";

    ParamsDecodeStage() = default;
    explicit ParamsDecodeStage(FieldDecodeInput input) : m_input(input) {}

    const char* Name() const override { return "ParamsDecodeStage"; }

    // 先解析 params field，后续 stage 才知道缓冲区尺寸和 codec 类型
    void Execute(DecodeContext& context, DecodeLeafWorkspace& workspace) override {
        std::vector<std::uint8_t> field;
        std::string error;
        if (!m_input.HasField()) {
            FailDecodeStage(
                context,
                workspace,
                "ParamsDecodeStage",
                CodecErrorCode::MissingInput,
                "failed to find params field");
            return;
        }
        if (m_input.field->rawSize > kMaxDecodedParamsBytes) {
            FailDecodeStage(
                context,
                workspace,
                "ParamsDecodeStage",
                CodecErrorCode::PipelineFailure,
                "rejected oversized params field");
            return;
        }
        decodefield::FieldDecodeStreamReader reader;
        if (!decodefield::OpenLeafPackageFieldDecodeStream(
                *m_input.field,
                workspace.CacheResourcesRef(),
                reader,
                &error)) {
            FailDecodeStage(
                context,
                workspace,
                "ParamsDecodeStage",
                CodecErrorCode::PipelineFailure,
                "failed to read params field: " + error);
            return;
        }
        decodefield::FieldDecodeByteStream stream(reader);
        if (!stream.ReadVector(field, m_input.field->rawSize, &error)) {
            FailDecodeStage(
                context,
                workspace,
                "ParamsDecodeStage",
                CodecErrorCode::PipelineFailure,
                "failed to read params bytes: " + error);
            return;
        }
        CodecStorageParams storageParams;
        CodecErrorCode paramsErrorCode = CodecErrorCode::PipelineFailure;
        if (!DeserializeCodecStorageParams(field, storageParams, &error, &paramsErrorCode)) {
            FailDecodeStage(
                context,
                workspace,
                "ParamsDecodeStage",
                paramsErrorCode,
                "failed to parse params: " + error);
            return;
        }
        std::string adapterError;
        if (!context.adapter->SetMeshType(storageParams.meshType, &adapterError)) {
            FailDecodeStage(
                context,
                workspace,
                "ParamsDecodeStage",
                CodecErrorCode::PipelineFailure,
                "failed to initialize decode adapter output: " + adapterError);
            return;
        }
        workspace.SetStorageParams(std::move(storageParams));
    }

private:
    FieldDecodeInput m_input;
};

} // namespace datacodec

#endif
