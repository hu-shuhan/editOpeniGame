#include "iGameSliceFilter.h"

#include "iGameThreadPool.h"

IGAME_NAMESPACE_BEGIN
SliceFilter::SliceFilter() {

    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    m_Contourer = iGame::ContourFilter::New();
    m_Clipper = iGame::ModelClip::New();
    m_PlaneOrigin[0] = 0;
    m_PlaneOrigin[1] = 0;
    m_PlaneOrigin[2] = 0;
    m_PlaneNormal[0] = 1;
    m_PlaneNormal[1] = 0;
    m_PlaneNormal[2] = 0;
}


SliceFilter::~SliceFilter() {}

void SliceFilter::SetPlane(double ox, double oy, double oz, double nx, double ny, double nz)
{
    m_PlaneOrigin[0] = ox;
    m_PlaneOrigin[1] = oy;
    m_PlaneOrigin[2] = oz;
    m_PlaneNormal[0] = nx;
    m_PlaneNormal[1] = ny;
    m_PlaneNormal[2] = nz;
    // 同步更新 Clipper 的平面设置
    if (m_Clipper) {
        m_Clipper->SetPlane(m_PlaneOrigin, m_PlaneNormal);
    }
}

void SliceFilter::SetCrinkle(bool crinkle) {
    this->m_Crinkle = crinkle;
}

bool SliceFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }

    // 根据模式选择执行方法
    if (m_Crinkle) {
        return this->ExecuteCrinkle(input);
    } else {
        return this->ExecuteContour(input);
    }
}

bool SliceFilter::ExecuteContour(DataObject::Pointer input) {
    if (!input) return false;
    
    m_Contourer->SetInput(input);
    
    // Compute point-to-plane signed distances and set as scalar data
    // All mesh types (UnstructuredMesh, SurfaceMesh, VolumeMesh, etc.) inherit from PointSet
    auto PointSet = DynamicCast<iGame::PointSet>(input);
    if (PointSet == nullptr) return false;
    auto Points = PointSet->GetPoints();
    auto PointNum = PointSet->GetNumberOfPoints();
    if (PointNum == 0) return false;
    
    DoubleArray::Pointer ScalarData = DoubleArray::New();
    ScalarData->Resize(PointNum);
    double* scalarData = ScalarData->RawPointer();
    Point p{0, 0, 0};
    for (int i = 0; i < PointNum; i++) {
        p = Points->GetPoint(i);
        scalarData[i] = m_PlaneNormal[0] * (p[0] - m_PlaneOrigin[0]) + 
                        m_PlaneNormal[1] * (p[1] - m_PlaneOrigin[1]) + 
                        m_PlaneNormal[2] * (p[2] - m_PlaneOrigin[2]);
    }
    m_Contourer->SetIsoScalarData(ScalarData, 0.0, 0);

    bool result = m_Contourer->Execute();
    this->SetOutput(m_Contourer->GetOutput());
    return result;
}

bool SliceFilter::ExecuteCrinkle(DataObject::Pointer input) {
    if (!input) return false;
    
    // 将输入转换为 UnstructuredMesh
    auto um = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    if (!um) return false;
    
    AttributeSet::Pointer inData = um->GetAttributeSet();
    AttributeSet::Pointer outData = AttributeSet::New();
    
    CellArray::Pointer OutConn = CellArray::New();
    UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
    Points::Pointer OutPoints = Points::New();
    UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
    std::vector<CellClip::InterpolateEdge> OriginEdge;
    std::vector<igIndex> OriginCell;
    
    auto inPoints = um->GetPoints();
    auto inPointNum = um->GetNumberOfPoints();
    auto inCells = um->GetCells();
    auto inTypes = um->GetCellTypes();
    igIndex inCellNum = um->GetNumberOfCells();
    
    DoubleArray::Pointer PointClipArray = DoubleArray::New();
    CharArray::Pointer CellVisible = CharArray::New();
    ComputePointValueAndCellVisible(inPoints, inCells, PointClipArray, CellVisible);
    auto PointClipValue = PointClipArray->RawPointer();
    auto cellVisible = CellVisible->RawPointer();
    
    // 只保留与平面相交的 cell（cellVisible[cellId] == 0）
    OutConn->Reserve(inCells->GetNumberOfCellIds() / 3);
    OutType->Reserve(inCellNum / 3);
    OriginCell.reserve(inCellNum / 3);
    igIndex cellId = 0;
    igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
    igIndex vcnt = 0;
    for (cellId = 0; cellId < inCellNum; cellId++) {
        if (cellVisible[cellId] == 0) {
            vcnt = inCells->GetCellIds(cellId, vhs);
            OutConn->AddCellIds(vhs, vcnt);
            OutType->AddValue(inTypes->GetValue(cellId));
            OriginCell.emplace_back(cellId);
        }
    }
    
    // 复制所有点
    OutPoints->Resize(inPointNum);
    std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPointNum * 3, OutPoints->RawPointer());
    OriginEdge.reserve(inPointNum * 1.2);
    for (int pointId = 0; pointId < inPointNum; pointId++) {
        OriginEdge.emplace_back(CellClip::InterpolateEdge(pointId));
    }
    
    // 复制属性数据
    CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), 
                        inData, outData, OriginEdge, OriginCell);
    
    OutMesh->SetCells(OutConn, OutType);
    OutMesh->SetPoints(OutPoints);
    OutMesh->SetAttributeSet(outData);
    this->SetOutput(OutMesh);
    
    std::vector<igIndex>().swap(OriginCell);
    std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
    return true;
}

void SliceFilter::ComputePointValueAndCellVisible(Points::Pointer inPoints, CellArray::Pointer inCells,
                                                  DoubleArray::Pointer PointClipArray, CharArray::Pointer CellVisible) {
    igIndex PointId = 0;
    igIndex inPointNum = inPoints->GetNumberOfPoints();
    PointClipArray->Resize(inPointNum);
    double* PointClipValue = PointClipArray->RawPointer();
    for (PointId = 0; PointId < inPointNum; PointId++) { 
        PointClipValue[PointId] = GetPointValue(PointId, inPoints); 
    }
    igIndex CellId = 0;
    IGsize CellNum = inCells->GetNumberOfCells();
    CellVisible->Resize(CellNum);
    auto cellVisible = CellVisible->RawPointer();
    std::fill(cellVisible, cellVisible + CellNum, 0);
    
    auto func = [&](igIndex start, igIndex end) -> void {
        igIndex cellId = 0;
        igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
        igIndex vcnt = 0;
        igIndex allIn = 1, allOut = 1;
        double value = 0;
        igIndex i = 0;
        for (cellId = start; cellId < end; cellId++) {
            vcnt = inCells->GetCellIds(cellId, vhs);
            allIn = 1;
            allOut = 1;
            for (i = 0; i < vcnt; i++) {
                value = PointClipValue[vhs[i]];
                if (value < 0.0) {
                    allOut = 0;
                } else if (value > 0.0) {
                    allIn = 0;
                } else {
                    allIn = 0;
                    allOut = 0;
                }
            }
            if (allIn) {
                cellVisible[cellId] = 1;
            } else if (allOut) {
                cellVisible[cellId] = 2;
            }
        }
    };
    ThreadPool::parallelFor(0, CellNum, func);
    PointClipValue = nullptr;
}

void SliceFilter::CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData,
                                     AttributeSet::Pointer outData, std::vector<CellClip::InterpolateEdge> OriginEdge,
                                     std::vector<igIndex> OriginCell) {
    igIndex i = 0, j = 0, k = 0;
    int dimension = 0;
    auto inAllAttr = inData->GetAllAttributes();
    double values[IGAME_CELL_MAX_SIZE] = {0};
    double values_1[IGAME_CELL_MAX_SIZE] = {0};
    double values_2[IGAME_CELL_MAX_SIZE] = {0};
    for (i = 0; i < inAllAttr->GetNumberOfElements(); i++) {
        auto attr = inAllAttr->GetElement(i);
        auto inArray = attr.pointer;
        auto outArray = FloatArray::New();
        outArray->SetName(inArray->GetName());
        outArray->SetDimension(inArray->GetDimension());
        if (attr.attachmentType == IG_CELL) {
            outArray->Resize(outCellNum);
            for (j = 0; j < outCellNum; j++) {
                inArray->GetElement(OriginCell[j], values);
                outArray->SetElement(j, values);
            }
            outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
        } else if (attr.attachmentType == IG_POINT) {
            outArray->Resize(outPointNum);
            dimension = inArray->GetDimension();
            for (j = 0; j < outPointNum; j++) {
                inArray->GetElement(OriginEdge[j].vh1, values_1);
                if (OriginEdge[j].vh2 == -1) {
                    outArray->SetElement(j, values_1);
                } else {
                    inArray->GetElement(OriginEdge[j].vh2, values_2);
                    for (k = 0; k < dimension; k++) {
                        values[k] = values_1[k] + OriginEdge[j].t * (values_2[k] - values_1[k]);
                    }
                    outArray->SetElement(j, values);
                }
            }
            outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
        }
    }
}

double SliceFilter::GetPointValue(igIndex pId, Points::Pointer points) {
    // 归一化法向量
    double sum = std::sqrt(m_PlaneNormal[0] * m_PlaneNormal[0] + 
                          m_PlaneNormal[1] * m_PlaneNormal[1] + 
                          m_PlaneNormal[2] * m_PlaneNormal[2]);
    if (sum < 1e-40) { sum = 1e-40; }
    double normal[3] = {m_PlaneNormal[0] / sum, m_PlaneNormal[1] / sum, m_PlaneNormal[2] / sum};
    
    Point p = points->GetPoint(pId);
    return normal[0] * (p[0] - m_PlaneOrigin[0]) + 
           normal[1] * (p[1] - m_PlaneOrigin[1]) + 
           normal[2] * (p[2] - m_PlaneOrigin[2]);
}

IGAME_NAMESPACE_END