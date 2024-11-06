#ifndef iGameTriangulation_h
#define iGameTriangulation_h

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class Triangulation : public Filter {
public:
    I_OBJECT(Triangulation);
    static Pointer New() { return new Triangulation; }

    bool Execute() override {
        
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
        if (mesh == nullptr) {
            return false;
        }

        auto attrbs = mesh->GetAttributeSet();

        CellArray::Pointer Faces = CellArray::New();
        Points::Pointer Points = mesh->GetPoints();

        igIndex face[16]{}, tri[3]{};
        auto painter = m_Model->GetPainter();
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
                for (int j = 0; j < size; j++) { 
                    center += Points->GetPoint(face[j]);
                }
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
                            for (int d = 0; d < dim; d++) {
                                sum[d] += val[d];
                            }
                
                        }
                        for (int d = 0; d < dim; d++) { sum[d] /= size; }
                        attrb.pointer->AddElement(sum);
                    }
                }

                for (int j = 0; j < size; j++) {
                    Faces->AddCellId3(newPtId, face[j], face[(j + 1) % size]);
                }
            }


            //while (size > 2) {
            //    //std::cout << i << std::endl;
            //    for (int j = 0; j < size; j++) {
            //        int prev = (j + size - 1) % size;
            //        int next = (j + 1) % size;

            //        Point a = mesh->GetPoint(face[prev]);
            //        Point b = mesh->GetPoint(face[j]);
            //        Point c = mesh->GetPoint(face[next]);
            //        double area = GetArea(a, b, c);

            //        if (area < Area * 0.01) { 
            //            std::cout << i << std::endl;
            //        }
            //        if (area > 0) {
            //            bool isEar = true;

            //            //// 检查其他点是否在三角形内部
            //            //for (int k = 0; k < size; k++) {
            //            //    if (k != prev && k != j && k != next &&
            //            //        IsPointInTriangle(mesh->GetPoint(face[k]), a, b, c)) {
            //            //        isEar = false;
            //            //        break;
            //            //    }
            //            //}

            //            if (isEar) {
            //                Face->AddCellId3(face[prev], face[j], face[next]);
            //                Delete(face, size, j);
            //                size--;
            //                break;
            //            }
            //        }
            //    }
            //}
        }

        SurfaceMesh::Pointer Mesh = SurfaceMesh::New();
        Mesh->SetPoints(Points);
        Mesh->SetFaces(Faces);
        Mesh->SetAttributeSet(mesh->GetAttributeSet());

        //for (int i = 0; i < Mesh->GetNumberOfFaces(); i++) {
        //    igIndex face[3]{};
        //    Mesh->GetFacePointIds(i, face);
        //    Point p0 = Mesh->GetPoint(face[0]);
        //    Point p1 = Mesh->GetPoint(face[1]);
        //    Point p2 = Mesh->GetPoint(face[2]);

        //    if (GetArea(p0, p1, p2) < 1e-8) {
        //        //std::cout << GetArea(p0, p1, p2) << std::endl;
        //        painter->DrawTriangle(p0, p1, p2);
        //    }
        //}


        SetOutput(Mesh);
        return true;
    }

protected:
    Triangulation() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~Triangulation() override = default;

    void Delete(igIndex* face, int size, int index) {
        for (int i = index; i < size - 1; i++) { 
            face[i] = face[i + 1];
        }
    }

    double GetArea(Vector3d a, Vector3d b, Vector3d c) {
        return CrossProduct(a - b, a - c).length() / 2;
    }

    //bool IsTriangle(Vector3d a, Vector3d b, Vector3d c) {
    //    return ;
    //}

    bool IsPointInTriangle(const Point& pt, const Point& a, const Point& b,
                           const Point& c) {
        // 计算向量和叉积来判断
        double areaABC = GetArea(a, b, c);
        double areaABP = GetArea(a, b, pt);
        double areaBCP = GetArea(b, c, pt);
        double areaCAP = GetArea(c, a, pt);

        return std::abs(areaABC - (areaABP + areaBCP + areaCAP)) <
               1e-9; // 允许的小误差
    }

    SurfaceMesh::Pointer mesh{};
};
IGAME_NAMESPACE_END
#endif