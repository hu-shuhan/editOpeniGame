#ifndef IGAME_CENTER_AXES_MODEL_H
#define IGAME_CENTER_AXES_MODEL_H


#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameDrawObject.h"


IGAME_NAMESPACE_BEGIN

/**
 * @class CenterAxesModel
 * @brief 显示旋转中心的固定三色坐标轴模型（始终渲染在最上层）
 * 
 * 特性：
 * - 固定几何数据（单位长度XYZ轴线）
 * - 通过模型矩阵动态控制位置/缩放
 * - 默认禁用深度测试确保不被遮挡
 */
class CenterAxesModel : public DrawObject {
public:
    I_OBJECT(CenterAxesModel);
    static Pointer New() { return new CenterAxesModel; }

    void SetRotationCenter(igm::vec3 center);

    igm::vec3 GetRotationCenter() const;

    void ConvertToDrawableData() override;
    /**
     * @brief 准备渲染数据
     */
    void PrepareForRendering();

    void HandleDrag(igm::vec3 worldOffset);

    // ��������
    void SetScreenSize(float pixelSize) { m_BaseScreenSize = pixelSize; }
    void UpdateAxisScale(float cameraDistance, float fov, int viewportHeight);


    CenterAxesModel();
    ~CenterAxesModel() override = default;

     
    

protected:
    void InitializeGeometry();
    // 重写数据转换方法
    //void ConvertToDrawableData() override;

private:
    void UpdateGeometry(); // 内部几何更新方法
    // 固定参数
    static constexpr float DEFAULT_AXIS_LENGTH = 0.2f;
    static constexpr float DEFAULT_LINE_WIDTH = 1.0f;
    bool m_GeometryInitialized{false};
    igm::vec3 m_RotationCenter;
    igm::mat4 m_ModelMatrix;

    // ��������̬���ſ���
    float m_CurrentAxisLength = DEFAULT_AXIS_LENGTH; // ��ǰ�᳤
    float m_BaseScreenSize = 20.0f;                  // Ŀ����Ļ���ش�С


};

IGAME_NAMESPACE_END
#endif // IGAME_CENTER_AXES_MODEL_H