#ifndef iGameGradient_h
#define iGameGradient_h

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class Gradient : public Filter {
public:
    I_OBJECT(Gradient);
    static Pointer New() { return new Gradient; }

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
        
        auto& attrb = mesh->GetAttributeSet()->GetScalar(0);
        int dim = attrb.pointer->GetDimension();

        FloatArray::Pointer grad = FloatArray::New();
        grad->SetName("Gradient");
        grad->Resize(mesh->GetNumberOfPoints());
        

        mesh->RequestEditStatus();

        IdArray::Pointer ids = IdArray::New();
        igIndex face[3]{};
        //for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
        //    mesh->GetPointToNeighborFaces(i, ids);
        //    float p = attrb.pointer->GetValue(i * dim);
        //    Point pt = mesh->GetPoint(i);
        //    double gradient = 0;
        //    for (int j = 0; j < ids->GetNumberOfIds(); j++) {
        //        mesh->GetFacePointIds(ids->GetId(j), face);
        //        auto center = GetCenter(ids->GetId(j));
        //        float sum_p = 0;
        //        for (int k = 0; k < 3; k++) { 
        //            sum_p += attrb.pointer->GetValue(face[k] * dim);
        //        }
        //        double len = (pt - center).length();
        //        if (len < 1e-1) {
        //            len = 1e-1;
        //        }
        //        gradient += (p - sum_p / 3);
        //    }
        //    
        //    gradient /= ids->GetNumberOfIds();
        //    //if (gradient > 1000 || gradient < -1000) { 
        //    //    gradient = 0;
        //    //}
        //    grad->SetValue(i, gradient);
        //}

        double min = 1e10, max = 0;
        for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
            mesh->GetPointToOneRingPoints(i, ids);
            float p = attrb.pointer->GetValue(i * dim);
            Point pt = mesh->GetPoint(i);
            double gradient = 0;
            for (int j = 0; j < ids->GetNumberOfIds(); j++) {
                gradient += std::abs(
                        p - attrb.pointer->GetValue(ids->GetId(j) * dim));
            }

            gradient /= ids->GetNumberOfIds();
            //if (gradient > 1000 || gradient < -1000) {
            //    gradient = 0;
            //}
            grad->SetValue(i, gradient);
            min = std::min(min, gradient);
            max = std::max(max, gradient);
        }

        std::cout << min << " " << max << std::endl;
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, grad);

        return true;
    }

    Vector3f GetCenter(igIndex faceId) {
        igIndex face[3]{};
        mesh->GetFacePointIds(faceId, face);
        Point p0 = mesh->GetPoint(face[0]);
        Point p1 = mesh->GetPoint(face[1]);
        Point p2 = mesh->GetPoint(face[2]);

        return (p0 + p1 + p2) / 3;
    }

protected:
    Gradient() { SetNumberOfInputs(1); }
    ~Gradient() override = default;


    SurfaceMesh::Pointer mesh{};
};
IGAME_NAMESPACE_END
#endif