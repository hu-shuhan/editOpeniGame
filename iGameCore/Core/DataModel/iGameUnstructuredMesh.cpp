#include "iGameUnstructuredMesh.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"
IGAME_NAMESPACE_BEGIN
void UnstructuredMesh::SetCells(CellArray::Pointer cell,
                                UnsignedIntArray::Pointer type) {
    m_Cells = cell;
    m_Types = type;
    this->Modified();
}

void UnstructuredMesh::AddCell(igIndex* cell, int size, IGenum type) {
    m_Cells->AddCellIds(cell, size);
    m_Types->AddValue(type);
    this->Modified();
}

IGsize UnstructuredMesh::GetNumberOfCells() const noexcept {
    return m_Cells->GetNumberOfCells();
}
void UnstructuredMesh::GetCellPointIds(const IGsize cellId,
                                       IdArray::Pointer cell) {
    if (cell == nullptr) { return; }
    m_Cells->GetCellIds(cellId, cell);
}

int UnstructuredMesh::GetCellPointIds(const IGsize cellId, igIndex* cell) {
    return m_Cells->GetCellIds(cellId, cell);
}

int UnstructuredMesh::GetCellPointIds(const IGsize cellId,
                                      const igIndex*& cell) {
    return m_Cells->GetCellIds(cellId, cell);
}

Cell* UnstructuredMesh::GetCell(const IGsize cellId) {
    Cell* cell = GetTypedCell(cellId);
    if (cell == nullptr) { return nullptr; }
    if (cell->GetCellType() != IG_POLYHEDRON) {
        GetCellPointIds(cellId, cell->PointIds);
        for (int i = 0; i < cell->PointIds->GetNumberOfIds(); i++) {
            cell->Points->AddPoint(GetPoint(cell->PointIds->GetId(i)));
        }
    } else {
        igIndex ids[IGAME_CELL_MAX_SIZE];
        igIndex size = m_Cells->GetCellIds(cellId, ids);
        Polyhedron::Pointer polyhedron = DynamicCast<Polyhedron>(cell);
        polyhedron->m_FaceOffset->Reserve(ids[0]);
        polyhedron->PointIds->Reserve(size);

        igIndex index = 1;
        igIndex realsize = 0;
        int offset = 0;
        while (index < size) {
            polyhedron->m_FaceOffset->AddId(offset);
            realsize = ids[index++];
            for (igIndex id = 0; id < realsize; id++) {
                polyhedron->PointIds->AddId(ids[index++]);
            }
            offset += realsize;
        }
        polyhedron->m_FaceOffset->AddId(offset);
        for (int i = 0; i < cell->PointIds->GetNumberOfIds(); i++) {
            cell->Points->AddPoint(GetPoint(cell->PointIds->GetId(i)));
        }
    }
    return cell;
}
UnsignedIntArray* UnstructuredMesh::GetCellTypes() const { return m_Types; }

IGenum UnstructuredMesh::GetCellType(const IGsize cellId) const {
    return m_Types->GetValue(cellId);
}

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

bool UnstructuredMesh::GenerateFromVolumeMesh(VolumeMesh::Pointer mesh) {
    if (!mesh) return false;
    int volumeNum = mesh->GetNumberOfVolumes();
    auto Volumes = mesh->GetVolumes();
    UnsignedIntArray::Pointer CellTypes = UnsignedIntArray::New();
    CellTypes->Reserve(volumeNum);
    igIndex vcnt = 0;
    igIndex vhs[IGAME_CELL_MAX_SIZE];
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
    return true;
}


bool UnstructuredMesh::TransferVolumeMeshToUnstructuredMesh(
        VolumeMesh::Pointer input, UnstructuredMesh::Pointer& output) {
    if (!output) { output = UnstructuredMesh::New(); }
    return output->GenerateFromVolumeMesh(input);
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
            if (m_QuadraticLine == nullptr) {
                m_QuadraticLine = QuadraticLine::New();
            }
            cell = m_QuadraticLine.get();
        } break;
        case IG_QUADRATIC_TRIANGLE: {
            if (m_QuadraticTriangle == nullptr) {
                m_QuadraticTriangle = QuadraticTriangle::New();
            }
            cell = m_QuadraticTriangle.get();
        } break;
        case IG_QUADRATIC_QUAD: {
            if (m_QuadraticQuad == nullptr) {
                m_QuadraticQuad = QuadraticQuad::New();
            }
            cell = m_QuadraticQuad.get();
        } break;
        case IG_QUADRATIC_TETRA: {
            if (m_QuadraticTetra == nullptr) {
                m_QuadraticTetra = QuadraticTetra::New();
            }
            cell = m_QuadraticTetra.get();
        } break;
        case IG_QUADRATIC_HEXAHEDRON: {
            if (m_QuadraticHexahedron == nullptr) {
                m_QuadraticHexahedron = QuadraticHexahedron::New();
            }
            cell = m_QuadraticHexahedron.get();
        } break;
        case IG_QUADRATIC_PRISM: {
            if (m_QuadraticPrism == nullptr) {
                m_QuadraticPrism = QuadraticPrism::New();
            }
            cell = m_QuadraticPrism.get();
        } break;
        case IG_QUADRATIC_PYRAMID: {
            if (m_QuadraticPyramid == nullptr) {
                m_QuadraticPyramid = QuadraticPyramid::New();
            }
            cell = m_QuadraticPyramid.get();
        } break;
        default: {
            if (m_EmptyCell == nullptr) { m_EmptyCell = EmptyCell::New(); }
            cell = m_EmptyCell.get();
        } break;
    }
    return cell;
}

//void UnstructuredMesh::Draw(Scene* scene) {
//    if (!m_Visibility) { return; }
//    if (m_DrawMesh) {
//        m_DrawMesh->SetViewStyle(m_ViewStyle);
//        return m_DrawMesh->Draw(scene);
//    }
//    // Update uniform buffer
//    if (m_UseColor) {
//        scene->UBO().useColor = true;
//    } else {
//        scene->UBO().useColor = false;
//    }
//    scene->UpdateUniformBuffer();
//
//    if (m_ViewStyle & IG_POINTS) {
//        scene->GetShader(Scene::NOLIGHT)->use();
//
//        m_PointVAO.bind();
//        glPointSize(m_PointSize);
//        glad_glDepthRange(0.000001, 1);
//        glad_glDrawArrays(GL_POINTS, 0, m_Positions->GetNumberOfValues() / 3);
//        glad_glDepthRange(0, 1);
//        m_PointVAO.release();
//    }
//    if (m_ViewStyle & IG_WIREFRAME) {
//        if (m_UseColor) {
//            scene->GetShader(Scene::NOLIGHT)->use();
//        } else {
//            auto shader = scene->GetShader(Scene::PURECOLOR);
//            shader->use();
//            shader->setUniform(shader->getUniformLocation("inputColor"),
//                               igm::vec3{0.0f, 0.0f, 0.0f});
//        }
//
//        m_LineVAO.bind();
//        glLineWidth(m_LineWidth);
//        glad_glDrawElements(GL_LINES, M_LineIndices->GetNumberOfValues(),
//                            GL_UNSIGNED_INT, 0);
//        m_LineVAO.release();
//    }
//    if (m_ViewStyle & IG_SURFACE) {
//        scene->GetShader(Scene::BLINNPHONG)->use();
//
//        m_TriangleVAO.bind();
//        glEnable(GL_POLYGON_OFFSET_FILL);
//        glPolygonOffset(-0.5f, -0.5f);
//        glad_glDrawElements(GL_TRIANGLES,
//                            M_TriangleIndices->GetNumberOfValues(),
//                            GL_UNSIGNED_INT, 0);
//        glDisable(GL_POLYGON_OFFSET_FILL);
//        m_TriangleVAO.release();
//
//        m_VertexVAO.bind();
//        glPointSize(m_PointSize);
//        glad_glDrawElements(GL_POINTS, M_VertexIndices->GetNumberOfValues(),
//                            GL_UNSIGNED_INT, 0);
//        m_PointVAO.release();
//    }
//    /*if (m_ViewStyle == IG_SURFACE_WITH_EDGE) {
//		if (m_UseColor) {
//			scene->GetShader(Scene::NOLIGHT)->use();
//		} else {
//			auto shader = scene->GetShader(Scene::PURECOLOR);
//			shader->use();
//			shader->setUniform(shader->getUniformLocation("inputColor"),
//							   igm::vec3{0.0f, 0.0f, 0.0f});
//		}
//
//		m_LineVAO.bind();
//		glLineWidth(m_LineWidth);
//		glad_glDrawElements(GL_LINES, M_LineIndices->GetNumberOfValues(),
//							GL_UNSIGNED_INT, 0);
//		m_LineVAO.release();
//
//		scene->GetShader(Scene::PATCH)->use();
//		m_TriangleVAO.bind();
//		glad_glDrawElements(GL_TRIANGLES,
//							M_TriangleIndices->GetNumberOfValues(),
//							GL_UNSIGNED_INT, 0);
//		m_TriangleVAO.release();
//	}*/
//}

void UnstructuredMesh::ConvertToDrawableData() {
    this->Create();

    if (m_Positions && m_Positions->GetMTime() > this->GetMTime()) { return; }

    //if (m_TransparencyChanged && m_Transparency == 1.0f) {
    //    m_TransparencyChanged = false;
    //
    //    if (m_DrawMesh == nullptr ||
    //        m_DrawMesh->GetMTime() < this->GetMTime()) {
    //        iGameModelGeometryFilter::Pointer extract =
    //                iGameModelGeometryFilter::New();
    //        // update clip status
    //        if (m_Clip.m_Extent.m_Use) {
    //            const auto& a = m_Clip.m_Extent.m_bmin;
    //            const auto& b = m_Clip.m_Extent.m_bmax;
    //            extract->SetExtent(a[0], b[0], a[1], b[1], a[2], b[2],
    //                               m_Clip.m_Extent.m_flip);
    //        }
    //        if (m_Clip.m_Plane.m_Use) {
    //            extract->SetClipPlane(m_Clip.m_Plane.m_origin,
    //                                  m_Clip.m_Plane.m_normal,
    //                                  m_Clip.m_Plane.m_flip);
    //        }
    //        m_DrawMesh = SurfaceMesh::New();
    //        if (!extract->Execute(this, m_DrawMesh)) { m_DrawMesh = nullptr; }
    //        if (m_DrawMesh) { m_DrawMesh->Modified(); }
    //    }
    //    if (m_DrawMesh) { return m_DrawMesh->ConvertToDrawableData(); }
    //}

    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();
    //M_VertexIndices = UnsignedIntArray::New();
    //M_LineIndices = UnsignedIntArray::New();
    //M_TriangleIndices = UnsignedIntArray::New();
    //M_LineIndices->SetDimension(2);
    //M_TriangleIndices->SetDimension(3);

    IdArray::Pointer pointIndices = IdArray::New();
    IdArray::Pointer edgeIndices = IdArray::New();
    IdArray::Pointer triangleIndices = IdArray::New();

    igIndex ids[128]{};
    for (int id = 0; id < GetNumberOfCells(); id++) {
        int size = GetCellPointIds(id, ids);
        IGenum type = GetCellType(id);
        switch (type) {
            case IG_VERTEX:
                //M_VertexIndices->AddValue(ids[0]);
                pointIndices->AddId(ids[0]);
                break;
            case IG_LINE:
            case IG_POLY_LINE: {
                for (int i = 1; i < size; i++) {
                    //M_LineIndices->AddElement2(ids[i - 1], ids[i]);
                    edgeIndices->AddId(ids[i - 1]);
                    edgeIndices->AddId(ids[i]);
                }
            } break;
            case IG_QUADRATIC_EDGE: {
                //M_LineIndices->AddElement2(ids[0], ids[2]);
                //M_LineIndices->AddElement2(ids[2], ids[1]);
                edgeIndices->AddId(ids[0]);
                edgeIndices->AddId(ids[2]);
                edgeIndices->AddId(ids[2]);
                edgeIndices->AddId(ids[1]);
            } break;
            case IG_TRIANGLE:
            case IG_QUAD:
            case IG_POLYGON: {
                for (int i = 2; i < size; i++) {
                    //M_TriangleIndices->AddElement3(ids[0], ids[i - 1], ids[i]);
                    triangleIndices->AddId(ids[0]);
                    triangleIndices->AddId(ids[i - 1]);
                    triangleIndices->AddId(ids[i]);
                }
                for (int i = 0; i < size; i++) {
                    //M_LineIndices->AddElement2(ids[i], ids[(i + 1) % size]);
                    edgeIndices->AddId(ids[i]);
                    edgeIndices->AddId(ids[(i + 1) % size]);
                }
            } break;
            case IG_QUADRATIC_TRIANGLE:
            case IG_QUADRATIC_QUAD: {
                int trueSize = size / 2;
                //M_TriangleIndices->AddElement3(ids[0], ids[trueSize],
                //                               ids[trueSize * 2 - 1]);
                triangleIndices->AddId(ids[0]);
                triangleIndices->AddId(ids[trueSize]);
                triangleIndices->AddId(ids[trueSize * 2 - 1]);
                for (int j = 1; j < trueSize; j++) {
                    //M_TriangleIndices->AddElement3(ids[j], ids[j + trueSize],
                    //                               ids[j + trueSize - 1]);
                    triangleIndices->AddId(ids[j]);
                    triangleIndices->AddId(ids[j + trueSize]);
                    triangleIndices->AddId(ids[j + trueSize - 1]);
                }
                for (int j = 2; j < trueSize; j++) {
                    //M_TriangleIndices->AddElement3(ids[trueSize],
                    //                               ids[trueSize + j - 1],
                    //                               ids[trueSize + j]);
                    triangleIndices->AddId(ids[trueSize]);
                    triangleIndices->AddId(ids[trueSize + j - 1]);
                    triangleIndices->AddId(ids[trueSize + j]);
                }
                for (int i = 0; i < trueSize; i++) {
                    //M_LineIndices->AddElement2(ids[i], ids[i + trueSize]);
                    //M_LineIndices->AddElement2(ids[(i + 1) % trueSize],
                    //                           ids[i + trueSize]);
                    edgeIndices->AddId(ids[i]);
                    edgeIndices->AddId(ids[i + trueSize]);
                    edgeIndices->AddId(ids[(i + 1) % trueSize]);
                    edgeIndices->AddId(ids[i + trueSize]);
                }
            } break;
            case IG_TETRA:
            case IG_HEXAHEDRON:
            case IG_PRISM:
            case IG_PYRAMID: {
                Volume* cell = dynamic_cast<Volume*>(GetTypedCell(id));
                if (cell == nullptr) { break; }
                const int *edge{}, *face{};
                for (int i = 0; i < cell->GetNumberOfEdges(); i++) {
                    cell->GetEdgePointIds(i, edge);
                    //M_LineIndices->AddElement2(ids[edge[0]], ids[edge[1]]);
                    edgeIndices->AddId(ids[edge[0]]);
                    edgeIndices->AddId(ids[edge[1]]);
                }
                for (int i = 0; i < cell->GetNumberOfFaces(); i++) {
                    int face_size = cell->GetFacePointIds(i, face);
                    for (int j = 2; j < face_size; j++) {
                        //M_TriangleIndices->AddElement3(
                        //        ids[face[0]], ids[face[j - 1]], ids[face[j]]);
                        triangleIndices->AddId(ids[face[0]]);
                        triangleIndices->AddId(ids[face[j - 1]]);
                        triangleIndices->AddId(ids[face[j]]);
                    }
                }
            } break;
            case IG_POLYHEDRON: {
                igIndex index = 1;
                igIndex realsize = 0;
                while (index < size) {
                    realsize = ids[index++];
                    for (igIndex id = 1; id < realsize; id++) {
                        //M_LineIndices->AddElement2(ids[index + id - 1],
                        //                           ids[index + id]);
                        edgeIndices->AddId(ids[index + id - 1]);
                        edgeIndices->AddId(ids[index + id]);
                    }
                    for (igIndex id = 2; id < realsize; id++) {
                        //M_TriangleIndices->AddElement3(ids[index],
                        //                               ids[index + id - 1],
                        //                               ids[index + id]);
                        triangleIndices->AddId(ids[index]);
                        triangleIndices->AddId(ids[index + id - 1]);
                        triangleIndices->AddId(ids[index + id]);
                    }
                    index += realsize;
                }
            } break;
            case IG_QUADRATIC_TETRA:
            case IG_QUADRATIC_HEXAHEDRON:
            case IG_QUADRATIC_PRISM:
            case IG_QUADRATIC_PYRAMID: {
                QuadraticVolume* cell =
                        dynamic_cast<QuadraticVolume*>(GetTypedCell(id));
                if (cell == nullptr) { break; }
                const int *edge{}, *face{};
                for (int i = 0; i < cell->GetNumberOfEdges(); i++) {
                    cell->GetEdgePointIds(i, edge);
                    //M_LineIndices->AddElement2(ids[edge[0]], ids[edge[2]]);
                    //M_LineIndices->AddElement2(ids[edge[2]], ids[edge[1]]);
                    edgeIndices->AddId(ids[0]);
                    edgeIndices->AddId(ids[2]);
                    edgeIndices->AddId(ids[2]);
                    edgeIndices->AddId(ids[1]);
                }
                for (int i = 0; i < cell->GetNumberOfFaces(); i++) {
                    int base_face_size = cell->GetFacePointIds(i, face) / 2;
                    //M_TriangleIndices->AddElement3(
                    //        ids[face[0]], ids[face[base_face_size]],
                    //        ids[face[base_face_size * 2 - 1]]);
                    triangleIndices->AddId(ids[face[0]]);
                    triangleIndices->AddId(ids[face[base_face_size]]);
                    triangleIndices->AddId(ids[face[base_face_size * 2 - 1]]);
                    for (int j = 1; j < base_face_size; j++) {
                        //M_TriangleIndices->AddElement3(
                        //        ids[face[j]], ids[face[j + base_face_size]],
                        //        ids[face[j + base_face_size - 1]]);
                        triangleIndices->AddId(ids[face[j]]);
                        triangleIndices->AddId(ids[face[j + base_face_size]]);
                        triangleIndices->AddId(
                                ids[face[j + base_face_size - 1]]);
                    }
                    for (int j = 2; j < base_face_size; j++) {
                        //M_TriangleIndices->AddElement3(
                        //        ids[face[base_face_size]],
                        //        ids[face[base_face_size + j - 1]],
                        //        ids[face[base_face_size + j]]);
                        triangleIndices->AddId(ids[face[base_face_size]]);
                        triangleIndices->AddId(
                                ids[face[base_face_size + j - 1]]);
                        triangleIndices->AddId(ids[face[base_face_size + j]]);
                    }
                }
            } break;
            default:
                break;
        }
    }

    m_PointIndices = pointIndices;
    m_LineIndices = edgeIndices;
    m_TriangleIndices = triangleIndices;

    GLAllocateGLBuffer(m_PositionVBO,
                       m_Positions->GetNumberOfValues() * sizeof(float),
                       m_Positions->RawPointer());

    //GLAllocateGLBuffer(m_VertexEBO,
    //                   M_VertexIndices->GetNumberOfValues() *
    //                           sizeof(unsigned int),
    //                   M_VertexIndices->RawPointer());

    GLAllocateGLBuffer(m_PointEBO,
                       m_PointIndices->GetNumberOfIds() * sizeof(igIndex),
                       m_PointIndices->RawPointer());

    //GLAllocateGLBuffer(m_LineEBO,
    //                   M_LineIndices->GetNumberOfValues() *
    //                           sizeof(unsigned int),
    //                   M_LineIndices->RawPointer());

    GLAllocateGLBuffer(m_LineEBO,
                       m_LineIndices->GetNumberOfIds() * sizeof(igIndex),
                       m_LineIndices->RawPointer());

    //GLAllocateGLBuffer(m_TriangleEBO,
    //                   M_TriangleIndices->GetNumberOfValues() *
    //                           sizeof(unsigned int),
    //                   M_TriangleIndices->RawPointer());

    GLAllocateGLBuffer(m_TriangleEBO,
                       m_TriangleIndices->GetNumberOfIds() * sizeof(igIndex),
                       m_TriangleIndices->RawPointer());

    m_PointVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);

    m_VertexVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_VertexVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
    m_VertexVAO.elementBuffer(m_VertexEBO);

    m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
    m_LineVAO.elementBuffer(m_LineEBO);

    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                               3 * sizeof(float));
    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                      GL_FLOAT, GL_FALSE, 0);
    m_TriangleVAO.elementBuffer(m_TriangleEBO);
}

void UnstructuredMesh::ViewCloudPicture(Scene* scene, int index,
                                        int demension) {
    if (m_DrawMesh) {
        return m_DrawMesh->ViewCloudPicture(scene, index, demension);
    }
    if (index == -1) {
        m_UseColor = false;
        m_ViewAttribute = nullptr;
        m_ViewDemension = -1;
        // m_ColorWithCell = false;
        scene->Update();
        return;
    }
    scene->MakeCurrent();
    m_AttributeIndex = index;
    auto& attr = this->GetAttributeSet()->GetAttribute(index);
    if (!attr.isDeleted) {
        if (attr.attachmentType == IG_POINT)
            this->SetAttributeWithPointData(attr.pointer, attr.dataRange,
                                            demension);
        else if (attr.attachmentType == IG_CELL)
            this->SetAttributeWithCellData(attr.pointer, demension);
    }
    scene->DoneCurrent();
    scene->Update();
}

void UnstructuredMesh::SetAttributeWithPointData(ArrayObject::Pointer attr,
                                                 std::pair<float, float>& range,
                                                 igIndex dimension) {
    if (m_ViewAttribute != attr || m_ViewDemension != dimension) {
        m_ViewAttribute = attr;
        m_ViewDemension = dimension;
        m_UseColor = true;
        m_ColorWithCell = false;

        if (range.first != range.second) {
            m_ColorMapper->SetRange(range.first, range.second);
        } else if (dimension == -1) {
            m_ColorMapper->InitRange(attr);
        } else {
            m_ColorMapper->InitRange(attr, dimension);
        }
        range.first = m_ColorMapper->GetRange()[0];
        range.second = m_ColorMapper->GetRange()[1];
        m_Colors = m_ColorMapper->MapScalars(attr, dimension);
        if (m_Colors == nullptr) { return; }

        GLAllocateGLBuffer(m_ColorVBO,
                           m_Colors->GetNumberOfValues() * sizeof(float),
                           m_Colors->RawPointer());


        m_PointVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);

        m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);

        m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
                                   3 * sizeof(float));
        GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);
    }
}

void UnstructuredMesh::SetAttributeWithCellData(ArrayObject::Pointer attr,
                                                igIndex i) {}
IGAME_NAMESPACE_END