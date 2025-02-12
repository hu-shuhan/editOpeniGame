#include "iGameModelClip.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameThreadPool.h"
IGAME_NAMESPACE_BEGIN

ModelClip::ModelClip() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    m_CutPlane[0] = 0.0;
    m_CutPlane[1] = 0.0;
    m_CutPlane[2] = 0.0;
    m_CutPlane[3] = 1.0;

    m_Normal[0] = 0.0;
    m_Normal[1] = 0.0;
    m_Normal[2] = 0.0;
    m_Origin[0] = 0.0;
    m_Origin[1] = 0.0;
    m_Origin[2] = 0.0;
}
ModelClip::~ModelClip() {}
bool ModelClip::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }
    switch (input->GetDataObjectType()) {
        case IG_NONE:
            return true;
        case IG_VOLUME_MESH:
            return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
        case IG_SURFACE_MESH:
            return this->ExecuteWithSurfaceMesh(DynamicCast<SurfaceMesh>(input));
        case IG_UNSTRUCTURED_MESH:
            return this->ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(input));
        case IG_STRUCTURED_MESH:
            return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
        default:
            return false;
    }
    return true;
}


bool ModelClip::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um) {
    if (!um) return false;
    auto m_UnstructuredMesh = um;
    AttributeSet::Pointer inData = m_UnstructuredMesh->GetAttributeSet();
    AttributeSet::Pointer outData = AttributeSet::New();

    CellArray::Pointer OutConn = CellArray::New();
    UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
    Points::Pointer OutPoints = Points::New();
    UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
    std::vector<CellClip::InterpolateEdge> OriginEdge;
    std::vector<igIndex> OriginCell;
    auto inPoints = m_UnstructuredMesh->GetPoints();
    auto inPointNum = m_UnstructuredMesh->GetNumberOfPoints();
    auto inCells = m_UnstructuredMesh->GetCells();
    auto inTypes = m_UnstructuredMesh->GetCellTypes();
    igIndex inCellNum = m_UnstructuredMesh->GetNumberOfCells();

    DoubleArray::Pointer PointClipArray = DoubleArray::New();
    CharArray::Pointer CellVisible = CharArray::New();
    ComputePointValueAndCellVisible(inPoints, inCells, PointClipArray, CellVisible);
    auto PointClipValue = PointClipArray->RawPointer();
    auto cellVisible = CellVisible->RawPointer();
    igIndex vcnt = 0;
    {
        auto Result_ExtractPart = iGame::UnstructuredMesh::New();
        auto ExtractCells = CellArray::New();
        auto ExtractTypes = UnsignedIntArray::New();
        ExtractCells->Reserve(inCells->GetNumberOfCellIds() * 2 / 3);
        ExtractTypes->Reserve(inCellNum * 2 / 3);
        OriginCell.reserve(inCellNum * 2 / 3);
        igIndex cellId = 0;
        igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
        for (cellId = 0; cellId < inCellNum; cellId++) {
            if (cellVisible[cellId] == 1) {
                vcnt = inCells->GetCellIds(cellId, vhs);
                ExtractCells->AddCellIds(vhs, vcnt);
                ExtractTypes->AddValue(inTypes->GetValue(cellId));
                OriginCell.emplace_back(cellId);
            }
        }
        OutPoints->Resize(inPointNum);
        std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPointNum * 3, OutPoints->RawPointer());
        OriginEdge.reserve(inPointNum * 1.2);
        for (int pointId = 0; pointId < inPointNum; pointId++) {
            OriginEdge.emplace_back(CellClip::InterpolateEdge(pointId));
        }
        Result_ExtractPart->SetPoints(OutPoints);
        Result_ExtractPart->SetCells(ExtractCells, ExtractTypes);
        OutConn = ExtractCells;
        OutType = ExtractTypes;
    }
    igIndex* vhs = nullptr;
    igIndex CellId = 0;
    igIndex i = 0;
    Cell::Pointer cell = nullptr;
    double CellClipValue[IGAME_CELL_MAX_SIZE] = {0};
    for (CellId = 0; CellId < inCellNum; CellId++) {
        if (cellVisible[CellId]) { continue; }
        cell = m_UnstructuredMesh->GetCell(CellId);
        vhs = cell->m_PointIds->RawPointer();
        vcnt = cell->GetNumberOfPoints();
        for (i = 0; i < vcnt; i++) { CellClipValue[i] = PointClipValue[vhs[i]]; }
        switch (cell->GetCellType()) {
            case IG_TRIANGLE:
                CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr,
                               nullptr, CellId, OriginEdge, OriginCell);
                break;
            case IG_QUAD:
                CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
            case IG_POLYGON:
                CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
            case IG_TETRA:
                CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
            case IG_QUADRATIC_TETRA:
                CellClip::Clip(DynamicCast<QuadraticTetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr,
                               nullptr, CellId, OriginEdge, OriginCell);
                break;
            case IG_POLYHEDRON:
                CellClip::Clip(DynamicCast<Polyhedron>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr,
                               nullptr, CellId, OriginEdge, OriginCell);
                break;
            default:
                if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
                    CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr,
                                   nullptr, CellId, OriginEdge, OriginCell, PointClipValue);
                }
                break;
        }
    }
    this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge,
                               OriginCell);

    OutMesh->SetCells(OutConn, OutType);
    OutMesh->SetPoints(OutPoints);
    OutMesh->SetAttributeSet(outData);
    this->SetOutput(0, OutMesh);
    std::vector<igIndex>().swap(OriginCell);
    std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);

    return true;
}

bool ModelClip::ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm) {
    if (!vm || vm->GetIsPolyhedronType() == false) { return false; }
    auto m_VolumeMesh = vm;
    AttributeSet::Pointer inData = m_VolumeMesh->GetAttributeSet();
    AttributeSet::Pointer outData = AttributeSet::New();
    CellArray::Pointer OutConn = CellArray::New();
    UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
    Points::Pointer OutPoints = Points::New();
    UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
    std::vector<CellClip::InterpolateEdge> OriginEdge;
    std::vector<igIndex> OriginCell;
    auto inPoints = m_VolumeMesh->GetPoints();
    auto inPointNum = m_VolumeMesh->GetNumberOfPoints();
    auto inVolumes = m_VolumeMesh->GetVolumes();
    auto inFaces = m_VolumeMesh->GetFaces();
    auto inVolumeNum = m_VolumeMesh->GetNumberOfVolumes();

    DoubleArray::Pointer PointClipArray = DoubleArray::New();
    CharArray::Pointer CellVisible = CharArray::New();
    ComputePointValueAndCellVisible(inPoints, inVolumes, PointClipArray, CellVisible);
    auto PointClipValue = PointClipArray->RawPointer();
    auto cellVisible = CellVisible->RawPointer();

    igIndex vcnt = 0;
    igIndex i = 0, j = 0;
    {
        auto Result_ExtractPart = iGame::UnstructuredMesh::New();
        auto ExtractCells = CellArray::New();
        auto ExtractTypes = UnsignedIntArray::New();
        ExtractCells->Reserve(inFaces->GetNumberOfCellIds());
        ExtractTypes->Reserve(inVolumeNum * 2 / 3);
        OriginCell.reserve(inVolumeNum * 2 / 3);
        igIndex cellId = 0;
        igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
        igIndex fcnt = 0;
        igIndex fhs[IGAME_CELL_MAX_SIZE] = {0};
        igIndex realVhs[IGAME_CELL_MAX_SIZE] = {0};
        igIndex realVcnt = 0;

        for (cellId = 0; cellId < inVolumeNum; cellId++) {
            if (cellVisible[cellId] == 1) {
                realVcnt = 0;
                fcnt = m_VolumeMesh->GetVolumeFaceIds(cellId, fhs);
                for (i = 0; i < fcnt; i++) {
                    vcnt = inFaces->GetCellIds(fhs[i], vhs);
                    realVhs[realVcnt++] = vcnt;
                    for (j = 0; j < vcnt; j++) { realVhs[realVcnt++] = vhs[j]; }
                }
                ExtractCells->AddCellIds(realVhs, realVcnt);
                ExtractTypes->AddValue(IG_POLYHEDRON);
                OriginCell.emplace_back(cellId);
            }
        }
        OutPoints->Resize(inPointNum);
        std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPointNum * 3, OutPoints->RawPointer());
        OriginEdge.reserve(inPointNum * 1.2);
        for (int pointId = 0; pointId < inPointNum; pointId++) {
            OriginEdge.emplace_back(CellClip::InterpolateEdge(pointId));
        }
        Result_ExtractPart->SetPoints(OutPoints);
        Result_ExtractPart->SetCells(ExtractCells, ExtractTypes);
        OutConn = ExtractCells;
        OutType = ExtractTypes;
    }
    igIndex* vhs = nullptr;
    igIndex CellId = 0;
    Cell::Pointer cell = nullptr;
    double CellClipValue[IGAME_CELL_MAX_SIZE] = {0};

    for (CellId = 0; CellId < inVolumeNum; CellId++) {
        if (cellVisible[CellId]) { continue; }
        cell = m_VolumeMesh->GetCell(CellId);
        vhs = cell->m_PointIds->RawPointer();
        vcnt = cell->GetNumberOfPoints();
        for (i = 0; i < vcnt; i++) { CellClipValue[i] = PointClipValue[vhs[i]]; }
        CellClip::Clip(DynamicCast<Polyhedron>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr,
                       CellId, OriginEdge, OriginCell);
    }

    this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge,
                               OriginCell);

    OutMesh->SetCells(OutConn, OutType);
    OutMesh->SetPoints(OutPoints);
    OutMesh->SetAttributeSet(outData);
    this->SetOutput(0, OutMesh);
    std::vector<igIndex>().swap(OriginCell);
    std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);

    return true;
}
bool ModelClip::ExecuteWithVolumeMesh(VolumeMesh::Pointer vm) {
    auto m_VolumeMesh = vm;
    if (!m_VolumeMesh) return false;
    if (m_VolumeMesh->GetIsPolyhedronType()) { return this->ExecuteWithVolumeMeshWithPolyhedronType(m_VolumeMesh); }
    AttributeSet::Pointer inData = m_VolumeMesh->GetAttributeSet();
    AttributeSet::Pointer outData = AttributeSet::New();

    CellArray::Pointer OutConn = CellArray::New();
    UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
    Points::Pointer OutPoints = Points::New();
    UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
    std::vector<CellClip::InterpolateEdge> OriginEdge;
    std::vector<igIndex> OriginCell;
    auto inPoints = m_VolumeMesh->GetPoints();
    auto inPointNum = m_VolumeMesh->GetNumberOfPoints();
    auto inCells = m_VolumeMesh->GetVolumes();
    auto inCellNum = m_VolumeMesh->GetNumberOfVolumes();


    DoubleArray::Pointer PointClipArray = DoubleArray::New();
    CharArray::Pointer CellVisible = CharArray::New();
    ComputePointValueAndCellVisible(inPoints, inCells, PointClipArray, CellVisible);
    auto PointClipValue = PointClipArray->RawPointer();
    auto cellVisible = CellVisible->RawPointer();
    igIndex vcnt = 0;
    {
        auto Result_ExtractPart = iGame::UnstructuredMesh::New();
        auto ExtractCells = CellArray::New();
        auto ExtractTypes = UnsignedIntArray::New();
        ExtractCells->Reserve(inCells->GetNumberOfCellIds() * 2 / 3);
        ExtractTypes->Reserve(inCellNum * 2 / 3);
        OriginCell.reserve(inCellNum * 2 / 3);
        igIndex cellId = 0;
        igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
        for (cellId = 0; cellId < inCellNum; cellId++) {
            if (cellVisible[cellId] == 1) {
                vcnt = inCells->GetCellIds(cellId, vhs);
                ExtractCells->AddCellIds(vhs, vcnt);
                ExtractTypes->AddValue(VolumeMesh::GetVolumeTypeWithPointNum(vcnt));
                OriginCell.emplace_back(cellId);
            }
        }
        OutPoints->Resize(inPointNum);
        std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPointNum * 3, OutPoints->RawPointer());
        OriginEdge.reserve(inPointNum * 1.2);
        for (int pointId = 0; pointId < inPointNum; pointId++) {
            OriginEdge.emplace_back(CellClip::InterpolateEdge(pointId));
        }
        Result_ExtractPart->SetPoints(OutPoints);
        Result_ExtractPart->SetCells(ExtractCells, ExtractTypes);
        OutConn = ExtractCells;
        OutType = ExtractTypes;
    }
    igIndex* vhs = nullptr;
    igIndex CellId = 0;
    igIndex i = 0;
    Cell::Pointer cell = nullptr;
    double CellClipValue[IGAME_CELL_MAX_SIZE] = {0};
    for (CellId = 0; CellId < inCellNum; CellId++) {
        if (cellVisible[CellId]) { continue; }
        cell = m_VolumeMesh->GetCell(CellId);
        vhs = cell->m_PointIds->RawPointer();
        vcnt = cell->GetNumberOfPoints();
        for (i = 0; i < vcnt; i++) { CellClipValue[i] = PointClipValue[vhs[i]]; }
        switch (cell->GetCellType()) {
            case IG_TETRA:
                CellClip::Clip(DynamicCast<Tetra>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
            default:
                if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
                    CellClip::Clip(DynamicCast<Volume>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr,
                                   nullptr, CellId, OriginEdge, OriginCell, PointClipValue);
                }
                break;
        }
    }
    this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge,
                               OriginCell);

    OutMesh->SetCells(OutConn, OutType);
    OutMesh->SetPoints(OutPoints);
    OutMesh->SetAttributeSet(outData);
    this->SetOutput(0, OutMesh);
    std::vector<igIndex>().swap(OriginCell);
    std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);

    return true;
}
bool ModelClip::ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm) {
    auto m_SurfaceMesh = sm;
    if (!m_SurfaceMesh) return false;
    AttributeSet::Pointer inData = m_SurfaceMesh->GetAttributeSet();
    AttributeSet::Pointer outData = AttributeSet::New();

    CellArray::Pointer OutConn = CellArray::New();
    UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
    Points::Pointer OutPoints = Points::New();
    UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();
    std::vector<CellClip::InterpolateEdge> OriginEdge;
    std::vector<igIndex> OriginCell;
    auto inPoints = m_SurfaceMesh->GetPoints();
    auto inPointNum = m_SurfaceMesh->GetNumberOfPoints();
    auto inCells = m_SurfaceMesh->GetFaces();
    auto inCellNum = m_SurfaceMesh->GetNumberOfFaces();


    DoubleArray::Pointer PointClipArray = DoubleArray::New();
    CharArray::Pointer CellVisible = CharArray::New();
    ComputePointValueAndCellVisible(inPoints, inCells, PointClipArray, CellVisible);
    auto PointClipValue = PointClipArray->RawPointer();
    auto cellVisible = CellVisible->RawPointer();
    igIndex vcnt = 0;
    {
        auto Result_ExtractPart = iGame::UnstructuredMesh::New();
        auto ExtractCells = CellArray::New();
        auto ExtractTypes = UnsignedIntArray::New();
        ExtractCells->Reserve(inCells->GetNumberOfCellIds() * 2 / 3);
        ExtractTypes->Reserve(inCellNum * 2 / 3);
        OriginCell.reserve(inCellNum * 2 / 3);
        igIndex cellId = 0;
        igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
        for (cellId = 0; cellId < inCellNum; cellId++) {
            if (cellVisible[cellId] == 1) {
                vcnt = inCells->GetCellIds(cellId, vhs);
                ExtractCells->AddCellIds(vhs, vcnt);
                ExtractTypes->AddValue(SurfaceMesh::GetFaceTypeWithPointNum(vcnt));
                OriginCell.emplace_back(cellId);
            }
        }
        OutPoints->Resize(inPointNum);
        std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPointNum * 3, OutPoints->RawPointer());
        OriginEdge.reserve(inPointNum * 1.2);
        for (int pointId = 0; pointId < inPointNum; pointId++) {
            OriginEdge.emplace_back(CellClip::InterpolateEdge(pointId));
        }
        Result_ExtractPart->SetPoints(OutPoints);
        Result_ExtractPart->SetCells(ExtractCells, ExtractTypes);
        OutConn = ExtractCells;
        OutType = ExtractTypes;
    }


    igIndex* vhs = nullptr;
    igIndex CellId = 0;
    igIndex i = 0;
    Cell::Pointer cell = nullptr;
    double CellClipValue[IGAME_CELL_MAX_SIZE] = {0};
    for (CellId = 0; CellId < inCellNum; CellId++) {
        if (cellVisible[CellId]) { continue; }
        cell = m_SurfaceMesh->GetFace(CellId);
        vhs = cell->m_PointIds->RawPointer();
        vcnt = cell->GetNumberOfPoints();
        for (i = 0; i < vcnt; i++) { CellClipValue[i] = PointClipValue[vhs[i]]; }
        switch (cell->GetNumberOfPoints()) {
            case 3:
                CellClip::Clip(DynamicCast<Triangle>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr,
                               nullptr, CellId, OriginEdge, OriginCell);
                break;
            case 4:
                CellClip::Clip(DynamicCast<Quad>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
            default:
                CellClip::Clip(DynamicCast<Polygon>(cell), CellClipValue, OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
        }
    }
    this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData, OriginEdge,
                               OriginCell);

    OutMesh->SetCells(OutConn, OutType);
    OutMesh->SetPoints(OutPoints);
    OutMesh->SetAttributeSet(outData);
    this->SetOutput(0, OutMesh);
    std::vector<igIndex>().swap(OriginCell);
    std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
    return true;
}
void ModelClip::ComputePointValueAndCellVisible(Points::Pointer inPoints, CellArray::Pointer inCells,
                                                DoubleArray::Pointer PointClipArray, CharArray::Pointer CellVisible) {
    igIndex PointId = 0;
    igIndex inPointNum = inPoints->GetNumberOfPoints();
    PointClipArray->Resize(inPointNum);
    double* PointClipValue = PointClipArray->RawPointer();
    for (PointId = 0; PointId < inPointNum; PointId++) { PointClipValue[PointId] = GetPointValue(PointId, inPoints); }
    igIndex CellId = 0;
    IGsize CellNum = inCells->GetNumberOfCells();
    CellVisible->Resize(CellNum);
    auto cellVisible = CellVisible->RawPointer();
    std::fill(cellVisible, cellVisible + CellNum, 0);
    clock_t time1 = clock();
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
void ModelClip::CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData,
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

void ModelClip::SetClipMethod(ClipMethod CM) { this->m_ClipMethod = CM; }
double ModelClip::GetCutValue(float x[3]) {
    return (this->m_Normal[0] * (x[0] - this->m_Origin[0]) + this->m_Normal[1] * (x[1] - this->m_Origin[1]) +
            this->m_Normal[2] * (x[2] - this->m_Origin[2]));
}
double ModelClip::GetCutValue(double x[3]) {
    return (this->m_Normal[0] * (x[0] - this->m_Origin[0]) + this->m_Normal[1] * (x[1] - this->m_Origin[1]) +
            this->m_Normal[2] * (x[2] - this->m_Origin[2]));
}
double ModelClip::GetCutValue(float x0, float x1, float x2) {
    return (this->m_Normal[0] * (x0 - this->m_Origin[0]) + this->m_Normal[1] * (x1 - this->m_Origin[1]) +
            this->m_Normal[2] * (x2 - this->m_Origin[2]));
}
double ModelClip::GetCutValue(double x0, double x1, double x2) {
    return (this->m_Normal[0] * (x0 - this->m_Origin[0]) + this->m_Normal[1] * (x1 - this->m_Origin[1]) +
            this->m_Normal[2] * (x2 - this->m_Origin[2]));
}
double ModelClip::GetCutValue(Point x) {
    return (this->m_Normal[0] * (x[0] - this->m_Origin[0]) + this->m_Normal[1] * (x[1] - this->m_Origin[1]) +
            this->m_Normal[2] * (x[2] - this->m_Origin[2]));
}
double ModelClip::GetPointValue(igIndex pId, Points::Pointer points) {
    switch (m_ClipMethod) {
        case iGame::ModelClip::IG_PLANE:
            return this->GetCutValue(points->GetPoint(pId));
            break;
        default:
            break;
    }
    return -1;
}
void ModelClip::SetPlane(float o[3], float n[3]) {
    double sum = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (sum < 1e-40) { sum = 1e-40; };
    m_Normal[0] = n[0] / sum;
    m_Normal[1] = n[1] / sum;
    m_Normal[2] = n[2] / sum;
    m_Origin[0] = o[0];
    m_Origin[1] = o[1];
    m_Origin[2] = o[2];
    this->SetClipMethod(IG_PLANE);
}
void ModelClip::SetPlane(double o[3], double n[3]) {
    double sum = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (sum < 1e-40) { sum = 1e-40; };
    m_Normal[0] = n[0] / sum;
    m_Normal[1] = n[1] / sum;
    m_Normal[2] = n[2] / sum;
    m_Origin[0] = o[0];
    m_Origin[1] = o[1];
    m_Origin[2] = o[2];
    this->SetClipMethod(IG_PLANE);
}

void ModelClip::GetPlane(float o[3], float n[3]) {
    n[0] = m_Normal[0];
    n[1] = m_Normal[1];
    n[2] = m_Normal[2];
    o[0] = m_Origin[0];
    o[1] = m_Origin[1];
    o[2] = m_Origin[2];
}
void ModelClip::GetPlane(double o[3], double n[3]) {
    n[0] = m_Normal[0];
    n[1] = m_Normal[1];
    n[2] = m_Normal[2];
    o[0] = m_Origin[0];
    o[1] = m_Origin[1];
    o[2] = m_Origin[2];
}

IGAME_NAMESPACE_END