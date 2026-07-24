#ifndef DATACODEC_VALIDATION_WORKFLOW_DECODEOUTPUTVALIDATOR_H
#define DATACODEC_VALIDATION_WORKFLOW_DECODEOUTPUTVALIDATOR_H

#include "DataCodec/Runtime/Workspace/DecodeLeafWorkspace.h"
#include "DataCodec/Validation/Attribute/AttributeValidator.h"
#include "DataCodec/Validation/Geometry/GeometryValidator.h"
#include "DataCodec/Validation/Topology/TopologyValidator.h"

#include <cstddef>
#include <span>

namespace datacodec::validation {

class DecodeOutputValidator final {
public:
    [[nodiscard]] static ValidationResult ValidateDecodedCacheShapes(
        const DecodeLeafWorkspace& workspace,
        const std::span<const std::size_t> attrIndices) {
        if (!workspace.ValidationPolicy().StrictDecodeEnabled()) {
            return ValidationResult::Success(
                ValidationDomain::Workflow,
                "decode.output.required");
        }
        const auto geometry = GeometryValidator::ValidateDecodedCacheShape(
            workspace.StorageParams(),
            workspace.geometry);
        if (!geometry) {
            return geometry;
        }
        const auto topology = TopologyValidator::ValidateDecodedCacheShape(
            workspace.StorageParams(),
            workspace.topology);
        if (!topology) {
            return topology;
        }
        const auto attributes = AttributeValidator::ValidateDecodedCacheShape(
            workspace.StorageParams(),
            workspace.attributes,
            attrIndices);
        if (!attributes) { return attributes; }
        if (workspace.ValidationPolicy().validateTopologyReferences) {
            const auto reference = TopologyValidator::ValidateReferenceState(
                workspace.topologyBorrowed,
                workspace.topology);
            if (!reference) { return reference; }
        }
        if (workspace.ValidationPolicy().validateFloatingPointValues) {
            const auto geometryValues = GeometryValidator::ValidateFiniteValues(
                workspace.StorageParams(),
                workspace.geometry);
            if (!geometryValues) { return geometryValues; }
            return AttributeValidator::ValidateFiniteValues(
                workspace.StorageParams(),
                workspace.attributes,
                attrIndices);
        }
        return ValidationResult::Success(
            ValidationDomain::Workflow,
            "decode.output.strict");
    }

    [[nodiscard]] static ValidationResult ValidateDecodedAttributes(
        const DecodeLeafWorkspace& workspace,
        const std::span<const std::size_t> attrIndices) {
        if (!workspace.ValidationPolicy().StrictDecodeEnabled()) {
            return ValidationResult::Success(
                ValidationDomain::Workflow,
                "decode.attribute.required");
        }
        const auto attributes = AttributeValidator::ValidateDecodedCacheShape(
            workspace.StorageParams(),
            workspace.attributes,
            attrIndices);
        if (!attributes || !workspace.ValidationPolicy().validateFloatingPointValues) {
            return attributes;
        }
        return AttributeValidator::ValidateFiniteValues(
            workspace.StorageParams(),
            workspace.attributes,
            attrIndices);
    }
};

} // namespace datacodec::validation

#endif
