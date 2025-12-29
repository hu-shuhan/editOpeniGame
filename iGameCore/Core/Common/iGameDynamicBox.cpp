#include "iGameDynamicBox.h"
#include <algorithm>
#include <cmath>

IGAME_NAMESPACE_BEGIN

static inline std::pair<Point, Point> MinMaxPoint() {
    return {Point(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                  std::numeric_limits<float>::max()),
            Point(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                  std::numeric_limits<float>::lowest())};
}

static inline void MinMaxPoint(Point& pMin, Point& pMax, const Point& p) {
    for (int i = 0; i < 3; i++) {
        pMin[i] = std::min<float>(pMin[i], p[i]);
        pMax[i] = std::max<float>(pMax[i], p[i]);
    }
}

static inline void MinPoint(Point& pMin, const Point& p) {
    for (int i = 0; i < 3; i++) { pMin[i] = std::min<float>(pMin[i], p[i]); }
}

static inline void MaxPoint(Point& pMax, const Point& p) {
    for (int i = 0; i < 3; i++) { pMax[i] = std::max<float>(pMax[i], p[i]); }
}

// 四元数转换为旋转矩阵
void DynamicBox::QuaternionToMatrix(const double q[4], double& r00, double& r01, double& r02, double& r10, double& r11,
                                    double& r12, double& r20, double& r21, double& r22) const {
    double w = q[0], x = q[1], y = q[2], z = q[3];

    // 计算四元数的平方
    double xx = x * x;
    double yy = y * y;
    double zz = z * z;
    double xy = x * y;
    double xz = x * z;
    double yz = y * z;
    double wx = w * x;
    double wy = w * y;
    double wz = w * z;

    // 行主序旋转矩阵（适用于行向量）
    r00 = 1.0 - 2.0 * (yy + zz);
    r01 = 2.0 * (xy - wz);
    r02 = 2.0 * (xz + wy);

    r10 = 2.0 * (xy + wz);
    r11 = 1.0 - 2.0 * (xx + zz);
    r12 = 2.0 * (yz - wx);

    r20 = 2.0 * (xz - wy);
    r21 = 2.0 * (yz + wx);
    r22 = 1.0 - 2.0 * (xx + yy);
}

// 获取旋转矩阵
void DynamicBox::GetRotationMatrix(double& r00, double& r01, double& r02, double& r10, double& r11, double& r12,
                                   double& r20, double& r21, double& r22) const {
    QuaternionToMatrix(m_Quaternion.data(), r00, r01, r02, r10, r11, r12, r20, r21, r22);
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

// 初始化消息
void DynamicBox::InitMsg(const Point& p1, const Point& p2) {
    m_Position = (p1 + p2) / 2.0;
    m_Length = Point(std::abs(p1[0] - p2[0]), std::abs(p1[1] - p2[1]), std::abs(p1[2] - p2[2]));

    // 初始化四元数为单位四元数（无旋转）
    m_Quaternion = {1.0, 0.0, 0.0, 0.0};
}

// 设置操作点
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

// 构造函数
DynamicBox::DynamicBox(const Point& p1, const Point& p2) {
    InitMsg(p1, p2);
    SetOpePoints();
}

// 移动操作点
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

// 根据操作点更新盒子
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

// 旋转盒子
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

// 移动位置
void DynamicBox::MovePosition(double x, double y, double z) { MovePosition(Point(x, y, z)); }

void DynamicBox::MovePosition(const Point& position) {
    m_Position = position;
    SetOpePoints();
}

// 获取中心点
const Point& DynamicBox::GetMidPoint() const { return m_Position; }

// 四元数归一化
void DynamicBox::NormalizeQuaternion() {
    double w = m_Quaternion[0];
    double x = m_Quaternion[1];
    double y = m_Quaternion[2];
    double z = m_Quaternion[3];

    double norm = std::sqrt(w * w + x * x + y * y + z * z);
    if (norm > 0.0) {
        m_Quaternion[0] = w / norm;
        m_Quaternion[1] = x / norm;
        m_Quaternion[2] = y / norm;
        m_Quaternion[3] = z / norm;
    } else {
        // 如果四元数为零，重置为单位四元数
        m_Quaternion = {1.0, 0.0, 0.0, 0.0};
    }
}

// 四元数乘法
void DynamicBox::MultiplyQuaternions(const double q1[4], const double q2[4], double result[4]) {
    result[0] = q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3]; // w
    result[1] = q1[0] * q2[1] + q1[1] * q2[0] + q1[2] * q2[3] - q1[3] * q2[2]; // x
    result[2] = q1[0] * q2[2] - q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1]; // y
    result[3] = q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0]; // z
}

// 欧拉角转四元数（顺序：ZYX，即先绕X轴，再Y轴，最后Z轴）
void DynamicBox::EulerToQuaternion(double x, double y, double z, double q[4]) const {
    double cx = cos(x * 0.5);
    double sx = sin(x * 0.5);
    double cy = cos(y * 0.5);
    double sy = sin(y * 0.5);
    double cz = cos(z * 0.5);
    double sz = sin(z * 0.5);

    q[0] = cx * cy * cz + sx * sy * sz; // w
    q[1] = sx * cy * cz - cx * sy * sz; // x
    q[2] = cx * sy * cz + sx * cy * sz; // y
    q[3] = cx * cy * sz - sx * sy * cz; // z
}

// 四元数转欧拉角（顺序：ZYX）
void DynamicBox::QuaternionToEuler(const double q[4], double& x, double& y, double& z) const {
    double w = q[0], xq = q[1], yq = q[2], zq = q[3];

    // 绕X轴的旋转
    double sinr_cosp = 2.0 * (w * xq + yq * zq);
    double cosr_cosp = 1.0 - 2.0 * (xq * xq + yq * yq);
    x = atan2(sinr_cosp, cosr_cosp);

    // 绕Y轴的旋转
    double sinp = 2.0 * (w * yq - zq * xq);
    if (fabs(sinp) >= 1.0) {
        y = copysign(M_PI / 2.0, sinp);
    } else {
        y = asin(sinp);
    }

    // 绕Z轴的旋转
    double siny_cosp = 2.0 * (w * zq + xq * yq);
    double cosy_cosp = 1.0 - 2.0 * (yq * yq + zq * zq);
    z = atan2(siny_cosp, cosy_cosp);
}

// 应用旋转（使用四元数）
void DynamicBox::ApplyRotation(const Point& axis, double angle) {
    // 创建增量旋转四元数
    double halfAngle = angle * 0.5;
    double s = sin(halfAngle);
    double c = cos(halfAngle);

    Point normalizedAxis = axis;
    normalizedAxis.normalize();

    double incrQ[4] = {c, normalizedAxis[0] * s, normalizedAxis[1] * s, normalizedAxis[2] * s};

    // 四元数乘法：新旋转 = 增量旋转 * 当前旋转
    double newQ[4];
    MultiplyQuaternions(incrQ, m_Quaternion.data(), newQ);

    // 更新四元数并归一化
    m_Quaternion[0] = newQ[0];
    m_Quaternion[1] = newQ[1];
    m_Quaternion[2] = newQ[2];
    m_Quaternion[3] = newQ[3];
    NormalizeQuaternion();
}

// 设置四元数
void DynamicBox::SetQuaternion(double w, double x, double y, double z) {
    m_Quaternion[0] = w;
    m_Quaternion[1] = x;
    m_Quaternion[2] = y;
    m_Quaternion[3] = z;
    NormalizeQuaternion();
}

// 设置旋转（欧拉角接口）
void DynamicBox::SetRotation(double xAngle, double yAngle, double zAngle) {
    double q[4];
    EulerToQuaternion(xAngle, yAngle, zAngle, q);

    m_Quaternion[0] = q[0];
    m_Quaternion[1] = q[1];
    m_Quaternion[2] = q[2];
    m_Quaternion[3] = q[3];

    // 更新所有操作点的位置
    SetOpePoints();
}

// 获取旋转（欧拉角接口）
const Point& DynamicBox::GetRotation() const {
    static Point eulerAngles; // 注意：这里使用static是为了返回引用，但这不是线程安全的

    double x, y, z;
    QuaternionToEuler(m_Quaternion.data(), x, y, z);

    eulerAngles[0] = x;
    eulerAngles[1] = y;
    eulerAngles[2] = z;

    return eulerAngles;
}

// 获取操作点
const std::array<Point, 6>& DynamicBox::GetOpePoints() const { return m_OpePoints; }

// 获取所有边
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

// 获取所有面
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

// 获取长度
const Point& DynamicBox::GetLength() const { return m_Length; }

// 设置长度（Point版本）
void DynamicBox::SetLength(const Point& newLength) {
    // 首先检查新的尺寸是否合法（不能是负数）
    Point finalLength = newLength;

    // 确保每个维度都是正数
    if (finalLength[0] < 0.0) finalLength[0] = 0.0;
    if (finalLength[1] < 0.0) finalLength[1] = 0.0;
    if (finalLength[2] < 0.0) finalLength[2] = 0.0;

    // 设置新的尺寸
    m_Length = finalLength;

    // 重要：更新所有操作点的位置
    // 因为中心点m_Position不变，只是尺寸变了
    SetOpePoints();
}

// 设置长度（三个double参数版本）
void DynamicBox::SetLength(double lengthX, double lengthY, double lengthZ) {
    // 创建Point对象并调用第一个版本
    SetLength(Point(lengthX, lengthY, lengthZ));
}

// 将局部坐标转换到世界坐标系
Point DynamicBox::LocalToWorld(const Point& localVec) const {
    double r00, r01, r02, r10, r11, r12, r20, r21, r22;
    GetRotationMatrix(r00, r01, r02, r10, r11, r12, r20, r21, r22);
    return ApplyRotationMatrix(localVec, r00, r01, r02, r10, r11, r12, r20, r21, r22);
}

// 获取极值点
std::pair<Point, Point> DynamicBox::GetExtremePoint() const {
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

    auto MinMaxP = MinMaxPoint();
    auto& [minP, maxP] = MinMaxP;
    for (int i = 0; i < 8; ++i) { MinMaxPoint(minP, maxP, worldVertices[i]); }
    return MinMaxP;
}

IGAME_NAMESPACE_END