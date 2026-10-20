#include "iGameSmoothFilter.h"
#include "iGameAttributeSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

template<typename Array>
ArrayObject::Pointer CopyArray(ArrayObject::Pointer source) {
    auto output = Array::New();
    if (!output->DeepCopy(DynamicCast<Array>(source))) return nullptr;
    return output;
}

// 按原类型复制数组，保留整数精度
ArrayObject::Pointer DeepCopyArray(ArrayObject::Pointer source) {
    switch (source->GetArrayType()) {
        case IG_FloatArray: return CopyArray<FloatArray>(source);
        case IG_DoubleArray: return CopyArray<DoubleArray>(source);
        case IG_IntArray: return CopyArray<IntArray>(source);
        case IG_UnsignedIntArray: return CopyArray<UnsignedIntArray>(source);
        case IG_CharArray: return CopyArray<CharArray>(source);
        case IG_UnsignedCharArray: return CopyArray<UnsignedCharArray>(source);
        case IG_ShortArray: return CopyArray<ShortArray>(source);
        case IG_UnsignedShortArray: return CopyArray<UnsignedShortArray>(source);
        case IG_LongLongArray: return CopyArray<LongLongArray>(source);
        case IG_UnsignedLongLongArray: return CopyArray<UnsignedLongLongArray>(source);
        default: return nullptr;
    }
}

// 独立复制属性及范围状态
AttributeSet::Pointer DeepCopyAttributes(AttributeSet* source) {
    auto output = AttributeSet::New();
    if (!source) return output;
    for (IGsize i = 0; i < source->GetNumberOfAttributes(); ++i) {
        auto attribute = source->GetAttribute(i);
        if (attribute.pointer) {
            attribute.pointer = DeepCopyArray(attribute.pointer);
            if (!attribute.pointer) return nullptr;
        }
        if (attribute.dataRange) {
            auto range = DoubleArray::New();
            range->DeepCopy(attribute.dataRange);
            attribute.dataRange = range;
        }
        output->GetAllAttributes()->AddElement(attribute);
    }
    return output;
}

// 逐单元复制连接关系
CellArray::Pointer DeepCopyCellArray(CellArray* source) {
    auto output = CellArray::New();
    if (!source) return output;
    for (IGsize i = 0; i < source->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        const int count = source->GetCellIds(i, ids);
        output->AddCellIds(ids, count);
    }
    return output;
}

}

SmoothFilter::SmoothFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void SmoothFilter::SetNumberOfIterations(int value) {
    if (m_NumberOfIterations != value) {
        m_NumberOfIterations = value;
        Modified();
    }
}

void SmoothFilter::SetRelaxationFactor(double value) {
    if (m_RelaxationFactor != value) {
        m_RelaxationFactor = value;
        Modified();
    }
}

void SmoothFilter::SetPreserveBoundary(bool value) {
    if (m_PreserveBoundary != value) {
        m_PreserveBoundary = value;
        Modified();
    }
}

void SmoothFilter::SetConvergence(double value) {
    if (m_Convergence != value) {
        m_Convergence = value;
        Modified();
    }
}

bool SmoothFilter::Execute() {
    SetOutput(nullptr);
    m_Message.clear();
    m_NumberOfIterationsPerformed = 0;
    UpdateProgress(0.0);

    auto input = GetInput(0);
    if (!input) {
        m_Message = "没有输入模型。";
        return false;
    }
    const IGenum type = input->GetDataObjectType();
    if (type != IG_SURFACE_MESH && type != IG_UNSTRUCTURED_MESH) {
        m_Message = "仅支持表面网格或由面单元组成的非结构网格；体网格请先提取表面。";
        return false;
    }
    if (m_NumberOfIterations < 0 || !std::isfinite(m_RelaxationFactor) ||
        m_RelaxationFactor < 0.0 || m_RelaxationFactor > 1.0 ||
        !std::isfinite(m_Convergence) || m_Convergence < 0.0 || m_Convergence > 1.0) {
        m_Message = "迭代次数不能为负，平滑系数和收敛阈值必须在 0 到 1 之间。";
        return false;
    }

    auto inputMesh = DynamicCast<PointSet>(input);
    const IGsize pointCount = inputMesh->GetNumberOfPoints();
    auto inputSurface = DynamicCast<SurfaceMesh>(input);
    auto inputUnstructured = DynamicCast<UnstructuredMesh>(input);
    CellArray::Pointer inputFaces;
    if (inputSurface) inputFaces = inputSurface->GetFaces();
    else inputFaces = inputUnstructured->GetCells();
    if (pointCount == 0 || !inputFaces || inputFaces->GetNumberOfCells() == 0) {
        m_Message = "输入模型必须包含点和面。";
        return false;
    }
    if (inputUnstructured && (!inputUnstructured->GetCellTypes() ||
        inputUnstructured->GetCellTypes()->GetNumberOfElements() != inputFaces->GetNumberOfCells())) {
        m_Message = "单元类型数量与单元数量不一致。";
        return false;
    }
    for (IGsize i = 0; i < pointCount; ++i) {
        const auto& point = inputMesh->GetPoint(i);
        for (int axis = 0; axis < 3; ++axis) {
            if (!std::isfinite(point[axis])) {
                m_Message = "点坐标包含 NaN 或无穷值。";
                return false;
            }
        }
    }

    // 核对面数据，避免超出现有邻接构建的数组容量
    for (IGsize i = 0; i < inputFaces->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        const int count = inputFaces->GetCellIds(i, ids);
        if (count < 3 || count > 32 || !ids) {
            m_Message = "每个面必须包含 3 到 32 个顶点。";
            return false;
        }
        if (inputUnstructured) {
            const auto cellType = inputUnstructured->GetCellType(i);
            if (Cell::GetCellDimension(cellType) != 2 ||
                (cellType == IG_TRIANGLE && count != 3) ||
                (cellType == IG_QUAD && count != 4)) {
                m_Message = "仅支持线性面单元，三角形和四边形的顶点数量必须匹配。";
                return false;
            }
        }
        for (int j = 0; j < count; ++j) {
            if (ids[j] < 0 || static_cast<IGsize>(ids[j]) >= pointCount ||
                std::find(ids, ids + j, ids[j]) != ids + j) {
                m_Message = "面包含越界或重复的顶点编号。";
                return false;
            }
        }
    }

    // 复制几何和属性，保持输入模型不变
    auto points = Points::New();
    points->DeepCopy(inputMesh->GetPoints());
    auto attributes = DeepCopyAttributes(input->GetAttributeSet());
    if (!attributes) {
        m_Message = "输入包含不支持复制的属性数组。";
        return false;
    }
    auto faces = DeepCopyCellArray(inputFaces);
    auto surface = SurfaceMesh::New();
    surface->SetPoints(points);
    surface->SetFaces(faces);
    surface->SetAttributeSet(attributes);

    PointSet::Pointer output = surface;
    if (inputUnstructured) {
        auto mesh = UnstructuredMesh::New();
        auto cellTypes = UnsignedIntArray::New();
        cellTypes->DeepCopy(inputUnstructured->GetCellTypes());
        mesh->SetPoints(points);
        mesh->SetCells(faces, cellTypes);
        mesh->SetAttributeSet(attributes);
        output = mesh;
    } else if (inputSurface->GetEdges()) {
        surface->SetEdges(DeepCopyCellArray(inputSurface->GetEdges()));
    }

    if (m_NumberOfIterations > 0 && m_RelaxationFactor > 0.0) {
        // 构建表面邻接关系
        surface->BuildEdges();
        std::vector<int> degrees(pointCount, 0);
        for (IGsize i = 0; i < surface->GetNumberOfEdges(); ++i) {
            igIndex edge[2]{};
            surface->GetEdgePointIds(i, edge);
            if (++degrees[edge[0]] > 255 || ++degrees[edge[1]] > 255) {
                m_Message = "顶点邻接边超过 255 条，超出现有邻接接口容量。";
                return false;
            }
        }
        std::vector<int> faceCounts(surface->GetNumberOfEdges(), 0);
        for (IGsize i = 0; i < surface->GetNumberOfFaces(); ++i) {
            igIndex edgeIds[32]{};
            const int count = surface->GetFaceEdgeIds(i, edgeIds);
            for (int j = 0; j < count; ++j) {
                if (++faceCounts[edgeIds[j]] > 255) {
                    m_Message = "一条边关联的面超过 255 个，超出现有邻接接口容量。";
                    return false;
                }
            }
        }
        surface->BuildEdgeLinks();
        std::vector<std::vector<igIndex>> neighbors(pointCount);
        std::vector<std::vector<igIndex>> boundaryNeighbors(pointCount);
        std::vector<bool> fixed(pointCount, false);
        auto ids = IdArray::New();
        for (IGsize i = 0; i < pointCount; ++i) {
            surface->GetPointToOneRingPoints(i, ids);
            for (IGsize j = 0; j < ids->GetNumberOfIds(); ++j) {
                neighbors[i].push_back(ids->GetId(j));
            }
        }
        // 边界点只沿边界移动，非流形边端点保持不变
        for (IGsize i = 0; i < surface->GetNumberOfEdges(); ++i) {
            igIndex edge[2]{};
            surface->GetEdgePointIds(i, edge);
            if (faceCounts[i] == 1) {
                boundaryNeighbors[edge[0]].push_back(edge[1]);
                boundaryNeighbors[edge[1]].push_back(edge[0]);
            } else if (faceCounts[i] > 2) {
                fixed[edge[0]] = fixed[edge[1]] = true;
            }
        }
        const double edgeAngleCos = std::cos(15.0 * std::acos(-1.0) / 180.0);
        for (IGsize i = 0; i < pointCount; ++i) {
            auto& boundary = boundaryNeighbors[i];
            if (boundary.empty()) continue;
            if (m_PreserveBoundary || boundary.size() != 2) {
                fixed[i] = true;
                continue;
            }
            Vector3d incoming = Vector3d(points->GetPoint(i)) - Vector3d(points->GetPoint(boundary[0]));
            Vector3d outgoing = Vector3d(points->GetPoint(boundary[1])) - Vector3d(points->GetPoint(i));
            const double lengthProduct = std::sqrt((incoming * incoming) * (outgoing * outgoing));
            if (lengthProduct == 0.0 || (incoming * outgoing) / lengthProduct < edgeAngleCos) fixed[i] = true;
            neighbors[i] = boundary;
        }
        UpdateProgress(0.2);

        // 按顶点顺序更新，与 VTK 的迭代方式对齐
        const double threshold = m_Convergence * output->GetBoundingBox().diag();
        for (int iteration = 0; iteration < m_NumberOfIterations; ++iteration) {
            double maxDisplacementSquared = 0.0;
            for (IGsize i = 0; i < pointCount; ++i) {
                if (fixed[i] || neighbors[i].empty()) continue;
                const Point point = points->GetPoint(i);
                Point next = point;
                double displacementSquared = 0.0;
                for (int axis = 0; axis < 3; ++axis) {
                    double average = 0.0;
                    for (igIndex neighbor : neighbors[i]) average += points->GetPoint(neighbor)[axis];
                    average /= neighbors[i].size();
                    next[axis] = static_cast<float>((1.0 - m_RelaxationFactor) * point[axis] +
                                                    m_RelaxationFactor * average);
                    const double delta = static_cast<double>(next[axis]) - point[axis];
                    displacementSquared += delta * delta;
                }
                points->SetPoint(i, next);
                maxDisplacementSquared = std::max(maxDisplacementSquared, displacementSquared);
            }
            m_NumberOfIterationsPerformed = iteration + 1;
            UpdateProgress(0.2 + 0.8 * (iteration + 1.0) / m_NumberOfIterations);
            if (m_Convergence > 0.0 && std::sqrt(maxDisplacementSquared) <= threshold) break;
        }
    }

    output->SetName("Smooth");
    output->SetViewStyle(IG_SURFACE);
    output->Modified();
    output->ForceReConvertToDrawableData();
    SetOutput(output);
    m_Message = "表面平滑完成。";
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
