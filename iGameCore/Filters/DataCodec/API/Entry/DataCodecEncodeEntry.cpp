#include "DataCodec/API/Entry/DataCodecEncodeEntry.h"

#include "DataCodec/Runtime/Record/RunRecordEmitter.h"
#include "DataCodec/Runtime/Record/RunRecordTimestamp.h"
#include "DataCodec/Workflow/Frame/FrameEncodeExecutor.h"
#include "DataCodec/Workflow/Leaf/LeafEncodeExecutor.h"

#include <span>

namespace datacodec {

EncodeInput EncodeInput::LeafAdapter(
    IEncodeAdapter* inputAdapter,
    BlockPath path,
    std::string name,
    std::string type,
    const std::uint32_t inputFrameIndex) {
    EncodeInput input;
    input.adapter = inputAdapter;
    input.leafPath = std::move(path);
    input.objectName = std::move(name);
    input.meshType = std::move(type);
    input.frameIndex = inputFrameIndex;
    return input;
}

EncodeInput EncodeInput::BlockTreeAdapter(
    IBlockTreeAdapter* inputAdapter,
    std::string name,
    const std::uint32_t inputFrameIndex,
    const std::uint32_t inputFrameCount,
    const float inputTimeValue) {
    EncodeInput input;
    input.adapter = inputAdapter;
    input.rootName = std::move(name);
    input.frameIndex = inputFrameIndex;
    input.frameCount = inputFrameCount;
    input.timeValue = inputTimeValue;
    return input;
}

EncodeOutput EncodeOutput::ByteRange(
    const EncodePackageKind kind,
    IByteRangeOutput* sink) {
    return EncodeOutput{.packageKind = kind, .target = sink};
}

namespace {

EncodeResult MakeEncodeEntryFailure(
    const EncodeRequest& request,
    std::string code,
    std::string text) {
    TelemetryMessageRecord message{
        .severity = TelemetryMessageSeverity::Error,
        .origin = "DataCodecEncodeEntry",
        .code = std::move(code),
        .text = std::move(text),
    };
    RunRecordEmitter records;
    records.Reset(
        RunRecordInfo{
            .generatedAtUtc = runrecorddetail::MakeTimestampUtc(),
            .runKind = TelemetryRunKind::Encode,
            .objectName = !request.input.objectName.empty()
                ? request.input.objectName
                : request.input.rootName,
            .leafPath = request.input.leafPath,
            .meshType = request.input.meshType,
        },
        request.runRecordSink.get());
    records.BeginRun();
    records.AddMessage(message);
    records.EndRun(RunEndRecord{.success = false});
    EncodeResult result;
    result.messages.push_back(std::move(message));
    return result;
}

EncodeResult MakeLeafEncodeResult(
    LeafEncodeResult result,
    const EncodePackageKind packageKind) {
    EncodeResult output;
    output.success = result.success;
    output.hasEncodedOutput = result.hasEncodedOutput;
    output.encodedBytes = std::move(result.encodedBytes);
    output.encodedByteCount = result.encodedByteCount;
    output.leafCount = result.hasEncodedOutput ? 1u : 0u;
    output.packageKind = packageKind == EncodePackageKind::Auto
        ? EncodePackageKind::LeafPackage
        : packageKind;
    output.messages = std::move(result.messages);
    return output;
}

EncodeResult MakeFrameEncodeResult(
    FrameEncodeResult result,
    const EncodePackageKind packageKind,
    const std::size_t leafCount) {
    EncodeResult output;
    output.success = result.success;
    output.hasEncodedOutput = result.hasEncodedOutput;
    output.encodedBytes = std::move(result.encodedBytes);
    output.encodedByteCount = result.encodedByteCount;
    output.leafCount = leafCount;
    output.packageKind = packageKind == EncodePackageKind::Auto
        ? EncodePackageKind::FramePackage
        : packageKind;
    output.messages = std::move(result.messages);
    return output;
}

} // 匿名命名空间

EncodeResult Encode(const EncodeRequest& request) {
    ResolvedDataCodecExecutionResources resolvedResources;
    std::string resourceError;
    if (!ResolveDataCodecExecutionResources(
            request.executionResources,
            request.execution.enableParallelStages,
            resolvedResources,
            &resourceError)) {
        return MakeEncodeEntryFailure(
            request,
            "execution.task-runner-required",
            std::move(resourceError));
    }
    const auto packageKind = request.output.packageKind;
    const auto outputSinkEntry = std::get_if<IByteRangeOutput*>(&request.output.target);
    const auto leafAdapterEntry = std::get_if<IEncodeAdapter*>(&request.input.adapter);
    const auto blockTreeAdapterEntry = std::get_if<IBlockTreeAdapter*>(&request.input.adapter);
    auto* outputSink = outputSinkEntry != nullptr ? *outputSinkEntry : nullptr;
    auto* leafAdapter = leafAdapterEntry != nullptr ? *leafAdapterEntry : nullptr;
    auto* blockTreeAdapter = blockTreeAdapterEntry != nullptr ? *blockTreeAdapterEntry : nullptr;
    const bool hasLeafAdapter = leafAdapter != nullptr;
    const bool hasBlockTreeAdapter = blockTreeAdapter != nullptr;
    if ((!hasLeafAdapter && !hasBlockTreeAdapter) ||
        (packageKind == EncodePackageKind::LeafPackage && !hasLeafAdapter) ||
        (packageKind == EncodePackageKind::FramePackage && !hasBlockTreeAdapter) ||
        (outputSinkEntry != nullptr && outputSink == nullptr)) {
        return MakeEncodeEntryFailure(
            request,
            "encode.request.contract",
            "encode request contains an invalid input or output combination");
    }

    DataCodecEncodeConfigurationParams runtimeConfiguration{
        .controlParams = request.controlParams,
        .pipelineControl = request.pipelineControl,
        .execution = request.execution,
        .source = request.configurationSource,
    };
    CodecControlParamsFactory::ApplyEncodeRuntimeConstraint(
        runtimeConfiguration,
        request.configurationSource.runtimeProfile);

    if (blockTreeAdapter != nullptr) {
        const auto leafCount = blockTreeAdapter->GetLeafRecords().size();
        return MakeFrameEncodeResult(
            FrameEncodeExecutor::Execute({
                .blockTreeAdapter = blockTreeAdapter,
                .rootName = request.input.rootName,
                .frameIndex = request.input.frameIndex,
                .frameCount = request.input.frameCount,
                .timeValue = request.input.timeValue,
                .controlParams = &runtimeConfiguration.controlParams,
                .pipelineControl = runtimeConfiguration.pipelineControl,
                .configurationSource = request.configurationSource,
                .runRecordSink = request.runRecordSink.get(),
                .outputSink = outputSink,
                .attributeTargets = std::span<const AttributeTarget>(request.attributeTargets),
                .enableParallelStages = runtimeConfiguration.execution.enableParallelStages,
                .parallelTaskRunner = resolvedResources.resources.parallelTaskRunner,
            }),
            packageKind,
            leafCount);
    }

    EncodeContext context;
    context.adapter = leafAdapter;
    context.objectName = request.input.objectName;
    context.meshType = request.input.meshType;
    context.path = request.input.leafPath;
    context.frameIndex = request.input.frameIndex;
    context.attributeTargets = std::span<const AttributeTarget>(request.attributeTargets);
    context.controlParams = &runtimeConfiguration.controlParams;

    return MakeLeafEncodeResult(
        LeafEncodeExecutor::Execute({
            .context = &context,
            .pipelineControl = runtimeConfiguration.pipelineControl,
            .configurationSource = request.configurationSource,
            .runRecordSink = request.runRecordSink.get(),
            .outputSink = outputSink,
            .enableParallelStages = runtimeConfiguration.execution.enableParallelStages,
            .parallelTaskRunner = resolvedResources.resources.parallelTaskRunner,
        }),
        packageKind);
}

} // namespace datacodec
