#include "iGameDynamicBox.h"
#include <algorithm>
#include <cmath>

IGAME_NAMESPACE_BEGIN

// 计算旋转矩阵的辅助函数
void DynamicBox::GetRotationMatrix(double& r00, double& r01, double& r02, double& r10, double& r11, double& r12,
                                   double& r20, double& r21, double& r22) const {
    double cx = cos(m_Rotation[0]);
    double sx = sin(m_Rotation[0]);
    double cy = cos(m_Rotation[1]);
    double sy = sin(m_Rotation[1]);
    double cz = cos(m_Rotation[2]);
    double sz = sin(m_Rotation[2]);

    // 组合旋转矩阵：Rz * Ry * Rx
    r00 = cy * cz;
    r01 = sx * sy * cz - cx * sz;
    r02 = cx * sy * cz + sx * sz;

    r10 = cy * sz;
    r11 = sx * sy * sz + cx * cz;
    r12 = cx * sy * sz - sx * cz;

    r20 = -sy;
    r21 = sx * cy;
    r22 = cx * cy;
}

// 应用旋转矩阵到点的辅助函数
Point DynamicBox::ApplyRotationMatrix(const Point& point, double r00, double r01, double r02, double r10, double r11,
                                      double r12, double r20, double r21, double r22) const {
    Point result;
    result[0] = point[0] * r00 + point[1] * r01 + point[2] * r02;
    result[1] = point[0] * r10 + point[1] * r11 + point[2] * r12;
    result[2] = point[0] * r20 + point[1] * r21 + point[2] * r22;
    return result;
}

// 获取面的局部法线方向
Point DynamicBox::GetFaceLocalNormal(OpeInt face) const {
    switch (face) {
        case UP:
            return Point(0, 1, 0);
        case BOTTOM:
            return Point(0, -1, 0);
        case LEFT:
            return Point(-1, 0, 0);
        case RIGHT:
            return Point(1, 0, 0);
        case FRONT:
            return Point(0, 0, 1);
        case BACK:
            return Point(0, 0, -1);
    }
    return Point(0, 0, 0);
}

void DynamicBox::InitMsg(const Point& p1, const Point& p2) {
    m_Position = (p1 + p2) / 2.0;
    m_Rotation.setZero();
    m_Length = Point(std::abs(p1[0] - p2[0]), std::abs(p1[1] - p2[1]), std::abs(p1[2] - p2[2]));
}

void DynamicBox::SetOpePoints() {
    // 获取半长（从中心到各面的距离）
    Point halfLength = m_Length / 2.0;

    // 未旋转时的局部坐标操作点
    std::array<Point, 6> localPoints = {
            Point(0, halfLength[1], 0),  // UP - 上表面中心
            Point(0, -halfLength[1], 0), // BOTTOM - 下表面中心
            Point(-halfLength[0], 0, 0), // LEFT - 左表面中心
            Point(halfLength[0], 0, 0),  // RIGHT - 右表面中心
            Point(0, 0, halfLength[2]),  // FRONT - 前表面中心
            Point(0, 0, -halfLength[2])  // BACK - 后表面中心
    };

    // 获取旋转矩阵
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);

    // 对每个局部操作点应用旋转和平移
    for (int i = 0; i < 6; ++i) {
        Point rotatedPoint = ApplyRotationMatrix(localPoints[i], r00, r01, r02, r10, r11, r12, r20, r21, r22);
        m_OpePoints[i] = rotatedPoint + m_Position;
    }
}

DynamicBox::DynamicBox(const Point& p1, const Point& p2) {
    InitMsg(p1, p2);
    SetOpePoints();
}

void DynamicBox::MoveOpePoint(OpeInt pointIndex, const Point& direction) {
    if (pointIndex < 0 || pointIndex >= 6) return;

    // 获取操作点对应的局部坐标轴方向
    Point localAxis = GetFaceLocalNormal(pointIndex);

    // 获取旋转矩阵
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);

    // 将局部坐标轴方向旋转到世界坐标系
    Point worldAxis = ApplyRotationMatrix(localAxis, r00, r01, r02, r10, r11, r12, r20, r21, r22);

    // 计算输入方向在局部轴方向上的投影
    double dotProduct = direction[0] * worldAxis[0] + direction[1] * worldAxis[1] + direction[2] * worldAxis[2];
    double projectionLength = dotProduct;

    // 计算实际移动向量（在世界坐标系中）
    Point moveVector = worldAxis * projectionLength;

    // 更新操作点位置
    m_OpePoints[pointIndex] = m_OpePoints[pointIndex] + moveVector;

    // 根据移动的操作点更新盒子的位置和尺寸
    UpdateBoxFromOpePoint(pointIndex, moveVector);
}

void DynamicBox::UpdateBoxFromOpePoint(OpeInt pointIndex, const Point& moveVector) {
    // 根据移动的操作点更新盒子的位置和尺寸
    switch (pointIndex) {
        case UP:
        case BOTTOM:
            // 更新Y方向的长度和位置
            m_Length[1] += moveVector[1] * (pointIndex == UP ? 2 : -2);
            m_Position[1] += moveVector[1] * 0.5;
            break;

        case LEFT:
        case RIGHT:
            // 更新X方向的长度和位置
            m_Length[0] += moveVector[0] * (pointIndex == RIGHT ? 2 : -2);
            m_Position[0] += moveVector[0] * 0.5;
            break;

        case FRONT:
        case BACK:
            // 更新Z方向的长度和位置
            m_Length[2] += moveVector[2] * (pointIndex == FRONT ? 2 : -2);
            m_Position[2] += moveVector[2] * 0.5;
            break;
    }

    // 更新所有操作点
    SetOpePoints();
}

void DynamicBox::RotateBox(OpeInt face, const Point& direction) {
    // 获取选定面的法线方向（在局部坐标系中）
    Point localNormal = GetFaceLocalNormal(face);

    // 将局部法线转换到世界坐标系
    Point worldNormal = LocalToWorld(localNormal);

    // 计算旋转轴：法线与方向向量的叉积
    Point rotationAxis = worldNormal.cross(direction);
    rotationAxis.normalize();

    // 计算旋转角度：基于方向向量的长度，可以添加一个缩放因子
    double rotationAngle = direction.length() * 0.01;

    // 应用旋转
    ApplyRotation(rotationAxis, rotationAngle);

    // 更新操作点
    SetOpePoints();
}

// 将局部坐标转换到世界坐标系
Point DynamicBox::LocalToWorld(const Point& localVec) const {
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);
    return ApplyRotationMatrix(localVec, r00, r01, r02, r10, r11, r12, r20, r21, r22);
}

// 应用旋转（使用四元数避免万向锁问题）
void DynamicBox::ApplyRotation(const Point& axis, double angle) {
    // 创建四元数表示旋转
    double halfAngle = angle / 2.0;
    double s = sin(halfAngle);
    double c = cos(halfAngle);

    Point normalizedAxis = axis;
    normalizedAxis.normalize();

    // 旋转四元数
    double qx = normalizedAxis[0] * s;
    double qy = normalizedAxis[1] * s;
    double qz = normalizedAxis[2] * s;
    double qw = c;

    // 将当前欧拉角转换为四元数
    double cx = cos(m_Rotation[0] / 2.0);
    double sx = sin(m_Rotation[0] / 2.0);
    double cy = cos(m_Rotation[1] / 2.0);
    double sy = sin(m_Rotation[1] / 2.0);
    double cz = cos(m_Rotation[2] / 2.0);
    double sz = sin(m_Rotation[2] / 2.0);

    double currentQw = cx * cy * cz + sx * sy * sz;
    double currentQx = sx * cy * cz - cx * sy * sz;
    double currentQy = cx * sy * cz + sx * cy * sz;
    double currentQz = cx * cy * sz - sx * sy * cz;

    // 四元数乘法：新旋转 = 当前旋转 * 增量旋转
    double newQw = currentQw * qw - currentQx * qx - currentQy * qy - currentQz * qz;
    double newQx = currentQw * qx + currentQx * qw + currentQy * qz - currentQz * qy;
    double newQy = currentQw * qy - currentQx * qz + currentQy * qw + currentQz * qx;
    double newQz = currentQw * qz + currentQx * qy - currentQy * qx + currentQz * qw;

    // 将四元数转换回欧拉角
    // 绕X轴的旋转
    double sinr_cosp = 2.0 * (newQw * newQx + newQy * newQz);
    double cosr_cosp = 1.0 - 2.0 * (newQx * newQx + newQy * newQy);
    m_Rotation[0] = atan2(sinr_cosp, cosr_cosp);

    // 绕Y轴的旋转
    double sinp = 2.0 * (newQw * newQy - newQz * newQx);
    if (fabs(sinp) >= 1.0) m_Rotation[1] = copysign(M_PI / 2.0, sinp);
    else
        m_Rotation[1] = asin(sinp);

    // 绕Z轴的旋转
    double siny_cosp = 2.0 * (newQw * newQz + newQx * newQy);
    double cosy_cosp = 1.0 - 2.0 * (newQy * newQy + newQz * newQz);
    m_Rotation[2] = atan2(siny_cosp, cosy_cosp);
}

IGAME_NAMESPACE_END