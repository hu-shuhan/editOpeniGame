#include "iGameSurfaceVectorsFilter.h"
#include "iGameUnstructuredMesh.h"
#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <queue>

IGAME_NAMESPACE_BEGIN

SurfaceVectorsFilter::SurfaceVectorsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

std::vector<char> SurfaceVectorsFilter::ComputeConsistencyFlips(SurfaceMesh::Pointer mesh) {
    auto faces = mesh->GetFaces();
    IGsize numFaces = faces->GetNumberOfCells();
    std::vector<char> flips(numFaces, -1); // -1 = 未访问

    // 边表：无向边 (min,max) -> [(faceId, 边在面环绕中方向是否与 (min,max) 一致)]
    std::map<std::pair<igIndex, igIndex>, std::vector<std::pair<IGsize, bool>>> edgeMap;
    for (IGsize f = 0; f < numFaces; f++) {
        const igIndex* ids = nullptr;
        int numIds = faces->GetCellIds(f, ids);
        for (int i = 0; i < numIds; i++) {
            igIndex a = ids[i], b = ids[(i + 1) % numIds];
            auto key = std::minmax(a, b);
            edgeMap[key].emplace_back(f, a == key.first);
        }
    }

    // BFS：共享边方向相同 -> 邻面环绕不一致（翻转标记取反）
    std::queue<IGsize> queue;
    for (IGsize seed = 0; seed < numFaces; seed++) {
        if (flips[seed] != -1) continue;
        flips[seed] = 0;
        queue.push(seed);
        while (!queue.empty()) {
            IGsize f = queue.front();
            queue.pop();
            const igIndex* ids = nullptr;
            int numIds = faces->GetCellIds(f, ids);
            for (int i = 0; i < numIds; i++) {
                igIndex a = ids[i], b = ids[(i + 1) % numIds];
                auto key = std::minmax(a, b);
                bool dirSameF = (a == key.first);
                for (const auto& [g, dirSameG] : edgeMap[key]) {
                    if (g == f || flips[g] != -1) continue;
                    flips[g] = (dirSameF == dirSameG) ? (flips[f] ^ 1) : flips[f];
                    queue.push(g);
                }
            }
        }
    }
    return flips;
}

std::array<double, 3> SurfaceVectorsFilter::ComputeFaceNormal(SurfaceMesh::Pointer mesh, igIndex faceId) {
    std::array<double, 3> normal = {0.0, 0.0, 0.0};
    if (!mesh) return normal;

    auto points = mesh->GetPoints();
    auto faces = mesh->GetFaces();
    if (!points || !faces) return normal;

    const igIndex* ids = nullptr;
    int numIds = faces->GetCellIds(faceId, ids);
    if (numIds < 3) return normal;

    // 与 VTK vtkSurfaceVectors 一致（经 pvpython 单 quad 受控实验逐位验证）：
    // 任意多边形 cell 的法向 = 前 3 顶点 (0,1,2) 构成的三角形叉积，
    // 不做 Newell、不做完整扇形拆分。非平面 quad 上两者差异可达 1e-4 量级
    double e1x = 0, e1y = 0, e1z = 0, e2x = 0, e2y = 0, e2z = 0;
    {
        Point p0 = points->GetPoint(ids[0]);
        Point p1 = points->GetPoint(ids[1]);
        Point p2 = points->GetPoint(ids[2]);
        e1x = (double)p1[0] - p0[0]; e1y = (double)p1[1] - p0[1]; e1z = (double)p1[2] - p0[2];
        e2x = (double)p2[0] - p0[0]; e2y = (double)p2[1] - p0[1]; e2z = (double)p2[2] - p0[2];
    }
    normal[0] = e1y * e2z - e1z * e2y;
    normal[1] = e1z * e2x - e1x * e2z;
    normal[2] = e1x * e2y - e1y * e2x;
    return normal; // 未归一化，长度 = 2*三角形面积（面积加权语义）
}

bool SurfaceVectorsFilter::ComputePointNormals(
    SurfaceMesh::Pointer mesh,
    std::vector<std::array<double, 3>>& outNormals) {
    if (!mesh) return false;

    auto points = mesh->GetPoints();
    auto faces = mesh->GetFaces();
    if (!points || !faces) return false;

    IGsize numPoints = points->GetNumberOfPoints();
    IGsize numFaces = faces->GetNumberOfCells();

    outNormals.assign(numPoints, {0.0, 0.0, 0.0});

    // 与 VTK vtkSurfaceVectors 一致（经 pvpython 实测逐位验证）：
    // 先做环绕一致性修正（Consistency），再累加【未归一化】面法向叉积
    // （面积加权），最后统一归一化。
    // 注意：vtkPolyDataNormals 是等权单位法向累加，与 vtkSurfaceVectors 不同，
    // 不能作为本 filter 的真值来源
    const std::vector<char> flips = ComputeConsistencyFlips(mesh);
    for (IGsize f = 0; f < numFaces; f++) {
        auto faceNormal = ComputeFaceNormal(mesh, f);
        if (flips[f]) {
            faceNormal[0] = -faceNormal[0];
            faceNormal[1] = -faceNormal[1];
            faceNormal[2] = -faceNormal[2];
        }

        const igIndex* ids = nullptr;
        int numIds = faces->GetCellIds(f, ids);
        for (int i = 0; i < numIds; i++) {
            igIndex pid = ids[i];
            if (pid < numPoints) {
                outNormals[pid][0] += faceNormal[0];
                outNormals[pid][1] += faceNormal[1];
                outNormals[pid][2] += faceNormal[2];
            }
        }
    }

    // 归一化
    for (auto& n : outNormals) {
        Normalize(n);
    }

    return true;
}

void SurfaceVectorsFilter::Normalize(std::array<double, 3>& v) {
    double len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1e-300) {
        double invLen = 1.0 / len;
        v[0] *= invLen;
        v[1] *= invLen;
        v[2] *= invLen;
    }
}

bool SurfaceVectorsFilter::Execute() {
    auto input = GetInput(0);
    if (!input) {
        m_Message = "SurfaceVectorsFilter: no input data.";
        return false;
    }

    SurfaceMesh::Pointer surfaceMesh = nullptr;
    IGenum dataType = input->GetDataObjectType();

    if (dataType == IG_SURFACE_MESH) {
        surfaceMesh = DynamicCast<SurfaceMesh>(input);
    } else if (dataType == IG_UNSTRUCTURED_MESH) {
        auto um = DynamicCast<UnstructuredMesh>(input);
        if (um) surfaceMesh = um->TransferToSurfaceMesh();
    } else {
        m_Message = "SurfaceVectorsFilter: only SurfaceMesh and UnstructuredMesh are supported.";
        return false;
    }

    if (!surfaceMesh) {
        m_Message = "SurfaceVectorsFilter: failed to get surface mesh.";
        return false;
    }

    // A new model-tree node must own its output attributes. Projecting directly
    // into the input (including TransferToSurfaceMesh's shared attributes)
    // changed the source model and made repeated runs project previous results.
    auto sourceSurface = surfaceMesh;
    surfaceMesh = SurfaceMesh::New();
    auto copiedPoints = Points::New();
    copiedPoints->DeepCopy(sourceSurface->GetPoints());
    auto copiedFaces = CellArray::New();
    copiedFaces->DeepCopy(sourceSurface->GetFaces());
    auto copiedAttributes = AttributeSet::New();
    if (!copiedAttributes->DeepCopy(sourceSurface->GetAttributeSet())) {
        m_Message = "SurfaceVectorsFilter: failed to copy input surface.";
        return false;
    }
    surfaceMesh->SetPoints(copiedPoints);
    surfaceMesh->SetFaces(copiedFaces);
    surfaceMesh->SetAttributeSet(copiedAttributes);
    if (sourceSurface->GetEdges()) {
        auto edges = CellArray::New();
        edges->DeepCopy(sourceSurface->GetEdges());
        surfaceMesh->SetEdges(edges);
    }
    surfaceMesh->SetName(input->GetName() + "_surface_vectors");

    auto attrSet = surfaceMesh->GetAttributeSet();
    if (!attrSet || attrSet->GetNumberOfAttributes() == 0) {
        m_Message = "SurfaceVectorsFilter: input has no attributes.";
        return false;
    }

    // 定位向量属性
    int attrIndex = m_VectorAttrIndex;
    if (attrIndex < 0 && !m_VectorAttrName.empty()) {
        attrIndex = attrSet->GetAttributeIndex(m_VectorAttrName);
    }
    if (attrIndex < 0 || attrIndex >= attrSet->GetNumberOfAttributes()) {
        m_Message = "SurfaceVectorsFilter: vector attribute not found.";
        return false;
    }

    auto& attr = attrSet->GetAttribute(attrIndex);
    if (attr.IsNone()) {
        m_Message = "SurfaceVectorsFilter: attribute is invalid.";
        return false;
    }

    auto vecArray = attr.pointer;
    if (vecArray->GetDimension() < 3) {
        m_Message = "SurfaceVectorsFilter: attribute dimension must be >= 3 (vector).";
        return false;
    }

    IGenum attachmentType = attr.attachmentType;
    IGsize numElements = vecArray->GetNumberOfElements();
    // 与 ParaView vtkSurfaceVectors 一致：结果写回输入向量的同名数组（替换原值）
    const std::string outAttrName = vecArray->GetName();

    // 创建输出数组
    FloatArray::Pointer outArray = FloatArray::New();
    outArray->SetName(outAttrName);
    outArray->SetDimension(3);
    outArray->Reserve(numElements);

    std::vector<float> vecBuf(vecArray->GetDimension(), 0.0f);

    if (attachmentType == IG_CELL) {
        // ========== 单元向量：每个面用自己的法向量 ==========
        IGsize numFaces = surfaceMesh->GetFaces() ? surfaceMesh->GetFaces()->GetNumberOfCells() : 0;
        if (numElements != numFaces) {
            m_Message = "SurfaceVectorsFilter: cell attribute size mismatch.";
            return false;
        }
        const std::vector<char> cellFlips = ComputeConsistencyFlips(surfaceMesh);

        // PerpendicularScale 模式输出标量，单独建数组（同名，与 ParaView 一致）
        FloatArray::Pointer scalarOutArray = nullptr;
        if (m_ConstraintMode == PerpendicularScale) {
            scalarOutArray = FloatArray::New();
            scalarOutArray->SetName(outAttrName);
            scalarOutArray->SetDimension(1);
            scalarOutArray->Reserve(numElements);
        }

        for (IGsize i = 0; i < numFaces; i++) {
            auto normal = ComputeFaceNormal(surfaceMesh, i);
            Normalize(normal);
            // 环绕一致性修正（与点法向分支同一套 flips，对齐 VTK 行为）
            if (cellFlips[i]) {
                normal[0] = -normal[0];
                normal[1] = -normal[1];
                normal[2] = -normal[2];
            }

            vecArray->GetElement(i, vecBuf.data());

            // 点积 = v·n
            double dot = (double)vecBuf[0] * normal[0] + (double)vecBuf[1] * normal[1] +
                         (double)vecBuf[2] * normal[2];

            if (m_ConstraintMode == Parallel) {
                // v' = v - (v·n) * n
                double tx = (double)vecBuf[0] - dot * normal[0];
                double ty = (double)vecBuf[1] - dot * normal[1];
                double tz = (double)vecBuf[2] - dot * normal[2];
                outArray->AddElement3((float)tx, (float)ty, (float)tz);
            } else if (m_ConstraintMode == Perpendicular) {
                // v' = (v·n) * n
                double tx = dot * normal[0];
                double ty = dot * normal[1];
                double tz = dot * normal[2];
                outArray->AddElement3((float)tx, (float)ty, (float)tz);
            } else {
                // PerpendicularScale: 输出标量 = v·n
                float dotF = (float)dot;
                scalarOutArray->AddElement(&dotF);
            }
        }

        // 原位替换数组指针：attrSet 结构与索引不变，避免留下空指针项
        // （旧数组由智能指针自动释放，持有其副本的调用方仍安全）
        {
            auto& outAttr = attrSet->GetAttribute(static_cast<IGsize>(attrIndex));
            if (m_ConstraintMode == PerpendicularScale) {
                outAttr.pointer = scalarOutArray;
                outAttr.type = IG_SCALAR; // 3 维向量变为 1 维标量
            } else {
                outAttr.pointer = outArray;
            }
            outAttr.UpdateAllDataRange(); // 值已变化，重算数值范围供渲染色标使用
        }

    } else if (attachmentType == IG_POINT) {
        // ========== 点向量：面积加权平均得到点法向量 ==========
        std::vector<std::array<double, 3>> pointNormals;
        if (!ComputePointNormals(surfaceMesh, pointNormals)) {
            m_Message = "SurfaceVectorsFilter: failed to compute point normals.";
            return false;
        }

        IGsize numPoints = surfaceMesh->GetNumberOfPoints();
        if (numElements != numPoints) {
            m_Message = "SurfaceVectorsFilter: point attribute size mismatch.";
            return false;
        }

        FloatArray::Pointer scalarOutArray = nullptr;
        if (m_ConstraintMode == PerpendicularScale) {
            scalarOutArray = FloatArray::New();
            scalarOutArray->SetName(outAttrName);
            scalarOutArray->SetDimension(1);
            scalarOutArray->Reserve(numElements);
        }

        for (IGsize i = 0; i < numPoints; i++) {
            const auto& normal = pointNormals[i];

            vecArray->GetElement(i, vecBuf.data());

            double dot = (double)vecBuf[0] * normal[0] + (double)vecBuf[1] * normal[1] +
                         (double)vecBuf[2] * normal[2];

            if (m_ConstraintMode == Parallel) {
                double tx = (double)vecBuf[0] - dot * normal[0];
                double ty = (double)vecBuf[1] - dot * normal[1];
                double tz = (double)vecBuf[2] - dot * normal[2];
                outArray->AddElement3((float)tx, (float)ty, (float)tz);
            } else if (m_ConstraintMode == Perpendicular) {
                double tx = dot * normal[0];
                double ty = dot * normal[1];
                double tz = dot * normal[2];
                outArray->AddElement3((float)tx, (float)ty, (float)tz);
            } else {
                float dotF = (float)dot;
                scalarOutArray->AddElement(&dotF);
            }
        }

        {
            auto& outAttr = attrSet->GetAttribute(static_cast<IGsize>(attrIndex));
            if (m_ConstraintMode == PerpendicularScale) {
                outAttr.pointer = scalarOutArray;
                outAttr.type = IG_SCALAR;
            } else {
                outAttr.pointer = outArray;
            }
            outAttr.UpdateAllDataRange();
        }

    } else {
        m_Message = "SurfaceVectorsFilter: unsupported attribute attachment type.";
        return false;
    }

    attrSet->ForceReConvertToDrawableData();

    const char* modeName =
        m_ConstraintMode == Parallel ? "Parallel" :
        m_ConstraintMode == Perpendicular ? "Perpendicular" : "PerpendicularScale";

    std::cout << "[SurfaceVectorsFilter] Projected '" << outAttrName
              << "' mode=" << modeName
              << " -> written back to '" << outAttrName << "'" << std::endl;
    std::cout << "  Elements: " << numElements
              << ", Type: " << (attachmentType == IG_POINT ? "point" : "cell")
              << std::endl;

    if (dataType == IG_SURFACE_MESH) {
        SetOutput(surfaceMesh);
    } else {
        auto source = DynamicCast<UnstructuredMesh>(input);
        auto output = UnstructuredMesh::New();
        auto cells = CellArray::New();
        cells->DeepCopy(source->GetCells());
        auto types = UnsignedIntArray::New();
        types->DeepCopy(source->GetCellTypes());
        output->SetPoints(surfaceMesh->GetPoints());
        output->SetCells(cells, types);
        output->SetAttributeSet(attrSet);
        output->SetName(surfaceMesh->GetName());
        SetOutput(output);
    }
    return true;
}

IGAME_NAMESPACE_END
