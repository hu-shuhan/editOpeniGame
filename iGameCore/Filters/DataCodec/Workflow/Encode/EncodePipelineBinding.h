#ifndef DATACODEC_WORKFLOW_ENCODE_ENCODEPIPELINEBINDING_H
#define DATACODEC_WORKFLOW_ENCODE_ENCODEPIPELINEBINDING_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/EncodePipelineParams.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstdint>
#include <string>
namespace datacodec {

enum class EncodePipelineBindingId : std::uint8_t {
    PointSetOrdinary = 0,
    PointSetReference = 1,
    StructuredOrdinary = 2,
    StructuredReference = 3,
    UnstructuredOrdinary = 4,
    UnstructuredReference = 5,
};

enum class EncodePipelineOutputKind : std::uint8_t {
    LeafPackage = 0,
    EncodedLeafFieldBundle = 1,
};

[[nodiscard]] inline const char* EncodePipelineBindingName(
    const EncodePipelineBindingId id) noexcept {
    switch (id) {
        case EncodePipelineBindingId::PointSetOrdinary:
            return "PointSetOrdinary";
        case EncodePipelineBindingId::PointSetReference:
            return "PointSetReference";
        case EncodePipelineBindingId::StructuredOrdinary:
            return "StructuredOrdinary";
        case EncodePipelineBindingId::StructuredReference:
            return "StructuredReference";
        case EncodePipelineBindingId::UnstructuredOrdinary:
            return "UnstructuredOrdinary";
        case EncodePipelineBindingId::UnstructuredReference:
            return "UnstructuredReference";
    }
    return "UnstructuredReference";
}

[[nodiscard]] inline const char* EncodePipelineOutputKindName(
    const EncodePipelineOutputKind kind) noexcept {
    return kind == EncodePipelineOutputKind::EncodedLeafFieldBundle
        ? "EncodedLeafFieldBundle"
        : "LeafPackage";
}

struct EncodePipelineDescriptor {
    EncodePipelineBindingId id{EncodePipelineBindingId::UnstructuredReference};
    EncodePipelineOutputKind outputKind{EncodePipelineOutputKind::LeafPackage};
    EncodePointOrderMode pointOrder{EncodePointOrderMode::Morton};
    EncodeCellOrderMode cellOrder{EncodeCellOrderMode::Morton};
    bool referenceEncode{false};
    bool includeTopology{true};
    PackageFieldEncodingParams packageFields;
};

struct EncodePipelineExecutionProfile {
    EncodeResourceBudgetControlParams resourceBudget;
    bool enableParallelStages{true};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
};

struct EncodePipelineBinding {
    EncodePipelineDescriptor descriptor;
    const CodecControlParams* algorithmParams{nullptr};
    EncodePipelineExecutionProfile executionProfile;
};

[[nodiscard]] inline bool UsesPointSpatialPartition(
    const EncodePipelineDescriptor& descriptor) noexcept {
    return descriptor.pointOrder == EncodePointOrderMode::Morton;
}

[[nodiscard]] inline bool UsesCellSpatialPartition(
    const EncodePipelineDescriptor& descriptor) noexcept {
    return descriptor.cellOrder == EncodeCellOrderMode::Morton;
}

[[nodiscard]] inline bool HasEnabledReferenceSemantics(
    const CodecControlParams& params) noexcept {
    return params.attrReference.enabled ||
        params.geometryReference.enabled;
}

[[nodiscard]] inline EncodePipelineDescriptor ResolveEncodePipelineDescriptor(
    const IEncodeAdapter& adapter,
    const CodecControlParams& params,
    const EncodePipelineControlParams& pipelineControl,
    const EncodePipelineOutputKind outputKind = EncodePipelineOutputKind::LeafPackage,
    const bool includeTopology = true) noexcept {
    const auto hasReference = HasEnabledReferenceSemantics(params);
    if (adapter.IsStructuredMesh()) {
        return EncodePipelineDescriptor{
            .id = hasReference
                ? EncodePipelineBindingId::StructuredReference
                : EncodePipelineBindingId::StructuredOrdinary,
            .outputKind = outputKind,
            .pointOrder = EncodePointOrderMode::Original,
            .cellOrder = EncodeCellOrderMode::Original,
            .referenceEncode = hasReference,
            .includeTopology = includeTopology,
            .packageFields = pipelineControl.packageFields,
        };
    }

    if (adapter.GetNumberOfCells() == 0u) {
        return EncodePipelineDescriptor{
            .id = hasReference
                ? EncodePipelineBindingId::PointSetReference
                : EncodePipelineBindingId::PointSetOrdinary,
            .outputKind = outputKind,
            .pointOrder = adapter.GetNumberOfPoints() > 1u
                    ? pipelineControl.pointOrder
                    : EncodePointOrderMode::Original,
            .cellOrder = EncodeCellOrderMode::Original,
            .referenceEncode = hasReference,
            .includeTopology = false,
            .packageFields = pipelineControl.packageFields,
        };
    }

    return EncodePipelineDescriptor{
        .id = hasReference
            ? EncodePipelineBindingId::UnstructuredReference
            : EncodePipelineBindingId::UnstructuredOrdinary,
        .outputKind = outputKind,
        .pointOrder = adapter.GetNumberOfPoints() > 1u
                ? pipelineControl.pointOrder
                : EncodePointOrderMode::Original,
        .cellOrder = adapter.GetNumberOfCells() > 1u
                ? pipelineControl.cellOrder
                : EncodeCellOrderMode::Original,
        .referenceEncode = hasReference,
        .includeTopology = includeTopology,
        .packageFields = pipelineControl.packageFields,
    };
}

inline bool ValidateEncodePipelineDescriptor(
    const EncodePipelineDescriptor& descriptor,
    std::string* error = nullptr) {
    if (descriptor.packageFields.workerCount == 0u) {
        return validation::AssignError(error, "package field worker count must be positive");
    }
    return true;
}

inline bool ValidateEncodeAlgorithmParams(
    const CodecControlParams& params,
    std::string* error = nullptr) {
    if (!ValidateEncodeResourceBudgetControlParams(params.resourceBudget, error)) {
        return false;
    }
    if (params.spatialBlockPolicy.pointElementCount == 0u ||
        params.spatialBlockPolicy.cellElementCount == 0u) {
        return validation::AssignError(
            error,
            "spatial block element counts must be positive");
    }
    if (params.attrReference.intraField.selectionMode == ReferenceSelectionMode::Forced &&
        params.attrReference.intraField.codec == IntraFieldReferenceCodec::Disabled) {
        return validation::AssignError(
            error,
            "forced intra-field reference requires a codec");
    }
    if (params.attrReference.temporalField.selectionMode == ReferenceSelectionMode::Forced &&
        params.attrReference.temporalField.codec == TemporalFieldReferenceCodec::Disabled) {
        return validation::AssignError(
            error,
            "forced temporal attribute reference requires a codec");
    }
    if (params.geometryReference.temporalField.selectionMode == ReferenceSelectionMode::Forced &&
        params.geometryReference.temporalField.codec == TemporalFieldReferenceCodec::Disabled) {
        return validation::AssignError(
            error,
            "forced temporal geometry reference requires a codec");
    }
    return true;
}

inline bool ResolveEncodePipelineBinding(
    const IEncodeAdapter& adapter,
    const CodecControlParams& params,
    const EncodePipelineControlParams& pipelineControl,
    const EncodePipelineExecutionProfile& executionProfile,
    EncodePipelineBinding& binding,
    std::string* error = nullptr,
    const EncodePipelineOutputKind outputKind = EncodePipelineOutputKind::LeafPackage,
    const bool includeTopology = true) {
    const auto descriptor = ResolveEncodePipelineDescriptor(
        adapter,
        params,
        pipelineControl,
        outputKind,
        includeTopology);
    if (!ValidateEncodePipelineDescriptor(descriptor, error) ||
        !ValidateEncodeAlgorithmParams(params, error)) {
        binding = {};
        return false;
    }
    if (executionProfile.enableParallelStages && executionProfile.parallelTaskRunner == nullptr) {
        binding = {};
        return validation::AssignError(error, "parallel DataCodec encode requires a task runner");
    }

    // Binding 只在执行前解析一次，Pipeline 不再重新推导并比较内部生成的配置
    binding = EncodePipelineBinding{
        .descriptor = descriptor,
        .algorithmParams = &params,
        .executionProfile = executionProfile,
    };
    return true;
}

} // namespace datacodec

#endif
