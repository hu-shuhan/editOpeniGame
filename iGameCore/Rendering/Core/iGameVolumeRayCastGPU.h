#ifndef IGAMEVIS_IGAMEVOLUMERAYCASTGPU_H
#define IGAMEVIS_IGAMEVOLUMERAYCASTGPU_H

#include "OpenGL/GLShader.h"
#include "OpenGL/GLTexture2d.h"
#include "OpenGL/GLTexture3d.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameCamera.h"
#include "iGameObject.h"
#include "iGameStructuredMesh.h"
#include "iGameVolumeTransferFunction.h"
#include "igm/igm.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameVolumeRayCastGPU
 * @brief 单进程 GPU 光线投射体渲染器（阶段 1 验证后端）。
 *
 * @details
 *  把 StructuredMesh 的点标量场上传到 3D 纹理，把传输函数烘焙成 1D LUT 纹理，
 *  在全屏三角形上逐像素做 slab 求交 + 光线步进采样 + front-to-back 合成，
 *  并回写首命中深度（reversed-z）。
 */
class iGameVolumeRayCastGPU : public Object {
public:
    I_OBJECT(iGameVolumeRayCastGPU);
    static Pointer New() { return new iGameVolumeRayCastGPU; }

    /** 设置输入体网格并上传标量场。失败（如非 3D 或无标量）返回 false。 */
    bool SetInput(StructuredMesh::Pointer mesh);

    /** 设置传输函数并烘焙/上传 LUT。 */
    void SetTransferFunction(iGameVolumeTransferFunction::Pointer tf);

    /**
     * 渲染体数据。调用前需已绑定目标帧缓冲并设置好深度/混合状态。
     * @param shader VOLUMERAYCAST 着色器程序。
     * @param camera 相机。
     * @param modelMatrix 场景模型矩阵（局部空间 -> 世界空间）。
     * @param viewport 视口尺寸（像素）。
     */
    void Render(SmartPointer<GLShaderProgram> shader,
                SmartPointer<Camera> camera, const igm::mat4& modelMatrix,
                const igm::uvec2& viewport);

    void SetMaxSamples(int maxSamples) {
        m_MaxSamples = maxSamples < 1 ? 1 : maxSamples;
    }
    int GetMaxSamples() const { return m_MaxSamples; }
    /** 局部空间步长；<=0 时按 maxSamples 自动推导。 */
    void SetStepSize(float stepSize) { m_StepSize = stepSize; }
    float GetStepSize() const { return m_StepSize; }

    /** 上传的标量场实际数据范围（用于颜色映射范围未初始化时的回退）。 */
    float GetDataMin() const { return m_DataMin; }
    float GetDataMax() const { return m_DataMax; }

protected:
    iGameVolumeRayCastGPU();
    ~iGameVolumeRayCastGPU() override;

    void EnsureGLResources();
    bool UploadVolumeTexture(StructuredMesh::Pointer mesh);
    void UploadLUTTexture();

    GLTexture3d::Pointer m_VolumeTexture;
    GLTexture2d::Pointer m_LUTTexture;
    GLVertexArray::Pointer m_EmptyVAO;

    StructuredMesh::Pointer m_Input; // 缓存输入，避免每帧重复上传 3D 纹理
    iGameVolumeTransferFunction::Pointer m_TransferFunction;

    igm::vec3 m_BoxMin{0.0f};
    igm::vec3 m_BoxMax{1.0f};
    float m_ScalarMin{0.0f};
    float m_ScalarMax{1.0f};
    float m_DataMin{0.0f};
    float m_DataMax{1.0f};
    int m_LUTResolution{-1};

    int m_MaxSamples{512};
    float m_StepSize{0.0f};

    bool m_GLReady{false};
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_IGAMEVOLUMERAYCASTGPU_H
