#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREPIPELINECONTRACTS_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREPIPELINECONTRACTS_H

#include "DataCodec/Codec/NumericArray/SpatialBlockLayout.h"
#include "DataCodec/Codec/Remap/Common/MortonRemapBuilder.h"
#include "DataCodec/Runtime/Cache/TransferCache/ReferenceTransferCacheBuilder.h"
#include "DataCodec/Codec/Reference/AttributeReferenceScheduleBuilder.h"
#include "DataCodec/Codec/Topology/TopologyFingerprint.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/API/Params/CodecParamDefaults.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Workflow/Encode/EncodePipelineBinding.h"
#include "DataCodec/Workflow/Encode/EncodePipeline.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Workflow/Leaf/LeafEncodeExecutor.h"
#include "DataCodec/Workflow/Session/EncodeSessionWorkspace.h"
#include "DataCodec/Workflow/Temporal/TemporalBuilder.h"
#include "DataCodec/Workflow/FrameSequence/FrameSequenceEncodeExecutor.h"
#include "DataCodec/Test/Adapter/DataCodecTestAdapter.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <algorithm>
#include <array>
#include <string>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec::test {

inline bool HasPipelineStageName(
    const std::vector<EncodeStageId>& stageIds,
    const std::string_view name) {
    return std::any_of(
        stageIds.begin(),
        stageIds.end(),
        [name](const EncodeStageId& stageId) {
            return stageId.name == name;
        });
}

inline bool HasPipelineStagePrefix(
    const std::vector<EncodeStageId>& stageIds,
    const std::string_view prefix) {
    return std::any_of(
        stageIds.begin(),
        stageIds.end(),
        [prefix](const EncodeStageId& stageId) {
            return stageId.name.starts_with(prefix);
        });
}

inline bool CheckResolvedFormalPipeline(
    TestResult& result,
    std::string* error = nullptr) {
    auto dataset = MakePipelineContractUnstructuredDataset();
    TestEncodeAdapter adapter(dataset);
    auto params = CodecControlParamsFactory::MakeDefault();
    params.SetSpatialBlockElementCounts(4u, 1u);
    const std::vector<AttributeTarget> targets{
        AttributeTarget{.frameIndex = 0u, .blockPath = {}, .attrIndex = 0u},
        AttributeTarget{.frameIndex = 0u, .blockPath = {}, .attrIndex = 1u},
        AttributeTarget{.frameIndex = 0u, .blockPath = {}, .attrIndex = 2u},
    };
    EncodeContext context;
    context.adapter = &adapter;
    context.controlParams = &params;
    context.frameIndex = 0u;
    context.path = {};
    context.attributeTargets = std::span<const AttributeTarget>(
        targets.data(),
        targets.size());
    auto controls = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{.tier = DataCodecEncodeTier::Balanced});
    // 本用例显式选择双 Morton 流程以验证 Point 到 Cell 的阶段依赖
    controls.pipelineControl.cellOrder = EncodeCellOrderMode::Morton;
    EncodePipelineBinding binding;
    if (!ResolveEncodePipelineBinding(
        adapter,
        params,
        controls.pipelineControl,
        EncodePipelineExecutionProfile{
            .resourceBudget = params.resourceBudget,
            .enableParallelStages = false,
            .parallelTaskRunner = nullptr,
        },
        binding,
        error,
        EncodePipelineOutputKind::LeafPackage)) {
        return false;
    }
    EncodePipeline pipeline(EncodePipelineOptions{
        .binding = std::move(binding),
    });
    const auto stageGraph = pipeline.DescribeStageGraph(context);
    const auto stageIds = pipeline.DescribeStageIds(context);
    if (context.HasFailure()) {
        return validation::AssignError(
            error,
            context.failure.has_value()
                ? context.failure->formattedMessage
                : "formal pipeline description failed");
    }

    const auto hasFormalStages =
        HasPipelineStagePrefix(stageIds, "PointSpatialPartition.Morton") &&
        HasPipelineStagePrefix(stageIds, "CellSpatialPartition.Morton") &&
        HasPipelineStageName(stageIds, "GeometryStage") &&
        HasPipelineStageName(stageIds, "TopoStage") &&
        HasPipelineStageName(stageIds, "PointAttributeStage") &&
        HasPipelineStageName(stageIds, "CellAttributeStage") &&
        HasPipelineStageName(stageIds, "ParamsEncodeStage") &&
        HasPipelineStagePrefix(stageIds, "ReferenceEncode.AttributeIntra.AffineSpatialBlock") &&
        HasPipelineStageName(stageIds, "PackageFieldZstd.Streaming") &&
        HasPipelineStageName(stageIds, "PackageAssembly.LeafPackage");
    const auto formalStagesOk = Require(
        result,
        hasFormalStages,
        "pipeline.resolvedFormalStages",
        "resolved unstructured pipeline is missing a required concrete stage variant");
    const auto pointIt = std::find_if(
        stageGraph.begin(),
        stageGraph.end(),
        [](const EncodeStageDescription& description) {
            return description.stageId.name.starts_with("PointSpatialPartition.Morton");
        });
    const auto cellIt = std::find_if(
        stageGraph.begin(),
        stageGraph.end(),
        [](const EncodeStageDescription& description) {
            return description.stageId.name.starts_with("CellSpatialPartition.Morton");
        });
    const auto hasHardDependency =
        pointIt != stageGraph.end() &&
        cellIt != stageGraph.end() &&
        std::find(
            cellIt->dependencies.begin(),
            cellIt->dependencies.end(),
            pointIt->stageId) != cellIt->dependencies.end();
    const auto dependencyOk = Require(
        result,
        hasHardDependency,
        "pipeline.pointBeforeCellDependency",
        "cell spatial partition has no explicit point spatial partition dependency");
    return formalStagesOk && dependencyOk;
}

inline bool CheckTopologyReuseSpatialDependency(TestResult& result) {
    auto ownerDataset = MakePipelineContractUnstructuredDataset();
    auto currentDataset = ownerDataset;
    currentDataset.points[0] += 0.5f;
    TestEncodeAdapter ownerAdapter(ownerDataset);
    TestEncodeAdapter currentAdapter(currentDataset);
    TopologyFingerprint ownerTopology;
    TopologyFingerprint currentTopology;
    std::string fingerprintError;
    const auto fingerprintsBuilt =
        TopologyFingerprintBuilder::Build(ownerAdapter, ownerTopology, &fingerprintError) &&
        TopologyFingerprintBuilder::Build(currentAdapter, currentTopology, &fingerprintError);
    const auto ownerSpatial = PointSpatialFingerprintBuilder::Build(ownerAdapter);
    const auto currentSpatial = PointSpatialFingerprintBuilder::Build(currentAdapter);
    return Require(
        result,
        fingerprintsBuilt && ownerTopology == currentTopology && !(ownerSpatial == currentSpatial),
        "pipeline.topologyReuseSpatialDependency",
        fingerprintError.empty()
            ? "topology reuse cannot distinguish changed point spatial partition input"
            : fingerprintError);
}

inline bool CheckFormalPipelineExecution(
    TestResult& result,
    std::string* error = nullptr) {
    auto dataset = MakePipelineContractUnstructuredDataset();
    TestEncodeAdapter adapter(dataset);
    auto params = CodecControlParamsFactory::MakeDefault();
    params.SetSpatialBlockElementCounts(4u, 1u);
    const std::vector<AttributeTarget> targets{
        AttributeTarget{.frameIndex = 0u, .blockPath = {}, .attrIndex = 0u},
        AttributeTarget{.frameIndex = 0u, .blockPath = {}, .attrIndex = 1u},
        AttributeTarget{.frameIndex = 0u, .blockPath = {}, .attrIndex = 2u},
    };
    EncodeContext context;
    context.adapter = &adapter;
    context.controlParams = &params;
    context.objectName = dataset.name;
    context.meshType = "UnstructuredMesh";
    context.frameIndex = 0u;
    context.path = {};
    context.attributeTargets = std::span<const AttributeTarget>(
        targets.data(),
        targets.size());
    const auto encoded = LeafEncodeExecutor::Execute(LeafEncodeRequest{
        .context = &context,
        .enableParallelStages = false,
        .parallelTaskRunner = nullptr,
    });
    if (!encoded.success || !encoded.hasEncodedOutput || encoded.encodedBytes.empty()) {
        if (error != nullptr) {
            *error = context.failure.has_value()
                ? context.failure->formattedMessage
                : "formal unstructured pipeline did not produce encoded output";
        }
        return false;
    }
    const auto allCompleted = std::all_of(
        encoded.stageExecutions.begin(),
        encoded.stageExecutions.end(),
        [](const EncodeStageExecutionRecord& record) {
            return record.status == EncodeStageExecutionStatus::Completed ||
                record.status == EncodeStageExecutionStatus::EmptyInput;
        });
    const auto hasPoint = std::any_of(
        encoded.stageExecutions.begin(),
        encoded.stageExecutions.end(),
        [](const EncodeStageExecutionRecord& record) {
            return record.stageId.name.starts_with("PointSpatialPartition.Morton");
        });
    const auto hasCell = std::any_of(
        encoded.stageExecutions.begin(),
        encoded.stageExecutions.end(),
        [](const EncodeStageExecutionRecord& record) {
            return record.stageId.name.starts_with("CellSpatialPartition.Morton");
        });
    const auto hasZstd = std::any_of(
        encoded.stageExecutions.begin(),
        encoded.stageExecutions.end(),
        [](const EncodeStageExecutionRecord& record) {
            return record.stageId.name == "PackageFieldZstd.Streaming";
        });
    const auto hasAssembly = std::any_of(
        encoded.stageExecutions.begin(),
        encoded.stageExecutions.end(),
        [](const EncodeStageExecutionRecord& record) {
            return record.stageId.name == "PackageAssembly.LeafPackage";
        });
    const auto zstdPipelineOk = Require(
        result,
        allCompleted && hasPoint && hasCell && hasZstd && hasAssembly,
        "pipeline.formalExecution",
        "formal unstructured pipeline did not complete every required stage");

    EncodeContext rawContext;
    rawContext.adapter = &adapter;
    rawContext.controlParams = &params;
    rawContext.objectName = dataset.name;
    rawContext.meshType = "UnstructuredMesh";
    rawContext.frameIndex = 0u;
    rawContext.path = {};
    rawContext.attributeTargets = std::span<const AttributeTarget>(
        targets.data(),
        targets.size());
    auto rawControl = EncodePipelineControlParams{};
    rawControl.packageFields.mode = PackageFieldEncodingMode::Raw;
    const auto rawEncoded = LeafEncodeExecutor::Execute(LeafEncodeRequest{
        .context = &rawContext,
        .pipelineControl = rawControl,
        .enableParallelStages = false,
        .parallelTaskRunner = nullptr,
    });
    const auto hasRaw = std::any_of(
        rawEncoded.stageExecutions.begin(),
        rawEncoded.stageExecutions.end(),
        [](const EncodeStageExecutionRecord& record) {
            return record.stageId.name == "PackageFieldRaw.Streaming";
        });
    const auto rawPipelineOk = Require(
        result,
        rawEncoded.success && rawEncoded.hasEncodedOutput && hasRaw,
        "pipeline.rawExecution",
        "raw package pipeline did not execute the explicit raw field stage");
    return zstdPipelineOk && rawPipelineOk;
}

inline bool CheckTemporalPipelineExecution(
    TestResult& result,
    std::string* error = nullptr) {
    auto keyFrameDataset = MakePipelineContractUnstructuredDataset();
    auto predictedDataset = keyFrameDataset;
    predictedDataset.name = "pipeline_contract_predicted";
    for (auto& field : predictedDataset.pointFields) {
        for (auto& value : field.values) {
            value = value * 1.01f + 0.001f;
        }
    }
    for (auto& field : predictedDataset.cellFields) {
        for (auto& value : field.values) {
            value += 0.01f;
        }
    }
    TestBlockTreeAdapter keyFrameAdapter(keyFrameDataset);
    TestBlockTreeAdapter predictedAdapter(predictedDataset);
    auto params = CodecControlParamsFactory::MakeDefault();
    params.SetSpatialBlockElementCounts(4u, 1u);
    params.attrReference.temporalField.selectionMode = ReferenceSelectionMode::Forced;
    params.geometryReference.temporalField.selectionMode = ReferenceSelectionMode::Forced;
    class TemporalSequenceSource final : public IFrameSequenceEncodeSource {
    public:
        TemporalSequenceSource(
            const TestDataset& keyFrame,
            const TestDataset& predictedFrame)
            : m_frames{&keyFrame, &predictedFrame} {}

        [[nodiscard]] std::size_t FrameCount() const noexcept override {
            return m_frames.size();
        }

        bool LoadFrame(
            const std::size_t frameOrdinal,
            FrameSequenceEncodeFrame& frame,
            std::string* error) override {
            if (frameOrdinal >= m_frames.size()) {
                return validation::AssignError(error, "temporal test frame ordinal is invalid");
            }
            const auto& dataset = *m_frames[frameOrdinal];
            frame = {};
            frame.blockTreeAdapter = std::make_unique<TestBlockTreeAdapter>(dataset);
            frame.rootName = dataset.name;
            frame.frameIndex = static_cast<std::uint32_t>(frameOrdinal);
            frame.timeValue = static_cast<float>(frameOrdinal);
            for (std::size_t attrIndex = 0u; attrIndex < 3u; ++attrIndex) {
                frame.attributeTargets.push_back(AttributeTarget{
                    .frameIndex = frame.frameIndex,
                    .blockPath = "leaf",
                    .attrIndex = attrIndex,
                });
            }
            return true;
        }

    private:
        std::array<const TestDataset*, 2u> m_frames;
    };

    class TemporalSequenceOutput final : public IFrameSequenceOutputSink {
    public:
        [[nodiscard]] std::unique_ptr<IByteRangeOutput> OpenFrame(
            std::size_t,
            std::uint32_t,
            std::string*) override {
            return std::make_unique<MemoryByteRangeOutput>();
        }

        bool CommitFrame(
            std::size_t,
            std::uint32_t,
            const std::uint64_t encodedByteCount,
            std::string* error) override {
            if (encodedByteCount == 0u) {
                return validation::AssignError(error, "temporal test frame output is empty");
            }
            m_encodedBytes += encodedByteCount;
            return true;
        }

        void AbortSequence() noexcept override {
            m_encodedBytes = 0u;
        }

        [[nodiscard]] std::uint64_t EncodedBytes() const noexcept {
            return m_encodedBytes;
        }

    private:
        std::uint64_t m_encodedBytes{0u};
    };

    TemporalSequenceSource source(keyFrameDataset, predictedDataset);
    TemporalSequenceOutput output;
    const auto encoded = FrameSequenceEncodeExecutor::Execute(FrameSequenceEncodeRequest{
        .source = &source,
        .outputSink = &output,
        .controlParams = &params,
        .enableParallelStages = false,
        .parallelTaskRunner = nullptr,
    });
    if (!encoded.success || encoded.encodedFrameCount != 2u || output.EncodedBytes() == 0u) {
        return validation::AssignError(
            error,
            "temporal pipeline did not produce encoded frame outputs");
    }
    const auto hasPredictorReference = std::any_of(
        encoded.messages.begin(),
        encoded.messages.end(),
        [](const TelemetryMessageRecord& message) {
            return message.origin == "ReferenceEncode.PredictorSpatialBlock" &&
                message.text == "stage result=Completed";
        });
    const auto hasPackageZstd = std::any_of(
        encoded.messages.begin(),
        encoded.messages.end(),
        [](const TelemetryMessageRecord& message) {
            return message.origin == "PackageFieldZstd.Streaming" &&
                message.text == "stage result=Completed";
        });
    const auto topologyStageCount = static_cast<std::size_t>(std::count_if(
        encoded.messages.begin(),
        encoded.messages.end(),
        [](const TelemetryMessageRecord& message) {
            return message.origin == "TopoStage" &&
                message.text == "stage result=Completed";
        }));

    TemporalBuilder::TemporalHistoryState temporalHistory;
    TemporalFrame keyFramePlan;
    TemporalFrame predictedPlan;
    std::string temporalError;
    if (!TemporalBuilder::BuildFrame(
            keyFrameAdapter,
            2u,
            0u,
            params.attrReference,
            params.geometryReference,
            params.topologyReference,
            temporalHistory,
            keyFramePlan,
            &temporalError) ||
        !TemporalBuilder::BuildFrame(
            predictedAdapter,
            2u,
            1u,
            params.attrReference,
            params.geometryReference,
            params.topologyReference,
            temporalHistory,
            predictedPlan,
            &temporalError)) {
        return validation::AssignError(error, temporalError);
    }
    const auto topologyReused =
        predictedPlan.topologyLeaves.size() == 1u &&
        predictedPlan.topologyLeaves[0].ownershipMode == TopologyOwnershipMode::Reused &&
        predictedPlan.topologyLeaves[0].ownerFrameIndex == 0u;
    return Require(
        result,
        hasPredictorReference && hasPackageZstd && topologyReused && topologyStageCount == 1u,
        "pipeline.temporalExecution",
        "temporal pipeline did not omit the reused topology stage or complete reference and package encoding");
}

inline bool CheckEncodeSessionContracts(
    TestResult& result,
    std::string* error = nullptr) {
    auto dataset = MakePipelineContractUnstructuredDataset();
    TestBlockTreeAdapter adapter(dataset);
    auto params = CodecControlParamsFactory::MakeDefault();
    params.attrReference.temporalField.forcePredFrames = true;
    params.attrReference.temporalField.keyFrameInterval = 0u;
    params.geometryReference.temporalField.forcePredFrames = true;
    params.geometryReference.temporalField.keyFrameInterval = 0u;

    EncodeSessionWorkspace workspace;
    FrameEncodeState firstPlan;
    if (!workspace.PrepareFrame(
            DataCodecEncodeFrameInput{
                .blockTreeAdapter = &adapter,
                .rootName = dataset.name,
                .frameIndex = 0u,
                .frameCount = 2u,
                .controlParams = &params,
            },
            firstPlan,
            error)) {
        return false;
    }
    const bool callerParamsPreserved =
        firstPlan.controlParams.attrReference.temporalField.forcePredFrames &&
        firstPlan.controlParams.attrReference.temporalField.keyFrameInterval == 0u &&
        firstPlan.controlParams.geometryReference.temporalField.forcePredFrames &&
        firstPlan.controlParams.geometryReference.temporalField.keyFrameInterval == 0u;

    FrameEncodeState repeatedPlan;
    std::string repeatedError;
    const bool rejectedRepeatedFrame = !workspace.PrepareFrame(
        DataCodecEncodeFrameInput{
            .blockTreeAdapter = &adapter,
            .rootName = dataset.name,
            .frameIndex = 0u,
            .frameCount = 2u,
            .controlParams = &params,
        },
        repeatedPlan,
        &repeatedError);

    workspace.ResetSession();
    FrameEncodeState resetPlan;
    std::string resetError;
    const bool resetAccepted = workspace.PrepareFrame(
        DataCodecEncodeFrameInput{
            .blockTreeAdapter = &adapter,
            .rootName = dataset.name,
            .frameIndex = 0u,
            .frameCount = 2u,
            .controlParams = &params,
        },
        resetPlan,
        &resetError);

    workspace.ResetSession();
    FrameEncodeState treeSignatureFirstPlan;
    std::string treeSignatureFirstError;
    const bool treeSignatureFirstAccepted = workspace.PrepareFrame(
        DataCodecEncodeFrameInput{
            .blockTreeAdapter = &adapter,
            .rootName = dataset.name,
            .frameIndex = 0u,
            .frameCount = 2u,
            .controlParams = &params,
        },
        treeSignatureFirstPlan,
        &treeSignatureFirstError);
    TestBlockTreeAdapter differentTreeAdapter(dataset, "different-leaf");
    FrameEncodeState differentTreePlan;
    std::string differentTreeError;
    const bool rejectedDifferentTree = !workspace.PrepareFrame(
        DataCodecEncodeFrameInput{
            .blockTreeAdapter = &differentTreeAdapter,
            .rootName = dataset.name,
            .frameIndex = 1u,
            .frameCount = 2u,
            .controlParams = &params,
        },
        differentTreePlan,
        &differentTreeError);

    return Require(
        result,
        callerParamsPreserved && rejectedRepeatedFrame && !repeatedError.empty() && resetAccepted &&
            treeSignatureFirstAccepted && treeSignatureFirstError.empty() &&
            rejectedDifferentTree && !differentTreeError.empty(),
        "pipeline.encodeSessionContracts",
        resetError.empty()
            ? "encode session accepted an inconsistent multi-frame block tree or a non-sequential frame"
            : resetError);
}

inline numericarray::NumericArraySource MakePipelineContractNumericSource(
    const std::vector<float>& values) {
    return numericarray::NumericArraySource{
        .values = NumericArrayView{
            .scalarType = ScalarType::Float32,
            .layout = ArrayLayout::CompactAOS,
            .origin = ViewBufferOrigin::Borrowed,
            .data = values.data(),
            .tupleCount = values.size(),
            .componentCount = 1,
            .tupleStrideBytes = sizeof(float),
        },
        .layout = numericarray::MakeNumericArrayLayout(
            DataType::Float32,
            sizeof(float),
            values.size(),
            1u),
    };
}

struct PipelineContractCountingFloatSource {
    const std::vector<float>* values{nullptr};
    mutable std::size_t tupleReadCount{0u};
};

inline bool ReadPipelineContractCountingFloatTuple(
    const void* userData,
    const std::size_t tupleIndex,
    void* output,
    std::string* error) {
    const auto* source = static_cast<const PipelineContractCountingFloatSource*>(userData);
    if (source == nullptr || source->values == nullptr || output == nullptr ||
        tupleIndex >= source->values->size()) {
        return validation::AssignError(error, "pipeline contract counting source read is invalid");
    }
    std::memcpy(output, source->values->data() + tupleIndex, sizeof(float));
    ++source->tupleReadCount;
    return true;
}

inline numericarray::NumericArraySource MakePipelineContractCountingNumericSource(
    const PipelineContractCountingFloatSource& source) {
    return numericarray::NumericArraySource{
        .values = NumericArrayView{
            .scalarType = ScalarType::Float32,
            .layout = ArrayLayout::GetterOnly,
            .origin = ViewBufferOrigin::Borrowed,
            .tupleCount = source.values != nullptr ? source.values->size() : 0u,
            .componentCount = 1,
            .userData = &source,
            .getTupleBytes = &ReadPipelineContractCountingFloatTuple,
        },
        .layout = numericarray::MakeNumericArrayLayout(
            DataType::Float32,
            sizeof(float),
            source.values != nullptr ? source.values->size() : 0u,
            1u),
    };
}

inline bool CheckSpatialReferenceBlockSelection(
    TestResult& result,
    std::string* error = nullptr) {
    constexpr std::size_t kBlockElementCount = 256u;
    constexpr std::size_t kBlockCount = 3u;
    std::vector<float> reference(kBlockElementCount * kBlockCount, 0.0f);
    std::vector<float> current(reference.size(), 0.0f);
    for (std::size_t index = 0u; index < reference.size(); ++index) {
        const auto position = static_cast<float>(index);
        reference[index] = std::sin(position * 0.031f) + 0.25f * std::cos(position * 0.007f);
        const auto blockIndex = index / kBlockElementCount;
        current[index] = blockIndex == 1u
            ? std::sin(position * 0.173f) + 0.6f * std::cos(position * 0.113f)
            : 1.75f * reference[index] + 0.125f;
    }

    NumericArrayStorageParams meta;
    meta.dataType = DataType::Float32;
    meta.valueSize = sizeof(float);
    meta.elementCount = current.size();
    meta.dimension = 1;

    CacheResources resources;
    resources.Configure(64u * 1024u, 8u * 1024u * 1024u);
    bytestore::ByteStoreSession byteStoreSession;
    std::shared_ptr<bytestore::IByteSource> transferCache;
    std::vector<NumericArrayBlockLayoutParams> layouts;
    const auto built = numericarrayreference::BuildNumericArrayReferenceTransferCache(
        meta,
        MakeDefaultAttributeValueCompressor(),
        MakePipelineContractNumericSource(current),
        numericarrayreference::NumericArrayReferenceSourceData{
            .candidate = NumericArrayReferenceCandidate{
                .scope = NumericArrayReferenceScope::IntraArray,
                .localParentFieldIndex = 0u,
            },
            .meta = meta,
            .source = MakePipelineContractNumericSource(reference),
        },
        NumericArrayReferenceCodecId::Affine,
        numericarrayreference::NumericArrayReferenceTransferControl{
            .affineBlockRSquared = 0.90,
            .selectionMode = ReferenceSelectionMode::Auto,
            .spatialBlockElementCount = static_cast<std::uint32_t>(kBlockElementCount),
            .useMemoryStaging = true,
            .useMemoryTransferCache = true,
        },
        resources.scratchBytePool,
        resources.windowBudget,
        resources.accessWindowBytes,
        transferCache,
        byteStoreSession,
        &layouts,
        error,
        "pipeline_contract_reference");
    if (!built) {
        return false;
    }
    const auto referenceBlockCount = static_cast<std::size_t>(std::count_if(
        layouts.begin(),
        layouts.end(),
        [](const NumericArrayBlockLayoutParams& layout) {
            return layout.codecId == NumericArrayReferenceCodecId::Affine;
        }));
    const auto ordinaryBlockCount = static_cast<std::size_t>(std::count_if(
        layouts.begin(),
        layouts.end(),
        [](const NumericArrayBlockLayoutParams& layout) {
            return layout.codecId == NumericArrayReferenceCodecId::NonReference;
        }));
    const auto selected = Require(
        result,
        layouts.size() == kBlockCount &&
            referenceBlockCount > 0u &&
            ordinaryBlockCount > 0u,
        "pipeline.spatialReferenceSelection",
        "spatial block auto mode did not preserve mixed ordinary and reference blocks");
    if (transferCache != nullptr) {
        transferCache->Release();
    }
    return selected;
}

inline bool CheckSampledIntraParentSelection(
    TestResult& result,
    std::string* error = nullptr) {
    constexpr std::size_t kElementCount = 1024u;
    constexpr std::size_t kSampleCount = 32u;
    std::vector<float> current(kElementCount, 0.0f);
    std::vector<float> parent(kElementCount, 0.0f);
    std::vector<float> unrelated(kElementCount, 0.0f);
    for (std::size_t index = 0u; index < kElementCount; ++index) {
        const auto position = static_cast<float>(index);
        parent[index] = std::sin(position * 0.019f) + 0.1f * std::cos(position * 0.071f);
        current[index] = parent[index] * 1.75f + 0.125f;
        unrelated[index] = std::cos(position * 0.173f) + 0.6f * std::sin(position * 0.113f);
    }
    std::vector<AttrStorageParams> metas(3u);
    for (std::size_t index = 0u; index < metas.size(); ++index) {
        metas[index].name = index == 0u
            ? "current"
            : (index == 1u ? "parent" : "unrelated");
        metas[index].dataType = DataType::Float32;
        metas[index].valueSize = sizeof(float);
        metas[index].elementCount = kElementCount;
        metas[index].dimension = 1;
        metas[index].attachmentType = AttrAttachment::Point;
    }
    PipelineContractCountingFloatSource currentSource{.values = &current};
    PipelineContractCountingFloatSource parentSource{.values = &parent};
    PipelineContractCountingFloatSource unrelatedSource{.values = &unrelated};
    const std::vector<numericarray::NumericArraySource> sources{
        MakePipelineContractCountingNumericSource(currentSource),
        MakePipelineContractCountingNumericSource(parentSource),
        MakePipelineContractCountingNumericSource(unrelatedSource),
    };
    const std::vector<std::size_t> metaIndices{0u, 1u, 2u};
    const std::vector<std::uint8_t> referenceAllowed{1u, 0u, 0u};
    ScratchByteBufferPool scratchBytePool;
    for (const auto codec : {
             IntraFieldReferenceCodec::Affine,
             IntraFieldReferenceCodec::Predictor,
             IntraFieldReferenceCodec::Wavelet}) {
        AttrReferenceControlParams dependency;
        dependency.enabled = true;
        dependency.intraField.codec = codec;
        dependency.intraField.selectionMode = ReferenceSelectionMode::Auto;
        dependency.intraField.sampleCount = kSampleCount;
        dependency.intraField.minimumSampleScore = 0.5;
        dependency.intraField.affine.precheckRSquared = 0.90;
        EncodeAttributeReferenceSchedule schedule;
        if (!BuildAttributeIntraFieldReferenceSchedule(
                metas,
                sources,
                metaIndices,
                referenceAllowed,
                dependency,
                scratchBytePool,
                schedule,
                error)) {
            return false;
        }
        if (!Require(
                result,
                schedule.initialized &&
                    schedule.entries.size() == 3u &&
                    schedule.entries[0].hasIntraParent &&
                    schedule.entries[0].parentMetaIndex == 1u,
                "pipeline.sampledParentSelection",
                "intra-field parent selection did not use the codec sample score")) {
            return false;
        }
    }
    return Require(
        result,
        currentSource.tupleReadCount == kSampleCount * 3u &&
            parentSource.tupleReadCount == kSampleCount * 3u &&
            unrelatedSource.tupleReadCount == kSampleCount * 3u,
        "pipeline.reusedReferenceSamples",
        "intra-field parent selection did not reuse one sample read per field");
}

inline bool CheckForcedReferenceFailure(
    TestResult& result,
    std::string* error = nullptr) {
    constexpr std::size_t kElementCount = 512u;
    std::vector<float> reference(kElementCount, 0.0f);
    std::vector<float> current(kElementCount, 0.0f);
    for (std::size_t index = 0u; index < kElementCount; ++index) {
        const auto position = static_cast<float>(index);
        reference[index] = std::sin(position * 0.013f);
        current[index] = std::cos(position * 0.173f) + 0.3f * std::sin(position * 0.097f);
    }
    NumericArrayStorageParams meta;
    meta.dataType = DataType::Float32;
    meta.valueSize = sizeof(float);
    meta.elementCount = kElementCount;
    meta.dimension = 1;
    CacheResources resources;
    resources.Configure(64u * 1024u, 8u * 1024u * 1024u);
    bytestore::ByteStoreSession byteStoreSession;
    std::shared_ptr<bytestore::IByteSource> transferCache;
    std::string forcedError;
    const auto built = numericarrayreference::BuildNumericArrayReferenceTransferCache(
        meta,
        MakeDefaultAttributeValueCompressor(),
        MakePipelineContractNumericSource(current),
        numericarrayreference::NumericArrayReferenceSourceData{
            .candidate = NumericArrayReferenceCandidate{
                .scope = NumericArrayReferenceScope::IntraArray,
                .localParentFieldIndex = 0u,
            },
            .meta = meta,
            .source = MakePipelineContractNumericSource(reference),
        },
        NumericArrayReferenceCodecId::Affine,
        numericarrayreference::NumericArrayReferenceTransferControl{
            .affineBlockRSquared = 0.999999,
            .selectionMode = ReferenceSelectionMode::Forced,
            .spatialBlockElementCount = 256u,
            .useMemoryStaging = true,
            .useMemoryTransferCache = true,
        },
        resources.scratchBytePool,
        resources.windowBudget,
        resources.accessWindowBytes,
        transferCache,
        byteStoreSession,
        nullptr,
        &forcedError,
        "pipeline_contract_forced_reference");
    if (transferCache != nullptr) {
        transferCache->Release();
    }
    return Require(
        result,
        !built && !forcedError.empty(),
        "pipeline.forcedReferenceFailure",
        "forced reference failure was converted into an ordinary block");
}

inline TestResult RunDataCodecFeaturePipelineContracts() noexcept {
    TestResult result;

    std::string error;
    const auto originalOrder = RemapOrderSource::Original();
    const auto computedIdentityOrder = RemapOrderSource::TryComputed(
        MakeIdentityRemapProvider(4u));
    Require(
        result,
        originalOrder.IsOriginal() &&
            originalOrder.Provider() == nullptr &&
            computedIdentityOrder.has_value() &&
            computedIdentityOrder->IsComputed() &&
            computedIdentityOrder->IsComputedIdentity() &&
            computedIdentityOrder->Provider() != nullptr,
        "pipeline.explicitOrderSources",
        "original and computed identity order sources are not distinguishable");

    const EncodePipelineDescriptor rawPipeline{
        .id = EncodePipelineBindingId::UnstructuredOrdinary,
        .pointOrder = EncodePointOrderMode::Original,
        .cellOrder = EncodeCellOrderMode::Original,
        .referenceEncode = false,
        .packageFields = PackageFieldEncodingParams{
            .mode = PackageFieldEncodingMode::Raw,
            .workerCount = 1u,
        },
    };
    Require(
        result,
        ValidateEncodePipelineDescriptor(rawPipeline, &error),
        "pipeline.rawPackageVariant",
        error.empty() ? "raw package pipeline was rejected" : error);

    std::vector<numericarray::SpatialBlockRange> layout;
    error.clear();
    const auto layoutOk = numericarray::BuildSpatialBlockLayout(10u, 4u, layout, &error);
    Require(
        result,
        layoutOk && layout.size() == 3u,
        "pipeline.spatialBlockCount",
        error.empty() ? "spatial block layout count is invalid" : error);
    if (layout.size() == 3u) {
        Require(
            result,
            layout[0].elementOffset == 0u && layout[0].elementCount == 4u &&
                layout[1].elementOffset == 4u && layout[1].elementCount == 4u &&
                layout[2].elementOffset == 8u && layout[2].elementCount == 2u,
            "pipeline.spatialBlockRanges",
            "spatial block layout is not stable and contiguous");
    }

    CodecStorageParams spatialParams;
    spatialParams.meshType = MeshType::PointSet;
    spatialParams.spatialBlockParams.pointElementCount = 4u;
    spatialParams.spatialBlockParams.cellElementCount = 4u;
    spatialParams.spatialBlockParams.pointElementTotal = 10u;
    spatialParams.spatialBlockParams.cellElementTotal = 0u;
    spatialParams.spatialBlockParams.pointBlockCount = 3u;
    spatialParams.spatialBlockParams.cellBlockCount = 0u;
    spatialParams.geomParams.codecType = EncodedFieldCodecType::NumericArrayBlocks;
    spatialParams.geomParams.dataType = DataType::Float32;
    spatialParams.geomParams.valueSize = sizeof(float);
    spatialParams.geomParams.elementCount = 10u;
    spatialParams.geomParams.dimension = 3;
    for (const auto& range : layout) {
        NumericArrayBlockLayoutParams block;
        block.mode = NumericArrayBlockMode::NonReference;
        block.codecId = NumericArrayReferenceCodecId::NonReference;
        block.elementOffset = range.elementOffset;
        block.elementCount = range.elementCount;
        block.encodedByteLength = 3u;
        block.bytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
        for (std::uint32_t component = 0u; component < 3u; ++component) {
            block.componentLayouts.push_back(NumericArrayComponentLayoutParams{
                .componentIndex = component,
                .bytesCodec = NumericArrayBytesCodec::NumericArrayCodec,
                .encodedByteLength = 1u,
            });
        }
        spatialParams.geomParams.blockLayouts.push_back(std::move(block));
    }
    bool requires64Bit = false;
    error.clear();
    Require(
        result,
        ValidateCodecStorageParamsContent(spatialParams, requires64Bit, &error),
        "pipeline.sharedSpatialLayoutParams",
        error.empty() ? "shared spatial block params were rejected" : error);
    auto mismatchedSpatialParams = spatialParams;
    mismatchedSpatialParams.geomParams.blockLayouts[1].elementOffset = 5u;
    requires64Bit = false;
    error.clear();
    Require(
        result,
        !ValidateCodecStorageParamsContent(mismatchedSpatialParams, requires64Bit, &error),
        "pipeline.rejectSpatialLayoutMismatch",
        "numeric array block mismatch was accepted outside the shared spatial layout");

    const auto timePriority = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{.tier = DataCodecEncodeTier::TimePriority});
    const auto balanced = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{.tier = DataCodecEncodeTier::Balanced});
    const auto memoryPriority = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{.tier = DataCodecEncodeTier::MemoryPriority});
    const auto timeEnhanced = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::TimePriority,
            .enableCompressionEnhancement = true,
        });
    const auto balancedEnhanced = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::Balanced,
            .enableCompressionEnhancement = true,
        });
    const auto memoryEnhanced = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::MemoryPriority,
            .enableCompressionEnhancement = true,
        });
    const auto wasm4Enhanced = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::TimePriority,
            .enableCompressionEnhancement = true,
        },
        DataCodecRuntimeProfile::Wasm4GiB);
    const auto explicitZstdEnhanced = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::MemoryPriority,
            .enableCompressionEnhancement = true,
            .packageZstdLevel = 17,
        });
    const auto sharedBlockAndReferenceParams =
        timePriority.controlParams.spatialBlockPolicy.pointElementCount ==
            balanced.controlParams.spatialBlockPolicy.pointElementCount &&
        balanced.controlParams.spatialBlockPolicy.pointElementCount ==
            memoryPriority.controlParams.spatialBlockPolicy.pointElementCount &&
        timePriority.controlParams.spatialBlockPolicy.cellElementCount ==
            balanced.controlParams.spatialBlockPolicy.cellElementCount &&
        balanced.controlParams.spatialBlockPolicy.cellElementCount ==
            memoryPriority.controlParams.spatialBlockPolicy.cellElementCount &&
        timePriority.controlParams.attrReference.intraField.codec ==
            balanced.controlParams.attrReference.intraField.codec &&
        balanced.controlParams.attrReference.intraField.codec ==
            memoryPriority.controlParams.attrReference.intraField.codec &&
        timePriority.controlParams.attrReference.temporalField.codec ==
            balanced.controlParams.attrReference.temporalField.codec &&
        balanced.controlParams.attrReference.temporalField.codec ==
            memoryPriority.controlParams.attrReference.temporalField.codec;
    Require(
        result,
        sharedBlockAndReferenceParams,
        "pipeline.performanceTierSharedSemantics",
        "encode performance tiers changed shared block or reference semantics");

    Require(
        result,
            timePriority.pipelineControl.pointOrder == EncodePointOrderMode::Morton &&
            timePriority.pipelineControl.cellOrder == EncodeCellOrderMode::Original &&
            timePriority.pipelineControl.packageFields.zstdLevel == 1 &&
            timePriority.pipelineControl.packageFields.workerCount == 8u &&
            balanced.pipelineControl.pointOrder == EncodePointOrderMode::Morton &&
            balanced.pipelineControl.cellOrder == EncodeCellOrderMode::Original &&
            balanced.pipelineControl.packageFields.zstdLevel == 3 &&
            balanced.pipelineControl.packageFields.workerCount == 4u &&
            memoryPriority.pipelineControl.pointOrder == EncodePointOrderMode::Morton &&
            memoryPriority.pipelineControl.cellOrder == EncodeCellOrderMode::Original &&
            memoryPriority.pipelineControl.packageFields.zstdLevel == 1 &&
            memoryPriority.pipelineControl.packageFields.workerCount == 1u &&
            memoryPriority.execution.enableParallelStages,
        "pipeline.performanceTierVariants",
        "encode performance tiers did not resolve to the documented pipeline variants");

    Require(
        result,
        timeEnhanced.pipelineControl.cellOrder == EncodeCellOrderMode::Morton &&
            balancedEnhanced.pipelineControl.cellOrder == EncodeCellOrderMode::Morton &&
            memoryEnhanced.pipelineControl.cellOrder == EncodeCellOrderMode::Morton &&
            balancedEnhanced.controlParams.attrReference.temporalField.predictor.enableLocalWindowSearch &&
            balancedEnhanced.controlParams.attrReference.temporalField.predictor.searchStrategy ==
                TemporalPredictorSearchStrategy::ExhaustiveEstimatedBytes &&
            balancedEnhanced.controlParams.geometryReference.temporalField.predictor.enableLocalWindowSearch &&
            balancedEnhanced.controlParams.geometryReference.temporalField.predictor.searchStrategy ==
                TemporalPredictorSearchStrategy::ExhaustiveEstimatedBytes &&
            timeEnhanced.source.compressionEnhancementEnabled &&
            balancedEnhanced.source.compressionEnhancementEnabled &&
            memoryEnhanced.source.compressionEnhancementEnabled,
        "pipeline.compressionEnhancementSemantics",
        "compression enhancement did not enable remap and exhaustive predictor search");

    Require(
        result,
        timeEnhanced.pipelineControl.packageFields.zstdLevel ==
                timePriority.pipelineControl.packageFields.zstdLevel &&
            timeEnhanced.pipelineControl.packageFields.workerCount ==
                timePriority.pipelineControl.packageFields.workerCount &&
            timeEnhanced.controlParams.resourceBudget.ResidentLimitBytes() ==
                timePriority.controlParams.resourceBudget.ResidentLimitBytes() &&
            timeEnhanced.controlParams.resourceBudget.AttributePressioLaneCount() ==
                timePriority.controlParams.resourceBudget.AttributePressioLaneCount() &&
            balancedEnhanced.pipelineControl.packageFields.zstdLevel ==
                balanced.pipelineControl.packageFields.zstdLevel &&
            balancedEnhanced.pipelineControl.packageFields.workerCount ==
                balanced.pipelineControl.packageFields.workerCount &&
            balancedEnhanced.controlParams.resourceBudget.ResidentLimitBytes() ==
                balanced.controlParams.resourceBudget.ResidentLimitBytes() &&
            balancedEnhanced.controlParams.resourceBudget.AttributePressioLaneCount() ==
                balanced.controlParams.resourceBudget.AttributePressioLaneCount() &&
            memoryEnhanced.pipelineControl.packageFields.zstdLevel ==
                memoryPriority.pipelineControl.packageFields.zstdLevel &&
            memoryEnhanced.pipelineControl.packageFields.workerCount ==
                memoryPriority.pipelineControl.packageFields.workerCount &&
            memoryEnhanced.controlParams.resourceBudget.ResidentLimitBytes() ==
                memoryPriority.controlParams.resourceBudget.ResidentLimitBytes() &&
            memoryEnhanced.controlParams.resourceBudget.AttributePressioLaneCount() ==
                memoryPriority.controlParams.resourceBudget.AttributePressioLaneCount() &&
            explicitZstdEnhanced.pipelineControl.packageFields.zstdLevel == 17 &&
            !balancedEnhanced.source.customControlParams &&
            explicitZstdEnhanced.source.customControlParams,
        "pipeline.compressionEnhancementComposition",
        "compression enhancement changed resource-tier or explicit ZSTD configuration");

    const auto remapWorkspaceFits = [](const DataCodecEncodeConfigurationParams& configuration) {
        constexpr std::size_t kLargeRemapElementCount = 100000000u;
        const auto& budget = configuration.controlParams.resourceBudget;
        const auto leafElementCount = mortonremap::ResolveLeafBudgetElements(
            budget.RemapMortonLeafBytes());
        const auto maximumInMemoryWorkspaceBytes =
            static_cast<std::uint64_t>(leafElementCount) *
            (sizeof(mortonremap::MortonKeyedIndex) + sizeof(IndexType) * 2u);
        const auto externalWorkspaceBytes = mortonremap::EstimateMortonWorkspaceBytes(
            kLargeRemapElementCount,
            budget.RemapMortonLeafBytes(),
            budget.RemapMortonRunBufferBytes());
        return maximumInMemoryWorkspaceBytes <= budget.RemapScratchQuotaBytes() &&
            externalWorkspaceBytes <= budget.RemapScratchQuotaBytes();
    };
    Require(
        result,
        remapWorkspaceFits(timeEnhanced) &&
            remapWorkspaceFits(balancedEnhanced) &&
            remapWorkspaceFits(memoryEnhanced) &&
            remapWorkspaceFits(wasm4Enhanced),
        "pipeline.compressionEnhancementRemapBudget",
        "compression enhancement exceeds the selected resource profile remap budget");

    Require(
        result,
        timePriority.controlParams.resourceBudget.AttributePressioLaneCount() ==
            balanced.controlParams.resourceBudget.AttributePressioLaneCount() &&
        balanced.controlParams.resourceBudget.AttributePressioLaneCount() >
            memoryPriority.controlParams.resourceBudget.AttributePressioLaneCount() &&
        timePriority.controlParams.resourceBudget.ResidentLimitBytes() >
            balanced.controlParams.resourceBudget.ResidentLimitBytes() &&
        balanced.controlParams.resourceBudget.ResidentLimitBytes() >
            memoryPriority.controlParams.resourceBudget.ResidentLimitBytes(),
        "pipeline.performanceTierResources",
        "encode performance tiers do not follow the time-to-memory resource axis");

    error.clear();
    if (!CheckSpatialReferenceBlockSelection(result, &error) && !error.empty()) {
        result.AddFailure("pipeline.spatialReferenceEncode", error);
    }
    error.clear();
    if (!CheckSampledIntraParentSelection(result, &error) && !error.empty()) {
        result.AddFailure("pipeline.codecAwareParentSelection", error);
    }
    error.clear();
    if (!CheckForcedReferenceFailure(result, &error) && !error.empty()) {
        result.AddFailure("pipeline.forcedReferenceFailure", error);
    }
    error.clear();
    if (!CheckResolvedFormalPipeline(result, &error) && !error.empty()) {
        result.AddFailure("pipeline.resolvedFormalStages", error);
    }
    CheckTopologyReuseSpatialDependency(result);
    error.clear();
    if (!CheckFormalPipelineExecution(result, &error) && !error.empty()) {
        result.AddFailure("pipeline.formalExecution", error);
    }
    error.clear();
    if (!CheckTemporalPipelineExecution(result, &error) && !error.empty()) {
        result.AddFailure("pipeline.temporalExecution", error);
    }
    error.clear();
    if (!CheckEncodeSessionContracts(result, &error) && !error.empty()) {
        result.AddFailure("pipeline.encodeSessionContracts", error);
    }

    return result;
}

} // namespace datacodec::test

#endif
