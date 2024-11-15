#include "iGameVolumeMeshClipper.h"
#include "iGameStructuredMesh.h"
#include "iGameThreadPool.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <array>
#include <cassert>
#include <unordered_map>

using namespace std;

constexpr int MAGIC1 = 1;
constexpr int MAGIC2 = 100;

IGAME_NAMESPACE_BEGIN

class FacesSet {
public:
    FacesSet() { m_Offset.push_back(0); }

    igIndex AddTriangle(const igIndex* ids) {
        IGsize offset = GetMinimalIndexOffset(ids, 3);
        igIndex idx = static_cast<igIndex>(m_Offset.size()) - 1;
        m_Offset.push_back(m_Offset.back() + 3);
        m_Ids.push_back(ids[offset]);
        m_Ids.push_back(ids[(offset + 1) % 3]);
        m_Ids.push_back(ids[(offset + 2) % 3]);
        if (GetIndex(&m_Ids[m_Offset[idx]], 3, idx)) {
            m_Offset.pop_back();
            m_Ids.resize(m_Offset.back());
        } else {
            map2Indices.emplace(ids[offset], idx);
        }
        return idx;
    }

    igIndex AddQuad(const igIndex* ids) {
        IGsize offset = GetMinimalIndexOffset(ids, 4);
        igIndex idx = static_cast<igIndex>(m_Offset.size()) - 1;
        m_Offset.push_back(m_Offset.back() + 4);
        m_Ids.push_back(ids[offset]);
        m_Ids.push_back(ids[(offset + 1) % 4]);
        m_Ids.push_back(ids[(offset + 2) % 4]);
        m_Ids.push_back(ids[(offset + 3) % 4]);
        if (GetIndex(&m_Ids[m_Offset[idx]], 4, idx)) {
            m_Offset.pop_back();
            m_Ids.resize(m_Offset.back());
        } else {
            map2Indices.emplace(ids[offset], idx);
        }
        return idx;
    }

    igIndex AddFace(const igIndex* ids, IGsize size) {
        IGsize offset = GetMinimalIndexOffset(ids, size);
        igIndex idx = static_cast<igIndex>(m_Offset.size()) - 1;
        m_Offset.push_back(m_Offset.back() + size);
        for (IGsize i = 0; i < size; ++i) {
            m_Ids.push_back(ids[(offset + i) % size]);
        }
        if (GetIndex(&m_Ids[m_Offset[idx]], size, idx)) {
            m_Offset.pop_back();
            m_Ids.resize(m_Offset.back());
        } else {
            map2Indices.emplace(ids[offset], idx);
        }
        return idx;
    }

    IGsize GetSize() const { return static_cast<IGsize>(m_Offset.size()) - 1; }


    IGsize GetFaceSize(igIndex fid) const {
        assert(fid + 1 < m_Offset.size() && "face index out of range");
        return m_Offset[fid + 1] - m_Offset[fid];
    }

    igIndex* GetFace(igIndex fid, IGsize& size) {
        assert(fid + 1 < m_Offset.size() && "face index out of range");
        size = m_Offset[fid + 1] - m_Offset[fid];
        return &m_Ids[m_Offset[fid]];
    }

    const igIndex* GetFace(igIndex fid, IGsize& size) const {
        assert(fid + 1 < m_Offset.size() && "face index out of range");
        size = m_Offset[fid + 1] - m_Offset[fid];
        return &m_Ids[m_Offset[fid]];
    }

private:
    IGsize GetMinimalIndexOffset(const igIndex* ids, IGsize size) {
        IGsize start = 0;
        igIndex minId = ids[0];
        for (IGsize i = 1; i < size; ++i) {
            if (ids[i] < minId) {
                minId = ids[i];
                start = i;
            }
        }
        return start;
    }

    bool IsEqual(const igIndex* f1, IGsize s1, const igIndex* f2,
                 IGsize s2) const {
        if (s1 != s2) { return false; }
        if (s1 == 3) {
            return (f1[1] == f2[1] && f1[2] == f2[2]) ||
                   (f1[1] == f2[2] && f1[2] == f2[1]);
        } else if (s1 == 4) {
            return f1[2] == f2[2] && ((f1[1] == f2[1] && f1[3] == f2[3]) ||
                                      (f1[1] == f2[3] && f1[3] == f2[1]));
        } else {
            bool res = true;
            for (IGsize i = 1; i < s1; ++i) {
                if (f1[i] != f2[i]) {
                    res = false;
                    break;
                }
            }
            if (res) return true;
            res = true;
            for (IGsize i = 1; i < s1; ++i) {
                if (f1[s1 - i] != f2[i]) {
                    res = false;
                    break;
                }
            }
            return res;
        }
    }

    // F1[0] must be the minimum
    bool GetIndex(const igIndex* f1, IGsize s1, igIndex& fid) const {
        auto range = map2Indices.equal_range(f1[0]);
        IGsize s2;
        for (auto it = range.first; it != range.second; ++it) {
            const igIndex* f2 = GetFace(it->second, s2);
            if (IsEqual(f1, s1, f2, s2)) {
                fid = it->second;
                return true;
            }
        }
        return false;
    }

private:
    vector<igIndex> m_Ids;
    vector<IGsize> m_Offset;
    unordered_multimap<igIndex, igIndex> map2Indices;
};


iGameVolumeMeshClipper::iGameVolumeMeshClipper() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(3);
    this->m_BoxClipper={};
    this->m_PlaneClipper={};
}

void iGameVolumeMeshClipper::InitializeResultMesh(Points::Pointer points) {
    m_OuterMeshVisible->SetPoints(points);
    m_OuterMeshVisible->SetAttributeSet(AttributeSet::New());
    m_OuterMeshVisible->SetName("visible_outer_mesh");

    m_InterMeshVisible->SetPoints(points);
    m_InterMeshVisible->SetAttributeSet(AttributeSet::New());
    m_InterMeshVisible->SetName("visible_inter_mesh");
    //m_InterMeshVisible->color

    m_OuterMeshInvisible->SetPoints(points);
    m_OuterMeshInvisible->SetAttributeSet(AttributeSet::New());
    m_OuterMeshInvisible->SetName("invisible_outer_mesh");
}

void iGameVolumeMeshClipper::SetExtent(double xMin, double xMax, double yMin,
                                       double yMax, double zMin, double zMax,
                                       bool flip) {
    m_BoxClipper.m_BoxMin[0] = xMin;
    m_BoxClipper.m_BoxMin[1] = yMin;
    m_BoxClipper.m_BoxMin[2] = zMin;
    m_BoxClipper.m_BoxMax[0] = xMax;
    m_BoxClipper.m_BoxMax[1] = yMax;
    m_BoxClipper.m_BoxMax[2] = zMax;
    m_BoxClipper.m_Flip = flip;
    SetExtentClipping(true);
}

void iGameVolumeMeshClipper::SetExtent(double boxMin[3], double boxMax[3],
                                       bool flip) {
    m_BoxClipper.m_BoxMin[0] = boxMin[0];
    m_BoxClipper.m_BoxMin[1] = boxMin[1];
    m_BoxClipper.m_BoxMin[2] = boxMin[2];
    m_BoxClipper.m_BoxMax[0] = boxMax[0];
    m_BoxClipper.m_BoxMax[1] = boxMax[1];
    m_BoxClipper.m_BoxMax[2] = boxMax[2];
    m_BoxClipper.m_Flip = flip;
    SetExtentClipping(true);
}

void iGameVolumeMeshClipper::SetPlane(double ox, double oy, double oz,
                                      double nx, double ny, double nz,
                                      bool flip) {
    m_PlaneClipper.m_Origin[0] = ox;
    m_PlaneClipper.m_Origin[1] = oy;
    m_PlaneClipper.m_Origin[2] = oz;
    m_PlaneClipper.m_Normal[0] = nx;
    m_PlaneClipper.m_Normal[1] = ny;
    m_PlaneClipper.m_Normal[2] = nz;
    m_PlaneClipper.m_Flip = flip;
    SetPlaneClipping(true);
}

void iGameVolumeMeshClipper::SetPlane(double orgin[3], double normal[3],
                                      bool flip) {
    m_PlaneClipper.m_Origin[0] = orgin[0];
    m_PlaneClipper.m_Origin[1] = orgin[1];
    m_PlaneClipper.m_Origin[2] = orgin[2];
    m_PlaneClipper.m_Normal[0] = normal[0];
    m_PlaneClipper.m_Normal[1] = normal[1];
    m_PlaneClipper.m_Normal[2] = normal[2];
    m_PlaneClipper.m_Flip = flip;
    SetPlaneClipping(true);
}

bool iGameVolumeMeshClipper::Execute() {
    m_OuterMeshVisible = SurfaceMesh::New();
    m_InterMeshVisible = SurfaceMesh::New();
    m_OuterMeshInvisible = SurfaceMesh::New();

    SetOutput(0, m_OuterMeshVisible);
    SetOutput(1, m_InterMeshVisible);
    SetOutput(2, m_OuterMeshInvisible);

    auto meshType = GetInput(0)->GetDataObjectType();
    switch (meshType) {
        case IG_VOLUME_MESH:
        case IG_STRUCTURED_MESH:
            return ExecuteWithVolumeMesh();
        case IG_UNSTRUCTURED_MESH:
            return ExecuteWithUnstructedMesh();
    }

    return false;
}

void BuildResult(FacesSet& facesSet,
                 unordered_map<igIndex, int>& numberOfAdjacency,
                 SurfaceMesh::Pointer outerMeshVisible,
                 SurfaceMesh::Pointer interMeshVisible,
                 SurfaceMesh::Pointer outerMeshInvisible) {
    CellArray::Pointer ov = CellArray::New();
    CellArray::Pointer iv = CellArray::New();
    CellArray::Pointer oiv = CellArray::New();

    IGsize fnb = facesSet.GetSize();
    igIndex* fvhs;
    IGsize fvnb;
    for (igIndex i = 0; i < fnb; ++i) {
        switch (numberOfAdjacency[i]) {
            case MAGIC1: {
                fvhs = facesSet.GetFace(i, fvnb);
                oiv->AddCellIds(fvhs, fvnb);
                break;
            }
            case MAGIC2 + MAGIC1: {
                fvhs = facesSet.GetFace(i, fvnb);
                ov->AddCellIds(fvhs, fvnb);
                break;
            }
            case MAGIC2 + 2 * MAGIC1: {
                fvhs = facesSet.GetFace(i, fvnb);
                iv->AddCellIds(fvhs, fvnb);
                break;
            }
            case 2 * MAGIC1:
            case 2 * MAGIC2 + 2 * MAGIC1:
                break;
            default:
                break;
        }
    }

    outerMeshVisible->SetFaces(ov);
    interMeshVisible->SetFaces(iv);
    outerMeshInvisible->SetFaces(oiv);
}


bool iGameVolumeMeshClipper::ExecuteWithVolumeMesh() {
    auto inputMesh = DynamicCast<VolumeMesh>(GetInput(0));
    auto cellsVisibility = ComputeCellsVisibility(inputMesh->GetPoints(),
                                                  inputMesh->GetVolumes());
    IGsize cellSize = inputMesh->GetNumberOfVolumes();
    auto cells = inputMesh->GetVolumes();

    FacesSet facesSet;
    unordered_map<igIndex, int> numberOfAdjacency;
    const igIndex* desc;
    igIndex cvhs[IGAME_CELL_MAX_SIZE];
    igIndex fvhs[IGAME_CELL_MAX_SIZE];
    for (igIndex i = 0; i < cellSize; ++i) {
        igIndex idx;
        int cvnb = cells->GetCellIds(i, cvhs);
        unsigned char visible =
                static_cast<unsigned char>(cellsVisibility->GetValue(i));
        switch (cvnb) {
            case Tetra::NumberOfPoints: {
                for (int j = 0; j < Tetra::NumberOfFaces; ++j) {
                    desc = Tetra::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    idx = facesSet.AddTriangle(fvhs);
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            case Pyramid::NumberOfPoints: {
                for (int j = 0; j < Pyramid::NumberOfFaces; ++j) {
                    desc = Pyramid::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    if (desc[3] == -1) {
                        idx = facesSet.AddTriangle(fvhs);
                    } else {
                        fvhs[3] = cvhs[desc[3]];
                        idx = facesSet.AddQuad(fvhs);
                    }
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            case Prism::NumberOfPoints: {
                for (int j = 0; j < Prism::NumberOfFaces; ++j) {
                    desc = Prism::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    if (desc[3] == -1) {
                        idx = facesSet.AddTriangle(fvhs);
                    } else {
                        fvhs[3] = cvhs[desc[3]];
                        idx = facesSet.AddQuad(fvhs);
                    }
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            case Hexahedron::NumberOfPoints: {
                for (int j = 0; j < Hexahedron::NumberOfFaces; ++j) {
                    desc = Hexahedron::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    fvhs[3] = cvhs[desc[3]];
                    idx = facesSet.AddQuad(fvhs);
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            default:
                break;
        }
    }

    InitializeResultMesh(inputMesh->GetPoints());
    BuildResult(facesSet, numberOfAdjacency, m_OuterMeshVisible,
                m_InterMeshVisible, m_OuterMeshInvisible);

    return true;
}

bool iGameVolumeMeshClipper::ExecuteWithUnstructedMesh() {
    auto inputMesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    auto cellsVisibility = ComputeCellsVisibility(inputMesh->GetPoints(),
                                                  inputMesh->GetCells());

    IGsize cellSize = inputMesh->GetNumberOfCells();
    auto cells = inputMesh->GetCells();
    auto cellsType = inputMesh->GetCellTypes();

    FacesSet facesSet;
    unordered_map<igIndex, int> numberOfAdjacency;
    const igIndex* desc;
    igIndex cvhs[IGAME_CELL_MAX_SIZE];
    igIndex fvhs[IGAME_CELL_MAX_SIZE];
    for (igIndex i = 0; i < cellSize; ++i) {
        igIndex idx;
        cells->GetCellIds(i, cvhs);
        unsigned char visible =
                static_cast<unsigned char>(cellsVisibility->GetValue(i));
        IGCellType type = static_cast<IGCellType>(cellsType->GetValue(i));
        switch (type) {
            case IG_TETRA: {
                for (int j = 0; j < Tetra::NumberOfFaces; ++j) {
                    desc = Tetra::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    idx = facesSet.AddTriangle(fvhs);
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            case IG_PYRAMID: {
                for (int j = 0; j < Pyramid::NumberOfFaces; ++j) {
                    desc = Pyramid::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    if (desc[3] == -1) {
                        idx = facesSet.AddTriangle(fvhs);
                    } else {
                        fvhs[3] = cvhs[desc[3]];
                        idx = facesSet.AddQuad(fvhs);
                    }
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            case IG_PRISM: {
                for (int j = 0; j < Prism::NumberOfFaces; ++j) {
                    desc = Prism::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    if (desc[3] == -1) {
                        idx = facesSet.AddTriangle(fvhs);
                    } else {
                        fvhs[3] = cvhs[desc[3]];
                        idx = facesSet.AddQuad(fvhs);
                    }
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            case IG_HEXAHEDRON: {
                for (int j = 0; j < Hexahedron::NumberOfFaces; ++j) {
                    desc = Hexahedron::faces[j];
                    fvhs[0] = cvhs[desc[0]];
                    fvhs[1] = cvhs[desc[1]];
                    fvhs[2] = cvhs[desc[2]];
                    fvhs[3] = cvhs[desc[3]];
                    idx = facesSet.AddQuad(fvhs);
                    numberOfAdjacency[idx] += MAGIC1;
                    if (visible) numberOfAdjacency[idx] += MAGIC2;
                }
                break;
            }
            default:
                break;
        }
    }

    InitializeResultMesh(inputMesh->GetPoints());
    BuildResult(facesSet, numberOfAdjacency, m_OuterMeshVisible,
                m_InterMeshVisible, m_OuterMeshInvisible);

    return true;
}

UnsignedCharArray::Pointer
iGameVolumeMeshClipper::ComputeCellsVisibility(Points::Pointer points,
                                               CellArray::Pointer cells) {
    UnsignedCharArray::Pointer pointsVisibility = UnsignedCharArray::New();
    UnsignedCharArray::Pointer cellsVisibility = UnsignedCharArray::New();
    IGsize pointSize = points->GetNumberOfPoints();
    IGsize cellSize = cells->GetNumberOfCells();
    pointsVisibility->Resize(pointSize);
    cellsVisibility->Resize(cellSize);

    if (!m_BoxClipper.m_Use && !m_PlaneClipper.m_Use) {
        auto func = [&](igIndex start, igIndex end) -> void {
            for (igIndex i = start; i < end; ++i) {
                cellsVisibility->SetValue(i, true);
            }
        };
        ThreadPool::parallelFor(0, cells->GetNumberOfCells(), func);
    } else {
        auto computePointsVisibility = [&](igIndex start, igIndex end) -> void {
            for (igIndex i = start; i < end; ++i) {
                const auto& position = points->GetPoint(i);
                unsigned char visibility = true;
                if (m_BoxClipper.m_Use) {
                    const auto& bmin = m_BoxClipper.m_BoxMin;
                    const auto& bmax = m_BoxClipper.m_BoxMax;
                    if (position[0] >= bmin[0] && position[0] <= bmax[0] &&
                        position[1] >= bmin[1] && position[1] <= bmax[1] &&
                        position[2] >= bmin[2] && position[2] <= bmax[2]) {
                        visibility = !m_BoxClipper.m_Flip;
                    } else {
                        visibility = m_BoxClipper.m_Flip;
                    }
                } else if (m_PlaneClipper.m_Use) {
                    const auto& o = m_PlaneClipper.m_Origin;
                    const auto& n = m_PlaneClipper.m_Normal;
                    if (((position[0] - o[0]) * n[0] +
                         (position[1] - o[1]) * n[1] +
                         (position[2] - o[2]) * n[2]) >= 0.) {
                        visibility = !m_PlaneClipper.m_Flip;
                    } else {
                        visibility = m_PlaneClipper.m_Flip;
                    }
                }
                pointsVisibility->SetValue(i, visibility);
            }
        };

        auto computeCellsVisibility = [&](igIndex start, igIndex end) -> void {
            igIndex ids[16];
            for (igIndex i = start; i < end; ++i) {
                int cvnb = cells->GetCellIds(i, ids);
                unsigned char visibility = true;
                for (igIndex j = 0; j < cvnb; ++j) {
                    unsigned char val = static_cast<unsigned char>(
                            pointsVisibility->GetValue(ids[j]));
                    if (!val) {
                        visibility = false;
                        break;
                    }
                }
                cellsVisibility->SetValue(i, visibility);
            }
        };


        ThreadPool::parallelFor(0, points->GetNumberOfPoints(),
                                computePointsVisibility);
        ThreadPool::parallelFor(0, cells->GetNumberOfCells(),
                                computeCellsVisibility);
    }

    return cellsVisibility;
}


IGAME_NAMESPACE_END