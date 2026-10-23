#include "iGameAppendDatasetsFilter.h"
#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"  // UnsignedIntArray (via iGameNewDataArrayMacro)
#include <cmath>
#include <cstdint>
#include <cstring>
#include <set>
#include <vector>

IGAME_NAMESPACE_BEGIN

AppendDatasetsFilter::AppendDatasetsFilter()
    : m_MergePoints(false)
    , m_Tolerance(0.0f)
{
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

AppendDatasetsFilter::~AppendDatasetsFilter() = default;

void AppendDatasetsFilter::AddInput(DataObject::Pointer data) {
    int n = this->GetNumberOfInputs();
    if (n == 1 && this->GetInput(0) == nullptr) {
        this->SetInput(0, data);
    } else {
        this->SetNumberOfInputs(n + 1);
        this->SetInput(n, data);
    }
}

void AppendDatasetsFilter::CollectMeshes(DataObject::Pointer obj,
                                        std::vector<UnstructuredMesh::Pointer>& meshes) {
    if (!obj) return;

    IGenum type = obj->GetDataObjectType();

    if (type == IG_SURFACE_MESH || type == IG_VOLUME_MESH || type == IG_UNSTRUCTURED_MESH) {
        // 直接归一为 UnstructuredMesh（保留原始 cell 类型）
        auto um = UnstructuredMesh::TransDataObjToUnstructuredMesh(obj);
        if (um) meshes.push_back(um);
    } else if (type == IG_DRAW_OBJECT) {
        auto drawObj = DynamicCast<DrawObject>(obj);
        if (drawObj) {
            auto renderable = drawObj->GetRenderableObject();
            if (renderable && renderable.GetPointer() != obj.GetPointer()) {
                auto um = DynamicCast<UnstructuredMesh>(renderable);
                if (um) {
                    meshes.push_back(um);
                } else {
                    CollectMeshes(renderable, meshes);
                }
            }
        }
        if (obj->HasSubDataObject()) {
            for (auto it = obj->SubDataObjectIteratorBegin();
                 it != obj->SubDataObjectIteratorEnd(); ++it) {
                CollectMeshes(it->second, meshes);
            }
        }
    } else if (type == IG_COMPOSITE_DATA_OBJECT) {
        if (obj->HasSubDataObject()) {
            for (auto it = obj->SubDataObjectIteratorBegin();
                 it != obj->SubDataObjectIteratorEnd(); ++it) {
                CollectMeshes(it->second, meshes);
            }
        }
    }
    // IG_STRUCTURED_MESH / IG_POINT_SET 等暂不支持（与 vtkAppendFilter
    // 仅处理 vtkDataSet 的范围一致）
}

void AppendDatasetsFilter::AppendMeshSimple(
    UnstructuredMesh::Pointer src,
    Points::Pointer outPoints,
    CellArray::Pointer outCells,
    UnsignedIntArray::Pointer outCellTypes,
    igIndex& pointOffset,
    std::vector<igIndex>& pointMap)
{
    if (!src) return;

    auto srcPoints = src->GetPoints();
    auto srcCells = src->GetCells();
    if (!srcPoints) return;

    IGsize numPoints = srcPoints->GetNumberOfPoints();
    pointMap.resize(numPoints);

    for (IGsize i = 0; i < numPoints; i++) {
        const Point& p = srcPoints->GetPoint(i);
        outPoints->AddPoint(p);
        pointMap[i] = pointOffset + (igIndex)i;
    }

    if (srcCells) {
        IGsize numCells = srcCells->GetNumberOfCells();
        for (IGsize c = 0; c < numCells; c++) {
            // 多面体等变长单元连接表可能超 IGAME_CELL_MAX_SIZE(256)，
            // 用动态缓冲避免越界（参照 CountCellVerticesFilter 的做法）
            const IGuint needed = srcCells->GetCellSize(c);
            std::vector<igIndex> ids(needed > 0 ? static_cast<size_t>(needed) : 1, 0);
            const int numIds = srcCells->GetCellIds(c, ids.data());

            std::vector<igIndex> adjustedIds(numIds);
            for (int i = 0; i < numIds; i++) {
                adjustedIds[i] = ids[i] + pointOffset;
            }
            outCells->AddCellIds(adjustedIds.data(), numIds);
            outCellTypes->AddValue(src->GetCellType(c));
        }
    }
    pointOffset += (igIndex)numPoints;
}

void AppendDatasetsFilter::AppendMeshWithMerge(
    UnstructuredMesh::Pointer src,
    Points::Pointer outPoints,
    CellArray::Pointer outCells,
    UnsignedIntArray::Pointer outCellTypes,
    std::unordered_map<int64_t, std::vector<igIndex>>& pointHash,
    std::vector<igIndex>& pointMap)
{
    if (!src) return;

    auto srcPoints = src->GetPoints();
    auto srcCells = src->GetCells();
    if (!srcPoints) return;

    IGsize numPoints = srcPoints->GetNumberOfPoints();
    pointMap.resize(numPoints);

    // ParaView 语义：Tolerance=0 时仅合并坐标完全一致的点（精确匹配），
    // 此时不能再用 1/tol 量化哈希（除零），改用坐标比特位做哈希键。
    const bool exactMerge = (m_Tolerance <= 0.0f);
    const float invTol = exactMerge ? 1.0f : 1.0f / m_Tolerance;
    const float tol2 = m_Tolerance * m_Tolerance;

    for (IGsize i = 0; i < numPoints; i++) {
        const Point& p = srcPoints->GetPoint(i);

        int64_t ix, iy, iz;
        if (exactMerge) {
            uint32_t bx, by, bz;
            std::memcpy(&bx, &p[0], sizeof(float));
            std::memcpy(&by, &p[1], sizeof(float));
            std::memcpy(&bz, &p[2], sizeof(float));
            ix = bx; iy = by; iz = bz;
        } else {
            ix = (int64_t)(p[0] * invTol);
            iy = (int64_t)(p[1] * invTol);
            iz = (int64_t)(p[2] * invTol);
        }
        int64_t key = (ix * 73856093LL) ^ (iy * 19349663LL) ^ (iz * 83492791LL);

        bool found = false;
        auto it = pointHash.find(key);
        if (it != pointHash.end()) {
            for (igIndex existingIdx : it->second) {
                const Point& existing = outPoints->GetPoint(existingIdx);
                float dx = p[0] - existing[0];
                float dy = p[1] - existing[1];
                float dz = p[2] - existing[2];
                const bool match = exactMerge
                        ? (dx == 0.0f && dy == 0.0f && dz == 0.0f)
                        : (dx * dx + dy * dy + dz * dz < tol2);
                if (match) {
                    pointMap[i] = existingIdx;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            igIndex newIdx = (igIndex)outPoints->AddPoint(p);
            pointHash[key].push_back(newIdx);
            pointMap[i] = newIdx;
        }
    }

    if (srcCells) {
        IGsize numCells = srcCells->GetNumberOfCells();
        for (IGsize c = 0; c < numCells; c++) {
            const IGuint needed = srcCells->GetCellSize(c);
            std::vector<igIndex> ids(needed > 0 ? static_cast<size_t>(needed) : 1, 0);
            const int numIds = srcCells->GetCellIds(c, ids.data());

            std::vector<igIndex> mappedIds(numIds);
            for (int i = 0; i < numIds; i++) {
                mappedIds[i] = pointMap[ids[i]];
            }
            outCells->AddCellIds(mappedIds.data(), numIds);
            outCellTypes->AddValue(src->GetCellType(c));
        }
    }
}

void AppendDatasetsFilter::MergeAttributes(
    const std::vector<UnstructuredMesh::Pointer>& meshes,
    const std::vector<AppendDatasetsFilter::MeshPointMap>& meshMaps,
    UnstructuredMesh::Pointer outMesh)
{
    if (meshes.empty() || meshMaps.empty()) return;

    auto outAttrSet = outMesh->GetAttributeSet();
    if (!outAttrSet) return;

    struct AttrKey {
        std::string name;
        IGenum type;
        IGenum attachmentType;
        bool operator<(const AttrKey& o) const {
            if (name != o.name) return name < o.name;
            if (type != o.type) return type < o.type;
            return attachmentType < o.attachmentType;
        }
    };

    std::map<AttrKey, int> attrCount;
    for (auto& mesh : meshes) {
        if (!mesh) continue;
        auto attrSet = mesh->GetAttributeSet();
        if (!attrSet) continue;
        std::set<AttrKey> seen;
        for (int i = 0; i < attrSet->GetNumberOfAttributes(); i++) {
            auto& attr = attrSet->GetAttribute(i);
            if (attr.IsNone()) continue;
            AttrKey key{attr.pointer->GetName(), attr.type, attr.attachmentType};
            seen.insert(key);
        }
        for (auto& k : seen) {
            attrCount[k]++;
        }
    }

    IGsize numOutPoints = outMesh->GetNumberOfPoints();
    IGsize numOutCells = outMesh->GetNumberOfCells();

    int mergedCount = 0;
    for (auto& [key, count] : attrCount) {
        // 仅保留所有输入都具备的属性（与 vtkAppendFilter 一致）
        if (count < (int)meshes.size()) continue;

        FloatArray::Pointer outArray = FloatArray::New();
        outArray->SetName(key.name);
        int dim = 1;

        std::vector<float> elemBuf(16, 0.0f);
        for (auto& mesh : meshes) {
            if (!mesh) continue;
            auto attrSet = mesh->GetAttributeSet();
            if (!attrSet) continue;
            auto& attr = attrSet->GetAttribute(key.name, key.type);
            if (attr.IsNone()) continue;
            dim = attr.pointer->GetDimension();
            break;
        }

        outArray->SetDimension(dim);
        IGsize numElements = (key.attachmentType == IG_POINT) ? numOutPoints : numOutCells;
        outArray->Resize(numElements);

        for (size_t m = 0; m < meshes.size(); m++) {
            if (!meshes[m]) continue;
            auto attrSet = meshes[m]->GetAttributeSet();
            if (!attrSet) continue;
            auto& attr = attrSet->GetAttribute(key.name, key.type);
            if (attr.IsNone()) continue;

            IGsize srcNum = attr.pointer->GetNumberOfElements();

            if (key.attachmentType == IG_POINT) {
                const auto& pmap = meshMaps[m].pointMap;
                for (IGsize i = 0; i < srcNum && i < pmap.size(); i++) {
                    igIndex outIdx = pmap[i];
                    if (outIdx < 0 || outIdx >= (igIndex)numElements) continue;
                    attr.pointer->GetElement(i, elemBuf.data());
                    outArray->SetElement(outIdx, elemBuf.data());
                }
            } else {
                igIndex cellOff = meshMaps[m].cellOffset;
                for (IGsize i = 0; i < srcNum; i++) {
                    igIndex outIdx = cellOff + (igIndex)i;
                    if (outIdx < 0 || outIdx >= (igIndex)numElements) continue;
                    attr.pointer->GetElement(i, elemBuf.data());
                    outArray->SetElement(outIdx, elemBuf.data());
                }
            }
        }

        outAttrSet->AddAttribute(key.type, key.attachmentType, outArray);
        mergedCount++;
    }
}

bool AppendDatasetsFilter::Execute() {
    std::vector<UnstructuredMesh::Pointer> meshes;
    int inputCount = this->GetNumberOfInputs();
    for (int i = 0; i < inputCount; i++) {
        auto input = this->GetInput(i);
        if (input) {
            this->CollectMeshes(input, meshes);
        }
    }

    if (meshes.empty()) {
        m_Message = "No appendable datasets on input (need surface/volume/unstructured mesh).";
        return false;
    }

    auto outMesh = UnstructuredMesh::New();
    auto outPoints = Points::New();
    auto outCells = CellArray::New();
    auto outCellTypes = UnsignedIntArray::New();

    // Single input: produce independent geometry and retain attribute types and
    // precision instead of converting every array to float during appending.
    if (inputCount == 1 && meshes.size() == 1) {
        const auto& source = meshes.front();
        outPoints->DeepCopy(source->GetPoints());
        if (source->GetCells()) { outCells->DeepCopy(source->GetCells()); }
        if (source->GetCellTypes()) { outCellTypes->DeepCopy(source->GetCellTypes()); }
        auto attributes = AttributeSet::New();
        if (!attributes->DeepCopy(source->GetAttributeSet())) {
            m_Message = "Unable to copy input attributes.";
            return false;
        }
        outMesh->SetPoints(outPoints);
        outMesh->SetCells(outCells, outCellTypes);
        outMesh->SetAttributeSet(attributes);
        outMesh->RequestEditStatus();
        outMesh->Modified();
        this->SetOutput(outMesh);
        return true;
    }

    std::vector<AppendDatasetsFilter::MeshPointMap> meshMaps(meshes.size());

    if (m_MergePoints) {
        std::unordered_map<int64_t, std::vector<igIndex>> pointHash;
        pointHash.reserve(1024);

        for (size_t i = 0; i < meshes.size(); i++) {
            AppendMeshWithMerge(meshes[i], outPoints, outCells, outCellTypes,
                                pointHash, meshMaps[i].pointMap);
        }
    } else {
        igIndex pointOffset = 0;
        for (size_t i = 0; i < meshes.size(); i++) {
            AppendMeshSimple(meshes[i], outPoints, outCells, outCellTypes,
                             pointOffset, meshMaps[i].pointMap);
        }
    }

    igIndex cellOffset = 0;
    for (size_t i = 0; i < meshes.size(); i++) {
        meshMaps[i].cellOffset = cellOffset;
        auto cells = meshes[i]->GetCells();
        if (cells) {
            cellOffset += (igIndex)cells->GetNumberOfCells();
        }
    }

    outMesh->SetPoints(outPoints);
    outMesh->SetCells(outCells, outCellTypes);
    outMesh->RequestEditStatus();
    outMesh->Modified();

    MergeAttributes(meshes, meshMaps, outMesh);

    // Force re-conversion of drawable data to ensure color mapping works
    // This matches the pattern used by CurvatureFilter, GradientFilter, etc.
    auto outAttrSet = outMesh->GetAttributeSet();
    if (outAttrSet && outAttrSet->GetNumberOfAttributes() > 0) {
        outAttrSet->ForceReConvertToDrawableData();
    }

    this->SetOutput(outMesh);
    return true;
}

IGAME_NAMESPACE_END
