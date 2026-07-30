#include "DataCodec/API/Entry/DataCodecEncodeEntry.h"
#include "DataCodec/Runtime/Output/DataCodecOutputRouter.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"

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

EncodeOutput EncodeOutput::Memory(const EncodePackageKind kind) {
    return EncodeOutput{.packageKind = kind};
}

EncodeOutput EncodeOutput::ByteRange(
    IByteRangeOutput& sink,
    const EncodePackageKind kind) {
    return EncodeOutput{.packageKind = kind, .target = &sink};
}

namespace {

void AppendAllAttributeTargets(
    const IEncodeAdapter& adapter,
    const std::uint32_t frameIndex,
    const BlockPath& blockPath,
    std::vector<AttributeTarget>& targets) {
    const auto pointAttributeCount = adapter.GetNumberOfPointAttrs();
    const auto cellAttributeCount = adapter.GetNumberOfCellAttrs();
    targets.reserve(targets.size() + pointAttributeCount + cellAttributeCount);
    for (std::size_t index = 0u; index < pointAttributeCount; ++index) {
        targets.push_back(AttributeTarget{
            .frameIndex = frameIndex,
            .blockPath = blockPath,
            .attrIndex = index,
        });
    }
    for (std::size_t index = 0u; index < cellAttributeCount; ++index) {
        targets.push_back(AttributeTarget{
            .frameIndex = frameIndex,
            .blockPath = blockPath,
            .attrIndex = pointAttributeCount + index,
        });
    }
}

bool ResolveEncodeAttributeTargets(
    const EncodeRequest& request,
    IEncodeAdapter* leafAdapter,
    IBlockTreeAdapter* blockTreeAdapter,
    std::vector<AttributeTarget>& targets,
    std::string& error) {
    targets.clear();
    if (request.attributeSelection != AttributeSelectionMode::Explicit &&
        !request.attributeTargets.empty()) {
        error = "explicit attribute targets require AttributeSelectionMode::Explicit";
        return false;
    }
    if (request.attributeSelection == AttributeSelectionMode::None) {
        return true;
    }
    if (request.attributeSelection == AttributeSelectionMode::Explicit) {
        targets = request.attributeTargets;
        return true;
    }
    if (request.attributeSelection != AttributeSelectionMode::AllAvailable) {
        error = "encode request contains an unknown attribute selection mode";
        return false;
    }
    if (leafAdapter != nullptr) {
        AppendAllAttributeTargets(
            *leafAdapter,
            request.input.frameIndex,
            request.input.leafPath,
            targets);
        return true;
    }
    if (blockTreeAdapter == nullptr) {
        error = "encode request has no adapter for attribute enumeration";
        return false;
    }
    for (const auto& leaf : blockTreeAdapter->GetLeafRecords()) {
        auto adapter = blockTreeAdapter->GetLeaf(leaf.path);
        if (adapter == nullptr) {
            error = "block tree adapter failed to create a leaf adapter for attribute enumeration";
            return false;
        }
        AppendAllAttributeTargets(
            *adapter,
            request.input.frameIndex,
            leaf.path,
            targets);
    }
    return true;
}

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
    RunRecordDispatcher outputRecords;
    outputRecords.AddSink(request.runRecordSink);
    if (!request.outputSinks.Empty()) {
        outputRecords.AddSink(std::make_shared<DataCodecOutputRouter>(request.outputSinks));
    }
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
            .language = request.configuration.language,
        },
        &outputRecords);
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
    const auto resolvedResources = ResolveDataCodecExecutionResources(
        request.executionResources);
    RunRecordDispatcher outputRecords;
    outputRecords.AddSink(request.runRecordSink);
    if (!request.outputSinks.Empty()) {
        outputRecords.AddSink(std::make_shared<DataCodecOutputRouter>(request.outputSinks));
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

    std::vector<AttributeTarget> attributeTargets;
    std::string attributeSelectionError;
    if (!ResolveEncodeAttributeTargets(
            request,
            leafAdapter,
            blockTreeAdapter,
            attributeTargets,
            attributeSelectionError)) {
        return MakeEncodeEntryFailure(
            request,
            "encode.attribute-selection",
            std::move(attributeSelectionError));
    }

    auto runtimeConfiguration = request.configuration;
    CodecControlParamsFactory::ApplyEncodeRuntimeConstraint(
        runtimeConfiguration,
        runtimeConfiguration.source.runtimeProfile);

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
                .configurationSource = runtimeConfiguration.source,
                .language = runtimeConfiguration.language,
                .runRecordSink = &outputRecords,
                .outputSink = outputSink,
                .attributeTargets = std::span<const AttributeTarget>(attributeTargets),
                .enableParallelStages = runtimeConfiguration.execution.enableParallelStages,
                .parallelTaskRunner = resolvedResources.parallelTaskRunner,
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
    context.language = runtimeConfiguration.language;
    context.attributeTargets = std::span<const AttributeTarget>(attributeTargets);
    context.controlParams = &runtimeConfiguration.controlParams;

    return MakeLeafEncodeResult(
        LeafEncodeExecutor::Execute({
            .context = &context,
            .pipelineControl = runtimeConfiguration.pipelineControl,
            .configurationSource = runtimeConfiguration.source,
            .language = runtimeConfiguration.language,
            .runRecordSink = &outputRecords,
            .outputSink = outputSink,
            .enableParallelStages = runtimeConfiguration.execution.enableParallelStages,
            .parallelTaskRunner = resolvedResources.parallelTaskRunner,
        }),
        packageKind);
}

} // namespace datacodec
