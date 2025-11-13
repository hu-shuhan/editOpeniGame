#pragma once
#include <iGameMacro.h>
#include <iGameObject.h>
#include <iGamePoints.h>
#include <iGameSmartPointer.h>
#include <array>
IGAME_NAMESPACE_BEGIN
class DynamicBox : public Object {
protected:
    DynamicBox() = default;
    ~DynamicBox() = default;
    DynamicBox(const Point& p1, const Point& p2);

public:
    enum OpeInt : int { UP = 0, BOTTOM = 1, LEFT = 2, RIGHT = 3, FRONT = 4, BACK = 5 };
    I_OBJECT(DynamicBox);
    static Pointer New(const Point& p1, const Point& p2) { return new DynamicBox(p1, p2); }
    void MoveOpePoint(OpeInt pointIndex, const Point& direction);
    void RotateBox(OpeInt face, const Point& direction);

private:
    //############ Ori Msg ############
    Point m_Position;
    Point m_Rotation;
    Point m_Length;
    //############ Exp Msg ############
    std::array<Point, 6> m_OpePoints;//up,bot,left,right,front,back

private:
    void InitMsg(const Point& p1, const Point& p2);
    void SetOpePoints();
    void UpdateBoxFromOpePoint(OpeInt pointIndex, const Point& moveVector);
    Point LocalToWorld(const Point& localVec) const;
    void ApplyRotation(const Point& axis, double angle);
    // 旋转矩阵计算和变换相关函数
    void GetRotationMatrix(double& r00, double& r01, double& r02, double& r10, double& r11, double& r12, double& r20,
                           double& r21, double& r22) const;
    Point ApplyRotationMatrix(const Point& point, double r00, double r01, double r02, double r10, double r11,
                              double r12, double r20, double r21, double r22) const;
    Point GetFaceLocalNormal(OpeInt face) const;
};


IGAME_NAMESPACE_END