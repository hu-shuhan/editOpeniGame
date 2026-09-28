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

inline int GetCellToPolyhedronPointIds(UnstructuredMesh::Pointer input, IGsize ci,igIndex *ids) {
    int count = -1;
    igIndex originIds[IGAME_CELL_MAX_SIZE]{};
    int numPoint = input->GetCellPointIds(ci, originIds); //get point ids and put them in ids
    auto cellType = input->GetCellType(ci);
    switch (cellType) {
        case (IG_HEXAHEDRON): {//六面体
            int p1 = 0;
            ids[p1++] = input->GetCell(ci)->GetNumberOfFaces();//六面体有六个四边形面
            for (int f = 0; f < 6; f++) {
                ids[p1++] = 4;
                for (int p = 0; p < 4; p++) ids[p1++] = originIds[Hexahedron::faces[f][p]];
            }
            count = p1;
            break;
        }
        case (IG_PYRAMID): {//金字塔
            int p1 = 0;
            ids[p1++] = input->GetCell(ci)->GetNumberOfFaces();//金字塔形有5个面，第一个是四边形底面，后四个是三角形侧面
            ids[p1++] = 4;
            for (int p = 0; p < 4; p++) ids[p1++] = originIds[Pyramid::faces[0][p]];
            for (int f = 1; f < 5; f++) {
                ids[p1++] = 3;
                for (int p = 0; p < 3; p++) ids[p1++] = originIds[Pyramid::faces[f][p]];
            }
            count = p1;
            break;
        }
        case (IG_PRISM): {//三棱柱
            int p1 = 0;
            ids[p1++] = input->GetCell(ci)->GetNumberOfFaces();//三棱柱有5个面，前两个是三角形面，后三个是四边形侧面
            for (int i = 0; i < 2; i++) {//处理前两个面
                ids[p1++] = 3;
                for (int p = 0; p < 3; p++) ids[p1++] = originIds[Prism::faces[i][p]];
            }
            for (int i = 2; i< 5; i++) {//处理后三个面
                ids[p1++] = 4;
                for (int p = 0; p < 4; p++) ids[p1++] = originIds[Prism::faces[i][p]];
            }
            count = p1;
            break;
        }
    }
    return count;
}

bool isConvexPolyhedron(Volume::Pointer input,std::string& reason) {
    if (!input || input->GetNumberOfFaces() < 4 || input->GetNumberOfPoints() < 4) { return false; }
    int numFaces = input->GetNumberOfFaces();
    int numPoints = input->GetNumberOfPoints();
    std::vector<Point> cellPoints{}; //收集这个单元的所有点
    for (int i = 0; i < numPoints; i++) {
        cellPoints.push_back(input->GetPoint(i));
    }
    for (int i = 0; i < numFaces; i++) {
        auto face = dynamic_cast<Face*>(input->GetFace(i));
        if (!face || face->GetNumberOfPoints() < 3) {
            reason = "单元存在点数小于3的面。";
            return false;
        }
        Vector3d normal(0.0, 0.0, 0.0);//计算法线
        const int count = face->GetNumberOfPoints();
        const Vector3d origin = ToVector3d(face->GetPoint(0));

        for (int j = 1; j + 1 < count; ++j) {
            Vector3d a = ToVector3d(face->GetPoint(j)) - origin;
            Vector3d b = ToVector3d(face->GetPoint(j + 1)) - origin;
            normal += a.cross(b);
        }
        auto length = normal.length();
        if (!std::isfinite(length) || length == 0.0) {
            reason = "单元存在退化面或无效法线，无法确认凸性。";
            return false;
        }
        normal /= length;
        const double eps = 1e-6;
        bool flag_p0;
        bool firstPoint = true;
        auto p0 = face->GetPoint(0);
        for (auto p: cellPoints) {//看看是否所有点都在面的同一边
            auto result = normal.dot(p - p0);
            if (!std::isfinite(result)) {
                reason = "单元存在异常法线";
                return false;
            }
            if (result > -eps && result < eps) continue;//几乎就在面上，就认为点在面上
            bool flag = result > 0;
            if (firstPoint) {
                flag_p0 = flag;
                firstPoint = false;
            } else {
                if (flag != flag_p0) {
                    reason = "单元未通过凸性检查：顶点分布在某个面的两侧。";
                    return false;//不是凸多面体
                }
            }
        }
        if (firstPoint) return false;//整个多面体的所有点都在面上
    }
    return true;//所有面都通过，才是凸多面体
}

inline bool IsTetLikePolyhedron(
        UnstructuredMesh::Pointer input,IGsize ci,std::vector<std::set<int>> &points) { //判断某个Cell是否是四面体形状的Polyhedron，如果是就把原先的Cell直接当成四面体
    auto cell = input->GetCell(ci);
    auto numFaces = cell->GetNumberOfFaces();
    auto numPoints = cell->GetNumberOfPoints();
    if (numFaces != 4) return false;
    std::set<int> cellPoints;
    for (int i = 0; i < numPoints; i++) {
        cellPoints.insert(cell->GetPointId(i));
    }
    if (cellPoints.size() == 4) {
        points.push_back(cellPoints);
        return true;
    }
    else
        return false;
}

struct NewPointSource {
    igIndex outPointId{-1};
    std::vector<igIndex> srcPointIds;
};

template<typename ArrayType>
ArrayObject::Pointer CopyAttribute(AttributeSet::Attribute& attr, const ArrayObject::Pointer& inArray,
                                   AttributeSet::Pointer outData, igIndex inPointNum, igIndex outPointNum,
                                   igIndex outCellNum, const std::vector<NewPointSource>& newPointSources,
                                   const std::vector<igIndex>& originCells, double values[], double tmp[]) {
    auto outArray = ArrayType::New();
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

        for (const auto& np: newPointSources) {
            if (np.outPointId < 0 || np.outPointId >= outPointNum) continue;
            const igIndex cnt = static_cast<igIndex>(np.srcPointIds.size());
            if (cnt <= 0) continue;

            for (int k = 0; k < dim; ++k) { values[k] = 0.0; }
            igIndex usedCount = 0;
            for (igIndex s = 0; s < cnt; ++s) {
                const igIndex srcId = np.srcPointIds[static_cast<size_t>(s)];
                if (srcId < 0 || srcId >= copyPointNum) continue;
                inArray->GetElement(srcId, tmp);
                for (int k = 0; k < dim; ++k) { values[k] += tmp[k]; }
                ++usedCount;
            }
            if (usedCount <= 0) continue;
            const double inv = 1.0 / static_cast<double>(usedCount);
            for (int k = 0; k < dim; ++k) { values[k] *= inv; }
            outArray->SetElement(np.outPointId, values);
        }

        outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
    } else if (attr.attachmentType == IG_CELL) {
        outArray->Resize(outCellNum);
        const igIndex copyCellNum = std::min<igIndex>(outCellNum, static_cast<igIndex>(originCells.size()));
        for (igIndex cid = 0; cid < copyCellNum; ++cid) {
            const igIndex srcCell = originCells[static_cast<size_t>(cid)];
            inArray->GetElement(srcCell, values);
            outArray->SetElement(cid, values);
        }
        outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
    } else {
        outData->AddAttribute(attr.type, attr.attachmentType, inArray, attr.GetDataRange());
    }
    return outArray;
}

} // namespace

bool MeshTetrahedralize::Execute()
{
    m_failReason.clear();
    auto obj = GetInput(0);
    if (!obj) return false;

    UnstructuredMesh::Pointer input = UnstructuredMesh::New();
    if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        input = DynamicCast<UnstructuredMesh>(obj);
    } else if (obj->GetDataObjectType() == IG_VOLUME_MESH) {
        auto vinput = DynamicCast<VolumeMesh>(obj);
        input->UnstructuredMesh::GenerateFromVolumeMesh(vinput);
    }
    if (!input) return false;

    const IGsize nCells = input->GetNumberOfCells();
    igIndex ids[IGAME_CELL_MAX_SIZE]{}, faceIds[IGAME_CELL_MAX_SIZE]{};
    auto faceTable = FaceTable::New();
    auto volumeFaces = CellArray::New();
    std::vector<igIndex> polyCellIds;
    polyCellIds.reserve(static_cast<size_t>(nCells));
    std::vector<igIndex> passthroughTetOriginCells;//记录被跳过的四面体的cell id
    std::vector<igIndex> passthroughTetLikePolys;//记录被跳过的、实际上是四面体的多面体的id
    std::vector<std::set<int>> tetLikePolysPoints;//记录被跳过的、实际上是四面体的多面体的点
    for (IGsize ci = 0; ci < nCells; ++ci) { //traverse evey cell
        if (input->GetCellType(ci) == IG_TETRA) {  //如果本来就是四面体，就不需要四面体化了
            passthroughTetOriginCells.push_back(ci);
            continue;
        }
        int size;
        if (input->GetCellType(ci) == IG_POLYHEDRON) {
            if (IsTetLikePolyhedron(input, ci,tetLikePolysPoints)) {
                passthroughTetLikePolys.push_back(ci);//实际上就是四面体，不需要四面体化
                continue;
            } else size = input->GetCellPointIds(ci, ids);
        } else {
            size = GetCellToPolyhedronPointIds(input, ci, ids);
            if (size == -1) {
                return false;//不支持的类型
            }
        }

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

    for (size_t i = 0; i < passthroughTetOriginCells.size(); i++) {//加入:不用处理的四面体
        igIndex tetIds[IGAME_CELL_MAX_SIZE]{};
        igIndex cellId = passthroughTetOriginCells[i];
        auto volume = dynamic_cast<Volume*>(input->GetCell(cellId));
        std::string reason = "";
        if (!volume || !isConvexPolyhedron(volume,reason)) {
            m_failReason = reason;
            return false;
        }
        const int count = input->GetCellPointIds(cellId, tetIds);
        if (count != 4) return false;
        outCells->AddCellId4(tetIds[0], tetIds[1], tetIds[2], tetIds[3]);
        originCells.push_back(cellId);
    }

    for (size_t i = 0; i < passthroughTetLikePolys.size(); i++) {//加入:四面体形状的多面体
        igIndex tetIds[IGAME_CELL_MAX_SIZE]{};
        igIndex cellId = passthroughTetLikePolys[i];
        auto volume = dynamic_cast<Volume*>(input->GetCell(cellId));
        std::string reason = "";
        if (!volume || !isConvexPolyhedron(volume, reason)) {
            m_failReason = reason;
            return false;
        }
        std::set<int> cellPoints = tetLikePolysPoints[i];
        int points[4]{};
        int p = 0;
        for (int v: cellPoints) points[p++] = v;
        outCells->AddCellId4(points[0], points[1], points[2], points[3]);
        originCells.push_back(cellId);
    }

    int nFaces = faces->GetNumberOfCells();
    for (IGsize fi = 0; fi < nFaces; ++fi) {
        igIndex faceVerts[IGAME_CELL_MAX_SIZE]{};
        const int nFaceVerts = faces->GetCellIds(fi, faceVerts);
        if (nFaceVerts < 3) {
            triFaceNums.push_back(0);
            continue;
        }

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
    for (IGsize vi = 0; vi < nPolyCells; ++vi) {// 遍历每个多面体，准备拆成四面体。
        const igIndex srcCellId = polyCellIds[static_cast<size_t>(vi)];
        igIndex cellVerts[IGAME_CELL_MAX_SIZE]{};
        const int nCellVerts = mesh->GetVolumePointIds(vi, cellVerts);
        if (nCellVerts < 4) {
            continue;
        }
        auto volume = mesh->GetVolume(vi);
        std::string reason = "";
        bool isConvex = isConvexPolyhedron(volume,reason);
        if (!isConvex) {
            m_failReason = reason;
            return false;
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
                auto &attr = inAllAttr->GetElement(ai);
                auto inArray = attr.pointer;
                if (!inArray) continue;
                switch (inArray->GetArrayType()) {
                    case IG_FloatArray: {
                        CopyAttribute<FloatArray>(attr,inArray,outData,inPointNum,outPointNum,outCellNum,newPointSources,originCells,values,tmp);
                        break;
                    }
                    case IG_IntArray: {
                        CopyAttribute<IntArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_DoubleArray: {
                        CopyAttribute<DoubleArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                   newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_UnsignedIntArray: {
                        CopyAttribute<UnsignedIntArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                        newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_CharArray: {
                        CopyAttribute<CharArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                 newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_UnsignedCharArray: {
                        CopyAttribute<UnsignedCharArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                         newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_ShortArray: {
                        CopyAttribute<ShortArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                  newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_UnsignedShortArray: {
                        CopyAttribute<UnsignedShortArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                          newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_LongLongArray: {
                        CopyAttribute<LongLongArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum,
                                                     newPointSources, originCells, values, tmp);
                        break;
                    }
                    case IG_UnsignedLongLongArray: {
                        CopyAttribute<UnsignedLongLongArray>(attr, inArray, outData, inPointNum, outPointNum, outCellNum, newPointSources, originCells, values, tmp);
                        break;
                    }

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


