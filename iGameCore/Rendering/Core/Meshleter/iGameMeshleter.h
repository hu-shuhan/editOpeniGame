/**
 * @class    Meshleter
 * @brief    Meshleter类用于生成和管理Meshlet（小网格）。
 *
 * Meshleter类是SurfaceMeshleter的基类，提供了构建和更新Meshlet的基础功能。
 * 该类支持多种渲染技术，管理Meshlet的顶点数据、三角形数据以及相关的OpenGL缓冲。
 * 子类（如SurfaceMeshleter）可以扩展该类的功能以实现特定的Meshlet处理逻辑。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLIndirectCommand.h"
#include "OpenGL/GLVertexArray.h"
#include "iGameDataObject.h"
#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

class Model;

class Meshleter : public Object {
public:
    I_OBJECT(Meshleter);
    static Pointer New() { return new Meshleter; }

    /**
    * @struct MeshletDescriptor
    * @brief  描述单个Meshlet的范围信息。
    */
    struct MeshletDescriptor {
        igm::vec4 spherebounds; ///< Meshlet的球形包围盒（中心与半径）
        igm::vec4 extents;      ///< Meshlet的范围（当前未使用）
    };

    /**
    * @struct Meshlet
    * @brief  表示一个Meshlet的数据偏移与统计信息。
    */
    struct Meshlet {
        unsigned int vertex_offset;   ///< Meshlet的顶点数据偏移量
        unsigned int triangle_offset; ///< Meshlet的三角形数据偏移量
        unsigned int vertex_count;    ///< Meshlet的顶点数量
        unsigned int triangle_count;  ///< Meshlet的三角形数量
    };

    /**
    * @brief 设置输入数据对象。
    * @param obj 数据对象指针，包含用于生成Meshlet的输入数据。
    */
    void SetInput(SmartPointer<DataObject> obj);

    /**
     * @brief 获取当前设置的输入数据对象。
     * @return 指向输入数据的智能指针，可能为空指针需调用方检查有效性。
     * @details 返回最近一次通过SetInput()设置的数据对象引用，
     *          该对象包含用于生成Meshlet的原始输入数据。
     * @warning 若从未调用SetInput()，将返回空指针
     */
    SmartPointer<DataObject> GetInput() const;

    /**
     * @brief 将计算后的Meshlet数据同步到GPU显存。
     * @details 该函数负责将CPU端的Meshlet计算结果上传至GPU显存，
     *          确保后续渲染操作使用最新的数据。
     * @warning 调用该函数时必须确保OpenGL上下文处于活跃状态
     */
    void SyncGpuBuffers();

    /**
     * @brief 释放GPU显存中的Meshlet相关资源。
     * @details 清除GPU显存中存储的Meshlet顶点/索引缓冲、加速结构等资源，
     *          适用于场景切换或资源重载时释放显存。
     * @warning 调用该函数时必须确保OpenGL上下文处于活跃状态
     * @warning 调用该函数后必须重新调用SyncToGpu()才能继续渲染
     */
    void ReleaseGpuBuffers();

    void SetRenderWithMeshlet(bool val) {
        m_RenderWithMeshlet = val;
        m_RenderWithMeshletChanged = true;
    }
    bool GetRenderWithMeshlet() const { return m_RenderWithMeshlet; }

protected:
    Meshleter();
    ~Meshleter() override;

    /**
    * @brief 构建Meshlet的内部实现。
    */
    virtual void Build();

    const size_t m_MaxVertices = 64;   ///< 每个Meshlet的最大顶点数量
    const size_t m_MaxTriangles = 124; ///< 每个Meshlet的最大三角形数量
    const float m_ConeWeight = 0.0f;   ///< 权重参数（当前未使用）

    SmartPointer<DataObject> m_DataObject; ///< 输入数据对象指针
    size_t m_MeshletCount = 0;             ///< Meshlet的数量

    bool m_RenderWithMeshlet = false; ///< 是否每个meshlet独立颜色
    bool m_RenderWithMeshletChanged = false;
    std::vector<unsigned int> m_MeshletIndices;
    std::vector<unsigned int> m_TriangleToFace;
    std::vector<MeshletDescriptor> m_MeshletDescriptors;
    std::vector<DrawElementsIndirectCommand> m_ElementsDrawCommands;
    std::vector<DrawArraysIndirectCommand> m_ArraysDrawCommands;

#ifdef GL_SUPPORTS_MESH_SHADER
    // 如果支持OpenGL的Mesh Shader，这些缓冲被用于存储Meshlet数据
    SmartPointer<GLBuffer> m_MeshletBuffer;           ///< 存储Meshlet信息的缓冲
    SmartPointer<GLBuffer> m_MeshletVertexBuffer;     ///< Meshlet顶点缓冲
    SmartPointer<GLBuffer> m_MeshletTriangleBuffer;   ///< Meshlet三角形缓冲
    SmartPointer<GLBuffer> m_MeshletDescriptorBuffer; ///< Meshlet描述符缓冲
    SmartPointer<GLBuffer> m_InvisibleMeshletBuffer;  ///< 不可见Meshlet缓冲

    SmartPointer<GLBuffer> m_PositionBuffer; ///< 顶点位置缓冲
    SmartPointer<GLBuffer> m_ColorBuffer;    ///< 顶点颜色缓冲
    SmartPointer<GLBuffer> m_NormalBuffer;   ///< 顶点法向量缓冲
    SmartPointer<GLBuffer> m_UVBuffer;       ///< 顶点UV缓冲
#else
    // 如果不支持Mesh Shader，使用VAO和EBO处理渲染
    SmartPointer<GLVertexArray> m_TriangleVAO; ///< 三角形顶点数组对象
    SmartPointer<GLBuffer> m_TriangleEBO;      ///< 三角形索引缓冲

    SmartPointer<GLBuffer> m_PositionVBO; ///< 顶点位置缓冲
    SmartPointer<GLBuffer> m_ColorVBO;    ///< 顶点颜色缓冲
    SmartPointer<GLBuffer> m_NormalVBO;   ///< 顶点法向量缓冲
    SmartPointer<GLBuffer> m_UVVBO;       ///< 顶点UV缓冲

    SmartPointer<GLBuffer> m_MeshletDescriptorBuffer; ///< Meshlet描述符缓冲
    SmartPointer<GLBuffer> m_VisibleMeshletBuffer;    ///< 可见Meshlet缓冲
    SmartPointer<GLBuffer> m_DrawCommandBuffer;       ///< 绘制命令缓冲
    SmartPointer<GLBuffer> m_FinalDrawCommandBuffer;  ///< 最终绘制命令缓冲

    // 单元渲染相关缓冲
    SmartPointer<GLVertexArray> m_CellTriangleVAO;  ///< 单元三角形顶点数组对象
    SmartPointer<GLBuffer> m_CellPositionVBO;       ///< 单元顶点位置缓冲
    SmartPointer<GLBuffer> m_CellColorVBO;          ///< 单元顶点颜色缓冲
    SmartPointer<GLBuffer> m_CellDrawCommandBuffer; ///< 绘制命令缓冲
    SmartPointer<GLBuffer> m_CellFinalDrawCommandBuffer; ///< 最终绘制命令缓冲
#endif

    friend class Model;
};

IGAME_NAMESPACE_END
