#ifndef DATACODEC_LOG_ANALYSIS_ADAPTERTREESIGNATURE_H
#define DATACODEC_LOG_ANALYSIS_ADAPTERTREESIGNATURE_H

#include "DataCodec/API/Adapter/IBlockTreeAdapter.h"
#include "DataCodec/Common/Views/AttributeViews.h"
#include "DataCodec/Common/Views/ArrayViews.h"
#include "DataCodec/Log/Analysis/LogAnalysisResult.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace datacodec::log {


struct AdapterTreeSignatureOptions {
    std::uint64_t maxTuplesPerField{4096u};
    double absTolerance{1.0e-3};
    double relTolerance{1.0e-3};
    bool compareLeafNames{false};
};

struct NumericFieldSignature {
    std::string name;
    ScalarType scalarType{ScalarType::Float32};
    std::uint32_t componentCount{0u};
    std::uint64_t tupleCount{0u};
    std::uint64_t sampledTupleCount{0u};
    std::vector<double> sums;
    std::vector<double> minimums;
    std::vector<double> maximums;
};

struct LeafAdapterSignature {
    BlockPath path;
    std::string name;
    MeshType meshType{MeshType::PointSet};
    std::size_t pointCount{0u};
    std::size_t cellCount{0u};
    std::size_t faceCount{0u};
    NumericFieldSignature geometry;
    std::vector<NumericFieldSignature> pointAttributes;
    std::vector<NumericFieldSignature> cellAttributes;
};

struct AdapterTreeSignature {
    std::string rootName;
    std::vector<LeafAdapterSignature> leaves;
};

struct AdapterSignatureOrderSet {
    std::unordered_map<BlockPath, std::vector<IndexType>> pointOrders;
    std::unordered_map<BlockPath, std::vector<IndexType>> cellOrders;
};

namespace detail {

template<typename TValue>
[[nodiscard]] inline double ReadScalarAsDouble(const std::uint8_t* bytes) {
    TValue value{};
    std::memcpy(&value, bytes, sizeof(TValue));
    return static_cast<double>(value);
}

[[nodiscard]] inline double ReadScalarAsDouble(const ScalarType type, const std::uint8_t* bytes) {
    switch (type) {
        case ScalarType::Float32:
            return ReadScalarAsDouble<float>(bytes);
        case ScalarType::Float64:
            return ReadScalarAsDouble<double>(bytes);
        case ScalarType::Int8:
            return ReadScalarAsDouble<std::int8_t>(bytes);
        case ScalarType::UInt8:
            return ReadScalarAsDouble<std::uint8_t>(bytes);
        case ScalarType::Int16:
            return ReadScalarAsDouble<std::int16_t>(bytes);
        case ScalarType::UInt16:
            return ReadScalarAsDouble<std::uint16_t>(bytes);
        case ScalarType::Int32:
            return ReadScalarAsDouble<std::int32_t>(bytes);
        case ScalarType::UInt32:
            return ReadScalarAsDouble<std::uint32_t>(bytes);
        case ScalarType::Int64:
            return ReadScalarAsDouble<std::int64_t>(bytes);
        case ScalarType::UInt64:
            return ReadScalarAsDouble<std::uint64_t>(bytes);
    }
    return 0.0;
}

[[nodiscard]] inline bool Near(
    const double expected,
    const double actual,
    const double absTolerance,
    const double relTolerance) {
    const auto absError = std::abs(expected - actual);
    if (absError <= absTolerance) {
        return true;
    }
    const auto relBase = std::max(std::abs(expected), 1.0e-30);
    return absError / relBase <= relTolerance;
}

inline void CompareDouble(
    LogAnalysisResult& result,
    const std::string& check,
    const double expected,
    const double actual,
    const AdapterTreeSignatureOptions& options,
    const double scale = 1.0) {
    if (Near(expected, actual, options.absTolerance * std::max(scale, 1.0), options.relTolerance)) {
        return;
    }
    result.AddFailure(
        check,
        "expected=" + std::to_string(expected) + " actual=" + std::to_string(actual));
}

inline void CompareNumericFieldSignature(
    LogAnalysisResult& result,
    const std::string& path,
    const NumericFieldSignature& expected,
    const NumericFieldSignature& actual,
    const AdapterTreeSignatureOptions& options) {
    RequireLogAnalysis(result, expected.name == actual.name, path + ".name", "numeric field name mismatch");
    RequireLogAnalysis(
        result,
        expected.scalarType == actual.scalarType,
        path + ".scalarType",
        "numeric field scalar type mismatch");
    RequireLogAnalysis(
        result,
        expected.componentCount == actual.componentCount,
        path + ".componentCount",
        "numeric field component count mismatch");
    RequireLogAnalysis(
        result,
        expected.tupleCount == actual.tupleCount,
        path + ".tupleCount",
        "numeric field tuple count mismatch");
    RequireLogAnalysis(
        result,
        expected.sampledTupleCount == actual.sampledTupleCount,
        path + ".sampledTupleCount",
        "numeric field sampled tuple count mismatch");
    if (expected.componentCount != actual.componentCount) {
        return;
    }

    const auto componentCount = static_cast<std::size_t>(expected.componentCount);
    for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex) {
        const auto componentPath = path + ".component[" + std::to_string(componentIndex) + "]";
        CompareDouble(
            result,
            componentPath + ".sum",
            expected.sums[componentIndex],
            actual.sums[componentIndex],
            options,
            static_cast<double>(std::max<std::uint64_t>(expected.sampledTupleCount, 1u)));
        CompareDouble(
            result,
            componentPath + ".min",
            expected.minimums[componentIndex],
            actual.minimums[componentIndex],
            options);
        CompareDouble(
            result,
            componentPath + ".max",
            expected.maximums[componentIndex],
            actual.maximums[componentIndex],
            options);
    }
}

inline void CompareNumericFieldVector(
    LogAnalysisResult& result,
    const std::string& path,
    const std::vector<NumericFieldSignature>& expected,
    const std::vector<NumericFieldSignature>& actual,
    const AdapterTreeSignatureOptions& options) {
    RequireLogAnalysis(result, expected.size() == actual.size(), path + ".size", "numeric field count mismatch");
    if (expected.size() != actual.size()) {
        return;
    }
    for (std::size_t index = 0u; index < expected.size(); ++index) {
        CompareNumericFieldSignature(
            result,
            path + "[" + std::to_string(index) + "]",
            expected[index],
            actual[index],
            options);
    }
}

}

inline bool BuildNumericFieldSignature(
    const std::string& name,
    const NumericArrayView& view,
    const AdapterTreeSignatureOptions& options,
    NumericFieldSignature& output,
    LogAnalysisResult& result,
    const std::string& checkPath,
    const std::vector<IndexType>* tupleOrder = nullptr) {
    output = {};
    output.name = name;
    output.scalarType = view.scalarType;
    output.componentCount = static_cast<std::uint32_t>(std::max(view.componentCount, 0));
    output.tupleCount = static_cast<std::uint64_t>(view.tupleCount);
    output.sampledTupleCount = std::min<std::uint64_t>(output.tupleCount, options.maxTuplesPerField);

    const auto componentCount = static_cast<std::size_t>(output.componentCount);
    output.sums.assign(componentCount, 0.0);
    output.minimums.assign(componentCount, std::numeric_limits<double>::infinity());
    output.maximums.assign(componentCount, -std::numeric_limits<double>::infinity());

    if (!view.IsValid()) {
        result.AddFailure(checkPath, "numeric view is invalid");
        return false;
    }
    const auto scalarSize = ScalarTypeSize(view.scalarType);
    if (scalarSize == 0u) {
        result.AddFailure(checkPath, "numeric scalar size is zero");
        return false;
    }
    if (componentCount == 0u || output.sampledTupleCount == 0u) {
        std::fill(output.minimums.begin(), output.minimums.end(), 0.0);
        std::fill(output.maximums.begin(), output.maximums.end(), 0.0);
        return true;
    }

    std::vector<std::uint8_t> tupleBytes(componentCount * scalarSize, 0u);
    for (std::uint64_t tupleIndex = 0u; tupleIndex < output.sampledTupleCount; ++tupleIndex) {
        std::uint64_t sourceTupleIndex = tupleIndex;
        if (tupleOrder != nullptr && !tupleOrder->empty()) {
            if (tupleIndex >= tupleOrder->size()) {
                result.AddFailure(checkPath, "tuple order is shorter than sampled tuple count");
                return false;
            }
            sourceTupleIndex = static_cast<std::uint64_t>((*tupleOrder)[static_cast<std::size_t>(tupleIndex)]);
        }
        std::string error;
        if (!ReadNumericArrayTupleBytes(
                view,
                static_cast<std::size_t>(sourceTupleIndex),
                componentCount,
                tupleBytes.data(),
                &error)) {
            result.AddFailure(checkPath, error.empty() ? "failed to read numeric tuple" : error);
            return false;
        }
        for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex) {
            const auto value = detail::ReadScalarAsDouble(
                view.scalarType,
                tupleBytes.data() + componentIndex * scalarSize);
            output.sums[componentIndex] += value;
            output.minimums[componentIndex] = std::min(output.minimums[componentIndex], value);
            output.maximums[componentIndex] = std::max(output.maximums[componentIndex], value);
        }
    }
    return true;
}

inline bool BuildAttributeSignature(
    const IEncodeAttrView& attr,
    const AdapterTreeSignatureOptions& options,
    NumericFieldSignature& output,
    LogAnalysisResult& result,
    const std::string& checkPath,
    const std::vector<IndexType>* tupleOrder = nullptr) {
    EncodeAttributeView attrView;
    if (!attr.BuildAttributeView(attrView)) {
        result.AddFailure(checkPath, "failed to build attribute view");
        return false;
    }
    return BuildNumericFieldSignature(attr.GetName(), attrView.values, options, output, result, checkPath, tupleOrder);
}

[[nodiscard]] inline const std::vector<IndexType>* FindSignatureOrder(
    const std::unordered_map<BlockPath, std::vector<IndexType>>& orders,
    const BlockPath& path) {
    if (const auto it = orders.find(path); it != orders.end()) {
        return &it->second;
    }
    if (const auto it = orders.find(BlockPath{}); it != orders.end()) {
        return &it->second;
    }
    return nullptr;
}

inline bool BuildAdapterTreeSignature(
    const IBlockTreeAdapter& tree,
    const AdapterTreeSignatureOptions& options,
    AdapterTreeSignature& output,
    LogAnalysisResult& result,
    const AdapterSignatureOrderSet* orderSet = nullptr) {
    output = {};
    output.rootName = tree.GetRootName();

    const auto leaves = tree.GetLeafRecords();
    output.leaves.reserve(leaves.size());
    for (const auto& leaf : leaves) {
        auto adapter = tree.GetLeaf(leaf.path);
        if (adapter == nullptr) {
            result.AddFailure("AdapterTreeSignature.leaf", "leaf adapter is null for " + leaf.path);
            return false;
        }

        LeafAdapterSignature signature;
        signature.path = leaf.path;
        signature.name = leaf.name;
        signature.meshType = adapter->GetMeshType();
        signature.pointCount = adapter->GetNumberOfPoints();
        signature.cellCount = adapter->GetNumberOfCells();
        signature.faceCount = adapter->GetNumberOfFaces();
        const auto* pointOrder = orderSet != nullptr
            ? FindSignatureOrder(orderSet->pointOrders, leaf.path)
            : nullptr;
        const auto* cellOrder = orderSet != nullptr
            ? FindSignatureOrder(orderSet->cellOrders, leaf.path)
            : nullptr;

        NumericArrayView geometry;
        if (!adapter->BuildGeometryView(geometry) ||
            !BuildNumericFieldSignature(
                "geometry",
                geometry,
                options,
                signature.geometry,
                result,
                "AdapterTreeSignature." + leaf.path + ".geometry",
                pointOrder)) {
            return false;
        }

        signature.pointAttributes.reserve(adapter->GetNumberOfPointAttrs());
        for (std::size_t attrIndex = 0u; attrIndex < adapter->GetNumberOfPointAttrs(); ++attrIndex) {
            NumericFieldSignature attrSignature;
            if (!BuildAttributeSignature(
                    adapter->GetPointAttr(attrIndex),
                    options,
                    attrSignature,
                    result,
                    "AdapterTreeSignature." + leaf.path + ".pointAttr[" + std::to_string(attrIndex) + "]",
                    pointOrder)) {
                return false;
            }
            signature.pointAttributes.push_back(std::move(attrSignature));
        }

        signature.cellAttributes.reserve(adapter->GetNumberOfCellAttrs());
        for (std::size_t attrIndex = 0u; attrIndex < adapter->GetNumberOfCellAttrs(); ++attrIndex) {
            NumericFieldSignature attrSignature;
            if (!BuildAttributeSignature(
                    adapter->GetCellAttr(attrIndex),
                    options,
                    attrSignature,
                    result,
                    "AdapterTreeSignature." + leaf.path + ".cellAttr[" + std::to_string(attrIndex) + "]",
                    cellOrder)) {
                return false;
            }
            signature.cellAttributes.push_back(std::move(attrSignature));
        }

        output.leaves.push_back(std::move(signature));
    }
    return true;
}

inline void CompareAdapterTreeSignatures(
    LogAnalysisResult& result,
    const AdapterTreeSignature& expected,
    const AdapterTreeSignature& actual,
    const AdapterTreeSignatureOptions& options) {
    RequireLogAnalysis(result, expected.leaves.size() == actual.leaves.size(), "signature.leafCount", "leaf count mismatch");
    if (expected.leaves.size() != actual.leaves.size()) {
        return;
    }

    for (std::size_t leafIndex = 0u; leafIndex < expected.leaves.size(); ++leafIndex) {
        const auto& expectedLeaf = expected.leaves[leafIndex];
        const auto& actualLeaf = actual.leaves[leafIndex];
        const auto leafPath = "signature.leaf[" + std::to_string(leafIndex) + "]";
        RequireLogAnalysis(result, expectedLeaf.path == actualLeaf.path, leafPath + ".path", "leaf path mismatch");
        if (options.compareLeafNames) {
            RequireLogAnalysis(result, expectedLeaf.name == actualLeaf.name, leafPath + ".name", "leaf name mismatch");
        }
        RequireLogAnalysis(result, expectedLeaf.meshType == actualLeaf.meshType, leafPath + ".meshType", "mesh type mismatch");
        RequireLogAnalysis(result, expectedLeaf.pointCount == actualLeaf.pointCount, leafPath + ".pointCount", "point count mismatch");
        RequireLogAnalysis(result, expectedLeaf.cellCount == actualLeaf.cellCount, leafPath + ".cellCount", "cell count mismatch");
        RequireLogAnalysis(result, expectedLeaf.faceCount == actualLeaf.faceCount, leafPath + ".faceCount", "face count mismatch");

        detail::CompareNumericFieldSignature(
            result,
            leafPath + ".geometry",
            expectedLeaf.geometry,
            actualLeaf.geometry,
            options);
        detail::CompareNumericFieldVector(
            result,
            leafPath + ".pointAttributes",
            expectedLeaf.pointAttributes,
            actualLeaf.pointAttributes,
            options);
        detail::CompareNumericFieldVector(
            result,
            leafPath + ".cellAttributes",
            expectedLeaf.cellAttributes,
            actualLeaf.cellAttributes,
            options);
    }
}

inline LogAnalysisResult CompareAdapterTrees(
    const IBlockTreeAdapter& expectedTree,
    const IBlockTreeAdapter& actualTree,
    const AdapterTreeSignatureOptions& options = {}) {
    LogAnalysisResult result;
    AdapterTreeSignature expected;
    AdapterTreeSignature actual;
    if (!BuildAdapterTreeSignature(expectedTree, options, expected, result) ||
        !BuildAdapterTreeSignature(actualTree, options, actual, result)) {
        return result;
    }
    CompareAdapterTreeSignatures(result, expected, actual, options);
    return result;
}

} // datacodec::log命名空间

#endif
