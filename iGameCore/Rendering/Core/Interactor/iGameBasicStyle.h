#ifndef IGAMEVIS_BASIC_STYLE_H
#define IGAMEVIS_BASIC_STYLE_H

#include "iGameCamera.h"
#include "iGameInteractorStyle.h"
#include "iGameVector.h"
#include "igm/igm.h"

IGAME_NAMESPACE_BEGIN
class Scene;
class Interactor;

class BasicStyle : public InteractorStyle {
public:
    I_OBJECT(BasicStyle);
    static Pointer New() { return new BasicStyle; }

    struct Vector3Tovec3 {
        igm::vec3 operator()(const Vector3f& v) {
            return igm::vec3{v[0], v[1], v[2]};
        }
        igm::vec3 operator()(const Vector3d& v) {
            return igm::vec3{static_cast<float>(v[0]), static_cast<float>(v[1]),
                             static_cast<float>(v[2])};
        }
    };
    struct vec3ToVector3d {
        Vector3d operator()(const igm::vec3& v) {
            return Vector3d(v.x, v.y, v.z);
        }
    };

    void Initialize(SmartPointer<Interactor> interactor) override;
    void MousePressEvent(IEvent event) override;
    void MouseMoveEvent(IEvent event) override;
    void MouseReleaseEvent(IEvent event) override;
    void WheelEvent(IEvent event) override;

protected:
    BasicStyle();
    ~BasicStyle() override;

    virtual void LeftButtonMouseMove();
    virtual void RightButtonMouseMove();
    virtual void MiddleButtonMouseMove();

    void ModelRotation();
    void ViewTranslation();
    void MapToSphere(igm::vec3& old_v3D, igm::vec3& new_v3D);
    void UpdateCameraMoveSpeed(const igm::vec4& center);

    // 两条直线的交点
    bool TwoLineIntersection(const igm::vec3& p1, const igm::vec3& p2,
                             const igm::vec3& v1, const igm::vec3& v2,
                             igm::vec3& intersection);

    // 计算直线与平面的交点
    bool LinePlaneIntersection(const igm::vec3& A, const igm::vec3& B,
                               const igm::vec3& P1, const igm::vec3& P2,
                               const igm::vec3& P3, igm::vec3& intersection);
    bool LinePlaneIntersection(const igm::vec3& A, const igm::vec3& B,
                               const igm::vec3& Point, const igm::vec3& Normal,
                               igm::vec3& intersection);

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

    SmartPointer<Interactor> m_Interactor;
    SmartPointer<Scene> m_Scene;
    SmartPointer<Camera> m_Camera;

    igm::vec2 m_OldPoint2D;
    igm::vec2 m_NewPoint2D;
    float m_CameraScaleSpeed;
    float m_CameraMoveSpeed;
    MouseButton m_MouseMode;
};
IGAME_NAMESPACE_END
#endif