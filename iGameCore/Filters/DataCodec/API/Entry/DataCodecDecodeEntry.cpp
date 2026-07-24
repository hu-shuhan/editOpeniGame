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
    ResolvedDataCodecExecutionResources resolvedResources;
    std::string resourceError;
    if (!ResolveDataCodecExecutionResources(
            request.executionResources,
            request.execution.enableParallelStages,
            resolvedResources,
            &resourceError)) {
        return MakeDecodeEntryFailure(
            request,
            "execution.task-runner-required",
            std::move(resourceError));
    }
    return ExecutePackageDecodeWorkflow(request, resolvedResources.resources);
}

} // namespace datacodec
