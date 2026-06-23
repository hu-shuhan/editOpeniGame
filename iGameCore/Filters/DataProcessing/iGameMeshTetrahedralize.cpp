#include "iGameMeshTetrahedralize.h"
#include "iGameFaceTable.h"
#include "iGameFlatArray.h"
#include <Eigen/Core>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <unordered_set>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

inline Vector3d ToVector3d(const Point& p) {
    return Vector3d(static_cast<double>(p[0]), static_cast<double>(p[1]), static_cast<double>(p[2]));
}

inline bool FaceHasNearCollinearVertex(UnstructuredMesh* mesh, const std::vector<igIndex>& faceVerts) {
    if (mesh == nullptr) return false;
    const igIndex n = static_cast<igIndex>(faceVerts.size());
    if (n < 3) return false;

    static constexpr double kPi = 3.1415926535897932384626433832795;
    static constexpr double kAngleThresholdDeg = 175.0;
    double kCosThreshold = std::cos(kAngleThresholdDeg * (kPi / 180.0));
    static constexpr double kEps = 1e-20;

    for (igIndex i = 0; i < n; ++i) {
        const igIndex iPrev = (i + n - 1) % n;
        const igIndex iNext = (i + 1) % n;

        const Point& pPrev = mesh->GetPoint(faceVerts[iPrev]);
        const Point& pCur = mesh->GetPoint(faceVerts[i]);
        const Point& pNext = mesh->GetPoint(faceVerts[iNext]);

        const Vector3d u = ToVector3d(pPrev) - ToVector3d(pCur);
        const Vector3d v = ToVector3d(pNext) - ToVector3d(pCur);

        const double nu = u.squaredNorm();
        const double nv = v.squaredNorm();
        if (nu <= kEps || nv <= kEps) continue;

        double c = u.dot(v) / std::sqrt(nu * nv);
        c = std::clamp(c, -1.0, 1.0);
        if (c <= kCosThreshold) {
            return true;
        }
    }
    return false;
}

inline Vector3d ComputeFaceCentroid(UnstructuredMesh* mesh, const std::vector<igIndex>& faceVerts) {
    Vector3d c(0.0, 0.0, 0.0);
    if (mesh == nullptr || faceVerts.empty()) return c;

    for (igIndex vid : faceVerts) {
        c += ToVector3d(mesh->GetPoint(vid));
    }
    c /= static_cast<double>(faceVerts.size());
    return c;
}

inline bool AddTetra(CellArray::Pointer outCells, UnsignedIntArray::Pointer outTypes, igIndex a, igIndex b,
                    igIndex c, igIndex d) {
    if (outCells == nullptr || outTypes == nullptr) return false;
    if (a == b || a == c || a == d || b == c || b == d || c == d) return false;

    igIndex t[4]{a, b, c, d};
    outCells->AddCellIds(t, 4);
    outTypes->AddValue(IG_TETRA);
    return true;
}

struct NewPointSource {
    igIndex outPointId{-1};
    std::vector<igIndex> srcPointIds;
};

} // namespace

bool MeshTetrahedralize::Execute() 
{ 
    auto obj = GetInput(0);
    if (!obj) return false;

    UnstructuredMesh::Pointer input;
    if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) { 
        input = DynamicCast<UnstructuredMesh>(obj);
    }
    if (!input) return false;

    const IGsize nCells = input->GetNumberOfCells();
    igIndex ids[IGAME_CELL_MAX_SIZE]{}, faceIds[IGAME_CELL_MAX_SIZE]{};
    auto faceTable = FaceTable::New();
    auto volumeFaces = CellArray::New();
    std::vector<igIndex> polyCellIds;
    polyCellIds.reserve(static_cast<size_t>(nCells));

    std::cout << 111111 << std::endl;

    for (IGsize ci = 0; ci < nCells; ++ci) { 
        if (input->GetCellType(ci) != IG_POLYHEDRON) {
            continue;
        }

        const int size = input->GetCellPointIds(ci, ids);
        igIndex cursor = 0, num = 0;
        igIndex numFaces = ids[cursor++];
        while (numFaces--) {
            int id_num = ids[cursor++];
            igIndex id = faceTable->IsFace(ids + cursor, id_num);
            if (id == -1) { 
                id = faceTable->GetNumberOfFaces();
                faceTable->InsertFace(ids + cursor, id_num);
            } 
            faceIds[num++] = id;
            cursor += id_num;
        }
        volumeFaces->AddCellIds(faceIds, num);
        polyCellIds.push_back(static_cast<igIndex>(ci));
    }
    auto faces = faceTable->GetOutput();
    auto mesh = VolumeMesh::New();
    mesh->SetPoints(input->GetPoints());
    mesh->InitVolumesWithPolyhedron(faces, volumeFaces);
    mesh->InitPolyhedronVertices();

    

    auto out = VolumeMesh::New();
    out->SetName(input->GetName());

    auto outPoints = Points::New();
    outPoints->DeepCopy(input->GetPoints());
    out->SetPoints(outPoints);

    auto outCells = CellArray::New();

    auto triFaces = CellArray::New();
    std::vector<int> triFaceNums;
    triFaceNums.reserve(faces->GetNumberOfCells());

    std::vector<NewPointSource> newPointSources;
    std::vector<igIndex> originCells;

    int nFaces = faces->GetNumberOfCells();
    for (IGsize fi = 0; fi < nFaces; ++fi) {
        igIndex faceVerts[IGAME_CELL_MAX_SIZE]{};
        const int nFaceVerts = faces->GetCellIds(fi, faceVerts);
        if (nFaceVerts < 3) {
            triFaceNums.push_back(0);
            continue;
        }

        // 补全这里的逻辑，遍历每一个face，检查是否存在近似共线的顶点，
        // 如果存在则为face添加一个重心再三角化，否则直接进行三角化。
        // 把三角化的face添加到triFaces中，并记录每个face对应的三角形数量到triFaceNums中

        std::vector<igIndex> fv;
        fv.reserve(static_cast<size_t>(nFaceVerts));
        for (int i = 0; i < nFaceVerts; ++i) {
            fv.push_back(faceVerts[i]);
        }

        const bool needFaceCenter = FaceHasNearCollinearVertex(input.get(), fv);
        if (needFaceCenter) {
            const Vector3d fc = ComputeFaceCentroid(input.get(), fv);
            const igIndex fcId = static_cast<igIndex>(outPoints->AddPoint(static_cast<float>(fc[0]),
                                                                          static_cast<float>(fc[1]),
                                                                          static_cast<float>(fc[2])));
            newPointSources.push_back(NewPointSource{fcId, fv});
            for (int i = 0; i < nFaceVerts; ++i) {
                const igIndex a = fcId;
                const igIndex b = faceVerts[i];
                const igIndex c = faceVerts[(i + 1) % nFaceVerts];
                triFaces->AddCellId3(a, b, c);
            }
            triFaceNums.push_back(nFaceVerts);
        } else {
            const igIndex v0 = faceVerts[0];
            for (int i = 1; i < nFaceVerts - 1; ++i) {
                triFaces->AddCellId3(v0, faceVerts[i], faceVerts[i + 1]);
            }
            triFaceNums.push_back(std::max(0, nFaceVerts - 2));
        }
    }

    std::vector<IGsize> triFaceOffsets(static_cast<size_t>(nFaces) + 1, 0);
    for (int f = 0; f < nFaces; ++f) {
        triFaceOffsets[static_cast<size_t>(f) + 1] =
            triFaceOffsets[static_cast<size_t>(f)] + static_cast<IGsize>(triFaceNums[static_cast<size_t>(f)]);
    }

    const IGsize nPolyCells = static_cast<IGsize>(polyCellIds.size());
    for (IGsize vi = 0; vi < nPolyCells; ++vi) {
        const igIndex srcCellId = polyCellIds[static_cast<size_t>(vi)];
        igIndex cellVerts[IGAME_CELL_MAX_SIZE]{};
        const int nCellVerts = mesh->GetVolumePointIds(vi, cellVerts);
        if (nCellVerts < 4) {
            continue;
        }

        Vector3d cc(0.0, 0.0, 0.0);
        for (int i = 0; i < nCellVerts; ++i) {
            cc += ToVector3d(input->GetPoint(cellVerts[i]));
        }
        cc /= static_cast<double>(nCellVerts);
        const igIndex centerId = static_cast<igIndex>(outPoints->AddPoint(static_cast<float>(cc[0]),
                                                                          static_cast<float>(cc[1]),
                                                                          static_cast<float>(cc[2])));
        {
            std::vector<igIndex> src;
            src.reserve(static_cast<size_t>(nCellVerts));
            for (int i = 0; i < nCellVerts; ++i) {
                src.push_back(cellVerts[i]);
            }
            newPointSources.push_back(NewPointSource{centerId, std::move(src)});
        }

        igIndex cellFaceIds[IGAME_CELL_MAX_SIZE]{};
        const int nCellFaces = mesh->GetVolumeFaceIds(vi, cellFaceIds);
        igIndex tri[3]{};
        for (int f = 0; f < nCellFaces; ++f) {
            const igIndex faceId = cellFaceIds[f];
            if (faceId < 0 || faceId >= nFaces) continue;
            const IGsize begin = triFaceOffsets[static_cast<size_t>(faceId)];
            const IGsize cnt = static_cast<IGsize>(triFaceNums[static_cast<size_t>(faceId)]);
            for (IGsize t = 0; t < cnt; ++t) {
                triFaces->GetCellIds(begin + t, tri);
                if (tri[0] == tri[1] || tri[0] == tri[2] || tri[0] == centerId || tri[1] == tri[2] ||
                    tri[1] == centerId || tri[2] == centerId) {
                    continue;
                }
                outCells->AddCellId4(tri[0], tri[1], tri[2], centerId);
                originCells.push_back(srcCellId);
            }
        }
    }

    {
        AttributeSet::Pointer inData = input->GetAttributeSet();
        AttributeSet::Pointer outData = AttributeSet::New();
        out->SetAttributeSet(outData);

        if (inData) {
            auto inAllAttr = inData->GetAllAttributes();
            const igIndex inPointNum = static_cast<igIndex>(input->GetNumberOfPoints());
            const igIndex outPointNum = static_cast<igIndex>(outPoints->GetNumberOfPoints());
            const igIndex outCellNum = static_cast<igIndex>(outCells->GetNumberOfCells());

            double values[IGAME_CELL_MAX_SIZE]{};
            double tmp[IGAME_CELL_MAX_SIZE]{};

            for (IGsize ai = 0; ai < inAllAttr->GetNumberOfElements(); ++ai) {
                auto attr = inAllAttr->GetElement(ai);
                auto inArray = attr.pointer;
                if (!inArray) continue;

                auto outArray = FloatArray::New();
                outArray->SetName(inArray->GetName());
                outArray->SetDimension(inArray->GetDimension());
                const int dim = inArray->GetDimension();

                if (attr.attachmentType == IG_POINT) {
                    outArray->Resize(outPointNum);

                    const igIndex copyPointNum =
                        std::min<igIndex>(inPointNum, static_cast<igIndex>(inArray->GetNumberOfElements()));
                    for (igIndex pid = 0; pid < copyPointNum; ++pid) {
                        inArray->GetElement(pid, values);
                        outArray->SetElement(pid, values);
                    }

                    for (const auto& np : newPointSources) {
                        if (np.outPointId < 0 || np.outPointId >= outPointNum) continue;
                        const igIndex cnt = static_cast<igIndex>(np.srcPointIds.size());
                        if (cnt <= 0) continue;

                        for (int k = 0; k < dim; ++k) {
                            values[k] = 0.0;
                        }
                        igIndex usedCount = 0;
                        for (igIndex s = 0; s < cnt; ++s) {
                            const igIndex srcId = np.srcPointIds[static_cast<size_t>(s)];
                            if (srcId < 0 || srcId >= copyPointNum) continue;
                            inArray->GetElement(srcId, tmp);
                            for (int k = 0; k < dim; ++k) {
                                values[k] += tmp[k];
                            }
                            ++usedCount;
                        }
                        if (usedCount <= 0) continue;
                        const double inv = 1.0 / static_cast<double>(usedCount);
                        for (int k = 0; k < dim; ++k) {
                            values[k] *= inv;
                        }
                        outArray->SetElement(np.outPointId, values);
                    }

                    outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
                } else if (attr.attachmentType == IG_CELL) {
                    outArray->Resize(outCellNum);
                    const igIndex copyCellNum =
                        std::min<igIndex>(outCellNum, static_cast<igIndex>(originCells.size()));
                    for (igIndex cid = 0; cid < copyCellNum; ++cid) {
                        const igIndex srcCell = originCells[static_cast<size_t>(cid)];
                        inArray->GetElement(srcCell, values);
                        outArray->SetElement(cid, values);
                    }
                    outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
                } else {
                    outData->AddAttribute(attr.type, attr.attachmentType, inArray, attr.GetDataRange());
                }
            }
        }
    }

    out->SetVolumes(outCells);
    SetOutput(out);
    return outCells->GetNumberOfCells() > 0;
}

MeshTetrahedralize::MeshTetrahedralize() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END


