#include "iGameUnstructuredMesh.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
IGAME_NAMESPACE_BEGIN
void UnstructuredMesh::SetCells(CellArray::Pointer cell, UnsignedIntArray::Pointer type) {
    m_Cells = cell;
    m_Types = type;
    this->Modified();
}

CellArray::Pointer UnstructuredMesh::GetCells() { return this->m_Cells; };

void UnstructuredMesh::AddCell(igIndex* cell, int size, IGenum type) {
    m_Cells->AddCellIds(cell, size);
    m_Types->AddValue(type);
    this->Modified();
}

IGsize UnstructuredMesh::GetNumberOfCells() const noexcept { return m_Cells->GetNumberOfCells(); }
void UnstructuredMesh::GetCellPointIds(const IGsize cellId, IdArray::Pointer cell) {
    if (cell == nullptr) { return; }
    m_Cells->GetCellIds(cellId, cell);
}

int UnstructuredMesh::GetCellPointIds(const IGsize cellId, igIndex* cell) { return m_Cells->GetCellIds(cellId, cell); }

int UnstructuredMesh::GetCellPointIds(const IGsize cellId, const igIndex*& cell) {
    return m_Cells->GetCellIds(cellId, cell);
}

Cell* UnstructuredMesh::GetCell(const IGsize cellId) {
    Cell* cell = GetTypedCell(cellId);
    if (cell == nullptr) { return nullptr; }
    cell->Reset();
    if (cell->GetCellType() != IG_POLYHEDRON) {
        GetCellPointIds(cellId, cell->m_PointIds);
        for (int i = 0; i < cell->m_PointIds->GetNumberOfIds(); i++) {
            cell->m_Points->AddPoint(GetPoint(cell->m_PointIds->GetId(i)));
        }
    } else {
        igIndex ids[IGAME_CELL_MAX_SIZE] = {};
        igIndex size = m_Cells->GetCellIds(cellId, ids);
        Polyhedron::Pointer polyhedron = DynamicCast<Polyhedron>(cell);
        polyhedron->m_FaceOffset->Reset();
        polyhedron->m_FaceOffset->Reserve(ids[0]);
        polyhedron->m_PointIds->Reserve(size);

        igIndex index = 1;
        igIndex faceVcnt = 0;
        int offset = 0;
        while (index < size) {
            polyhedron->m_FaceOffset->AddId(offset);
            faceVcnt = ids[index++];
            for (igIndex id = 0; id < faceVcnt; id++) { polyhedron->m_PointIds->AddId(ids[index++]); }
            offset += faceVcnt;
        }
        polyhedron->m_FaceOffset->AddId(offset);
        for (int i = 0; i < cell->m_PointIds->GetNumberOfIds(); i++) {
            cell->m_Points->AddPoint(GetPoint(cell->m_PointIds->GetId(i)));
        }
    }
    return cell;
}
UnsignedIntArray* UnstructuredMesh::GetCellTypes() const { return m_Types; }

IGenum UnstructuredMesh::GetCellType(const IGsize cellId) const { return m_Types->GetValue(cellId); }

UnstructuredMesh::UnstructuredMesh() {
    m_ViewStyle = IG_SURFACE;
    m_Cells = CellArray::New();
    m_Types = UnsignedIntArray::New();
}
SurfaceMesh::Pointer UnstructuredMesh::TransferToSurfaceMesh() {

    int cellNum = this->GetNumberOfCells();
    bool CouldTransfer = true;
    igIndex cellType = IG_NONE;
    for (igIndex i = 0; i < cellNum; i++) {
        cellType = this->GetCellType(i);
        if (Cell::GetCellDimension(cellType) != 2) {
            CouldTransfer = false;
            break;
        }
    }
    if (CouldTransfer == false) { return nullptr; }
    SurfaceMesh::Pointer mesh = SurfaceMesh::New();
    mesh->SetName(this->GetName());
    mesh->SetPoints(this->m_Points);
    mesh->SetFaces(this->m_Cells);
    mesh->SetAttributeSet(this->m_Attributes);
    return mesh;
}

VolumeMesh::Pointer UnstructuredMesh::TransferToVolumeMesh() {

    int cellNum = this->GetNumberOfCells();
    bool CouldTransfer = true;
    igIndex cellType = IG_NONE;
    for (igIndex i = 0; i < cellNum; i++) {
        cellType = this->GetCellType(i);
        if (Cell::GetCellDimension(cellType) != 3) {
            CouldTransfer = false;
            break;
        }
    }
    if (CouldTransfer == false) { return nullptr; }
    VolumeMesh::Pointer mesh = VolumeMesh::New();
    mesh->SetPoints(this->m_Points);
    mesh->SetVolumes(this->m_Cells);
    mesh->SetAttributeSet(this->m_Attributes);
    return mesh;
}

SurfaceMesh::Pointer UnstructuredMesh::ExtractSurfaceMesh() {
    int cellNum = this->GetNumberOfCells();
    bool CouldTransfer = true;
    igIndex cellType = IG_NONE;
    CellArray::Pointer faces = CellArray::New();
    igIndex vcnt = 0;
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    for (igIndex i = 0; i < cellNum; i++) {
        cellType = this->GetCellType(i);
        if (Cell::GetCellDimension(cellType) == 2) {
            vcnt = this->m_Cells->GetCellIds(i, vhs);
            faces->AddCellIds(vhs, vcnt);
        }
    }
    if (!faces->GetNumberOfCells()) { return nullptr; }
    SurfaceMesh::Pointer mesh = SurfaceMesh::New();
    mesh->SetPoints(this->m_Points);
    mesh->SetFaces(faces);
    return mesh;
}
VolumeMesh::Pointer UnstructuredMesh::ExtractVolumeMesh() {
    int cellNum = this->GetNumberOfCells();
    bool CouldTransfer = true;
    igIndex cellType = IG_NONE;
    CellArray::Pointer volumes = CellArray::New();
    igIndex vcnt = 0;
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    for (igIndex i = 0; i < cellNum; i++) {
        cellType = this->GetCellType(i);
        if (Cell::GetCellDimension(cellType) == 3) {
            vcnt = this->m_Cells->GetCellIds(i, vhs);
            volumes->AddCellIds(vhs, vcnt);
        }
    }
    if (!volumes->GetNumberOfCells()) { return nullptr; }
    VolumeMesh::Pointer mesh = VolumeMesh::New();
    mesh->SetPoints(this->m_Points);
    mesh->SetVolumes(volumes);
    return mesh;
}
bool UnstructuredMesh::GenerateFromSurfaceMesh(SurfaceMesh::Pointer mesh) {
    if (!mesh) return false;
    UnsignedIntArray::Pointer Types = UnsignedIntArray::New();
    auto inFaces = mesh->GetFaces();
    auto inFcnt = mesh->GetNumberOfFaces();
    Types->Resize(inFcnt);
    unsigned int type = 0;
    int vcnt = 0;
    for (int i = 0; i < inFcnt; i++) {
        vcnt = inFaces->GetCellSize(i);
        if (vcnt == 3) {
            type = IG_TRIANGLE;
        } else if (vcnt == 4) {
            type = IG_QUAD;
        } else {
            type = IG_POLYGON;
        }
        Types->SetValue(i, type);
    }
    this->SetPoints(mesh->GetPoints());
    this->SetCells(inFaces, Types);
    this->SetAttributeSet(mesh->GetAttributeSet());
    return true;
}
bool UnstructuredMesh::GenerateFromVolumeMesh(VolumeMesh::Pointer mesh) {
    if (!mesh) return false;
    int volumeNum = mesh->GetNumberOfVolumes();
    auto Volumes = mesh->GetVolumes();
    UnsignedIntArray::Pointer CellTypes = UnsignedIntArray::New();
    igIndex vcnt = 0;
    igIndex vhs[IGAME_CELL_MAX_SIZE];
    if (mesh->GetIsPolyhedronType()) {
        CellTypes->Resize(volumeNum);
        std::fill(CellTypes->RawPointer(), CellTypes->RawPointer() + volumeNum, IG_POLYHEDRON);
        igIndex realVcnt = 0;
        igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
        igIndex realVhs[IGAME_CELL_MAX_SIZE] = {0};
        igIndex fcnt = 0;
        igIndex fhs[IGAME_CELL_MAX_SIZE] = {0};
        igIndex i = 0, j = 0;
        auto inFaces = mesh->GetFaces();
        auto realCells = CellArray::New();
        realCells->Reserve(inFaces->GetNumberOfCellIds());
        for (igIndex cellId = 0; cellId < volumeNum; cellId++) {
            realVcnt = 0;
            fcnt = mesh->GetVolumeFaceIds(cellId, fhs);
            realVhs[realVcnt++] = fcnt;
            for (i = 0; i < fcnt; i++) {
                vcnt = inFaces->GetCellIds(fhs[i], vhs);
                realVhs[realVcnt++] = vcnt;
                for (j = 0; j < vcnt; j++) { realVhs[realVcnt++] = vhs[j]; }
            }
            realCells->AddCellIds(realVhs, realVcnt);
        }
        this->SetPoints(mesh->GetPoints());
        this->SetCells(realCells, CellTypes);
        this->SetAttributeSet(mesh->GetAttributeSet());
    } else {
        CellTypes->Reserve(volumeNum);
        for (igIndex i = 0; i < volumeNum; i++) {
            vcnt = Volumes->GetCellIds(i, vhs);
            switch (vcnt) {
                case 4:
                    CellTypes->AddValue(IG_TETRA);
                    break;
                case 5:
                    CellTypes->AddValue(IG_PYRAMID);
                    break;
                case 6:
                    CellTypes->AddValue(IG_PRISM);
                    break;
                case 8:
                    CellTypes->AddValue(IG_HEXAHEDRON);
                    break;
                default:
                    igError("Not support this volume with " << vcnt << "'s verts.");
                    return false;
            }
        }
        this->SetPoints(mesh->GetPoints());
        this->SetCells(mesh->GetCells(), CellTypes);
        this->SetAttributeSet(mesh->GetAttributeSet());
    }

    return true;
}


bool UnstructuredMesh::TransferVolumeMeshToUnstructuredMesh(VolumeMesh::Pointer input,
                                                            UnstructuredMesh::Pointer& output) {
    if (!output) { output = UnstructuredMesh::New(); }
    return output->GenerateFromVolumeMesh(input);
}

UnstructuredMesh::Pointer UnstructuredMesh::TransDataObjToUnstructuredMesh(DataObject::Pointer dataObj) {
    auto meshType = dataObj->GetDataObjectType();
    UnstructuredMesh::Pointer re;
    switch (meshType) {
        case IG_DATA_OBJECT:
            return re;
        case IG_COMPOSITE_DATA_OBJECT:
            return re;
        case IG_DRAW_OBJECT:
            return re;
        case IG_POINT_SET:
            return re;
        case IG_SURFACE_MESH:
            re = UnstructuredMesh::New();
            re->GenerateFromSurfaceMesh(DynamicCast<SurfaceMesh>(dataObj));
            return re;
        case IG_VOLUME_MESH:
            re = UnstructuredMesh::New();
            re->GenerateFromVolumeMesh(DynamicCast<VolumeMesh>(dataObj));
            return re;
        case IG_UNSTRUCTURED_MESH:
            return DynamicCast<UnstructuredMesh>(dataObj);
        case IG_STRUCTURED_MESH:
            return re;
        case IG_MULTIBLOCK_MESH:
            return re;
        case IG_SPLINE_GEOMETRY:
            return re;
        case IG_DATA_OBJECT_COUNT:
            return re;
        default:
            return re;
    }
}

IGsize UnstructuredMesh::GetRealMemorySize() {
    IGsize res = this->PointSet::GetRealMemorySize();
    if (m_Cells) res += m_Cells->GetRealMemorySize();
    if (m_Types) res += m_Types->GetRealMemorySize();
    return res + sizeof(IGsize);
}
Cell* UnstructuredMesh::GetTypedCell(const IGsize cellId) {
    Cell* cell = nullptr;
    switch (GetCellType(cellId)) {
        case IG_LINE: {
            if (m_Line == nullptr) { m_Line = Line::New(); }
            cell = m_Line.get();
        } break;
        case IG_POLY_LINE: {
            if (m_PolyLine == nullptr) { m_PolyLine = PolyLine::New(); }
            cell = m_PolyLine.get();
        } break;
        case IG_TRIANGLE: {
            if (m_Triangle == nullptr) { m_Triangle = Triangle::New(); }
            cell = m_Triangle.get();
        } break;
        case IG_QUAD: {
            if (m_Quad == nullptr) { m_Quad = Quad::New(); }
            cell = m_Quad.get();
        } break;
        case IG_POLYGON: {
            if (m_Polygon == nullptr) { m_Polygon = Polygon::New(); }
            cell = m_Polygon.get();
        } break;
        case IG_TETRA: {
            if (m_Tetra == nullptr) { m_Tetra = Tetra::New(); }
            cell = m_Tetra.get();
        } break;
        case IG_HEXAHEDRON: {
            if (m_Hexahedron == nullptr) { m_Hexahedron = Hexahedron::New(); }
            cell = m_Hexahedron.get();
        } break;
        case IG_PRISM: {
            if (m_Prism == nullptr) { m_Prism = Prism::New(); }
            cell = m_Prism.get();
        } break;
        case IG_PYRAMID: {
            if (m_Pyramid == nullptr) { m_Pyramid = Pyramid::New(); }
            cell = m_Pyramid.get();
        } break;
        case IG_POLYHEDRON: {
            if (m_Polyhedron == nullptr) { m_Polyhedron = Polyhedron::New(); }
            cell = m_Polyhedron.get();
        } break;
        case IG_QUADRATIC_EDGE: {
            if (m_QuadraticLine == nullptr) { m_QuadraticLine = QuadraticLine::New(); }
            cell = m_QuadraticLine.get();
        } break;
        case IG_QUADRATIC_TRIANGLE: {
            if (m_QuadraticTriangle == nullptr) { m_QuadraticTriangle = QuadraticTriangle::New(); }
            cell = m_QuadraticTriangle.get();
        } break;
        case IG_QUADRATIC_QUAD: {
            if (m_QuadraticQuad == nullptr) { m_QuadraticQuad = QuadraticQuad::New(); }
            cell = m_QuadraticQuad.get();
        } break;
        case IG_QUADRATIC_TETRA: {
            if (m_QuadraticTetra == nullptr) { m_QuadraticTetra = QuadraticTetra::New(); }
            cell = m_QuadraticTetra.get();
        } break;
        case IG_QUADRATIC_HEXAHEDRON: {
            if (m_QuadraticHexahedron == nullptr) { m_QuadraticHexahedron = QuadraticHexahedron::New(); }
            cell = m_QuadraticHexahedron.get();
        } break;
        case IG_QUADRATIC_PRISM: {
            if (m_QuadraticPrism == nullptr) { m_QuadraticPrism = QuadraticPrism::New(); }
            cell = m_QuadraticPrism.get();
        } break;
        case IG_QUADRATIC_PYRAMID: {
            if (m_QuadraticPyramid == nullptr) { m_QuadraticPyramid = QuadraticPyramid::New(); }
            cell = m_QuadraticPyramid.get();
        } break;
        default: {
            if (m_EmptyCell == nullptr) { m_EmptyCell = EmptyCell::New(); }
            cell = m_EmptyCell.get();
        } break;
    }
    return cell;
}

void UnstructuredMesh::ConvertToDrawableData() {
    if (m_Points->GetMTime() > m_Positions->GetMTime() || m_Clipper->GetMTime() > m_Positions->GetMTime() ||
        m_ReConvertToDrawableData) {
        m_ReConvertToDrawableData = false;
        bool ShellSuccess = true;
        if (m_ExecuteShell) {
            iGameModelGeometryFilter::Pointer extract = iGameModelGeometryFilter::New();

            // update clip status
            auto box = m_Clipper->m_Box;
            if (box.m_Use) {
                const auto& a = box.m_Bmin;
                const auto& b = box.m_Bmax;
                extract->SetExtent(a[0], b[0], a[1], b[1], a[2], b[2], box.m_Flip);
            }

            auto plane = m_Clipper->m_Plane;
            if (plane.m_Use) { extract->SetClipPlane(plane.m_Origin, plane.m_Normal, plane.m_Flip); }
            // shell algorithm
            SurfaceMesh::Pointer surfaceMesh = SurfaceMesh::New();
            if (extract->Execute(this, surfaceMesh)) {
                SetRenderableObject(surfaceMesh);
                m_PointMap = extract->GetPointMap();
            } else {
                ShellSuccess = false;
                this->m_RenderableMesh.SurfaceMesh = nullptr;
                this->m_RenderableMesh.SimplifiedMesh = nullptr;
                //igError("Failed to execute the shell algorithm.");
            }
        }
        if (ShellSuccess == false) {
            auto pointIndices = UnsignedIntArray::New();
            pointIndices->SetDimension(1);
            auto edgeIndices = UnsignedIntArray::New();
            edgeIndices->SetDimension(2);
            auto triangleIndices = UnsignedIntArray::New();
            triangleIndices->SetDimension(3);
            auto triangleEdgeMasks = UnsignedCharArray::New();
            triangleEdgeMasks->SetDimension(1);

            igIndex ids[IGAME_CELL_MAX_SIZE]{};
            for (int id = 0; id < GetNumberOfCells(); id++) {
                int size = GetCellPointIds(id, ids);
                if (!m_Clipper->IsAllDisable()) {
                    bool visible = true;
                    for (int i = 0; i < size; i++) {
                        const auto& point = this->GetPoint(ids[i]);
                        if (!m_Clipper->IsVisible(point.pointer())) {
                            visible = false;
                            break;
                        }
                    }
                    if (!visible) continue;
                }

                IGenum type = GetCellType(id);
                switch (type) {
                    case IG_VERTEX:
                        pointIndices->AddValue(ids[0]);
                        break;
                    case IG_LINE:
                    case IG_POLY_LINE: {
                        for (int i = 1; i < size; i++) { edgeIndices->AddElement2(ids[i - 1], ids[i]); }
                    } break;
                    case IG_QUADRATIC_EDGE: {
                        edgeIndices->AddElement2(ids[0], ids[2]);
                        edgeIndices->AddElement2(ids[2], ids[1]);
                    } break;
                    case IG_TRIANGLE:
                    case IG_QUAD:
                    case IG_POLYGON: {
                        // add line
                        for (int i = 0; i < size; i++) { edgeIndices->AddElement2(ids[i], ids[(i + 1) % size]); }
                        // add triangles
                        for (int i = 1; i < size - 1; i++) {
                            triangleIndices->AddElement3(ids[0], ids[i], ids[i + 1]);
                            // add edge mask
                            int mask = size == 3 ? 7 : i == 1 ? 3 : i == size - 2 ? 6 : 2;
                            triangleEdgeMasks->AddValue(mask);
                        }
                    } break;
                    case IG_QUADRATIC_TRIANGLE:
                    case IG_QUADRATIC_QUAD: {
                        int trueSize = size / 2;
                        // add lines
                        for (int i = 0; i < trueSize; i++) {
                            edgeIndices->AddElement2(ids[i], ids[i + trueSize]);
                            edgeIndices->AddElement2(ids[(i + 1) % trueSize], ids[i + trueSize]);
                        }
                        // add triangles
                        triangleIndices->AddElement3(ids[0], ids[trueSize], ids[trueSize * 2 - 1]);
                        triangleEdgeMasks->AddValue(5);
                        for (int j = 1; j < trueSize; j++) {
                            triangleIndices->AddElement3(ids[j], ids[j + trueSize], ids[j + trueSize - 1]);
                            triangleEdgeMasks->AddValue(5);
                        }
                        for (int j = 2; j < trueSize; j++) {
                            triangleIndices->AddElement3(ids[trueSize], ids[trueSize + j - 1], ids[trueSize + j]);
                            triangleEdgeMasks->AddValue(0);
                        }
                    } break;
                    case IG_TETRA:
                    case IG_HEXAHEDRON:
                    case IG_PRISM:
                    case IG_PYRAMID: {
                        Volume* cell = dynamic_cast<Volume*>(GetTypedCell(id));
                        if (cell == nullptr) { break; }
                        const int *edge{}, *face{};
                        // add lines
                        for (int i = 0; i < cell->GetNumberOfEdges(); i++) {
                            cell->GetEdgePointIds(i, edge);
                            edgeIndices->AddElement2(ids[edge[0]], ids[edge[1]]);
                        }
                        // add triangles
                        for (int i = 0; i < cell->GetNumberOfFaces(); i++) {
                            int face_size = cell->GetFacePointIds(i, face);
                            for (int j = 1; j < face_size - 1; j++) {
                                triangleIndices->AddElement3(ids[face[0]], ids[face[j]], ids[face[j + 1]]);
                                // add edge mask
                                int mask = face_size == 3 ? 7 : j == 1 ? 3 : j == face_size - 2 ? 6 : 2;
                                triangleEdgeMasks->AddValue(mask);
                            }
                        }
                    } break;
                    case IG_POLYHEDRON: {
                        igIndex index = 1;
                        igIndex realsize = 0;
                        while (index < size) {
                            realsize = ids[index++];
                            for (igIndex i = 1; i < realsize; i++) {
                                edgeIndices->AddElement2(ids[index + i - 1], ids[index + i]);
                            }
                            for (igIndex i = 1; i < realsize - 1; i++) {
                                triangleIndices->AddElement3(ids[index], ids[index + i], ids[index + i + 1]);
                                // add edge mask
                                int mask = realsize == 3 ? 7 : i == 1 ? 3 : i == realsize - 2 ? 6 : 2;
                                triangleEdgeMasks->AddValue(mask);
                            }
                            index += realsize;
                        }
                    } break;
                    case IG_QUADRATIC_TETRA:
                    case IG_QUADRATIC_HEXAHEDRON:
                    case IG_QUADRATIC_PRISM:
                    case IG_QUADRATIC_PYRAMID: {
                        QuadraticVolume* cell = dynamic_cast<QuadraticVolume*>(GetTypedCell(id));
                        if (cell == nullptr) { break; }
                        const int *edge{}, *face{};
                        for (int i = 0; i < cell->GetNumberOfEdges(); i++) {
                            cell->GetEdgePointIds(i, edge);
                            edgeIndices->AddElement2(ids[edge[0]], ids[edge[2]]);
                            edgeIndices->AddElement2(ids[edge[2]], ids[edge[1]]);
                        }
                        for (int i = 0; i < cell->GetNumberOfFaces(); i++) {
                            int base_face_size = cell->GetFacePointIds(i, face) / 2;
                            triangleIndices->AddElement3(ids[face[0]], ids[face[base_face_size]],
                                                         ids[face[base_face_size * 2 - 1]]);
                            triangleEdgeMasks->AddValue(5);
                            for (int j = 1; j < base_face_size; j++) {
                                triangleIndices->AddElement3(ids[face[j]], ids[face[j + base_face_size]],
                                                             ids[face[j + base_face_size - 1]]);
                                triangleEdgeMasks->AddValue(5);
                            }
                            for (int j = 2; j < base_face_size; j++) {
                                triangleIndices->AddElement3(ids[face[base_face_size]],
                                                             ids[face[base_face_size + j - 1]],
                                                             ids[face[base_face_size + j]]);
                                triangleEdgeMasks->AddValue(0);
                            }
                        }
                    } break;
                    default:
                        break;
                }
            }
            m_Positions = m_Points->ConvertToArray();
            m_Positions->Modified();

            m_PointIndices = pointIndices;
            m_PointIndices->Modified();

            m_LineIndices = edgeIndices;
            m_LineIndices->Modified();

            m_TriangleIndices = triangleIndices;
            m_TriangleIndices->Modified();

            m_TriangleEdgeMasks = triangleEdgeMasks;
            m_TriangleEdgeMasks->Modified();
        }
    }

    // convert scalar data
    if (m_AttributeIndex == -1) {
        m_UseColor = false;
        m_ColorWithCell = false;
    } else {
        m_UseColor = true;

        auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
        if (attr.type == IG_RGB) {
            this->m_ColorMapper->SetVectorModeToRGBColors();
        } else {
            this->m_ColorMapper->SetVectorModeToComponent();
        }
        if (!attr.isDeleted) {
            if (attr.attachmentType == IG_POINT) {
                if (m_AttributeHelper->GetMTime() > m_Colors->GetMTime() ||
                    m_ColorMapper->GetMTime() > m_Colors->GetMTime()) {
                    m_ColorWithCell = false;
                    this->SetAttributeWithPointData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                }

            } else if (attr.attachmentType == IG_CELL) {
                if (m_AttributeHelper->GetMTime() > m_CellColors->GetMTime() ||
                    m_ColorMapper->GetMTime() > m_CellColors->GetMTime()) {
                    m_ColorWithCell = true;
                    this->SetAttributeWithCellData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                }
            }
        }
    }
}

//void UnstructuredMesh::SetDisplayMesh(SurfaceMesh::Pointer& surfaceMesh) {
//    surfaceMesh->GetDrawableArray(m_Positions, m_LineIndices,
//                                  m_TriangleIndices);
//    m_Positions->Modified();
//    m_LineIndices->Modified();
//    m_TriangleIndices->Modified();
//}

//void UnstructuredMesh::ViewCloudPicture(Scene* scene, int index,
//                                        int demension) {
//    if (index == -1) {
//        m_UseColor = false;
//        m_ViewAttribute = nullptr;
//        m_ViewDemension = -1;
//        // m_ColorWithCell = false;
//        scene->Update();
//        return;
//    }
//
//    m_AttributeIndex = index;
//    auto& attr = this->GetAttributeSet()->GetAttribute(index);
//    if (!attr.isDeleted) {
//        if (attr.attachmentType == IG_POINT)
//            this->SetAttributeWithPointData(attr.pointer, attr.dataRange,
//                                            demension);
//        else if (attr.attachmentType == IG_CELL)
//            this->SetAttributeWithCellData(attr.pointer, demension);
//    }
//
//    scene->Update();
//}

//void UnstructuredMesh::SetAttributeWithPointData(ArrayObject::Pointer attr,
//                                                 std::pair<float, float>& range,
//                                                 igIndex dimension) {
//    if (m_ViewAttribute != attr || m_ViewDemension != dimension ||
//        m_ColorMapper->GetMTime() >= this->GetMTime()) {
//        m_ViewAttribute = attr;
//        m_ViewDemension = dimension;
//        m_UseColor = true;
//        m_ColorWithCell = false;
//
//        if (m_ColorMapper->GetMTime() <= this->GetMTime()) {
//            if (range.first != range.second) {
//                m_ColorMapper->SetRange(range.first, range.second);
//            } else if (dimension == -1) {
//                m_ColorMapper->InitRange(attr);
//            } else {
//                m_ColorMapper->InitRange(attr, dimension);
//            }
//        }
//        range.first = m_ColorMapper->GetRange()[0];
//        range.second = m_ColorMapper->GetRange()[1];
//        m_Colors = m_ColorMapper->MapScalars(attr, dimension);
//        m_Colors->Modified();
//        if (m_Colors == nullptr) { return; }
//    }
//}

void UnstructuredMesh::SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                                igIndex dimension) {}
IGAME_NAMESPACE_END
