#include "iGameUnstructuredMesh.h"
#include "ModelSurface/iGameModelGeometryFilter.h"
#include <vector>
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
void UnstructuredMesh::GetCellPointIds(const IGsize cellId, IdArray::Pointer cell) const {
    if (cell == nullptr) { return; }
    m_Cells->GetCellIds(cellId, cell);
}

int UnstructuredMesh::GetCellPointIds(const IGsize cellId, igIndex* cell) const {
    return m_Cells->GetCellIds(cellId, cell);
}

int UnstructuredMesh::GetCellPointIds(const IGsize cellId, const igIndex*& cell) const {
    return m_Cells->GetCellIds(cellId, cell);
}

Cell* UnstructuredMesh::GetCell(const IGsize cellId) {
    Cell* cell = GetTypedCell(cellId);
    auto result = _GetCell(cellId, cell);
    if (!result) return nullptr;
    return cell;
}

bool UnstructuredMesh::GetCell(const IGsize cellId, Cell::Pointer& cell) {
    GetTypedCell(cellId, cell);
    return _GetCell(cellId, cell);
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
    if (CouldTransfer == false) {
        return nullptr;
    }
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
    this->SetSelection(mesh->GetSelection());
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
        this->SetSelection(mesh->GetSelection());
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
                    igError("Not support this volume with ", vcnt, "'s verts.");
                    return false;
            }
        }
        this->SetPoints(mesh->GetPoints());
        this->SetCells(mesh->GetCells(), CellTypes);
        this->SetAttributeSet(mesh->GetAttributeSet());
        this->SetSelection(mesh->GetSelection());
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
    UnstructuredMesh::Pointer re = nullptr;
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

bool UnstructuredMesh::_GetCell(const IGsize cellId, Cell* cell) const {
    if (cell == nullptr) { return false; }
    cell->Reset();
    if (cell->GetCellType() != IG_POLYHEDRON) {
        GetCellPointIds(cellId, cell->m_PointIds);
        for (int i = 0; i < cell->m_PointIds->GetNumberOfIds(); i++) {
            cell->m_Points->AddPoint(GetPoint(cell->m_PointIds->GetId(i)));
        }
    } else {
        // 多面体单元的 id 列表可能超过 IGAME_CELL_MAX_SIZE(256)，改用指针版避免固定数组溢出
        const igIndex* ids = nullptr;
        igIndex size = static_cast<igIndex>(m_Cells->GetCellIds(cellId, ids));
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
    return true;
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

void UnstructuredMesh::GetTypedCell(const IGsize cellId, Cell::Pointer& cell) const {
    if (cell != nullptr && cell->GetCellType() == GetCellType(cellId)) return;
    switch (GetCellType(cellId)) {
        case IG_LINE: {
            cell = Line::New();
        } break;
        case IG_POLY_LINE: {
            cell = PolyLine::New();
        } break;
        case IG_TRIANGLE: {
            cell = Triangle::New();
        } break;
        case IG_QUAD: {
            cell = Quad::New();
        } break;
        case IG_POLYGON: {
            cell = Polygon::New();
        } break;
        case IG_TETRA: {
            cell = Tetra::New();
        } break;
        case IG_HEXAHEDRON: {
            cell = Hexahedron::New();
        } break;
        case IG_PRISM: {
            cell = Prism::New();
        } break;
        case IG_PYRAMID: {
            cell = Pyramid::New();
        } break;
        case IG_POLYHEDRON: {
            cell = Polyhedron::New();
        } break;
        case IG_QUADRATIC_EDGE: {
            cell = QuadraticLine::New();
        } break;
        case IG_QUADRATIC_TRIANGLE: {
            cell = QuadraticTriangle::New();
        } break;
        case IG_QUADRATIC_QUAD: {
            cell = QuadraticQuad::New();
        } break;
        case IG_QUADRATIC_TETRA: {
            cell = QuadraticTetra::New();
        } break;
        case IG_QUADRATIC_HEXAHEDRON: {
            cell = QuadraticHexahedron::New();
        } break;
        case IG_QUADRATIC_PRISM: {
            cell = QuadraticPrism::New();
        } break;
        case IG_QUADRATIC_PYRAMID: {
            cell = QuadraticPyramid::New();
        } break;
        default: {
            cell = EmptyCell::New();
        } break;
    }
}

void UnstructuredMesh::ConvertToDrawableData() {
    bool needReConvertGeometry = m_ReConvertToDrawableData;
    needReConvertGeometry |= m_Points->GetMTime() > m_ReConvertHelper->GetMTime();
    needReConvertGeometry |= m_Clipper->GetMTime() > m_ReConvertHelper->GetMTime();

    bool needReConvertScalar = needReConvertGeometry;
    needReConvertScalar |= m_AttributeHelper->GetMTime() > m_ReConvertHelper->GetMTime();

    // extract surface mesh
    if (m_ShellRendering) {
        if (!needReConvertGeometry && !needReConvertScalar) { return; }

        ModelGeometryFilter::Pointer extract = ModelGeometryFilter::New();
        {
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
                m_ReConvertToDrawableData = false;
                m_ReConvertHelper->Modified();
                return;
            } else {
                m_ShellRendering = false;
                this->m_RenderableMesh.SurfaceMesh = nullptr;
                this->m_RenderableMesh.SimplifiedMesh = nullptr;
                igDebug("Failed to execute the shell algorithm.");
            }
        }
    }

    // convert original data
    if (needReConvertGeometry) {
        auto pointIndices = UnsignedIntArray::New();
        pointIndices->SetDimension(1);
        auto edgeIndices = UnsignedIntArray::New();
        edgeIndices->SetDimension(2);
        auto triangleIndices = UnsignedIntArray::New();
        triangleIndices->SetDimension(3);
        auto triangleEdgeMasks = UnsignedCharArray::New();
        triangleEdgeMasks->SetDimension(1);

        int skippedInvalidCells = 0;
        for (int id = 0; id < GetNumberOfCells(); id++) {
            int sizeHint = static_cast<int>(m_Cells->GetCellSize(id));
            if (sizeHint <= 0) {
                skippedInvalidCells++;
                continue;
            }

            std::vector<igIndex> cellIds(sizeHint);
            int size = GetCellPointIds(id, cellIds.data());
            if (size <= 0) {
                skippedInvalidCells++;
                continue;
            }

            const igIndex* ids = cellIds.data();
            bool hasInvalidPointId = false;
            const IGsize pointCount = GetNumberOfPoints();
            for (int i = 0; i < size; i++) {
                if (ids[i] < 0 || static_cast<IGsize>(ids[i]) >= pointCount) {
                    hasInvalidPointId = true;
                    break;
                }
            }
            if (hasInvalidPointId) {
                skippedInvalidCells++;
                continue;
            }

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
                            triangleIndices->AddElement3(ids[face[base_face_size]], ids[face[base_face_size + j - 1]],
                                                         ids[face[base_face_size + j]]);
                            triangleEdgeMasks->AddValue(0);
                        }
                    }
                } break;
                default:
                    break;
            }
        }
        if (skippedInvalidCells > 0) {
            igDebug("UnstructuredMesh::ConvertToDrawableData skipped invalid cells: {}", skippedInvalidCells);
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

    // convert scalar data
    bool updateColorMapper = m_ColorMapper->GetMTime() > m_ReConvertHelper->GetMTime();


    // Debug info
    //    if(m_AttributeIndex != -1){
    //        auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
    //
    //        std::cout << "Unstructured Mesh : " << this << " color Mapper : " << m_ColorMapper << " dimension " << m_AttributeDimension << ' '
    //                  << " Color Map Range : " << m_ColorMapper->GetRange()[0] << ' ' << m_ColorMapper->GetRange()[1]
    //                  << " Data Range : " <<  attr.GetDataRange()->GetValue(2 + m_AttributeDimension * 2 + 0) << ' '  << attr.GetDataRange()->GetValue(2 + m_AttributeDimension * 2 + 1) << std::endl;
    //    }
    if (needReConvertScalar || m_AttributeChanged || updateColorMapper) {
        m_AttributeChanged = false;
        if (m_AttributeIndex != -1) {
            auto& attr = this->GetAttributeSet()->GetAttribute(m_AttributeIndex);
            if (attr.type == IG_RGB) {
                this->m_ColorMapper->SetVectorModeToRGBColors();
            } else {
                this->m_ColorMapper->SetVectorModeToComponent();
            }

            //            if(updateColorMapper){
            //                std::cout << m_ColorMapper << " dimension " << m_AttributeDimension << ' '
            //                          << " Color Map Range : " << m_ColorMapper->GetRange()[0] << ' ' << m_ColorMapper->GetRange()[1]
            //                          << " Data Range : " <<  attr.GetDataRange()->GetValue(2 + m_AttributeDimension * 2 + 0) << ' '  << attr.GetDataRange()->GetValue(2 + m_AttributeDimension * 2 + 1) << std::endl;
            //            }

            // m_AttributeHelper : 调用DrawObject::ViewCloudPicture时更新(Modified)
            // m_ColorMapper     : 外部ScalarView更新时(igQtScalarViewWidget::showScalarView)更新
            if (!attr.isDeleted) {
                if (attr.attachmentType == IG_POINT) {
                    m_ColorWithCell = false;
                    this->SetAttributeWithPointData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                } else if (attr.attachmentType == IG_CELL) {
                    m_ColorWithCell = true;
                    this->SetAttributeWithCellData(attr.pointer, attr.GetDataRange(), m_AttributeDimension);
                }
            }
        }
    }

    m_ReConvertToDrawableData = false;
    m_ReConvertHelper->Modified();
}

void UnstructuredMesh::SetAttributeWithCellData(ArrayObject::Pointer attr, DoubleArray::Pointer attrRange,
                                                igIndex dimension) {
    /* 当pointMapper 外部更新（调整颜色映射的 Range）， 则不用调整ColorMap的范围*/
    if (!m_ColorMapper->GetStable() && m_ColorMapper->GetMTime() <= attrRange->GetMTime()) {
        // Configure color mapper range using provided attrRange if available; otherwise initialize from data
        double minimal_val = attrRange ? attrRange->GetValue(2 + dimension * 2 + 0) : 0.0;
        double maximal_val = attrRange ? attrRange->GetValue(2 + dimension * 2 + 1) : 0.0;
        if (attrRange && minimal_val < maximal_val) {
            m_ColorMapper->SetRange(minimal_val, maximal_val);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
    }
    FloatArray::Pointer colors = m_ColorMapper->MapScalars(attr, dimension, 4);
    if (colors == nullptr) { return; }

    FloatArray::Pointer newPositions = FloatArray::New();
    FloatArray::Pointer newColors = FloatArray::New();
    UnsignedCharArray::Pointer newEdgeMasks = UnsignedCharArray::New();
    newPositions->SetDimension(3);
    newColors->SetDimension(4);
    newEdgeMasks->SetDimension(3);

    float color[4]{};
    igIndex ids[IGAME_CELL_MAX_SIZE]{};

    const IGsize nCells = this->GetNumberOfCells();
    for (IGsize cid = 0; cid < nCells; ++cid) {
        const int size = this->GetCellPointIds(cid, ids);
        colors->GetElement(cid, color);

        const IGenum type = this->GetCellType(cid);
        switch (type) {
            case IG_VERTEX:
            case IG_LINE:
            case IG_POLY_LINE:
            case IG_QUADRATIC_EDGE: {
                // No triangle surface to color for 0D/1D cells.
            } break;
            case IG_TRIANGLE:
            case IG_QUAD:
            case IG_POLYGON: {
                // Fan triangulation from vertex 0
                for (int j = 1; j < size - 1; ++j) {
                    const auto& p0 = this->GetPoint(ids[0]);
                    const auto& p1 = this->GetPoint(ids[j]);
                    const auto& p2 = this->GetPoint(ids[j + 1]);

                    newPositions->AddElement3(p0[0], p0[1], p0[2]);
                    newPositions->AddElement3(p1[0], p1[1], p1[2]);
                    newPositions->AddElement3(p2[0], p2[1], p2[2]);

                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);

                    int mask = size == 3 ? 7 : j == 1 ? 3 : j == size - 2 ? 6 : 2;
                    newEdgeMasks->AddValue(mask);
                }
            } break;
            case IG_QUADRATIC_TRIANGLE:
            case IG_QUADRATIC_QUAD: {
                // Follow the same subdivision pattern as in ConvertToDrawableData
                const int trueSize = size / 2;
                // First triangle
                {
                    const auto& p0 = this->GetPoint(ids[0]);
                    const auto& p1 = this->GetPoint(ids[trueSize]);
                    const auto& p2 = this->GetPoint(ids[trueSize * 2 - 1]);
                    newPositions->AddElement3(p0[0], p0[1], p0[2]);
                    newPositions->AddElement3(p1[0], p1[1], p1[2]);
                    newPositions->AddElement3(p2[0], p2[1], p2[2]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newEdgeMasks->AddValue(5);
                }
                for (int j = 1; j < trueSize; ++j) {
                    const auto& p0 = this->GetPoint(ids[j]);
                    const auto& p1 = this->GetPoint(ids[j + trueSize]);
                    const auto& p2 = this->GetPoint(ids[j + trueSize - 1]);
                    newPositions->AddElement3(p0[0], p0[1], p0[2]);
                    newPositions->AddElement3(p1[0], p1[1], p1[2]);
                    newPositions->AddElement3(p2[0], p2[1], p2[2]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newEdgeMasks->AddValue(5);
                }
                for (int j = 2; j < trueSize; ++j) {
                    const auto& p0 = this->GetPoint(ids[trueSize]);
                    const auto& p1 = this->GetPoint(ids[trueSize + j - 1]);
                    const auto& p2 = this->GetPoint(ids[trueSize + j]);
                    newPositions->AddElement3(p0[0], p0[1], p0[2]);
                    newPositions->AddElement3(p1[0], p1[1], p1[2]);
                    newPositions->AddElement3(p2[0], p2[1], p2[2]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newColors->AddElement4(color[0], color[1], color[2], color[3]);
                    newEdgeMasks->AddValue(0);
                }
            } break;
            case IG_TETRA:
            case IG_HEXAHEDRON:
            case IG_PRISM:
            case IG_PYRAMID: {
                Volume* cell = dynamic_cast<Volume*>(GetTypedCell(cid));
                if (cell == nullptr) { break; }
                const int* face = nullptr;
                for (int f = 0; f < cell->GetNumberOfFaces(); ++f) {
                    int fsz = cell->GetFacePointIds(f, face);
                    for (int k = 1; k < fsz - 1; ++k) {
                        const auto& p0 = this->GetPoint(ids[face[0]]);
                        const auto& p1 = this->GetPoint(ids[face[k]]);
                        const auto& p2 = this->GetPoint(ids[face[k + 1]]);
                        newPositions->AddElement3(p0[0], p0[1], p0[2]);
                        newPositions->AddElement3(p1[0], p1[1], p1[2]);
                        newPositions->AddElement3(p2[0], p2[1], p2[2]);

                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);

                        int mask = fsz == 3 ? 7 : k == 1 ? 3 : k == fsz - 2 ? 6 : 2;
                        newEdgeMasks->AddValue(mask);
                    }
                }
            } break;
            case IG_POLYHEDRON: {
                igIndex index = 1;
                while (index < size) {
                    igIndex realsize = ids[index++];
                    for (igIndex i = 1; i < realsize - 1; ++i) {
                        const auto& p0 = this->GetPoint(ids[index + 0]);
                        const auto& p1 = this->GetPoint(ids[index + i]);
                        const auto& p2 = this->GetPoint(ids[index + i + 1]);
                        newPositions->AddElement3(p0[0], p0[1], p0[2]);
                        newPositions->AddElement3(p1[0], p1[1], p1[2]);
                        newPositions->AddElement3(p2[0], p2[1], p2[2]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        int mask = realsize == 3 ? 7 : i == 1 ? 3 : i == realsize - 2 ? 6 : 2;
                        newEdgeMasks->AddValue(mask);
                    }
                    index += realsize;
                }
            } break;
            case IG_QUADRATIC_TETRA:
            case IG_QUADRATIC_HEXAHEDRON:
            case IG_QUADRATIC_PRISM:
            case IG_QUADRATIC_PYRAMID: {
                QuadraticVolume* cell = dynamic_cast<QuadraticVolume*>(GetTypedCell(cid));
                if (cell == nullptr) { break; }
                const int* face = nullptr;
                for (int f = 0; f < cell->GetNumberOfFaces(); ++f) {
                    int face_size = cell->GetFacePointIds(f, face);
                    int base_face_size = face_size / 2;
                    // First triangle of the face
                    {
                        const auto& p0 = this->GetPoint(ids[face[0]]);
                        const auto& p1 = this->GetPoint(ids[face[base_face_size]]);
                        const auto& p2 = this->GetPoint(ids[face[base_face_size * 2 - 1]]);
                        newPositions->AddElement3(p0[0], p0[1], p0[2]);
                        newPositions->AddElement3(p1[0], p1[1], p1[2]);
                        newPositions->AddElement3(p2[0], p2[1], p2[2]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newEdgeMasks->AddValue(5);
                    }
                    for (int j = 1; j < base_face_size; ++j) {
                        const auto& p0 = this->GetPoint(ids[face[j]]);
                        const auto& p1 = this->GetPoint(ids[face[j + base_face_size]]);
                        const auto& p2 = this->GetPoint(ids[face[j + base_face_size - 1]]);
                        newPositions->AddElement3(p0[0], p0[1], p0[2]);
                        newPositions->AddElement3(p1[0], p1[1], p1[2]);
                        newPositions->AddElement3(p2[0], p2[1], p2[2]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newEdgeMasks->AddValue(5);
                    }
                    for (int j = 2; j < base_face_size; ++j) {
                        const auto& p0 = this->GetPoint(ids[face[base_face_size]]);
                        const auto& p1 = this->GetPoint(ids[face[base_face_size + j - 1]]);
                        const auto& p2 = this->GetPoint(ids[face[base_face_size + j]]);
                        newPositions->AddElement3(p0[0], p0[1], p0[2]);
                        newPositions->AddElement3(p1[0], p1[1], p1[2]);
                        newPositions->AddElement3(p2[0], p2[1], p2[2]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newColors->AddElement4(color[0], color[1], color[2], color[3]);
                        newEdgeMasks->AddValue(0);
                    }
                }
            } break;
            default:
                break;
        }
    }

    m_CellPositionSize = newPositions->GetNumberOfElements();

    m_CellPositions = newPositions;
    m_CellPositions->Modified();

    m_CellColors = newColors;
    m_CellColors->Modified();

    m_CellTriangleEdgeMasks = newEdgeMasks;
    m_CellTriangleEdgeMasks->Modified();
}
IGAME_NAMESPACE_END
