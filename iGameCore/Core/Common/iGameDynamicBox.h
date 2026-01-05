#pragma once
#include <array>
#include <iGameMacro.h>
#include <iGameObject.h>
#include <iGamePoints.h>
#include <iGameSmartPointer.h>
#include <utility>
#include <vector>
#include "igm/igm.h"
#include <iGameCamera.h>
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
    void RotateBox(const Point& oldP, const Point& newP);
    Point GetRotation() const;
    void MovePosition(double x, double y, double z);
    void MovePosition(const Point& position);
    const Point& GetMidPoint() const;

    // 旋转相关函数
    void SetRotation(float xAngle, float yAngle, float zAngle);
    void SetRotation(const igm::mat4& rotationMatrix);
    const igm::mat4& GetRotationMatrix() const;

    const std::array<Point, 6>& GetOpePoints() const;
    std::vector<std::pair<Point, Point>> GetAllEdges() const;
    std::array<std::array<Point, 4>, 6> GetAllFaces() const;
    const Point& GetLength() const;
    void SetLength(const Point& newLength);
    void SetLength(double lengthX, double lengthY, double lengthZ);
    std::pair<Point, Point> GetExtremePoint() const;

public:
    //############ TEST ############
    Point OldP;
    Point NewP;

private:
    //############ Ori Msg ############
    Point m_Position;
    Point m_Length;

    // 使用4x4旋转矩阵表示旋转，包含平移信息
    igm::mat4 m_RotationMatrix = igm::mat4(1.0f); // 初始为单位矩阵

    //############ Exp Msg ############
    std::array<Point, 6> m_OpePoints; // up,bot,left,right,front,back

private:
    void InitMsg(const Point& p1, const Point& p2);
    void SetOpePoints();
    void UpdateBoxFromOpePoint(OpeInt pointIndex, const Point& moveVector);

    // 辅助函数
    Point LocalToWorld(const Point& localVec) const;
    Point WorldToLocal(const Point& worldVec) const;
    igm::mat4 GetTransformMatrix() const;

    // 旋转矩阵相关函数
    void ApplyRotation(const igm::vec3& axis, float angle);
    void ApplyRotation(const igm::mat4& rotationMatrix);

    // 局部坐标系辅助函数
    Point GetFaceLocalNormal(OpeInt face) const;
    std::array<Point, 4> GetFaceLocalVertices(OpeInt face) const;

    // 工具函数
    static igm::mat4 CreateRotationMatrix(const igm::vec3& axis, float angle);
    static igm::mat4 CreateTranslationMatrix(const Point& translation);
};

IGAME_NAMESPACE_END