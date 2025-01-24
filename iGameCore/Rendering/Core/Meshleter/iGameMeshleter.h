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
    void SetInput(DataObject::Pointer obj);

    /**
    * @brief 更新Meshleter，重新计算Meshlet数据。
    */
    void Update();

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

    DataObject::Pointer m_DataObject; ///< 输入数据对象指针
    size_t m_MeshletCount;            ///< Meshlet的数量

#ifdef GL_SUPPORTS_MESH_SHADER
    // 如果支持OpenGL的Mesh Shader，这些缓冲被用于存储Meshlet数据
    GLBuffer::Pointer m_MeshletBuffer;         ///< 存储Meshlet信息的缓冲
    GLBuffer::Pointer m_MeshletVertexBuffer;   ///< Meshlet顶点缓冲
    GLBuffer::Pointer m_MeshletTriangleBuffer; ///< Meshlet三角形缓冲
    GLBuffer::Pointer m_MeshletDescriptorBuffer; ///< Meshlet描述符缓冲
    GLBuffer::Pointer m_InvisibleMeshletBuffer;  ///< 不可见Meshlet缓冲

    GLBuffer::Pointer m_PositionBuffer; ///< 顶点位置缓冲
    GLBuffer::Pointer m_ColorBuffer;    ///< 顶点颜色缓冲
    GLBuffer::Pointer m_NormalBuffer;   ///< 顶点法向量缓冲
    GLBuffer::Pointer m_UVBuffer;       ///< 顶点UV缓冲
#else
    // 如果不支持Mesh Shader，使用VAO和EBO处理渲染
    GLVertexArray::Pointer m_TriangleVAO; ///< 三角形顶点数组对象
    GLBuffer::Pointer m_TriangleEBO;      ///< 三角形索引缓冲

    GLBuffer::Pointer m_PositionVBO; ///< 顶点位置缓冲
    GLBuffer::Pointer m_ColorVBO;    ///< 顶点颜色缓冲
    GLBuffer::Pointer m_NormalVBO;   ///< 顶点法向量缓冲
    GLBuffer::Pointer m_UVVBO;       ///< 顶点UV缓冲

    GLBuffer::Pointer m_MeshletDescriptorBuffer; ///< Meshlet描述符缓冲
    GLBuffer::Pointer m_DrawCommandBuffer;       ///< 绘制命令缓冲
    GLBuffer::Pointer m_VisibleMeshletBuffer;    ///< 可见Meshlet缓冲
    GLBuffer::Pointer m_FinalDrawCommandBuffer;  ///< 最终绘制命令缓冲
#endif

    friend class Model; ///< Model类可以访问Meshleter的私有成员
};

IGAME_NAMESPACE_END
