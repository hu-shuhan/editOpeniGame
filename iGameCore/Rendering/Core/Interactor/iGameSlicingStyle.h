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

    void ComputeSlicingPlane(std::vector<Vector3d>& plane);
    void DrawSlicingPlane(const std::vector<Vector3d>& plane);

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

    int selectId; // 0:center 1:head 2:rear 3:line

    igm::mat4 mvp;
    igm::mat4 invMVP;

    double pickRadius;
    double len;
    double radius;
    Vector3d center;
    Vector3d head;
    Vector3d rear;
    Vector3d top, left; // 切平面的上/左顶点
    Vector3d normal;    // 切平面的法向量
    IGuint boxHandle;
    IGuint centerHandle;
    IGuint headHandle;
    IGuint rearHandle;
    IGuint lineHandle;
    IGuint circleHandle;
    IGuint planeHandle[10]{0};

    SlicingPlane slicingPlane;

    Vector3Tovec3 v;
    vec3ToVector3d V;
};
IGAME_NAMESPACE_END
#endif