#include "iGameDynamicBox.h"
#include "igm/transform.h"
#include <algorithm>
#include <cmath>

IGAME_NAMESPACE_BEGIN

// 静态辅助函数保持不变
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

// 创建旋转矩阵
igm::mat4 DynamicBox::CreateRotationMatrix(const igm::vec3& axis, float angle) {
    return igm::rotate(igm::mat4(1.0f), angle, axis);
}

// 创建平移矩阵
igm::mat4 DynamicBox::CreateTranslationMatrix(const Point& translation) {
    return igm::translate(igm::mat4(1.0f),
                          igm::vec3(static_cast<float>(translation[0]), static_cast<float>(translation[1]),
                                    static_cast<float>(translation[2])));
}

// 初始化消息
void DynamicBox::InitMsg(const Point& p1, const Point& p2) {
    m_Position = (p1 + p2) / 2.0;

    //############ TEST ############
    //OldP = m_Position;
    //NewP = m_Position;
    //############ TEST ############

    m_Length = Point(std::abs(p1[0] - p2[0]), std::abs(p1[1] - p2[1]), std::abs(p1[2] - p2[2]));

    // 初始化旋转矩阵为单位矩阵
    m_RotationMatrix = igm::mat4(1.0f);

    // 设置操作点
    SetOpePoints();
}

// 设置操作点
void DynamicBox::SetOpePoints() {
    // 获取半长（从中心到各面的距离）
    Point halfLength = m_Length / 2.0;

    // 局部坐标系中的操作点位置
    std::array<Point, 6> localPoints = {
            Point(0, halfLength[1], 0),  // UP
            Point(0, -halfLength[1], 0), // BOTTOM
            Point(-halfLength[0], 0, 0), // LEFT
            Point(halfLength[0], 0, 0),  // RIGHT
            Point(0, 0, halfLength[2]),  // FRONT
            Point(0, 0, -halfLength[2])  // BACK
    };

    // 获取完整的变换矩阵
    igm::mat4 transform = GetTransformMatrix();

    // 对每个局部操作点应用变换
    for (int i = 0; i < 6; ++i) {
        igm::vec4 localPoint(localPoints[i][0], localPoints[i][1], localPoints[i][2], 1.0f);
        igm::vec4 worldPoint = transform * localPoint;
        m_OpePoints[i] = Point(worldPoint.x, worldPoint.y, worldPoint.z);
    }
}

// 获取变换矩阵（旋转+平移）
igm::mat4 DynamicBox::GetTransformMatrix() const {
    igm::mat4 translation = CreateTranslationMatrix(m_Position);
    return translation * m_RotationMatrix; // 先旋转，后平移
}

// 构造函数
DynamicBox::DynamicBox(const Point& p1, const Point& p2) { InitMsg(p1, p2); }

// 移动操作点
void DynamicBox::MoveOpePoint(OpeInt pointIndex, const Point& direction) {
    if (pointIndex < 0 || pointIndex >= 6) return;

    // 获取逆变换矩阵（世界到局部）
    igm::mat4 transform = GetTransformMatrix();
    igm::mat4 inverseTransform = transform.invert();

    // 将方向向量转换到局部坐标系
    igm::vec4 worldDir(direction[0], direction[1], direction[2], 0.0f);
    igm::vec4 localDir = inverseTransform * worldDir;

    // 获取操作点对应的局部坐标轴方向
    Point localAxis = GetFaceLocalNormal(pointIndex);

    // 计算在局部轴方向上的投影
    double dotProduct = localDir.x * localAxis[0] + localDir.y * localAxis[1] + localDir.z * localAxis[2];
    //double projectionLength = dotProduct;
    double projectionLength = std::copysign(direction.length(), dotProduct);

    // 在局部坐标系中计算移动向量
    Point localMoveVector = localAxis * projectionLength;

    // 根据移动的操作点更新盒子的位置和尺寸
    UpdateBoxFromOpePoint(pointIndex, localMoveVector);
}

// 根据操作点更新盒子
void DynamicBox::UpdateBoxFromOpePoint(OpeInt pointIndex, const Point& localMoveVector) {
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
    if (newLength[0] < 0.01) newLength[0] = 0.01;
    if (newLength[1] < 0.01) newLength[1] = 0.01;
    if (newLength[2] < 0.01) newLength[2] = 0.01;

    // 更新尺寸
    m_Length = newLength;

    // 更新位置
    if (positionOffset.length() > 0) {
        // 将位置偏移从局部坐标系转换到世界坐标系
        igm::vec4 localOffset(positionOffset[0], positionOffset[1], positionOffset[2], 0.0f);
        igm::vec4 worldOffset = m_RotationMatrix * localOffset;
        m_Position = m_Position + Point(worldOffset.x, worldOffset.y, worldOffset.z);
    }

    // 更新所有操作点
    SetOpePoints();
}
// 从旋转矩阵中提取欧拉角（XYZ顺序）
static Point ExtractEulerAnglesFromMatrix(const igm::mat4& rotationMatrix) {
    // 获取3x3旋转部分
    float m00 = rotationMatrix[0][0];
    float m01 = rotationMatrix[0][1];
    float m02 = rotationMatrix[0][2];
    float m10 = rotationMatrix[1][0];
    float m11 = rotationMatrix[1][1];
    float m12 = rotationMatrix[1][2];
    float m20 = rotationMatrix[2][0];
    float m21 = rotationMatrix[2][1];
    float m22 = rotationMatrix[2][2];

    float x, y, z;

    // 处理万向锁情况
    if (std::abs(m20) < 0.999999f) {
        // 没有万向锁
        y = std::asin(-m20); // pitch (绕Y轴)
        float cosY = std::cos(y);
        x = std::atan2(m21 / cosY, m22 / cosY); // yaw (绕X轴)
        z = std::atan2(m10 / cosY, m00 / cosY); // roll (绕Z轴)
    } else {
        // 万向锁情况
        z = 0.0f; // 可以任意设置

        if (m20 < 0.0f) { // m20 = -1
            y = IGM_PI / 2.0f;
            x = std::atan2(m01, m02);
        } else { // m20 = 1
            y = -IGM_PI / 2.0f;
            x = std::atan2(-m01, -m02);
        }
    }

    return Point(x, y, z);
}

// 获取旋转矩阵转换的欧拉角
Point DynamicBox::GetRotation() const { return ExtractEulerAnglesFromMatrix(m_RotationMatrix); }

// 旋转盒子（基于BasicStyle中的旋转方法）
void DynamicBox::RotateBox(const Point& oldP, const Point& newP) {
    // 计算旋转轴
    Point cToO = oldP - m_Position;
    Point cToN = newP - m_Position;
    Point direction = newP - oldP;
    igm::vec3 cToOVec(cToO[0], cToO[1], cToO[2]);
    igm::vec3 cToNVec(cToN[0], cToN[1], cToN[2]);

    igm::vec3 axis = igm::cross(cToOVec, cToNVec);

    if (axis.length() < 1e-7) {
        axis = igm::vec3(1.0f, 0.0f, 0.0f); // 默认绕X轴旋转
    } else {
        axis.normalize();
    }

    // 计算旋转角度
    const float trackballRadius = 0.6f;
    float t = 0.5f * direction.length() / (trackballRadius * this->GetLength().length());
    t = std::max(-1.0f, std::min(1.0f, t));

    float phi = 2.0f * asin(t);
    float angle = phi * 180.0f / IGM_PI;

    // 创建绕中心点旋转的矩阵（类似BasicStyle的方法）
    igm::vec4 centerVec(m_Position[0], m_Position[1], m_Position[2], 1.0f);

    // 构建旋转矩阵
    igm::mat4 translateToOrigin = igm::translate(igm::mat4(1.0f), -centerVec.xyz());
    igm::mat4 translateBack = igm::translate(igm::mat4(1.0f), centerVec.xyz());
    igm::mat4 rotate = CreateRotationMatrix(axis, static_cast<float>(igm::radians(angle)));

    // 完整的旋转变换
    igm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;

    // 更新旋转矩阵
    ApplyRotation(rotate);
}

// 应用旋转矩阵
void DynamicBox::ApplyRotation(const igm::mat4& rotationMatrix) {
    // 更新旋转矩阵：新旋转 = 增量旋转 * 当前旋转
    m_RotationMatrix = rotationMatrix * m_RotationMatrix;

    // 更新操作点位置
    SetOpePoints();
}

// 应用旋转（轴角）
void DynamicBox::ApplyRotation(const igm::vec3& axis, float angle) {
    igm::mat4 rotation = CreateRotationMatrix(axis, angle);
    ApplyRotation(rotation);
}


// 移动位置
void DynamicBox::MovePosition(double x, double y, double z) { MovePosition(Point(x, y, z)); }

void DynamicBox::MovePosition(const Point& position) {
    m_Position = position;
    SetOpePoints();
}

void DynamicBox::MoveBox(const Point& dir) {
    m_Position += dir;
    SetOpePoints();
}

// 获取中心点
const Point& DynamicBox::GetMidPoint() const { return m_Position; }

// 获取旋转矩阵
const igm::mat4& DynamicBox::GetRotationMatrix() const { return m_RotationMatrix; }

// 设置旋转（欧拉角）
void DynamicBox::SetRotation(float xAngle, float yAngle, float zAngle) {
    // 创建绕各轴旋转的矩阵
    igm::mat4 rotX = igm::rotate(igm::mat4(1.0f), xAngle, igm::vec3(1.0f, 0.0f, 0.0f));
    igm::mat4 rotY = igm::rotate(igm::mat4(1.0f), yAngle, igm::vec3(0.0f, 1.0f, 0.0f));
    igm::mat4 rotZ = igm::rotate(igm::mat4(1.0f), zAngle, igm::vec3(0.0f, 0.0f, 1.0f));

    // 组合旋转矩阵（通常的顺序是：先绕Z，再绕Y，最后绕X）
    m_RotationMatrix = rotX * rotY * rotZ;

    SetOpePoints();
}

// 设置旋转（矩阵）
void DynamicBox::SetRotation(const igm::mat4& rotationMatrix) {
    m_RotationMatrix = rotationMatrix;
    SetOpePoints();
}

// 获取操作点
const std::array<Point, 6>& DynamicBox::GetOpePoints() const { return m_OpePoints; }

// 获取面的局部法线
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
        default:
            return Point(0, 0, 0);
    }
}

// 获取面的局部顶点
std::array<Point, 4> DynamicBox::GetFaceLocalVertices(OpeInt face) const {
    Point halfLength = m_Length / 2.0;

    switch (face) {
        case FRONT: // Z正方向
            return {Point(-halfLength[0], -halfLength[1], halfLength[2]),
                    Point(halfLength[0], -halfLength[1], halfLength[2]),
                    Point(halfLength[0], halfLength[1], halfLength[2]),
                    Point(-halfLength[0], halfLength[1], halfLength[2])};
        case BACK: // Z负方向
            return {Point(-halfLength[0], -halfLength[1], -halfLength[2]),
                    Point(-halfLength[0], halfLength[1], -halfLength[2]),
                    Point(halfLength[0], halfLength[1], -halfLength[2]),
                    Point(halfLength[0], -halfLength[1], -halfLength[2])};
        case UP: // Y正方向
            return {Point(-halfLength[0], halfLength[1], -halfLength[2]),
                    Point(-halfLength[0], halfLength[1], halfLength[2]),
                    Point(halfLength[0], halfLength[1], halfLength[2]),
                    Point(halfLength[0], halfLength[1], -halfLength[2])};
        case BOTTOM: // Y负方向
            return {Point(-halfLength[0], -halfLength[1], -halfLength[2]),
                    Point(halfLength[0], -halfLength[1], -halfLength[2]),
                    Point(halfLength[0], -halfLength[1], halfLength[2]),
                    Point(-halfLength[0], -halfLength[1], halfLength[2])};
        case RIGHT: // X正方向
            return {Point(halfLength[0], -halfLength[1], -halfLength[2]),
                    Point(halfLength[0], halfLength[1], -halfLength[2]),
                    Point(halfLength[0], halfLength[1], halfLength[2]),
                    Point(halfLength[0], -halfLength[1], halfLength[2])};
        case LEFT: // X负方向
            return {Point(-halfLength[0], halfLength[1], halfLength[2]),
                    Point(-halfLength[0], halfLength[1], -halfLength[2]),
                    Point(-halfLength[0], -halfLength[1], -halfLength[2]),
                    Point(-halfLength[0], -halfLength[1], halfLength[2])};
        default:
            return {};
    }
}

// 获取所有边
std::vector<std::pair<Point, Point>> DynamicBox::GetAllEdges() const {
    std::vector<std::pair<Point, Point>> edges;
    edges.reserve(12);

    // 获取8个顶点
    Point halfLength = m_Length / 2.0;
    std::array<Point, 8> localVertices = {
            // 底面四个顶点
            Point(-halfLength[0], -halfLength[1], -halfLength[2]), Point(halfLength[0], -halfLength[1], -halfLength[2]),
            Point(halfLength[0], halfLength[1], -halfLength[2]), Point(-halfLength[0], halfLength[1], -halfLength[2]),
            // 顶面四个顶点
            Point(-halfLength[0], -halfLength[1], halfLength[2]), Point(halfLength[0], -halfLength[1], halfLength[2]),
            Point(halfLength[0], halfLength[1], halfLength[2]), Point(-halfLength[0], halfLength[1], halfLength[2])};

    // 变换到世界坐标系
    igm::mat4 transform = GetTransformMatrix();
    std::array<Point, 8> worldVertices;
    for (int i = 0; i < 8; ++i) {
        igm::vec4 localVert(localVertices[i][0], localVertices[i][1], localVertices[i][2], 1.0f);
        igm::vec4 worldVert = transform * localVert;
        worldVertices[i] = Point(worldVert.x, worldVert.y, worldVert.z);
    }

    // 定义12条边
    // 底面4条边
    edges.push_back({worldVertices[0], worldVertices[1]});
    edges.push_back({worldVertices[1], worldVertices[2]});
    edges.push_back({worldVertices[2], worldVertices[3]});
    edges.push_back({worldVertices[3], worldVertices[0]});

    // 顶面4条边
    edges.push_back({worldVertices[4], worldVertices[5]});
    edges.push_back({worldVertices[5], worldVertices[6]});
    edges.push_back({worldVertices[6], worldVertices[7]});
    edges.push_back({worldVertices[7], worldVertices[4]});

    // 侧面4条垂直边
    edges.push_back({worldVertices[0], worldVertices[4]});
    edges.push_back({worldVertices[1], worldVertices[5]});
    edges.push_back({worldVertices[2], worldVertices[6]});
    edges.push_back({worldVertices[3], worldVertices[7]});

    return edges;
}

// 获取所有面
std::array<std::array<Point, 4>, 6> DynamicBox::GetAllFaces() const {
    std::array<std::array<Point, 4>, 6> faces;

    igm::mat4 transform = GetTransformMatrix();

    for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
        std::array<Point, 4> localVerts = GetFaceLocalVertices(static_cast<OpeInt>(faceIndex));
        std::array<Point, 4> worldVerts;

        for (int i = 0; i < 4; ++i) {
            igm::vec4 localVert(localVerts[i][0], localVerts[i][1], localVerts[i][2], 1.0f);
            igm::vec4 worldVert = transform * localVert;
            worldVerts[i] = Point(worldVert.x, worldVert.y, worldVert.z);
        }

        faces[faceIndex] = worldVerts;
    }

    return faces;
}

// 获取长度
const Point& DynamicBox::GetLength() const { return m_Length; }

// 设置长度
void DynamicBox::SetLength(const Point& newLength) {
    Point finalLength = newLength;

    if (finalLength[0] < 0.01) finalLength[0] = 0.01;
    if (finalLength[1] < 0.01) finalLength[1] = 0.01;
    if (finalLength[2] < 0.01) finalLength[2] = 0.01;

    m_Length = finalLength;
    SetOpePoints();
}

void DynamicBox::SetLength(double lengthX, double lengthY, double lengthZ) {
    SetLength(Point(lengthX, lengthY, lengthZ));
}

// 局部坐标转世界坐标
Point DynamicBox::LocalToWorld(const Point& localVec) const {
    igm::vec4 local(localVec[0], localVec[1], localVec[2], 1.0f);
    igm::vec4 world = GetTransformMatrix() * local;
    return Point(world.x, world.y, world.z);
}

// 世界坐标转局部坐标
Point DynamicBox::WorldToLocal(const Point& worldVec) const {
    igm::mat4 transform = GetTransformMatrix();
    igm::mat4 inverseTransform = transform.invert();
    igm::vec4 world(worldVec[0], worldVec[1], worldVec[2], 1.0f);
    igm::vec4 local = inverseTransform * world;
    return Point(local.x, local.y, local.z);
}

// 获取极值点
std::pair<Point, Point> DynamicBox::GetExtremePoint() const {
    Point halfLength = m_Length / 2.0;

    std::array<Point, 8> localVertices = {
            Point(-halfLength[0], -halfLength[1], -halfLength[2]), Point(halfLength[0], -halfLength[1], -halfLength[2]),
            Point(halfLength[0], halfLength[1], -halfLength[2]),   Point(-halfLength[0], halfLength[1], -halfLength[2]),
            Point(-halfLength[0], -halfLength[1], halfLength[2]),  Point(halfLength[0], -halfLength[1], halfLength[2]),
            Point(halfLength[0], halfLength[1], halfLength[2]),    Point(-halfLength[0], halfLength[1], halfLength[2])};

    igm::mat4 transform = GetTransformMatrix();
    std::array<Point, 8> worldVertices;
    for (int i = 0; i < 8; ++i) {
        igm::vec4 localVert(localVertices[i][0], localVertices[i][1], localVertices[i][2], 1.0f);
        igm::vec4 worldVert = transform * localVert;
        worldVertices[i] = Point(worldVert.x, worldVert.y, worldVert.z);
    }

    auto [minP, maxP] = MinMaxPoint();
    for (int i = 0; i < 8; ++i) { MinMaxPoint(minP, maxP, worldVertices[i]); }

    return {minP, maxP};
}

IGAME_NAMESPACE_END