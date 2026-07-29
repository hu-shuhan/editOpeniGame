#include "DataCodec/API/Entry/DataCodecDecodeEntry.h"

#include "DataCodec/Runtime/Record/RunRecordEmitter.h"
#include "DataCodec/Runtime/Record/RunRecordTimestamp.h"
#include "DataCodec/Workflow/Decode/PackageDecodeWorkflow.h"

namespace datacodec {

namespace {

DecodePackageResult MakeDecodeEntryFailure(
    const DecodePackageRequest& request,
    std::string code,
    std::string text) {
    TelemetryMessageRecord message{
        .severity = TelemetryMessageSeverity::Error,
        .origin = "DataCodecDecodeEntry",
        .code = std::move(code),
        .text = std::move(text),
    };
    RunRecordEmitter records;
    records.Reset(
        RunRecordInfo{
            .generatedAtUtc = runrecorddetail::MakeTimestampUtc(),
            .runKind = TelemetryRunKind::Decode,
        },
        request.runRecordSink.get());
    records.BeginRun();
    records.AddMessage(message);
    records.EndRun(RunEndRecord{.success = false});
    DecodePackageResult result;
    result.messages.push_back(std::move(message));
    return result;
}

} // 匿名命名空间

DecodePackageResult DecodePackage(const DecodePackageRequest& request) {
    if (request.attributeSelection != AttributeSelectionMode::Explicit &&
        !request.attributeTargets.empty()) {
        return MakeDecodeEntryFailure(
            request,
            "decode.attribute-selection",
            "explicit attribute targets require AttributeSelectionMode::Explicit");
    }
    if (request.attributeSelection != AttributeSelectionMode::None &&
        request.attributeSelection != AttributeSelectionMode::AllAvailable &&
        request.attributeSelection != AttributeSelectionMode::Explicit) {
        return MakeDecodeEntryFailure(
            request,
            "decode.attribute-selection",
            "decode request contains an unknown attribute selection mode");
    }
    const auto resolvedResources = ResolveDataCodecExecutionResources(
        request.executionResources);
    return ExecutePackageDecodeWorkflow(request, resolvedResources);
}

} // namespace datacodec
