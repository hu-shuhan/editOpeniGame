#include "iGameAppendArcLengthFilter.h"

#include "iGameCell.h"
#include "iGameStructuredMesh.h"

#include <algorithm>

IGAME_NAMESPACE_BEGIN

namespace
{
ArrayObject::Pointer CopyArray(ArrayObject::Pointer source) {
#define COPY_ARRAY(Type)                                                                                               \
    if (auto array = DynamicCast<Type>(source)) {                                                                      \
        auto copy = Type::New();                                                                                       \
        copy->DeepCopy(array);                                                                                         \
        copy->SetName(source->GetName());                                                                              \
        return copy;                                                                                                   \
    }
    COPY_ARRAY(FloatArray)
    COPY_ARRAY(DoubleArray)
    COPY_ARRAY(IntArray)
    COPY_ARRAY(UnsignedIntArray)
    COPY_ARRAY(LongLongArray)
    COPY_ARRAY(UnsignedLongLongArray)
    COPY_ARRAY(CharArray)
    COPY_ARRAY(UnsignedCharArray)
    COPY_ARRAY(ShortArray)
    COPY_ARRAY(UnsignedShortArray)
#undef COPY_ARRAY
    return nullptr;
}

CellArray::Pointer CopyCells(CellArray* source) {
    if (!source) { return nullptr; }
    auto copy = CellArray::New();
    copy->DeepCopy(source);
    return copy;
}

// Geometry and attributes belong to a new pipeline result. In particular, keep
// explicit POLYDATA LINES: SurfaceMesh::DeepCopy currently copies only faces.
PointSet::Pointer CopyGeometry(PointSet::Pointer source) {
    auto result = DynamicCast<PointSet>(DataObject::CreateDataObject(source->GetDataObjectType()));
    if (!result) { return nullptr; }
    auto points = Points::New();
    points->DeepCopy(source->GetPoints());
    result->SetPoints(points);
    if (auto mesh = DynamicCast<UnstructuredMesh>(source)) {
        auto types = UnsignedIntArray::New();
        if (mesh->GetCellTypes()) { types->DeepCopy(mesh->GetCellTypes()); }
        DynamicCast<UnstructuredMesh>(result)->SetCells(CopyCells(mesh->GetCells().get()), types);
    } else if (auto mesh = DynamicCast<StructuredMesh>(source)) {
        auto output = DynamicCast<StructuredMesh>(result);
        output->SetDimensionSize(mesh->GetDimensionSize());
        output->GenStructuredCellConnectivities();
    } else if (auto mesh = DynamicCast<VolumeMesh>(source)) {
        auto output = DynamicCast<VolumeMesh>(result);
        if (mesh->GetIsPolyhedronType()) {
            output->InitVolumesWithPolyhedron(CopyCells(mesh->GetFaces()), CopyCells(mesh->GetVolumeFaces()));
        } else {
            output->SetVolumes(CopyCells(mesh->GetVolumes()));
        }
    } else if (auto mesh = DynamicCast<SurfaceMesh>(source)) {
        auto output = DynamicCast<SurfaceMesh>(result);
        output->SetFaces(CopyCells(mesh->GetFaces()));
        // Derived edges can be rebuilt from faces; copying them as explicit
        // edges would incorrectly turn a face-only mesh into an arc-length source.
        if (!mesh->AreEdgesDerivedFromFaces()) { output->SetEdges(CopyCells(mesh->GetEdges())); }
    }
    result->SetViewStyle(static_cast<IGenum>(source->GetViewStyle()));
    result->SetPointSize(source->GetPointSize());
    result->SetLineWidth(source->GetLineWidth());
    result->SetName("PolylineLength");
    return result;
}
} // namespace

AppendArcLengthFilter::AppendArcLengthFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool AppendArcLengthFilter::Execute() {
    m_Message.clear();
    m_LineCount = 0;
    SetOutput(0, nullptr);

    if (!m_Inputs || m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "没有输入数据，请先加载模型。";
        igError("AppendArcLengthFilter: no input data");
        return false;
    }
    DataObject::Pointer input = m_Inputs->GetElement(0);
    if (!input) {
        m_Message = "输入数据为空。";
        igError("AppendArcLengthFilter: input is null");
        return false;
    }

    auto pointSet = DynamicCast<PointSet>(input);
    if (!pointSet) {
        m_Message = "不支持的数据类型，请使用点集及其派生网格（PointSet / SurfaceMesh / VolumeMesh / "
                    "UnstructuredMesh / StructuredMesh）。";
        igError("AppendArcLengthFilter: unsupported data type {}", (int) input->GetDataObjectType());
        return false;
    }
    Points::Pointer points = pointSet->GetPoints();
    if (!points || points->GetNumberOfPoints() == 0) {
        m_Message = "输入没有点，无法计算弧长。";
        igError("AppendArcLengthFilter: no points in input");
        return false;
    }

    const IGsize numPoints = points->GetNumberOfPoints();

    // 输出数组与输入点同精度（VTK：double 点 -> double 数组，其余 -> float 数组；本项目 Points 固定 float32）
    FloatArray::Pointer arcLength = FloatArray::New();
    arcLength->SetName(GetArrayName());
    arcLength->SetDimension(1);
    arcLength->Resize(numPoints);
    for (IGsize i = 0; i < numPoints; ++i) { arcLength->SetValue(i, 0.0); }

    // 折线来源（对应 vtkPolyData::GetLines()）：
    //  1) UnstructuredMesh 的显式线单元（IG_LINE / IG_POLY_LINE），有类型表时按类型过滤；
    //  2) SurfaceMesh 的边数组：legacy VTK POLYDATA 的 LINES 段被读进 SetEdges()。
    //     BuildEdges()（渲染线框 / 若干过滤器会调用）会用"面派生边"覆盖 m_Edges，
    //     因此只在边不是面派生（AreEdgesDerivedFromFaces() == false）时才把边当作折线源。
    CellArray* lineCells = nullptr;
    UnsignedIntArray* lineCellTypes = nullptr;
    UnstructuredMesh::Pointer lineMesh = DynamicCast<UnstructuredMesh>(input);
    if (lineMesh) {
        lineCells = lineMesh->GetCells().get();
        lineCellTypes = lineMesh->GetCellTypes();
    } else {
        auto surfaceMesh = DynamicCast<SurfaceMesh>(input);
        if (surfaceMesh) {
            CellArray* edges = surfaceMesh->GetEdges();
            const IGsize numEdges = edges ? edges->GetNumberOfCells() : static_cast<IGsize>(0);
            if (numEdges > 0 && !surfaceMesh->AreEdgesDerivedFromFaces()) { lineCells = edges; }
        }
    }

    if (lineCells) {
        const IGsize numCells = lineCells->GetNumberOfCells();
        const IGsize checkAbortInterval = std::min<IGsize>(numCells / 10 + 1, 1000);

        for (IGsize cid = 0; cid < numCells; ++cid) {
            if (checkAbortInterval > 0 && cid % checkAbortInterval == 0) {
                UpdateProgress(numCells > 0 ? static_cast<double>(cid) / static_cast<double>(numCells) : 1.0);
            }
            if (lineCellTypes != nullptr) {
                const IGenum cellType = static_cast<IGenum>(lineCellTypes->GetValue(cid));
                if (cellType != IG_LINE && cellType != IG_POLY_LINE) { continue; }
            }

            // 不要拷贝到固定长度的栈缓冲：IGAME_CELL_MAX_SIZE 只有 256，而真实折线常有几百上千个点
            // （实测 347 点的折线就会写穿栈、在函数返回时触发 /GS 检查 0xC0000409）。
            // 用返回内部指针的重载，零拷贝且不受单元点数上限限制。
            const igIndex* pointIds = nullptr;
            const int vcnt = lineCells->GetCellIds(cid, pointIds);
            if (vcnt < 2 || pointIds == nullptr) {
                continue;
            } // 空单元 / 单点单元不产生弧长（与 VTK numCellPoints==0 分支等价）

            double arcDistance = 0.0;
            for (int k = 1; k < vcnt; ++k) {
                const igIndex prevId = pointIds[k - 1];
                const igIndex curId = pointIds[k];
                if (prevId < 0 || prevId >= numPoints || curId < 0 || curId >= numPoints) { continue; }
                // 用 double 计算段长（坐标为 float32，与 vtkAppendArcLength 里 GetPoint(double*) +
                // vtkMath::Distance2BetweenPoints 的算法一致），最终写入 FloatArray 时再落到 float32
                Vector3d prevPoint, curPoint;
                points->GetPoint(prevId, prevPoint);
                points->GetPoint(curId, curPoint);
                arcDistance += (curPoint - prevPoint).norm();
                arcLength->SetValue(curId, arcDistance);
            }
            ++m_LineCount;
        }
    }

    auto result = CopyGeometry(pointSet);
    if (!result) {
        m_Message = "无法为此数据类型创建追加弧长的输出模型。";
        return false;
    }
    auto attributes = AttributeSet::New();
    if (auto inputAttributes = input->GetAttributeSet()) {
        for (IGsize i = 0; i < inputAttributes->GetNumberOfAttributes(); ++i) {
            const auto& source = inputAttributes->GetAttribute(i);
            if (source.isDeleted || !source.pointer) { continue; }
            if (source.attachmentType == IG_POINT && source.pointer->GetName() == GetArrayName()) { continue; }
            auto copy = source;
            copy.pointer = CopyArray(source.pointer);
            if (!copy.pointer) {
                m_Message = "无法复制输入属性：" + source.pointer->GetName();
                return false;
            }
            if (source.dataRange) {
                copy.dataRange = DoubleArray::New();
                copy.dataRange->DeepCopy(source.dataRange);
            }
            const IGsize index = attributes->AddAttribute(copy.type, copy.attachmentType, copy.pointer, copy.dataRange);
            attributes->GetAttribute(index) = copy;
        }
    }
    attributes->AddScalar(IG_POINT, arcLength);
    result->SetAttributeSet(attributes);
    result->Modified();

    this->SetOutput(0, result);
    UpdateProgress(1.0);
    igDebug("AppendArcLengthFilter: points = {}, polylines = {}, array = {}", numPoints, m_LineCount, GetArrayName());
    return true;
}

IGAME_NAMESPACE_END
