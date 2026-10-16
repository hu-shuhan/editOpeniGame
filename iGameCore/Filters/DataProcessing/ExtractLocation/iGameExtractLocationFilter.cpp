#include "iGameExtractLocationFilter.h"

#include <iGameVolume.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

IGAME_NAMESPACE_BEGIN

namespace {

bool IsPointInsideTetrahedron(const Point& point, const std::array<Point, 4>& tetra, double tolerance) {
    const auto signedVolume6 = [](const Point& a, const Point& b, const Point& c, const Point& d) {
        return (b - a).dot((c - a).cross(d - a));
    };

    const double total = signedVolume6(tetra[0], tetra[1], tetra[2], tetra[3]);
    if (std::abs(total) <= tolerance) return false;

    const std::array<double, 4> barycentric{{
            signedVolume6(point, tetra[1], tetra[2], tetra[3]) / total,
            signedVolume6(tetra[0], point, tetra[2], tetra[3]) / total,
            signedVolume6(tetra[0], tetra[1], point, tetra[3]) / total,
            signedVolume6(tetra[0], tetra[1], tetra[2], point) / total,
    }};
    const double sum = barycentric[0] + barycentric[1] + barycentric[2] + barycentric[3];
    return std::abs(sum - 1.0) <= tolerance * 8.0 &&
           std::all_of(barycentric.begin(), barycentric.end(),
                       [tolerance](double value) { return value >= -tolerance && value <= 1.0 + tolerance; });
}

template<typename TArray>
ArrayObject::Pointer CopySelectedValues(const ArrayObject::Pointer& input,
                                        const std::vector<igIndex>& sourceIds) {
    auto typedInput = DynamicCast<TArray>(input);
    if (typedInput.IsNull()) return nullptr;

    auto output = TArray::New();
    output->SetName(input->GetName());
    output->SetDimension(input->GetDimension());
    output->Resize(sourceIds.size());
    const int dimension = input->GetDimension();
    for (IGsize outputIndex = 0; outputIndex < sourceIds.size(); ++outputIndex) {
        std::copy_n(typedInput->RawPointer(sourceIds[outputIndex]), dimension,
                    output->RawPointer(outputIndex));
    }
    return output;
}

ArrayObject::Pointer CopySelectedArrayValues(const ArrayObject::Pointer& input,
                                             const std::vector<igIndex>& sourceIds) {
    if (DynamicCast<FloatArray>(input)) return CopySelectedValues<FloatArray>(input, sourceIds);
    if (DynamicCast<DoubleArray>(input)) return CopySelectedValues<DoubleArray>(input, sourceIds);
    if (DynamicCast<IntArray>(input)) return CopySelectedValues<IntArray>(input, sourceIds);
    if (DynamicCast<UnsignedIntArray>(input)) return CopySelectedValues<UnsignedIntArray>(input, sourceIds);
    if (DynamicCast<CharArray>(input)) return CopySelectedValues<CharArray>(input, sourceIds);
    if (DynamicCast<UnsignedCharArray>(input)) return CopySelectedValues<UnsignedCharArray>(input, sourceIds);
    if (DynamicCast<ShortArray>(input)) return CopySelectedValues<ShortArray>(input, sourceIds);
    if (DynamicCast<UnsignedShortArray>(input)) return CopySelectedValues<UnsignedShortArray>(input, sourceIds);
    if (DynamicCast<LongLongArray>(input)) return CopySelectedValues<LongLongArray>(input, sourceIds);
    if (DynamicCast<UnsignedLongLongArray>(input))
        return CopySelectedValues<UnsignedLongLongArray>(input, sourceIds);
    return nullptr;
}

} // namespace

ExtractLocationFilter::ExtractLocationFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ExtractLocationFilter::Execute() {
    m_ExtractedCellIds.clear();
    m_LastError.clear();
    m_OutputMesh = nullptr;
    m_InputMesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_InputMesh.IsNull()) {
        m_LastError = "当前版本仅支持非结构网格 (UnstructuredMesh) 输入。";
        return false;
    }
    if (m_InputMesh->GetNumberOfCells() == 0) {
        m_LastError = "输入网格不包含单元。";
        return false;
    }

    for (igIndex cellId = 0; cellId < m_InputMesh->GetNumberOfCells(); ++cellId) {
        const IGenum cellType = m_InputMesh->GetCellType(cellId);
        if (cellType != IG_TETRA && cellType != IG_HEXAHEDRON && cellType != IG_PRISM && cellType != IG_PYRAMID) {
            m_LastError = std::string("不支持单元 ") + std::to_string(cellId) + " 的类型：" +
                          GetCellTypeAsString(cellType) +
                          "。当前支持线性 Tetra、Hexahedron、Prism 和 Pyramid。";
            return false;
        }
    }

    const double tolerance = std::max(1.0e-9, m_InputMesh->GetBoundingBox().diag() * 1.0e-8);
    for (igIndex cellId = 0; cellId < m_InputMesh->GetNumberOfCells(); ++cellId) {
        const IGenum cellType = m_InputMesh->GetCellType(cellId);
        const igIndex* cellPointIds = nullptr;
        const int cellPointCount = m_InputMesh->GetCellPointIds(cellId, cellPointIds);
        if (cellPointCount <= 0 || cellPointIds == nullptr) continue;

        // Broad phase: reject cells whose axis-aligned bounds do not contain
        // the query point before constructing/decomposing the geometric cell.
        Point minimum = m_InputMesh->GetPoint(cellPointIds[0]);
        Point maximum = minimum;
        for (int pointIndex = 1; pointIndex < cellPointCount; ++pointIndex) {
            const auto& cellPoint = m_InputMesh->GetPoint(cellPointIds[pointIndex]);
            for (int component = 0; component < 3; ++component) {
                minimum[component] = std::min(minimum[component], cellPoint[component]);
                maximum[component] = std::max(maximum[component], cellPoint[component]);
            }
        }
        bool insideBounds = true;
        for (int component = 0; component < 3; ++component) {
            if (m_Location[component] < minimum[component] - tolerance ||
                m_Location[component] > maximum[component] + tolerance) {
                insideBounds = false;
                break;
            }
        }
        if (!insideBounds) continue;

        Cell::Pointer cell;
        if (!m_InputMesh->GetCell(cellId, cell)) continue;
        auto volume = DynamicCast<Volume>(cell);
        if (volume.IsNull()) continue;

        bool containsLocation = false;
        for (const auto& tetraCell : volume->clipCelltoTetra()) {
            if (tetraCell.IsNull() || tetraCell->GetNumberOfPoints() != 4) continue;
            std::array<Point, 4> tetra{};
            for (int pointId = 0; pointId < 4; ++pointId) tetra[pointId] = tetraCell->GetPoint(pointId);
            if (IsPointInsideTetrahedron(m_Location, tetra, tolerance)) {
                containsLocation = true;
                break;
            }
        }
        if (containsLocation) {
            // ParaView/vtkDataSet::FindCell returns one containing cell. On a shared
            // face/edge/vertex, preserve deterministic input order and take the first.
            m_ExtractedCellIds.push_back(cellId);
            break;
        }
    }

    m_OutputMesh = UnstructuredMesh::New();
    m_OutputMesh->SetName(m_InputMesh->GetName() + "_ExtractLocation");
    std::unordered_map<igIndex, igIndex> pointIdMap;
    std::vector<igIndex> outputToInputPointIds;
    for (const igIndex inputCellId : m_ExtractedCellIds) {
        const igIndex* inputPointIds = nullptr;
        const int pointCount = m_InputMesh->GetCellPointIds(inputCellId, inputPointIds);
        std::vector<igIndex> outputPointIds(pointCount);
        for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            const igIndex inputPointId = inputPointIds[pointIndex];
            const auto [it, inserted] = pointIdMap.emplace(
                    inputPointId, static_cast<igIndex>(m_OutputMesh->GetNumberOfPoints()));
            if (inserted) {
                m_OutputMesh->AddPoint(m_InputMesh->GetPoint(inputPointId));
                outputToInputPointIds.push_back(inputPointId);
            }
            outputPointIds[pointIndex] = it->second;
        }
        m_OutputMesh->AddCell(outputPointIds.data(), pointCount, m_InputMesh->GetCellType(inputCellId));
    }

    // Preserve point/cell data using existing AttributeSet/ArrayObject types.
    auto outputAttributes = AttributeSet::New();
    auto inputAttributes = m_InputMesh->GetAttributeSet();
    for (igIndex attributeIndex = 0; attributeIndex < inputAttributes->GetNumberOfAttributes(); ++attributeIndex) {
        auto& inputAttribute = inputAttributes->GetAttribute(attributeIndex);
        const auto inputArray = inputAttribute.GetPointer();
        if (inputArray == nullptr || (inputAttribute.GetAttachmentType() != IG_POINT &&
                                      inputAttribute.GetAttachmentType() != IG_CELL)) {
            continue;
        }
        const auto& sourceIds = inputAttribute.GetAttachmentType() == IG_POINT
                                        ? outputToInputPointIds
                                        : m_ExtractedCellIds;
        auto outputArray = CopySelectedArrayValues(inputArray, sourceIds);
        if (outputArray.IsNull()) {
            m_LastError = std::string("无法保留数组 '") + inputArray->GetName() +
                          "' 的数值类型。";
            return false;
        }
        outputAttributes->AddAttribute(inputAttribute.GetType(), inputAttribute.GetAttachmentType(), outputArray);
    }

    auto originalPointIds = LongLongArray::New();
    originalPointIds->SetName(OriginalPointIdsArrayName());
    originalPointIds->SetDimension(1);
    originalPointIds->Reserve(outputToInputPointIds.size());
    for (const igIndex pointId : outputToInputPointIds)
        originalPointIds->AddValue(static_cast<igIndex64>(pointId));
    outputAttributes->AddAttribute(IG_SCALAR, IG_POINT, originalPointIds);

    auto originalCellIds = LongLongArray::New();
    originalCellIds->SetName(OriginalCellIdsArrayName());
    originalCellIds->SetDimension(1);
    originalCellIds->Reserve(m_ExtractedCellIds.size());
    for (const igIndex cellId : m_ExtractedCellIds)
        originalCellIds->AddValue(static_cast<igIndex64>(cellId));
    outputAttributes->AddAttribute(IG_SCALAR, IG_CELL, originalCellIds);
    m_OutputMesh->SetAttributeSet(outputAttributes);
    SetOutput(m_OutputMesh);
    return true;
}

IGAME_NAMESPACE_END
