#ifndef IGAMEVIS_SLICING_STYLE_H
#define IGAMEVIS_SLICING_STYLE_H

#include "iGameBasicStyle.h"

IGAME_NAMESPACE_BEGIN
class Model;
class DataObject;
class Painter3D;

class SlicingStyle : public BasicStyle {
public:
    I_OBJECT(SlicingStyle);
    static Pointer New() { return new SlicingStyle; }

    struct SlicingPlane {
        float point[3]{};
        float normal[3]{};
    };

    void Initialize(SmartPointer<Interactor> interactor) override;

    void MousePressEvent(IEvent _event) override;
    void MouseMoveEvent(IEvent _event) override;
    void MouseReleaseEvent(IEvent _event) override;

    void RightButtonMouseMove() override;
    void MiddleButtonMouseMove() override;

protected:
    SlicingStyle();
    ~SlicingStyle() override;

    void Draw();

    void ComputeSlicingPlane();
    void DrawSlicingPlane();

    // 计算直线与平面的交点，AB直线，P平面上的点，N平面法向量
    // Calculate the intersection of a line with a plane, a line AB, a point in the p-plane, and a normal vector in the n-plane
    bool LinePlaneIntersection2(const Vector3d& A, const Vector3d& B,
                                const Vector3d& P, const Vector3d& N,
                                Vector3d& intersection);

    // 计算两条直线的交点
    // Calculate the intersection of two lines
    bool IsIntersect(const Vector3d& p1, const Vector3d& p2, const Vector3d& p3,
                     const Vector3d& p4, Vector3d& intersection);

    bool MapToSphere(const igm::vec2& v2D, igm::vec3& v3D, double radius);

    void Invoke();

    SmartPointer<Model> m_Model;
    SmartPointer<DataObject> m_DataObject;
    SmartPointer<Painter3D> m_Painter3D;

private:
  
    SlicingPlane EmitPlane;

    Vector3Tovec3 v;
    vec3ToVector3d V;

    igm::vec3 Start, End, Center;
    igm::vec3 Center2Start, Center2End;
    IGuint StartHandle, EndHandle, CenterHandle;
    IGuint BoxHandle;
    IGuint LineHandle;
    IGuint PlaneHandle[10]{0};
    int Selected = -1; // 0:center 1:head 2:rear 3:line
    float NDC_Z;
    float PickRadius;
    igm::vec3 Intersection;
    igm::mat4 MVP;
    igm::mat4 InvertedMVP;
    std::vector<Vector3d> Plane;
};
IGAME_NAMESPACE_END
#endif