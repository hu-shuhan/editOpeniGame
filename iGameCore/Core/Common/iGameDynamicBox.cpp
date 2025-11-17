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

// 辅助函数：将面的局部顶点变换到世界坐标系
std::array<Point, 4> DynamicBox::TransformFaceVertices(const std::array<Point, 4>& localVertices, double r00,
                                                       double r01, double r02, double r10, double r11, double r12,
                                                       double r20, double r21, double r22) const {
    std::array<Point, 4> worldVertices;

    for (int i = 0; i < 4; ++i) {
        Point rotatedPoint = ApplyRotationMatrix(localVertices[i], r00, r01, r02, r10, r11, r12, r20, r21, r22);
        worldVertices[i] = rotatedPoint + m_Position;
    }

    return worldVertices;
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

    // 获取旋转矩阵的逆矩阵（世界到局部坐标系的变换）
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);

    // 旋转矩阵的逆矩阵就是转置矩阵
    Point localDirection = ApplyRotationMatrix(direction, r00, r10, r20, r01, r11, r21, r02, r12, r22);

    // 获取操作点对应的局部坐标轴方向
    Point localAxis = GetFaceLocalNormal(pointIndex);

    // 计算输入方向在局部轴方向上的投影（在局部坐标系中）
    double dotProduct =
            localDirection[0] * localAxis[0] + localDirection[1] * localAxis[1] + localDirection[2] * localAxis[2];
    double projectionLength = dotProduct;

    // 在局部坐标系中计算移动向量
    Point localMoveVector = localAxis * projectionLength;

    // 根据移动的操作点更新盒子的位置和尺寸（在局部坐标系中处理）
    UpdateBoxFromOpePoint(pointIndex, localMoveVector);
}

void DynamicBox::UpdateBoxFromOpePoint(OpeInt pointIndex, const Point& localMoveVector) {
    // 获取旋转矩阵
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);

    // 在局部坐标系中更新盒子的尺寸
    Point newLength = m_Length;
    Point positionOffset(0, 0, 0);

    switch (pointIndex) {
        case UP:
            newLength[1] += localMoveVector[1] * 1.0;
            positionOffset[1] = localMoveVector[1] / 2.0;
            break;
        case BOTTOM:
            newLength[1] -= localMoveVector[1] * 1.0;
            positionOffset[1] = localMoveVector[1] / 2.0;
            break;
        case LEFT:
            newLength[0] -= localMoveVector[0] * 1.0;
            positionOffset[0] = localMoveVector[0] / 2.0;
            break;
        case RIGHT:
            newLength[0] += localMoveVector[0] * 1.0;
            positionOffset[0] = localMoveVector[0] / 2.0;
            break;
        case FRONT:
            newLength[2] += localMoveVector[2] * 1.0;
            positionOffset[2] = localMoveVector[2] / 2.0;
            break;
        case BACK:
            newLength[2] -= localMoveVector[2] * 1.0;
            positionOffset[2] = localMoveVector[2] / 2.0;
            break;
    }

    // 防止负长度
    if (newLength[0] < 0) newLength[0] = 0;
    if (newLength[1] < 0) newLength[1] = 0;
    if (newLength[2] < 0) newLength[2] = 0;

    // 更新尺寸
    m_Length = newLength;

    // 将位置偏移转换到世界坐标系并更新位置
    Point worldPositionOffset = ApplyRotationMatrix(positionOffset, r00, r01, r02, r10, r11, r12, r20, r21, r22);
    m_Position = m_Position + worldPositionOffset;

    // 更新所有操作点
    SetOpePoints();
}

void DynamicBox::RotateBox(const Point& camera, const Point& direction) {
    // 计算从相机指向盒子中心的向量
    Point cameraToCenter = m_Position - camera;

    // 计算旋转轴：相机到中心向量与旋转向量的叉积
    Point rotationAxis = direction.cross(cameraToCenter);

    // 归一化旋转轴
    if (rotationAxis.length() > 0) {
        rotationAxis.normalize();

        // 计算旋转角度（使用旋转向量的长度作为角度大小）
        double rotationAngle = direction.length();

        // 应用旋转
        ApplyRotation(rotationAxis, rotationAngle);

        // 更新操作点的位置
        SetOpePoints();
    }
}

void DynamicBox::MovePosition(const Point& position) {
    m_Position = position;
    SetOpePoints();
}

const Point& DynamicBox::GetMidPoint() const { return m_Position; }

const std::array<Point, 6>& DynamicBox::GetOpePoints() const { return m_OpePoints; }

std::vector<std::pair<Point, Point>> DynamicBox::GetAllEdges() const {
    std::vector<std::pair<Point, Point>> edges;
    edges.reserve(12); // 立方体有12条边

    // 获取半长（从中心到各面的距离）
    Point halfLength = m_Length / 2.0;

    // 局部坐标系下的8个顶点（未旋转状态）
    std::array<Point, 8> localVertices = {
            // 底面四个顶点（从前面左下角开始逆时针）
            Point(-halfLength[0], -halfLength[1], -halfLength[2]), // 0: 后-左-下
            Point(halfLength[0], -halfLength[1], -halfLength[2]),  // 1: 后-右-下
            Point(halfLength[0], halfLength[1], -halfLength[2]),   // 2: 后-右-上
            Point(-halfLength[0], halfLength[1], -halfLength[2]),  // 3: 后-左-上

            // 顶面四个顶点（从前面左下角开始逆时针）
            Point(-halfLength[0], -halfLength[1], halfLength[2]), // 4: 前-左-下
            Point(halfLength[0], -halfLength[1], halfLength[2]),  // 5: 前-右-下
            Point(halfLength[0], halfLength[1], halfLength[2]),   // 6: 前-右-上
            Point(-halfLength[0], halfLength[1], halfLength[2])   // 7: 前-左-上
    };

    // 获取旋转矩阵
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);

    // 将局部顶点转换到世界坐标系
    std::array<Point, 8> worldVertices;
    for (int i = 0; i < 8; ++i) {
        Point rotatedPoint = ApplyRotationMatrix(localVertices[i], r00, r01, r02, r10, r11, r12, r20, r21, r22);
        worldVertices[i] = rotatedPoint + m_Position;
    }

    // 定义12条边（每对数字代表两个顶点的索引）
    // 底面4条边
    edges.push_back({worldVertices[0], worldVertices[1]}); // 后边下
    edges.push_back({worldVertices[1], worldVertices[2]}); // 右边下
    edges.push_back({worldVertices[2], worldVertices[3]}); // 前边下
    edges.push_back({worldVertices[3], worldVertices[0]}); // 左边下

    // 顶面4条边
    edges.push_back({worldVertices[4], worldVertices[5]}); // 后边上
    edges.push_back({worldVertices[5], worldVertices[6]}); // 右边上
    edges.push_back({worldVertices[6], worldVertices[7]}); // 前边上
    edges.push_back({worldVertices[7], worldVertices[4]}); // 左边上

    // 侧面4条垂直边
    edges.push_back({worldVertices[0], worldVertices[4]}); // 左下垂直
    edges.push_back({worldVertices[1], worldVertices[5]}); // 右下垂直
    edges.push_back({worldVertices[2], worldVertices[6]}); // 右上垂直
    edges.push_back({worldVertices[3], worldVertices[7]}); // 左上垂直

    return edges;
}

std::array<std::array<Point, 4>, 6> DynamicBox::GetAllFaces() const {
    std::array<std::array<Point, 4>, 6> faces;

    // 获取旋转矩阵
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);

    // 获取半长
    Point halfLength = m_Length / 2.0;

    // 前面 (FRONT) - Z正方向 - 保持正确
    {
        std::array<Point, 4> localVerts = {
                Point(-halfLength[0], -halfLength[1], halfLength[2]), // 左下
                Point(halfLength[0], -halfLength[1], halfLength[2]),  // 右下
                Point(halfLength[0], halfLength[1], halfLength[2]),   // 右上
                Point(-halfLength[0], halfLength[1], halfLength[2])   // 左上
        };
        faces[FRONT] = TransformFaceVertices(localVerts, r00, r01, r02, r10, r11, r12, r20, r21, r22);
    }

    // 后面 (BACK) - Z负方向 - 修正为逆时针
    {
        std::array<Point, 4> localVerts = {
                Point(-halfLength[0], -halfLength[1], -halfLength[2]), // 左下
                Point(-halfLength[0], halfLength[1], -halfLength[2]),  // 左上
                Point(halfLength[0], halfLength[1], -halfLength[2]),   // 右上
                Point(halfLength[0], -halfLength[1], -halfLength[2])   // 右下
        };
        faces[BACK] = TransformFaceVertices(localVerts, r00, r01, r02, r10, r11, r12, r20, r21, r22);
    }

    // 上面 (UP) - Y正方向 - 修正为逆时针
    {
        std::array<Point, 4> localVerts = {
                Point(-halfLength[0], halfLength[1], -halfLength[2]), // 后左
                Point(-halfLength[0], halfLength[1], halfLength[2]),  // 前左
                Point(halfLength[0], halfLength[1], halfLength[2]),   // 前右
                Point(halfLength[0], halfLength[1], -halfLength[2])   // 后右
        };
        faces[UP] = TransformFaceVertices(localVerts, r00, r01, r02, r10, r11, r12, r20, r21, r22);
    }

    // 下面 (BOTTOM) - Y负方向 - 保持正确
    {
        std::array<Point, 4> localVerts = {
                Point(-halfLength[0], -halfLength[1], -halfLength[2]), // 后左
                Point(halfLength[0], -halfLength[1], -halfLength[2]),  // 后右
                Point(halfLength[0], -halfLength[1], halfLength[2]),   // 前右
                Point(-halfLength[0], -halfLength[1], halfLength[2])   // 前左
        };
        faces[BOTTOM] = TransformFaceVertices(localVerts, r00, r01, r02, r10, r11, r12, r20, r21, r22);
    }

    // 右面 (RIGHT) - X正方向 - 保持正确
    {
        std::array<Point, 4> localVerts = {
                Point(halfLength[0], -halfLength[1], -halfLength[2]), // 下后
                Point(halfLength[0], halfLength[1], -halfLength[2]),  // 上后
                Point(halfLength[0], halfLength[1], halfLength[2]),   // 上前
                Point(halfLength[0], -halfLength[1], halfLength[2])   // 下前
        };
        faces[RIGHT] = TransformFaceVertices(localVerts, r00, r01, r02, r10, r11, r12, r20, r21, r22);
    }

    // 左面 (LEFT) - X负方向 - 修正为逆时针
    {
        std::array<Point, 4> localVerts = {
                Point(-halfLength[0], halfLength[1], halfLength[2]),   // 上前
                Point(-halfLength[0], halfLength[1], -halfLength[2]),  // 上后
                Point(-halfLength[0], -halfLength[1], -halfLength[2]), // 下后
                Point(-halfLength[0], -halfLength[1], halfLength[2])   // 下前
        };
        faces[LEFT] = TransformFaceVertices(localVerts, r00, r01, r02, r10, r11, r12, r20, r21, r22);
    }

    return faces;
}

const Point& DynamicBox::GetLength() const { return m_Length; }

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