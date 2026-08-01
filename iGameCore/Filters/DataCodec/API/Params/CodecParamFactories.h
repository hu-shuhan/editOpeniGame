#ifndef DATACODEC_API_PARAMS_CODECPARAMFACTORIES_H
#define DATACODEC_API_PARAMS_CODECPARAMFACTORIES_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Codec/NumericArray/NumericArrayCodec.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

class AttrStorageParamsFactory {
public:
    [[nodiscard]] static bool TryFromEncodeAttributeView(
        const EncodeAttributeView& attr,
        AttrStorageParams& params,
        std::string* error = nullptr) {
        if (!attr.values.IsValid()) {
            params = {};
            return validation::AssignError(error, "attribute view is invalid");
        }
        params = {};
        params.name = attr.name;
        params.dataType = ToDataType(attr.values.scalarType);
        params.type = attr.role;
        params.attachmentType = attr.attachment;
        params.dimension = attr.values.componentCount;
        params.elementCount = attr.values.tupleCount;
        params.codecType = EncodedFieldCodecType::Unknown;
        return true;
    }
};

class GeometryStorageParamsFactory {
public:
    [[nodiscard]] static GeometryStorageParams MakeDefault(const std::size_t pointCount = 0) {
        GeometryStorageParams params;
        ApplyGeometryShape(params, pointCount);
        return params;
    }

private:
    static void ApplyGeometryShape(GeometryStorageParams& params, const std::size_t pointCount) {
        params.codecType = EncodedFieldCodecType::Unknown;
        params.dataType = DataType::Float32;
        params.elementCount = pointCount;
        params.dimension = 3;
    }
};

class CodecStorageParamsFactory {
public:
    [[nodiscard]] static bool TryFromEncodeAdapter(
        const IEncodeAdapter& adapter,
        const CodecControlParams* controlParams,
        const std::span<const std::size_t> selectedAttrIndices,
        CodecStorageParams& output,
        std::string* error = nullptr,
        std::vector<std::string>* warnings = nullptr) {
        CodecStorageParams params;
        params.meshType = adapter.GetMeshType();
        params.structuredMeshParams = {};
        params.topoParams = {};
        // 拓扑规模属于输入固有元数据，复用拓扑的流水线也必须保留
        params.topoParams.cellCount = static_cast<ParamSize>(adapter.GetNumberOfCells());
        params.geomParams = GeometryStorageParamsFactory::MakeDefault(adapter.GetNumberOfPoints());
        const auto spatialPolicy = controlParams != nullptr
            ? controlParams->spatialBlockPolicy
            : SpatialBlockPolicyParams{};
        params.spatialBlockParams.pointElementCount = spatialPolicy.pointElementCount;
        params.spatialBlockParams.cellElementCount = spatialPolicy.cellElementCount;

        int axisSize[3]{0, 0, 0};
        if (adapter.GetStructuredAxisSize(axisSize)) {
            params.structuredMeshParams.axisSize = {axisSize[0], axisSize[1], axisSize[2]};
            params.topoParams.isStructured = true;
        }

        params.attrParams.clear();
        params.attrParams.reserve(selectedAttrIndices.size());
        params.attrSourceIndices.clear();
        params.attrSourceIndices.reserve(selectedAttrIndices.size());
        if (!AppendSelectedAttrs(
                params,
                adapter,
                selectedAttrIndices,
                warnings,
                error)) {
            output = {};
            return false;
        }
        params.attrPayloadOrder = MakeDefaultAttributePayloadOrder(params.attrParams.size());
        output = std::move(params);
        return true;
    }

private:
    [[nodiscard]] static std::string AttributeSourceLabel(
        const char* attachment,
        const std::size_t sourceIndex,
        const std::string& name) {
        std::string label = std::string(attachment) + " attribute at source index " + std::to_string(sourceIndex);
        if (!name.empty()) {
            label += " ('" + name + "')";
        }
        return label;
    }

    [[nodiscard]] static bool ValidateEncodableAttributeParams(
        const AttrStorageParams& params,
        std::string* error = nullptr) {
        if (DataTypeSize(params.dataType) == 0u) {
            validation::AssignError(error, "attribute data type is unsupported");
            return false;
        }
        if (params.elementCount > 0u && params.dimension <= 0) {
            validation::AssignError(error, "attribute dimension is invalid");
            return false;
        }
        return true;
    }

    [[nodiscard]] static bool AppendAttribute(
        CodecStorageParams& params,
        const IEncodeAdapter& adapter,
        const AttrAttachment attachment,
        const std::size_t sourceIndex,
        std::vector<std::string>* warnings,
        std::string* error) {
        EncodeAttributeView attrView;
        AttrStorageParams attrParams;
        std::string attrError;
        const auto* attachmentName = attachment == AttrAttachment::Point ? "point" : "cell";
        const auto built = attachment == AttrAttachment::Point
            ? adapter.BuildPointAttributeView(sourceIndex, attrView)
            : adapter.BuildCellAttributeView(sourceIndex, attrView);
        if (!built) {
            (void)warnings;
            return validation::AssignError(
                error,
                AttributeSourceLabel(attachmentName, sourceIndex, {}) + " failed to get view");
        }
        if (!AttrStorageParamsFactory::TryFromEncodeAttributeView(attrView, attrParams, &attrError)) {
            return validation::AssignError(
                error,
                AttributeSourceLabel(attachmentName, sourceIndex, attrView.name) +
                    " failed to resolve view: " + attrError);
        }
        if (!ValidateEncodableAttributeParams(attrParams, &attrError)) {
            return validation::AssignError(
                error,
                AttributeSourceLabel(attachmentName, sourceIndex, attrParams.name) +
                    " has unsupported params: " + attrError);
        }
        params.attrParams.push_back(std::move(attrParams));
        params.attrSourceIndices.push_back(sourceIndex);
        return true;
    }

    [[nodiscard]] static bool AppendSelectedAttrs(
        CodecStorageParams& params,
        const IEncodeAdapter& adapter,
        const std::span<const std::size_t> selectedAttrIndices,
        std::vector<std::string>* warnings,
        std::string* error) {
        const auto pointAttrCount = adapter.GetNumberOfPointAttrs();
        const auto totalAttrCount = pointAttrCount + adapter.GetNumberOfCellAttrs();
        std::vector<std::size_t> orderedIndices(selectedAttrIndices.begin(), selectedAttrIndices.end());
        std::sort(orderedIndices.begin(), orderedIndices.end());
        if (std::adjacent_find(orderedIndices.begin(), orderedIndices.end()) != orderedIndices.end()) {
            return validation::AssignError(error, "attribute target list contains duplicate indices");
        }
        for (const auto flattenedIndex : orderedIndices) {
            if (flattenedIndex >= totalAttrCount) {
                return validation::AssignError(error, "attribute target index is out of range");
            }
            const auto attachment = flattenedIndex < pointAttrCount
                ? AttrAttachment::Point
                : AttrAttachment::Cell;
            const auto sourceIndex = attachment == AttrAttachment::Point
                ? flattenedIndex
                : flattenedIndex - pointAttrCount;
            if (!AppendAttribute(
                    params,
                    adapter,
                    attachment,
                    sourceIndex,
                    warnings,
                    error)) {
                return false;
            }
        }
        return true;
    }
};

} // namespace datacodec

#endif
