#ifndef OPENIGAME_SLICING_STYLE_H
#define OPENIGAME_SLICING_STYLE_H

#include "iGameBasicStyle.h"

IGAME_NAMESPACE_BEGIN
class SlicingStyle : public BasicStyle {
public:
    I_OBJECT(SlicingStyle);
    static Pointer New() { return new SlicingStyle; }

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

    bool LinePlaneIntersection2(const Vector3d& A, const Vector3d& B,
                               const Vector3d& P, const Vector3d& N,
                               Vector3d& intersection) {
        // 直线的方向向量
        Vector3d u = B - A;

        // 平面方程的 D 值
        double D = -N.dot(P);

        // 计算分母
        double denominator = N.dot(u);
        if (denominator == 0) {
            return false; // 直线与平面平行
        }

        // 计算 t
        double t = -(N.dot(A) + D) /
                   denominator;

        if (t < 0 || t > 1) {
            return false; // 交点不在直线段上
        }

        // 计算交点
        intersection[0] = A[0] + t * u[0];
        intersection[1] = A[1] + t * u[1];
        intersection[2] = A[2] + t * u[2];

        return true;
    }

    // 计算两条直线的交点
    bool IsIntersect(const Vector3d& p1, const Vector3d& p2, const Vector3d& p3,
                   const Vector3d& p4, Vector3d& intersection) {
        Vector3d d1 = p2 - p1;
        Vector3d d2 = p4 - p3;
        Vector3d r = p1 - p3;  // 线段之间的向量

        double d = d1.dot(d2.cross(d2)); // 计算行列式的绝对值

        if (std::abs(d) < 1e-10) {
            // 直线平行或重合
            return false;
        }

        // 计算参数
        double t = (r.cross(d2).dot(d2)) / d;
        double u = (r.cross(d1).dot(d1)) / d;

        // 检查参数范围
        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            intersection = p1 + t * d1; // 计算交点
            return true;
        }

        return false; // 不相交
    }

    Model::Pointer m_Model;
    DataObject::Pointer m_DataObject;
    Painter::Pointer m_Painter;

private:
    int selectId{-1}; // 0:center 1:head 2:rear

    igm::mat4 mvp{};
    igm::mat4 invMVP{};

    double len;
    Vector3d center;
    Vector3d head, headBound;
    Vector3d rear, rearBound;
    Vector3d top, left;
    Vector3d normal;
    Vector3d direction;
    IGuint boxHandle{0};
    IGuint centerHandle{0};
    IGuint headHandle{0};
    IGuint rearHandle{0};
    IGuint lineHandle{0};
    IGuint planeHandle[10]{0};
    std::vector<Vector3d> plane;

    Vector3Tovec3 v{};
    vec3ToVector3d V{};
};
IGAME_NAMESPACE_END
#endif