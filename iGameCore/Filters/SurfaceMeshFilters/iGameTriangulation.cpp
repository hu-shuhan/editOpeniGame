#include "iGameTriangulation.h"
IGAME_NAMESPACE_BEGIN

bool Triangulation::Execute() {

    auto input = GetInput(0);
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            mesh = DynamicCast<SurfaceMesh>(input);
            break;
        case IG_UNSTRUCTURED_MESH:
            mesh = DynamicCast<UnstructuredMesh>(input)->TransferToSurfaceMesh();
            break;
        default:
            break;
    }
    if (mesh == nullptr) { return false; }

    {
        bool f = true;
        igIndex face[16]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
            int size = mesh->GetFacePointIds(i, face);
            if (size != 3) { 
                f = false;
                break;
            }
        }
        
        if (f) { 
            SetOutput(mesh);
            return true;
        }
    }

    auto attrbs = mesh->GetAttributeSet();

    CellArray::Pointer Faces = CellArray::New();
    Points::Pointer Points = mesh->GetPoints();

    igIndex face[16]{}, tri[3]{};
    for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
        int size = mesh->GetFacePointIds(i, face);

        if (size == 3) {
            Faces->AddCellId3(face[0], face[1], face[2]);
        } else if (size == 4) {
            Point p0 = mesh->GetPoint(face[0]);
            Point p1 = mesh->GetPoint(face[1]);
            Point p2 = mesh->GetPoint(face[2]);
            Point p3 = mesh->GetPoint(face[3]);
            double area01 = GetArea(p0, p1, p3);
            double area02 = GetArea(p1, p2, p3);

            double area11 = GetArea(p0, p1, p2);
            double area12 = GetArea(p2, p3, p0);

            double r0 = area01 / area02;
            double r1 = area11 / area12;

            if (r0 < 1) r0 = 1 / r0;
            if (r1 < 1) r1 = 1 / r1;
            if (r0 < r1) {
                Faces->AddCellId3(face[0], face[1], face[3]);
                Faces->AddCellId3(face[1], face[2], face[3]);
            } else {
                Faces->AddCellId3(face[0], face[1], face[2]);
                Faces->AddCellId3(face[2], face[3], face[0]);
            }

        } else {
            Point center(0, 0, 0);
            for (int j = 0; j < size; j++) { center += Points->GetPoint(face[j]); }
            center /= size;
            igIndex newPtId = Points->AddPoint(center);

            for (int j = 0; j < attrbs->GetNumberOfAttributes(); j++) {
                auto& attrb = attrbs->GetAttribute(j);
                if (attrb.isDeleted) continue;
                if (attrb.attachmentType == IG_POINT) {
                    double val[8]{0}, sum[8]{0};
                    int dim = attrb.pointer->GetDimension();
                    for (int k = 0; k < size; k++) {
                        attrb.pointer->GetElement(face[k], val);
                        for (int d = 0; d < dim; d++) { sum[d] += val[d]; }
                    }
                    for (int d = 0; d < dim; d++) { sum[d] /= size; }
                    attrb.pointer->AddElement(sum);
                }
            }

            for (int j = 0; j < size; j++) { Faces->AddCellId3(newPtId, face[j], face[(j + 1) % size]); }
        }
    }

    SurfaceMesh::Pointer Mesh = SurfaceMesh::New();
    Mesh->SetName(mesh->GetName());
    Mesh->SetPoints(Points);
    Mesh->SetFaces(Faces);
    Mesh->SetAttributeSet(mesh->GetAttributeSet());

    SetOutput(Mesh);
    return true;
}

Triangulation::Triangulation() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

double Triangulation::GetArea(Vector3d a, Vector3d b, Vector3d c) { return CrossProduct(a - b, a - c).length() / 2; }
IGAME_NAMESPACE_END