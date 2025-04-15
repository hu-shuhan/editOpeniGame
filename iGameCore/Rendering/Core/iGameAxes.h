/**
 * @class    Axes
 * @brief    Axes类用于绘制三维坐标轴以及相关操作。
 *
 * Axes类提供了坐标轴的初始化、绘制和数据处理等功能。它支持坐标系的旋转与变换，
 * 并通过OpenGL上下文进行渲染，同时能够实现显示坐标和世界坐标的相互转换。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLTexture2d.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameFontManager.h"

IGAME_NAMESPACE_BEGIN

class Scene;

class Axes : public Object {
public:
    I_OBJECT(Axes);
    static Pointer New() { return new Axes; }

    /**
     * @brief 设置关联的场景对象。
     * @param scene 场景对象的智能指针。
     */
    void SetScene(SmartPointer<Scene> scene);

    /**
     * @brief 获取当前关联的场景对象。
     * @return 场景对象的智能指针，可能为空需调用方检查有效性。
     */
    SmartPointer<Scene> GetScene() const;

    /**
     * @brief 初始化Axes对象，设置所需的OpenGL缓冲区和数据。必须在OpenGL上下文中调用。
     */
    void Initialize();

    /**
     * @brief 在指定的场景中绘制三维坐标轴，范围在场景的左下角，视口为原始场景的1/10。
     * @param scene 渲染坐标轴的目标场景。
     */
    void Draw();

protected:
    Axes();
    ~Axes() override;

    /**
     * @brief 生成坐标轴的数据，包括顶点和颜色。
     * @param vertices 存储生成的顶点数据。
     * @param colors 存储生成的颜色数据。
     */
    void RequestData(std::vector<igm::vec3>& vertices,
                     std::vector<igm::vec3>& colors);

    /**
     * @brief 更新坐标轴的模型-视图-投影矩阵和视口信息。
     * @param mvp 模型-视图-投影矩阵。
     * @param viewport 视口数组，包含视口的左、下、宽、高。
     */
    void Update(const igm::mat4& mvp, const igm::ivec4& viewport);

    /**
     * @brief 将显示坐标转换为世界坐标。
     * @param dc 显示坐标（Device Coordinates）。
     * @param wc 世界坐标（World Coordinates）。
     */
    void DisplayCoordToWorldCoord(igm::vec4& dc, igm::vec4& wc);

    /**
     * @brief 将世界坐标转换为显示坐标。
     * @param wc 世界坐标（World Coordinates）。
     * @param dc 显示坐标（Device Coordinates）。
     */
    void WorldCoordToDisplayCoord(igm::vec4& wc, igm::vec4& dc);

    SmartPointer<GLVertexArray> m_TriangleVAO;
    SmartPointer<GLBuffer> m_PositionVBO;
    SmartPointer<GLBuffer> m_ColorVBO;
    SmartPointer<GLBuffer> m_TriangleEBO;

    SmartPointer<GLVertexArray> m_FontVAO;
    SmartPointer<GLBuffer> m_TextureCoordVBO;
    SmartPointer<GLBuffer> m_WorldCoordVBO;
    SmartPointer<GLBuffer> m_FontTextureEBO;

    SmartPointer<Scene> m_Scene;

    int m_Viewport[4];
    igm::mat4 m_Mvp;
    igm::mat4 m_MvpInv;

    float m_ShaftLength;
    float m_ShaftSize;
    float m_ArrowSize;
    float m_OriginSize;
};

IGAME_NAMESPACE_END
