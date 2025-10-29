#ifndef iGameDrawObject_h
#define iGameDrawObject_h

#include "iGameClipper.h"
#include "iGameDataObject.h"
#include "iGameIdArray.h"
#include "iGameMarker.h"
#include "iGamePoints.h"

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLTexture2d.h"
#include "OpenGL/GLTextureBuffer.h"
#include "OpenGL/GLVertexArray.h"

#include "Meshleter/iGameMeshleter.h"

IGAME_NAMESPACE_BEGIN
class Scene;

class DrawObject : public DataObject {
public:
    I_OBJECT(DrawObject);
    static Pointer New() { return new DrawObject; }

protected:
    DrawObject();
    ~DrawObject() override = default;

public:
    bool IsDrawable() override { return true; }       // 标识可以被渲染
    virtual void ConvertToDrawableData();             //转化为可渲染模式（当前对象及其所有子对象）
    virtual bool IsUseSinglePassWireframeRendering(); // 是否使用单通道线框渲染
    IGenum GetDataObjectType() const override;
    IGsize GetRealMemorySize() override;

    bool IsUseColor();        //是否使用颜色
    bool IsUseNormalSmooth(); //是否使用法线平滑

    void SetVisibility(bool f); //设置可见性
    bool GetVisibility();       //获取可见性
    /*ViewStyle's detail. See iGameType.h */
    //样式设置，添加，删除，获取，对模型根节点添加样式，获取模型根节点样式
    void SetViewStyle(IGenum mode);
    void AddViewStyle(IGenum mode);
    void RemoveViewStyle(IGenum mode);
    unsigned int GetViewStyle();
    void AddViewStyleOfModel(IGenum mode);
    unsigned int GetViewStyleOfModel();

    virtual bool GetClipped(); //是否允许裁剪
    iGameClipper::Pointer GetClipper();
    //设置和获取透明度
    void SetTransparency(float transparency);
    float GetTransparency();
    //点的大小
    void SetPointSize(float size);
    int GetPointSize();
    //线的宽度
    void SetLineWidth(float size);
    int GetLineWidth();
    //对当前对象及其子对象/当前对象所属的整个模型设置属性可视化参数
    void ViewCloudPicture(Scene* scene, int index, int dimension = -1);
    void ViewCloudPictureOfModel(Scene* scene, int index, int dimension = -1);

    void SetShellRenderingOption(bool option);

    FloatArray::Pointer GetRenderPoints();            // 获取当前渲染用的顶点数据
    void SetRenderPoints(FloatArray::Pointer points); // 直接设置顶点数据
    // 设置多边形偏移
    void SetPolygonOffsetParameters(float factor, float units);
    void GetPolygonOffsetParameters(float& factor, float& units);
    // 设置线偏移
    void SetLineOffsetParameters(float factor, float units);
    void GetLineOffsetParameters(float& factor, float& units);
    // 设置点偏移
    void SetPointOffsetParameters(float units);
    void GetPointOffsetParameters(float& units);
    // 设置和获取显示对象
    void SetRenderableObject(DataObject::Pointer dataObject);
    DrawObject::Pointer GetRenderableObject(bool useSimplified = false);

    // 设置/获取"始终置顶"标志位
    void SetAlwaysOnTop(bool enable);
    bool IsAlwaysOnTop() const;

    /**
     * @brief 设置是否启用加速渲染模式。
     * @param enabled 若为 true，则启用加速渲染（如使用 Meshlet 结构化）；若为 false，则关闭。
     */
    void SetAccelerationOption(bool enabled);

    /**
     * @brief 获取加速渲染模式当前状态。
     * @return true 表示加速结构已启用，false 表示已禁用。
     */
    bool GetAccelerationOption() const;

protected:
    // OpenGL资源管理
    void CreateDrawBuffer();
    void SyncGpuBuffers();
    // VAO配置辅助方法
    static void SetPositionBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);
    static void SetColorBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);
    static void SetNormalBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);
    static void SetTextureBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);

    bool m_AutoUpdateDrawData; // 是否自动更新GPU数据

    struct RenderableMesh {
        DrawObject::Pointer SurfaceMesh;    // 表面网格
        DrawObject::Pointer SimplifiedMesh; // 简化后的网格
    };
    RenderableMesh m_RenderableMesh;

    // 加速结构
    bool m_AccelerationOption = false;
    Meshleter::Pointer m_Meshleter = nullptr;

    GLVertexArray::Pointer m_PointVAO, m_LineVAO, m_TriangleVAO;
    GLBuffer::Pointer m_PositionVBO, m_ColorVBO, m_NormalVBO, m_TextureVBO;
    GLBuffer::Pointer m_PointEBO, m_LineEBO, m_TriangleEBO;
    GLVertexArray::Pointer m_CellVAO;
    GLBuffer::Pointer m_CellPositionVBO, m_CellColorVBO;
    GLBuffer::Pointer m_CellEBO;
    //顶点的坐标颜色法线纹理
    FloatArray::Pointer m_Positions;
    FloatArray::Pointer m_Colors;
    FloatArray::Pointer m_Normals;
    FloatArray::Pointer m_Textures;
    //点线三角形索引
    UnsignedIntArray::Pointer m_PointIndices;
    UnsignedIntArray::Pointer m_LineIndices;
    UnsignedIntArray::Pointer m_TriangleIndices;
    // 单通道线框渲染
    bool m_UseSinglePassWireframeRendering{true};
    UnsignedCharArray::Pointer m_TriangleEdgeMasks;
    GLBuffer::Pointer m_EdgeMaskBuffer;
    GLTextureBuffer::Pointer m_EdgeMaskTexture;
    // 单元数据
    FloatArray::Pointer m_CellPositions;
    FloatArray::Pointer m_CellColors;
    UnsignedIntArray::Pointer m_CellIndices;

    unsigned int m_ViewStyle; // 视图样式
    bool m_Visibility;        //是否可见

    bool m_AlwaysOnTop = false; // 是否置顶默认不置顶

    bool m_Flag;            // 标记是否已初始化OpenGL缓冲区
    bool m_UseColor;        //是否使用颜色属性
    bool m_UseNormalSmooth; // 是否启用法线平滑
    bool m_ColorWithCell;   // 颜色是否基于单元（非顶点）
    float m_PointSize;
    float m_LineWidth;
    int m_CellPositionSize; // 单元位置数据的大小（似乎没用到）

    // 深度偏移相关参数
    // https://www.khronos.org/opengl/wiki/Polygon_Offset_and_Point_and_Lines
    float m_PolygonFactor; // now implement with GL_POLYGON_OFFSET_FILL
    float m_PolygonOffset; // now implement with GL_POLYGON_OFFSET_FILL
    float m_LineFactor;    // now not implemented
    float m_LineOffset;    // now not implemented
    float m_PointOffset;   // now not implemented
    //float m_PolygonFactor{0.0f};
    //float m_PolygonOffset{0.0f};
    //float m_LineFactor{0.0f};
    //float m_LineOffset{-4.0f};
    //float m_PointOffset{-8.0f};

    float m_Transparency; // 透明度
    bool m_ExecuteShell;
    bool m_ReConvertToDrawableData; // 是否需要重新转换数据

    iGameClipper::Pointer m_Clipper; // 裁剪器对象

    friend class Model;
    friend class Scene;
    friend class UnstructuredMesh;

    template<typename Functor, typename... Args>
    void ProcessSubDataObjects(Functor&& functor, Args&&... args);
};
//递归处理所有子对象的模板函数实现
template<typename Functor, typename... Args>
inline void DrawObject::ProcessSubDataObjects(Functor&& functor, Args&&... args) {
    if (HasSubDataObject()) {
        for (auto it = m_SubDataObjectsHelper->Begin(); it != m_SubDataObjectsHelper->End(); ++it) {
            (DynamicCast<DrawObject>(it->second)->*functor)(std::forward<Args>(args)...);
        }
    }
}
IGAME_NAMESPACE_END
#endif
