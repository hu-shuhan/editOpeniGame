#ifndef DATACODEC_LOG_ANALYSIS_ADAPTERPRECISIONMETRICS_H
#define DATACODEC_LOG_ANALYSIS_ADAPTERPRECISIONMETRICS_H

#include "DataCodec/Log/Analysis/AdapterTreeSignature.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>
namespace datacodec::log {

enum class NumericFieldPrecisionKind : std::uint8_t {
    Geometry,
    PointAttribute,
    CellAttribute,
};

struct NumericFieldPrecisionMetric {
    BlockPath leafPath;
    NumericFieldPrecisionKind kind{NumericFieldPrecisionKind::Geometry};
    std::string name;
    ScalarType expectedScalarType{ScalarType::Float32};
    ScalarType actualScalarType{ScalarType::Float32};
    std::uint32_t componentCount{0u};
    std::uint64_t tupleCount{0u};
    std::uint64_t valueCount{0u};
    bool ok{false};
    std::string error;
    double maxAbsError{0.0};
    double rmsAbsError{0.0};
    double maxPointRelativeError{0.0};
    double rangeRelativeError{0.0};
    bool hasRangeRelativeError{false};
    double expectedMin{0.0};
    double expectedMax{0.0};
    double sumSquaredError{0.0};
};

struct AdapterPrecisionReport {
    LogAnalysisResult status;
    std::vector<NumericFieldPrecisionMetric> fields;
};

namespace detail {

inline void FailPrecisionMetric(
    NumericFieldPrecisionMetric& metric,
    LogAnalysisResult& status,
    std::string check,
    std::string message) {
    metric.ok = false;
    metric.error = message;
    status.AddFailure(std::move(check), std::move(message));
}

inline bool ValidatePrecisionViews(
    NumericFieldPrecisionMetric& metric,
    const NumericArrayView& expected,
    const NumericArrayView& actual,
    LogAnalysisResult& status,
    const std::string& checkPath) {
    if (!expected.IsValid() || !actual.IsValid()) {
        FailPrecisionMetric(metric, status, checkPath, "numeric view is invalid");
        return false;
    }
    if (expected.componentCount != actual.componentCount) {
        FailPrecisionMetric(metric, status, checkPath + ".componentCount", "component count mismatch");
        return false;
    }
    if (expected.tupleCount != actual.tupleCount) {
        FailPrecisionMetric(metric, status, checkPath + ".tupleCount", "tuple count mismatch");
        return false;
    }
    const auto expectedScalarSize = ScalarTypeSize(expected.scalarType);
    const auto actualScalarSize = ScalarTypeSize(actual.scalarType);
    if (expectedScalarSize == 0u || actualScalarSize == 0u) {
        FailPrecisionMetric(metric, status, checkPath + ".scalarSize", "numeric scalar size is zero");
        return false;
    }
    return true;
}

} // namespace detail

inline NumericFieldPrecisionMetric ComputeNumericFieldPrecision(
    const BlockPath& leafPath,
    const NumericFieldPrecisionKind kind,
    const std::string& name,
    const NumericArrayView& expected,
    const NumericArrayView& actual,
    LogAnalysisResult& status,
    const std::string& checkPath,
    const std::vector<IndexType>* expectedTupleOrder = nullptr) {
    NumericFieldPrecisionMetric metric;
    metric.leafPath = leafPath;
    metric.kind = kind;
    metric.name = name;
    metric.expectedScalarType = expected.scalarType;
    metric.actualScalarType = actual.scalarType;
    metric.componentCount = static_cast<std::uint32_t>(std::max(expected.componentCount, 0));
    metric.tupleCount = static_cast<std::uint64_t>(expected.tupleCount);
    metric.expectedMin = std::numeric_limits<double>::infinity();
    metric.expectedMax = -std::numeric_limits<double>::infinity();

    if (!detail::ValidatePrecisionViews(metric, expected, actual, status, checkPath)) {
        return metric;
    }

    const auto componentCount = static_cast<std::size_t>(std::max(expected.componentCount, 0));
    if (componentCount == 0u || expected.tupleCount == 0u) {
        metric.ok = true;
        metric.expectedMin = 0.0;
        metric.expectedMax = 0.0;
        metric.hasRangeRelativeError = true;
        return metric;
    }

    const auto expectedScalarSize = ScalarTypeSize(expected.scalarType);
    const auto actualScalarSize = ScalarTypeSize(actual.scalarType);
    std::vector<std::uint8_t> expectedTuple(componentCount * expectedScalarSize, 0u);
    std::vector<std::uint8_t> actualTuple(componentCount * actualScalarSize, 0u);

    for (std::uint64_t tupleIndex = 0u; tupleIndex < metric.tupleCount; ++tupleIndex) {
        std::uint64_t expectedTupleIndex = tupleIndex;
        if (expectedTupleOrder != nullptr && !expectedTupleOrder->empty()) {
            if (tupleIndex >= expectedTupleOrder->size()) {
                detail::FailPrecisionMetric(
                    metric,
                    status,
                    checkPath + ".tupleOrder",
                    "tuple order is shorter than tuple count");
                return metric;
            }
            expectedTupleIndex = static_cast<std::uint64_t>(
                (*expectedTupleOrder)[static_cast<std::size_t>(tupleIndex)]);
        }

        std::string error;
        if (!ReadNumericArrayTupleBytes(
                expected,
                static_cast<std::size_t>(expectedTupleIndex),
                componentCount,
                expectedTuple.data(),
                &error) ||
            !ReadNumericArrayTupleBytes(
                actual,
                static_cast<std::size_t>(tupleIndex),
                componentCount,
                actualTuple.data(),
                &error)) {
            detail::FailPrecisionMetric(
                metric,
                status,
                checkPath + ".tuple",
                error.empty() ? "failed to read numeric tuple" : error);
            return metric;
        }

        for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex) {
            const double expectedValue = detail::ReadScalarAsDouble(
                expected.scalarType,
                expectedTuple.data() + componentIndex * expectedScalarSize);
            const double actualValue = detail::ReadScalarAsDouble(
                actual.scalarType,
                actualTuple.data() + componentIndex * actualScalarSize);
            if (!std::isfinite(expectedValue) || !std::isfinite(actualValue)) {
                detail::FailPrecisionMetric(metric, status, checkPath + ".finite", "non-finite value");
                return metric;
            }

            const double absError = std::abs(expectedValue - actualValue);
            const double relBase = std::max(std::abs(expectedValue), 1.0e-30);
            metric.maxAbsError = std::max(metric.maxAbsError, absError);
            metric.maxPointRelativeError = std::max(metric.maxPointRelativeError, absError / relBase);
            metric.sumSquaredError += absError * absError;
            metric.expectedMin = std::min(metric.expectedMin, expectedValue);
            metric.expectedMax = std::max(metric.expectedMax, expectedValue);
            ++metric.valueCount;
        }
    }

    metric.ok = true;
    if (metric.valueCount == 0u) {
        metric.expectedMin = 0.0;
        metric.expectedMax = 0.0;
        metric.hasRangeRelativeError = true;
        return metric;
    }

    metric.rmsAbsError = std::sqrt(metric.sumSquaredError / static_cast<double>(metric.valueCount));
    const double range = metric.expectedMax - metric.expectedMin;
    if (range > 0.0) {
        metric.rangeRelativeError = metric.maxAbsError / range;
        metric.hasRangeRelativeError = true;
    } else if (metric.maxAbsError == 0.0) {
        metric.rangeRelativeError = 0.0;
        metric.hasRangeRelativeError = true;
    }
    return metric;
}

inline AdapterPrecisionReport ComputeAdapterPrecisionMetrics(
    const IBlockTreeAdapter& expectedTree,
    const IBlockTreeAdapter& actualTree,
    const AdapterSignatureOrderSet* orderSet = nullptr) {
    AdapterPrecisionReport result;
    const auto expectedLeaves = expectedTree.GetLeafRecords();
    const auto actualLeaves = actualTree.GetLeafRecords();
    if (!RequireLogAnalysis(
            result.status,
            expectedLeaves.size() == actualLeaves.size(),
            "AdapterPrecision.leafCount",
            "leaf count mismatch")) {
        return result;
    }

    for (std::size_t leafIndex = 0u; leafIndex < expectedLeaves.size(); ++leafIndex) {
        const auto& expectedLeaf = expectedLeaves[leafIndex];
        const auto& actualLeaf = actualLeaves[leafIndex];
        const auto leafCheck = "AdapterPrecision.leaf[" + std::to_string(leafIndex) + "]";
        auto expectedAdapter = expectedTree.GetLeaf(expectedLeaf.path);
        auto actualAdapter = actualTree.GetLeaf(actualLeaf.path);
        if (expectedAdapter == nullptr || actualAdapter == nullptr) {
            result.status.AddFailure(leafCheck, "leaf adapter is null");
            continue;
        }

        RequireLogAnalysis(result.status, expectedLeaf.path == actualLeaf.path, leafCheck + ".path", "leaf path mismatch");
        RequireLogAnalysis(
            result.status,
            expectedAdapter->GetMeshType() == actualAdapter->GetMeshType(),
            leafCheck + ".meshType",
            "mesh type mismatch");
        RequireLogAnalysis(
            result.status,
            expectedAdapter->GetNumberOfPoints() == actualAdapter->GetNumberOfPoints(),
            leafCheck + ".pointCount",
            "point count mismatch");
        RequireLogAnalysis(
            result.status,
            expectedAdapter->GetNumberOfCells() == actualAdapter->GetNumberOfCells(),
            leafCheck + ".cellCount",
            "cell count mismatch");

        const auto* pointOrder = orderSet != nullptr
            ? FindSignatureOrder(orderSet->pointOrders, expectedLeaf.path)
            : nullptr;
        const auto* cellOrder = orderSet != nullptr
            ? FindSignatureOrder(orderSet->cellOrders, expectedLeaf.path)
            : nullptr;

        NumericArrayView expectedGeometry;
        NumericArrayView actualGeometry;
        if (expectedAdapter->BuildGeometryView(expectedGeometry) &&
            actualAdapter->BuildGeometryView(actualGeometry)) {
            result.fields.push_back(ComputeNumericFieldPrecision(
                expectedLeaf.path,
                NumericFieldPrecisionKind::Geometry,
                "geometry",
                expectedGeometry,
                actualGeometry,
                result.status,
                leafCheck + ".geometry",
                pointOrder));
        } else {
            result.status.AddFailure(leafCheck + ".geometry", "failed to build geometry view");
        }

        const auto pointAttrCount = std::min(
            expectedAdapter->GetNumberOfPointAttrs(),
            actualAdapter->GetNumberOfPointAttrs());
        RequireLogAnalysis(
            result.status,
            expectedAdapter->GetNumberOfPointAttrs() == actualAdapter->GetNumberOfPointAttrs(),
            leafCheck + ".pointAttrCount",
            "point attribute count mismatch");
        for (std::size_t attrIndex = 0u; attrIndex < pointAttrCount; ++attrIndex) {
            EncodeAttributeView expectedAttr;
            EncodeAttributeView actualAttr;
            const auto& expectedAttrView = expectedAdapter->GetPointAttr(attrIndex);
            const auto& actualAttrView = actualAdapter->GetPointAttr(attrIndex);
            if (!expectedAttrView.BuildAttributeView(expectedAttr) ||
                !actualAttrView.BuildAttributeView(actualAttr)) {
                result.status.AddFailure(
                    leafCheck + ".pointAttr[" + std::to_string(attrIndex) + "]",
                    "failed to build attribute view");
                continue;
            }
            result.fields.push_back(ComputeNumericFieldPrecision(
                expectedLeaf.path,
                NumericFieldPrecisionKind::PointAttribute,
                expectedAttrView.GetName(),
                expectedAttr.values,
                actualAttr.values,
                result.status,
                leafCheck + ".pointAttr[" + std::to_string(attrIndex) + "]",
                pointOrder));
        }

        const auto cellAttrCount = std::min(
            expectedAdapter->GetNumberOfCellAttrs(),
            actualAdapter->GetNumberOfCellAttrs());
        RequireLogAnalysis(
            result.status,
            expectedAdapter->GetNumberOfCellAttrs() == actualAdapter->GetNumberOfCellAttrs(),
            leafCheck + ".cellAttrCount",
            "cell attribute count mismatch");
        for (std::size_t attrIndex = 0u; attrIndex < cellAttrCount; ++attrIndex) {
            EncodeAttributeView expectedAttr;
            EncodeAttributeView actualAttr;
            const auto& expectedAttrView = expectedAdapter->GetCellAttr(attrIndex);
            const auto& actualAttrView = actualAdapter->GetCellAttr(attrIndex);
            if (!expectedAttrView.BuildAttributeView(expectedAttr) ||
                !actualAttrView.BuildAttributeView(actualAttr)) {
                result.status.AddFailure(
                    leafCheck + ".cellAttr[" + std::to_string(attrIndex) + "]",
                    "failed to build attribute view");
                continue;
            }
            result.fields.push_back(ComputeNumericFieldPrecision(
                expectedLeaf.path,
                NumericFieldPrecisionKind::CellAttribute,
                expectedAttrView.GetName(),
                expectedAttr.values,
                actualAttr.values,
                result.status,
                leafCheck + ".cellAttr[" + std::to_string(attrIndex) + "]",
                cellOrder));
        }
    }
    return result;
}

} // datacodec::log命名空间

#endif
