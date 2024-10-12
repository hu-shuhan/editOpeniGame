#ifndef OPENIGAME_BASIC_STYLE_H
#define OPENIGAME_BASIC_STYLE_H

#include "iGameInteractorStyle.h"
#include "iGameScene.h"
#include "iGameCamera.h"

IGAME_NAMESPACE_BEGIN

class BasicStyle : public InteractorStyle {
public:
    I_OBJECT(BasicStyle);
    static Pointer New() { return new BasicStyle; }

    struct Vector3Tovec3 {
        igm::vec3 operator()(const Vector3f& v) {
            return igm::vec3(v[0], v[1], v[2]);
        }
        igm::vec3 operator()(const Vector3d& v) {
            return igm::vec3(v[0], v[1], v[2]);
        }
    };
    struct vec3ToVector3d {
        Vector3d operator()(const igm::vec3& v) {
            return Vector3d(v.x, v.y, v.z);
        }
    };


    void Initialize(Interactor* a) override;
    void MousePressEvent(IEvent _event) override;
    void MouseMoveEvent(IEvent _event) override;
    void MouseReleaseEvent(IEvent _event) override;
    void WheelEvent(IEvent _event) override;

    virtual void LeftButtonMouseMove();
    virtual void RightButtonMouseMove();
    virtual void MiddleButtonMouseMove();

protected:
    BasicStyle() = default;
    ~BasicStyle() override = default;

    void ModelRotation();
    void ViewTranslation();
    void MapToSphere(igm::vec3& old_v3D, igm::vec3& new_v3D);
    void UpdateCameraMoveSpeed(const igm::vec4& center);

    // 计算直线与平面的交点
    bool LinePlaneIntersection(const igm::vec3& A, const igm::vec3& B, 
                               const igm::vec3& P1, const igm::vec3& P2,
                               const igm::vec3& P3, igm::vec3& intersection) {
        // 直线的方向向量
        double u[3] = {B.x - A.x, B.y - A.y, B.z - A.z};

        // 计算平面的法向量
        double v1[3] = {P2.x - P1.x, P2.y - P1.y, P2.z - P1.z};
        double v2[3] = {P3.x - P1.x, P3.y - P1.y, P3.z - P1.z};

        // 法向量 N = v1 × v2
        double N[3] = {v1[1] * v2[2] - v1[2] * v2[1],
                       v1[2] * v2[0] - v1[0] * v2[2],
                       v1[0] * v2[1] - v1[1] * v2[0]};

        // 平面方程的 D 值
        double D = -(N[0] * P1.x + N[1] * P1.y + N[2] * P1.z);

        // 代入平面方程求交点
        double denominator = N[0] * u[0] + N[1] * u[1] + N[2] * u[2];
        if (denominator == 0) {
            // 直线与平面平行
            return false;
        }

        // 计算 t 的值
        double t = -(N[0] * A.x + N[1] * A.y + N[2] * A.z + D) / denominator;

        // 计算交点坐标
        intersection.x = A.x + t * u[0];
        intersection.y = A.y + t * u[1];
        intersection.z = A.z + t * u[2];

        return true;
    }

    bool IsIntersectTriangle(igm::vec3 orig, igm::vec3 end, igm::vec3 v0,
                             igm::vec3 v1, igm::vec3 v2,
                             igm::vec3& intersection);

    double DistancePointToPlane(igm::vec3 point, igm::vec3 p1, igm::vec3 p2,
                                igm::vec3 p3);

    double DistancePointToLine(igm::vec3 point, igm::vec3 p1, igm::vec3 p2);
    double DistancePointToLine(Vector3d point, Vector3d p1, Vector3d p2);

    igm::vec4 GetPlane(const igm::vec3& p, const igm::vec3& normal);

    igm::vec3 GetNearWorldCoord(const igm::vec2& screenCoord,
                                const igm::mat4& invertedMvp);

    igm::vec3 GetFarWorldCoord(const igm::vec2& screenCoord,
                               const igm::mat4& invertedMvp);

    Interactor* m_Interactor{nullptr};
    Scene* m_Scene{nullptr};
    Camera* m_Camera{nullptr};

    igm::vec2 m_OldPoint2D{};
    igm::vec2 m_NewPoint2D{};
    float m_CameraScaleSpeed{1.0f};
    float m_CameraMoveSpeed{0.01f};
    MouseButton m_MouseMode{NoButton};
    
};
IGAME_NAMESPACE_END
#endif