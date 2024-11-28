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

    bool Execute() override;

protected:
    Triangulation();
    ~Triangulation() override = default;

    //void Delete(igIndex* face, int size, int index) {
    //    for (int i = index; i < size - 1; i++) { 
    //        face[i] = face[i + 1];
    //    }
    //}

    double GetArea(Vector3d a, Vector3d b, Vector3d c);

    //bool IsPointInTriangle(const Point& pt, const Point& a, const Point& b,
    //                       const Point& c) {
    //    // 计算向量和叉积来判断
    //    double areaABC = GetArea(a, b, c);
    //    double areaABP = GetArea(a, b, pt);
    //    double areaBCP = GetArea(b, c, pt);
    //    double areaCAP = GetArea(c, a, pt);

    //    return std::abs(areaABC - (areaABP + areaBCP + areaCAP)) <
    //           1e-9; // 允许的小误差
    //}

    SurfaceMesh::Pointer mesh{};
};
IGAME_NAMESPACE_END
#endif