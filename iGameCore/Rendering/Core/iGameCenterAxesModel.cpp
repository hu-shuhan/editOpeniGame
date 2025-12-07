#include "iGameCenterAxesModel.h"
#include "iGameCenterAxesModel.h"
#include "OpenGL/GLShader.h"

IGAME_NAMESPACE_BEGIN

CenterAxesModel::CenterAxesModel() {
    // 初始化所有必需的数组
    m_RotationCenter = igm::vec3(0.0f, 0.0f, 0.0f);
    m_Positions = FloatArray::New();
    m_Colors = FloatArray::New();
    m_LineIndices = UnsignedIntArray::New();

    InitializeGeometry();

    // 设置渲染参数（使用可用的公共接口）
    SetVisibility(false);
    SetLineWidth(DEFAULT_LINE_WIDTH);
}

void CenterAxesModel::SetRotationCenter(igm::vec3 center) {
    m_RotationCenter = center;
}

igm::vec3 CenterAxesModel::GetRotationCenter() const {
    return m_RotationCenter;
}

void CenterAxesModel::InitializeGeometry() {
    if (m_GeometryInitialized) return;

    //=== 1. 初始化顶点数据 ===
    //m_Positions = FloatArray::New();
    m_Positions->SetDimension(3);
    m_Positions->AddElement3(m_RotationCenter.x, m_RotationCenter.y,
                             m_RotationCenter.z); // 原点 (索引0)
    m_Positions->AddElement3(-DEFAULT_AXIS_LENGTH, 0.0f,
                             0.0f); // X轴起点 (索引1)
    m_Positions->AddElement3(DEFAULT_AXIS_LENGTH, 0.0f,
                             0.0f); // X轴终点 (索引2)
    m_Positions->AddElement3(0.0f, -DEFAULT_AXIS_LENGTH,
                             0.0f); // Y轴起点 (索引3)
    m_Positions->AddElement3(0.0f, DEFAULT_AXIS_LENGTH,
                             0.0f); // Y轴终点 (索引4)
    m_Positions->AddElement3(0.0f, 0.0f,
                             -DEFAULT_AXIS_LENGTH); // Z轴起点 (索引5)
    m_Positions->AddElement3(0.0f, 0.0f,
                             DEFAULT_AXIS_LENGTH); // Z轴终点 (索引6)

    //=== 2. 初始化颜色数据 ===
    //m_Colors = FloatArray::New();
    m_Colors->SetDimension(3);
    // 每个顶点对应颜色（与位置一一对应）
    m_Colors->AddElement3(1.0f, 0.0f, 0.0f); // 原点颜色
    m_Colors->AddElement3(1.0f, 0.0f, 0.0f); // X轴红色
    m_Colors->AddElement3(1.0f, 0.0f, 0.0f);
    m_Colors->AddElement3(0.0f, 1.0f, 0.0f); // Y轴绿色
    m_Colors->AddElement3(0.0f, 1.0f, 0.0f);
    m_Colors->AddElement3(0.0f, 0.0f, 1.0f); // Z轴蓝色
    m_Colors->AddElement3(0.0f, 0.0f, 1.0f);

    //=== 3. 初始化线段索引 ===
    //m_LineIndices = UnsignedIntArray::New();
    m_LineIndices->SetDimension(2);
    m_LineIndices->AddElement2(1, 2); // X轴
    m_LineIndices->AddElement2(3, 4); // Y轴
    m_LineIndices->AddElement2(5, 6); // Z轴

    //=== 4. 配置渲染参数 ===
    SetLineWidth(DEFAULT_LINE_WIDTH);

    //=== 5. 标记数据已更新 ===
    m_UseColor = true; // 启用颜色
    m_Positions->Modified();
    m_Colors->Modified();
    m_LineIndices->Modified();
    //Modified(); // 通知DrawObject基类

    m_GeometryInitialized = true;
}

void CenterAxesModel::PrepareForRendering() {
    ConvertToDrawableData();
    SyncGpuBuffers();
}

void CenterAxesModel::UpdateAxisScale(float cameraDistance, float fov,
                                      int viewportHeight) {
    // 根据相机距离动态计算轴长，保持屏幕固定大小
    float scaleFactor = (cameraDistance * m_BaseScreenSize) /
                        (viewportHeight * tan(fov * 0.5f));
    m_CurrentAxisLength = scaleFactor;

    // 触发几何更新
    ConvertToDrawableData();
}

void CenterAxesModel::ConvertToDrawableData() {
    // 如果几何数据未初始化，先初始化基本结构
    if (!m_GeometryInitialized) {
        InitializeGeometry();
        return;
    }

    // 获取当前旋转中心位置
    const auto& center = m_RotationCenter;

    //=== 1. 更新顶点位置数据 ===
    m_Positions->Reset();
    m_Positions->SetDimension(3);

    // 原点 (索引0)
    m_Positions->AddElement3(center.x, center.y, center.z);

    // X轴起点和终点 (索引1和2)
    m_Positions->AddElement3(center[0] - m_CurrentAxisLength, center[1],
                             center[2]);
    m_Positions->AddElement3(center[0] + m_CurrentAxisLength, center[1],
                             center[2]);

    // Y轴起点和终点 (索引3和4)
    m_Positions->AddElement3(center[0], center[1] - m_CurrentAxisLength,
                             center[2]);
    m_Positions->AddElement3(center[0], center[1] + m_CurrentAxisLength,
                             center[2]);

    // Z轴起点和终点 (索引5和6)
    m_Positions->AddElement3(center[0], center[1],
                             center[2] - m_CurrentAxisLength);
    m_Positions->AddElement3(center[0], center[1],
                             center[2] + m_CurrentAxisLength);

    //=== 2. 更新颜色数据 (保持不变) ===

    //=== 3. 更新线段索引 (保持不变) ===


    //=== 4. 标记数据已更新 ===
    m_Positions->Modified();
    Modified(); // 通知基类数据已更新
}

void CenterAxesModel::HandleDrag(igm::vec3 worldOffset) {
    // 更新旋转中心（保持原深度）
    m_RotationCenter += worldOffset;
    SetRotationCenter(m_RotationCenter); // 同步到场景

    // 立即刷新几何数据
    ConvertToDrawableData();
    SyncGpuBuffers();
}

void CenterAxesModel::SetHighlight(bool highlight) {
    if (!m_Colors) return;

    if (highlight) {
        // 应用高亮颜色：所有轴变为橙色
        m_Colors->Reset();
        m_Colors->SetDimension(3);

        // 所有顶点使用橙色
        igm::vec3 orange(1.0f, 0.5f, 0.0f);
        for (int i = 0; i < 7; i++) {
            m_Colors->AddElement3(orange.x, orange.y, orange.z);
        }
    } else {
        // 恢复原始颜色
        m_Colors->Reset();
        m_Colors->SetDimension(3);

        // 原始颜色布局
        m_Colors->AddElement3(1.0f, 0.0f, 0.0f); // 原点红色
        m_Colors->AddElement3(1.0f, 0.0f, 0.0f); // X轴红色
        m_Colors->AddElement3(1.0f, 0.0f, 0.0f);
        m_Colors->AddElement3(0.0f, 1.0f, 0.0f); // Y轴绿色
        m_Colors->AddElement3(0.0f, 1.0f, 0.0f);
        m_Colors->AddElement3(0.0f, 0.0f, 1.0f); // Z轴蓝色
        m_Colors->AddElement3(0.0f, 0.0f, 1.0f);
    }

    // 标记数据已修改
    m_Colors->Modified();
    Modified();
}
IGAME_NAMESPACE_END
