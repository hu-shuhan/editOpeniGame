#include "iGameConnectivityFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

IGAME_NAMESPACE_BEGIN

namespace {

// Keep the exact native values when gathering a subset of point/cell tuples.
template<class Array>
ArrayObject::Pointer CopySelectedTypedAttribute(ArrayObject* source, const std::vector<igIndex>& selected) {
    auto input = DynamicCast<Array>(source);
    if (!input) return nullptr;
    auto output = Array::New();
    output->SetName(input->GetName());
    const int dim = input->GetDimension();
    output->SetDimension(dim);
    output->Resize(selected.size());
    for (IGsize i = 0; i < selected.size(); ++i) {
        if (selected[i] < 0 || static_cast<IGsize>(selected[i]) >= input->GetNumberOfElements()) return nullptr;
        std::copy_n(input->RawPointer(selected[i]), dim, output->RawPointer(i));
    }
    return output;
}

ArrayObject::Pointer CopySelectedAttribute(ArrayObject* input, const std::vector<igIndex>& selected) {
    switch (input->GetArrayType()) {
        case IG_DoubleArray: return CopySelectedTypedAttribute<DoubleArray>(input, selected);
        case IG_IntArray: return CopySelectedTypedAttribute<IntArray>(input, selected);
        case IG_INTARRAY: return CopySelectedTypedAttribute<IntArray>(input, selected);
        case IG_UnsignedIntArray: return CopySelectedTypedAttribute<UnsignedIntArray>(input, selected);
        case IG_CharArray: return CopySelectedTypedAttribute<CharArray>(input, selected);
        case IG_UnsignedCharArray: return CopySelectedTypedAttribute<UnsignedCharArray>(input, selected);
        case IG_ShortArray: return CopySelectedTypedAttribute<ShortArray>(input, selected);
        case IG_UnsignedShortArray: return CopySelectedTypedAttribute<UnsignedShortArray>(input, selected);
        case IG_LongLongArray: return CopySelectedTypedAttribute<LongLongArray>(input, selected);
        case IG_UnsignedLongLongArray: return CopySelectedTypedAttribute<UnsignedLongLongArray>(input, selected);
        case IG_FloatArray: return CopySelectedTypedAttribute<FloatArray>(input, selected);
        default: return nullptr;
    }
}

} // namespace

ConnectivityFilter::ConnectivityFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void ConnectivityFilter::BuildPointFaceAdjacency() {
    const IGsize nPts = m_Mesh->GetNumberOfPoints();
    const IGsize nCells = m_Mesh->GetNumberOfFaces();
    auto faces = m_Mesh->GetFaces();

    std::vector<IGsize> counts(nPts, 0);
    const igIndex* ids = nullptr;
    for (IGsize cellId = 0; cellId < nCells; ++cellId) {
        const int n = faces->GetCellIds(cellId, ids);
        for (int j = 0; j < n; ++j) { ++counts[ids[j]]; }
    }

    m_PointFaces.assign(nPts, {});
    for (IGsize p = 0; p < nPts; ++p) { m_PointFaces[p].reserve(counts[p]); }
    for (IGsize cellId = 0; cellId < nCells; ++cellId) {
        const int n = faces->GetCellIds(cellId, ids);
        for (int j = 0; j < n; ++j) { m_PointFaces[ids[j]].push_back(cellId); }
    }
}

bool ConnectivityFilter::CellIsConnected(igIndex cellId) const {
    if (!m_ScalarConnectivity || m_ScalarArray.IsNull()) { return true; }
    const igIndex* ids = nullptr;
    const int n = m_Mesh->GetFaces()->GetCellIds(cellId, ids);
    if (n <= 0) { return false; }

    const double r0 = std::min(m_ScalarRange[0], m_ScalarRange[1]);
    const double r1 = std::max(m_ScalarRange[0], m_ScalarRange[1]);
    bool any = false;
    for (int j = 0; j < n; ++j) {
        const double value = m_ScalarArray->GetElementValue(ids[j], 0);
        const bool inside = std::isfinite(value) && value >= r0 && value <= r1;
        if (m_FullScalarConnectivity && !inside) return false;
        any = any || inside;
    }
    return m_FullScalarConnectivity || any;
}

bool ConnectivityFilter::ResolveScalarArray() {
    auto attrSet = m_Mesh->GetAttributeSet();
    if (attrSet == nullptr) {
        m_Message = "ScalarConnectivity requires a point scalar array";
        return false;
    }
    auto valid = [this](const AttributeSet::Attribute& attr) {
        return !attr.isDeleted && attr.type == IG_SCALAR && attr.attachmentType == IG_POINT &&
               attr.pointer && attr.pointer->GetDimension() == 1 &&
               attr.pointer->GetNumberOfElements() == m_Mesh->GetNumberOfPoints();
    };
    auto all = attrSet->GetAllAttributes();
    if (!m_ScalarArrayName.empty()) {
        bool foundName = false;
        if (all) {
            for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
                auto& attr = all->GetElement(i);
                if (attr.isDeleted || !attr.pointer || attr.pointer->GetName() != m_ScalarArrayName) continue;
                foundName = true;
                if (valid(attr)) {
                    m_ScalarArray = attr.pointer;
                    return true;
                }
            }
        }
        m_Message = foundName
                ? "ScalarConnectivity requires a complete one-component point scalar array"
                : "ScalarConnectivity: array not found: " + m_ScalarArrayName;
        return false;
    }

    if (all) {
        const IGsize n = all->GetNumberOfElements();
        // 优先使用当前显示属性（若为点属性）。
        const int cur = m_Mesh->GetAttributeIndex();
        if (cur >= 0 && cur < static_cast<int>(n)) {
            auto& attr = all->GetElement(cur);
            if (valid(attr)) {
                m_ScalarArray = attr.pointer;
                return true;
            }
        }
        for (IGsize i = 0; i < n; ++i) {
            auto& attr = all->GetElement(i);
            if (valid(attr)) {
                m_ScalarArray = attr.pointer;
                return true;
            }
        }
    }
    m_Message = "ScalarConnectivity: no point attribute available";
    return false;
}

IGsize ConnectivityFilter::TraverseAndMark(const std::vector<igIndex>& seedCells, int regionNumber) {
    IGsize count = 0;
    std::vector<igIndex> queue;
    auto enqueue = [&](igIndex cell) {
        if (m_Visited[cell] >= 0) return;
        // Mark on insertion so multiple incident points cannot enqueue the same face repeatedly.
        m_Visited[cell] = regionNumber;
        queue.push_back(cell);
        ++count;
    };
    for (igIndex cell : seedCells) enqueue(cell);
    const igIndex* ids = nullptr;
    for (size_t head = 0; head < queue.size(); ++head) {
        const igIndex cell = queue[head];
        // Ineligible faces remain separate regions in non-seeded modes, regardless of face order.
        if (!m_CellEligible[cell]) continue;
        const int n = m_Mesh->GetFaces()->GetCellIds(cell, ids);
        for (int j = 0; j < n; ++j) {
            const igIndex point = ids[j];
            if (m_ExpandedPoints[point]) continue;
            m_ExpandedPoints[point] = 1;
            for (igIndex neighbor : m_PointFaces[point]) {
                if (m_CellEligible[neighbor]) enqueue(neighbor);
            }
        }
    }
    return count;
}

bool ConnectivityFilter::Execute() {
    m_Message.clear();
    SetOutput(nullptr);
    m_RegionSizes.clear();
    m_ScalarArray = nullptr;
    m_Mesh = DynamicCast<SurfaceMesh>(GetInput(0));
    if (m_Mesh == nullptr) {
        m_Message = "ConnectivityFilter only supports SurfaceMesh input";
        return false;
    }
    const IGsize nPts = m_Mesh->GetNumberOfPoints();
    const IGsize nCells = m_Mesh->GetNumberOfFaces();
    if (nPts == 0 || nCells == 0) {
        m_Message = "Input surface mesh is empty";
        return false;
    }
    if (m_ExtractionMode < POINT_SEEDED_REGIONS || m_ExtractionMode > CLOSEST_POINT_REGION) {
        m_Message = "Invalid extraction mode";
        return false;
    }

    if (m_RegionIdAssignmentMode < UNSPECIFIED || m_RegionIdAssignmentMode > CELL_COUNT_DESCENDING) {
        m_Message = "Invalid region ID assignment mode";
        return false;
    }
    if (m_ScalarConnectivity && (!std::isfinite(m_ScalarRange[0]) || !std::isfinite(m_ScalarRange[1]))) {
        m_Message = "Scalar range must contain finite values";
        return false;
    }
    if (m_ExtractionMode == CLOSEST_POINT_REGION &&
        (!std::isfinite(m_ClosestPoint[0]) || !std::isfinite(m_ClosestPoint[1]) || !std::isfinite(m_ClosestPoint[2]))) {
        m_Message = "Closest point must contain finite coordinates";
        return false;
    }
    // Validate point references before building adjacency or indexing scalar arrays.
    for (IGsize cell = 0; cell < nCells; ++cell) {
        const igIndex* ids = nullptr;
        const int n = m_Mesh->GetFaces()->GetCellIds(cell, ids);
        if (n <= 0) {
            m_Message = "Face contains no points";
            return false;
        }
        for (int j = 0; j < n; ++j) {
            if (ids[j] < 0 || static_cast<IGsize>(ids[j]) >= nPts) {
                m_Message = "Face contains an invalid point ID";
                return false;
            }
        }
    }
    if (m_ScalarConnectivity && !ResolveScalarArray()) { return false; }

    BuildPointFaceAdjacency();
    m_Visited.assign(nCells, -1);
    m_ExpandedPoints.assign(nPts, 0);
    m_CellEligible.resize(nCells);
    for (IGsize cell = 0; cell < nCells; ++cell) m_CellEligible[cell] = CellIsConnected(cell);

    igIndex largestRegionId = 0;
    IGsize maxCellsInRegion = 0;

    const bool seededMode = (m_ExtractionMode == POINT_SEEDED_REGIONS ||
                             m_ExtractionMode == CELL_SEEDED_REGIONS ||
                             m_ExtractionMode == CLOSEST_POINT_REGION);

    if (!seededMode) {
        int regionNumber = 0;
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            if (m_Visited[cellId] >= 0) { continue; }
            const IGsize marked =
                TraverseAndMark(std::vector<igIndex>{static_cast<igIndex>(cellId)}, regionNumber);
            m_RegionSizes.push_back(marked);
            if (marked > maxCellsInRegion) {
                maxCellsInRegion = marked;
                largestRegionId = regionNumber;
            }
            ++regionNumber;
            UpdateProgress(0.7 * static_cast<double>(cellId + 1) / nCells);
        }
    } else {
        std::vector<igIndex> seeds;
        if (m_ExtractionMode == POINT_SEEDED_REGIONS) {
            for (igIndex pt : m_Seeds) {
                if (pt < 0 || pt >= static_cast<igIndex>(nPts)) { continue; }
                for (igIndex face : m_PointFaces[pt]) { seeds.push_back(face); }
            }
        } else if (m_ExtractionMode == CELL_SEEDED_REGIONS) {
            for (igIndex cell : m_Seeds) {
                if (cell >= 0 && cell < static_cast<igIndex>(nCells)) { seeds.push_back(cell); }
            }
        } else { // CLOSEST_POINT_REGION
            igIndex minId = 0;
            double minDist = std::numeric_limits<double>::max();
            const double* c = m_ClosestPoint;
            for (IGsize p = 0; p < nPts; ++p) {
                const Point& q = m_Mesh->GetPoint(p);
                const double dx = q[0] - c[0], dy = q[1] - c[1], dz = q[2] - c[2];
                const double d = dx * dx + dy * dy + dz * dz;
                if (d < minDist) { minDist = d; minId = static_cast<igIndex>(p); }
            }
            for (igIndex face : m_PointFaces[minId]) { seeds.push_back(face); }
        }
        seeds.erase(std::remove_if(seeds.begin(), seeds.end(),
                                   [this](igIndex cell) { return !m_CellEligible[cell]; }), seeds.end());
        if (seeds.empty()) {
            m_Message = "No valid seed cells satisfy the extraction conditions";
            return false;
        }
        const IGsize marked = TraverseAndMark(seeds, 0);
        m_RegionSizes.push_back(marked);
        largestRegionId = 0;
    }

    if (m_RegionSizes.empty()) {
        m_Message = "No connected region found";
        return false;
    }

    // RegionId 重编号映射（按区域单元数排序）。
    std::vector<int> regionMap(m_RegionSizes.size());
    std::iota(regionMap.begin(), regionMap.end(), 0);
    if (m_RegionIdAssignmentMode == CELL_COUNT_ASCENDING ||
        m_RegionIdAssignmentMode == CELL_COUNT_DESCENDING) {
        std::stable_sort(regionMap.begin(), regionMap.end(), [&](int a, int b) {
            return m_RegionIdAssignmentMode == CELL_COUNT_ASCENDING
                       ? m_RegionSizes[a] < m_RegionSizes[b]
                       : m_RegionSizes[a] > m_RegionSizes[b];
        });
        std::vector<igIndex> reordered(m_RegionSizes.size());
        std::vector<int> oldToNew(m_RegionSizes.size());
        for (size_t newId = 0; newId < regionMap.size(); ++newId) {
            oldToNew[regionMap[newId]] = static_cast<int>(newId);
            reordered[newId] = m_RegionSizes[regionMap[newId]];
        }
        m_RegionSizes = reordered;
        regionMap = oldToNew;
    }

    // 决定保留哪些面。
    std::vector<char> keep(nCells, 0);
    if (m_ExtractionMode == ALL_REGIONS) {
        std::fill(keep.begin(), keep.end(), 1);
    } else if (m_ExtractionMode == LARGEST_REGION) {
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            if (m_Visited[cellId] == static_cast<int>(largestRegionId)) { keep[cellId] = 1; }
        }
    } else if (m_ExtractionMode == SPECIFIED_REGIONS) {
        for (int id : m_SpecifiedRegionIds) {
            if (id < 0 || static_cast<size_t>(id) >= regionMap.size()) {
                m_Message = "Specified region ID is outside the valid range";
                return false;
            }
        }
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            const int r = regionMap[m_Visited[cellId]];
            if (r >= 0 && std::find(m_SpecifiedRegionIds.begin(), m_SpecifiedRegionIds.end(), r) !=
                              m_SpecifiedRegionIds.end()) {
                keep[cellId] = 1;
            }
        }
    } else { // 种子类模式：保留全部被访问到的面
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            if (m_Visited[cellId] >= 0) { keep[cellId] = 1; }
        }
    }

    // 重建输出网格。
    auto output = SurfaceMesh::New();
    output->SetName(m_Mesh->GetName());
    auto outPoints = Points::New();
    auto outFaces = CellArray::New();

    std::vector<igIndex> oldToNewPoint(nPts, -1);
    std::vector<int> pointRegion; // 输出点 -> 区域号
    std::vector<int> cellRegion;  // 输出面 -> 区域号
    std::vector<igIndex> keptCells;

    std::vector<igIndex> newIds;
    const igIndex* ids = nullptr;
    for (IGsize cellId = 0; cellId < nCells; ++cellId) {
        if (!keep[cellId]) { continue; }
        const int n = m_Mesh->GetFaces()->GetCellIds(cellId, ids);
        if (n <= 0) { continue; }
        newIds.resize(n);
        for (int j = 0; j < n; ++j) {
            const igIndex old = ids[j];
            if (oldToNewPoint[old] < 0) {
                oldToNewPoint[old] = static_cast<igIndex>(outPoints->GetNumberOfPoints());
                outPoints->AddPoint(m_Mesh->GetPoint(old));
                pointRegion.push_back(m_Visited[cellId]);
            }
            newIds[j] = oldToNewPoint[old];
        }
        outFaces->AddCellIds(newIds.data(), static_cast<int>(n));
        cellRegion.push_back(m_Visited[cellId]);
        keptCells.push_back(cellId);
    }

    if (keptCells.empty()) {
        m_Message = "Extraction produced an empty mesh";
        return false;
    }
    output->SetPoints(outPoints);
    output->SetFaces(outFaces);

    // 搬运原有点/单元属性（保持原类型与数组长度）。
    auto inAttrs = m_Mesh->GetAttributeSet();
    auto outAttrs = AttributeSet::New();
    const IGsize outPointNum = outPoints->GetNumberOfPoints();
    const IGsize outCellNum = static_cast<IGsize>(keptCells.size());
    std::vector<igIndex> newToOld(outPointNum, -1);
    for (IGsize old = 0; old < nPts; ++old) {
        if (oldToNewPoint[old] >= 0) newToOld[oldToNewPoint[old]] = old;
    }
    if (inAttrs != nullptr) {
        auto all = inAttrs->GetAllAttributes();
        if (all) {
            const IGsize attrNum = all->GetNumberOfElements();
            for (IGsize a = 0; a < attrNum; ++a) {
                auto& attr = all->GetElement(a);
                if (attr.isDeleted || attr.pointer.IsNull()) { continue; }
                if (attr.attachmentType != IG_POINT && attr.attachmentType != IG_CELL) { continue; }

                // Replace old generated IDs instead of appending shadowed duplicates on repeat execution.
                if (m_ColorRegions && attr.pointer->GetName() == RegionIdName) continue;
                auto inArray = attr.pointer;
                auto outArray = CopySelectedAttribute(inArray, attr.attachmentType == IG_POINT ? newToOld : keptCells);
                if (!outArray) {
                    m_Message = "Unsupported or incomplete attribute array: " + inArray->GetName();
                    return false;
                }
                const IGsize index = outAttrs->AddAttribute(attr.type, attr.attachmentType, outArray);
                if (index != static_cast<IGsize>(-1)) {
                    outAttrs->GetAttribute(index).UpdateAllDataRange();
                }
            }
        }
    }

    // 写入 RegionId 标量（点 + 单元）。
    int regionIdAttrIndex = -1;
    if (m_ColorRegions) {
        auto pointRegionId = IntArray::New();
        pointRegionId->SetName(RegionIdName);
        pointRegionId->SetDimension(1);
        pointRegionId->Resize(outPointNum);
        for (IGsize t = 0; t < outPointNum; ++t) {
            pointRegionId->SetValue(t, regionMap[pointRegion[t]]);
        }
        regionIdAttrIndex = static_cast<int>(
            outAttrs->AddAttribute(IG_SCALAR, IG_POINT, pointRegionId));
        if (regionIdAttrIndex >= 0) {
            outAttrs->GetAttribute(regionIdAttrIndex).UpdateAllDataRange();
        }

        auto cellRegionId = IntArray::New();
        cellRegionId->SetName(RegionIdName);
        cellRegionId->SetDimension(1);
        cellRegionId->Resize(outCellNum);
        for (IGsize t = 0; t < outCellNum; ++t) {
            cellRegionId->SetValue(t, regionMap[cellRegion[t]]);
        }
        const IGsize cellIdx = outAttrs->AddAttribute(IG_SCALAR, IG_CELL, cellRegionId);
        if (cellIdx != static_cast<IGsize>(-1)) {
            outAttrs->GetAttribute(cellIdx).UpdateAllDataRange();
        }
    }

    output->SetAttributeSet(outAttrs);
    output->ConvertToDrawableData();

    UpdateProgress(1.0);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
