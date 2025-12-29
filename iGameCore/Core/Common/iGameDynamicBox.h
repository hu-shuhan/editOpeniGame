#pragma once
#include <array>
#include <iGameMacro.h>
#include <iGameObject.h>
#include <iGamePoints.h>
#include <iGameSmartPointer.h>
#include <utility>
#include <vector>
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

    void MoveOpePoint(OpeInt opePoint, const Point& direction);
    void RotateBox(const Point& camera, const Point& direction);
    void MovePosition(double x, double y, double z);
    void MovePosition(const Point& position);
    const Point& GetMidPoint() const;

    // 旋转相关函数
    void SetRotation(double xAngle, double yAngle, double zAngle);
    const Point& GetRotation() const; // 返回欧拉角（从四元数计算）

    const std::array<Point, 6>& GetOpePoints() const;
    std::vector<std::pair<Point, Point>> GetAllEdges() const;
    std::array<std::array<Point, 4>, 6> GetAllFaces() const;
    const Point& GetLength() const;
    void SetLength(const Point& newLength);
    void SetLength(double lengthX, double lengthY, double lengthZ);
    std::pair<Point, Point> GetExtremePoint() const;

private:
    //############ Ori Msg ############
    Point m_Position;
    Point m_Length;

    // 四元数表示旋转 [w, x, y, z]
    std::array<double, 4> m_Quaternion = {1.0, 0.0, 0.0, 0.0}; // 初始为单位四元数

    //############ Exp Msg ############
    std::array<Point, 6> m_OpePoints; // up,bot,left,right,front,back

private:
    void InitMsg(const Point& p1, const Point& p2);
    void SetOpePoints();
    void UpdateBoxFromOpePoint(OpeInt pointIndex, const Point& moveVector);
    Point LocalToWorld(const Point& localVec) const;

    // 四元数相关函数
    void ApplyRotation(const Point& axis, double angle);
    void SetQuaternion(double w, double x, double y, double z);
    void NormalizeQuaternion();
    void MultiplyQuaternions(const double q1[4], const double q2[4], double result[4]);
    void EulerToQuaternion(double x, double y, double z, double q[4]) const;
    void QuaternionToEuler(const double q[4], double& x, double& y, double& z) const;
    void QuaternionToMatrix(const double q[4], double& r00, double& r01, double& r02, double& r10, double& r11,
                            double& r12, double& r20, double& r21, double& r22) const;

    // 旋转矩阵计算和变换相关函数
    void GetRotationMatrix(double& r00, double& r01, double& r02, double& r10, double& r11, double& r12, double& r20,
                           double& r21, double& r22) const;
    Point ApplyRotationMatrix(const Point& point, double r00, double r01, double r02, double r10, double r11,
                              double r12, double r20, double r21, double r22) const;
    Point GetFaceLocalNormal(OpeInt face) const;
    std::array<Point, 4> TransformFaceVertices(const std::array<Point, 4>& localVertices, double r00, double r01,
                                               double r02, double r10, double r11, double r12, double r20, double r21,
                                               double r22) const;
};

IGAME_NAMESPACE_END