#ifndef OPENIGAME_SLICING_STYLE_H
#define OPENIGAME_SLICING_STYLE_H

#include "iGameBasicStyle.h"

IGAME_NAMESPACE_BEGIN
class SlicingStyle : public BasicStyle {
public:
    I_OBJECT(SlicingStyle);
    static Pointer New() { return new SlicingStyle; }

    struct SlicingPlane {
        float point[3]{};
        float normal[3]{};
    };

    void Initialize(Interactor* a) override;

    void MousePressEvent(IEvent _event) override;
    void MouseMoveEvent(IEvent _event) override;
    void MouseReleaseEvent(IEvent _event) override;

    void RightButtonMouseMove() override;
    void MiddleButtonMouseMove() override;

protected:
    SlicingStyle() = default;
    ~SlicingStyle() override;

    void ComputeSlicingPlane(std::vector<Vector3d>& plane);
    void DrawSlicingPlane(std::vector<Vector3d>& plane);

    // 计算直线与平面的交点，AB直线，P平面上的点，N平面法向量
    // Calculate the intersection of a line with a plane, a line AB, a point in the p-plane, and a normal vector in the n-plane
    bool LinePlaneIntersection2(const Vector3d& A, const Vector3d& B,
                                const Vector3d& P, const Vector3d& N,
                                Vector3d& intersection);

    // 计算两条直线的交点
    // Calculate the intersection of two lines
    bool IsIntersect(const Vector3d& p1, const Vector3d& p2, const Vector3d& p3,
                     const Vector3d& p4, Vector3d& intersection);

    bool MapToSphere(const igm::vec2& v2D, igm::vec3& v3D, double radius) {
        auto center = v(this->center);

        igm::mat4 model = m_Scene->ModelMatrix();
        igm::mat4 view = m_Camera->GetViewMatrix();
        igm::mat4 proj = m_Camera->GetProjectionMatrix();

        auto p = igm::vec4{center, 1.0f};
        auto p_mvp = (proj * view * model * p);
        p_mvp /= p_mvp.w;

        // if the perspective enters the model, rotate around (0,0)
        if (p_mvp.x > 1.0f || p_mvp.x < -1.0f || p_mvp.y > 1.0f ||
            p_mvp.y < -1.0f) {
            // p_mvp = igm::vec4{0.0f, 0.0f, 0.0f, 0.0f};
            return false;
        }

        auto width = m_Camera->GetViewPort().x;
        auto height = m_Camera->GetViewPort().y;

        //const double trackballradius = 0.6;
        const double rsqr = radius * radius;

        // calculate old hit sphere point3D
        double x = (2.0 * v2D.x - width) / width - p_mvp.x;
        double y = -(2.0 * v2D.y - height) / height - p_mvp.y;
        double x2y2 = x * x + y * y;

        v3D[0] = x;
        v3D[1] = y;
        if (x2y2 < 0.5 * rsqr) {
            v3D[2] = sqrt(rsqr - x2y2);
        } else {
            v3D[2] = 0.5 * rsqr / sqrt(x2y2);
        }

        return true;
    }

    void Invoke();

    Model::Pointer m_Model;
    DataObject::Pointer m_DataObject;
    Painter3D::Pointer m_Painter;

private:
    int selectId{-1}; // 0:center 1:head 2:rear 3:line

    igm::mat4 mvp{};
    igm::mat4 invMVP{};

    double len;
    double radius;
    Vector3d center;
    Vector3d head;
    Vector3d rear;
    Vector3d top, left; // 切平面的上/左顶点
    Vector3d normal; // 切平面的法向量
    IGuint boxHandle{0};
    IGuint centerHandle{0};
    IGuint headHandle{0};
    IGuint rearHandle{0};
    IGuint lineHandle{0}, circleHandle{0};
    IGuint planeHandle[10]{0};
    std::vector<Vector3d> plane;

    SlicingPlane slicingPlane;

    Vector3Tovec3 v{};
    vec3ToVector3d V{};
};
IGAME_NAMESPACE_END
#endif