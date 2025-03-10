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
    bool IsDrawable() override { return true; }
    virtual void ConvertToDrawableData();
    virtual bool IsUseSinglePassWireframeRendering();
    IGenum GetDataObjectType() const override;
    IGsize GetRealMemorySize() override;

    bool IsUseColor();
    bool IsUseNormalSmooth();

    void SetVisibility(bool f);
    bool GetVisibility();
    /*ViewStyle's detail. See iGameType.h */
    void SetViewStyle(IGenum mode);
    void AddViewStyle(IGenum mode);
    void RemoveViewStyle(IGenum mode);
    unsigned int GetViewStyle();
    void AddViewStyleOfModel(IGenum mode);
    unsigned int GetViewStyleOfModel();

    virtual bool GetClipped();
    iGameClipper::Pointer GetClipper();

    void SetTransparency(float transparency);
    float GetTransparency();

    void SetPointSize(float size);
    int GetPointSize();

    void SetLineWidth(float size);
    int GetLineWidth();

    void ViewCloudPicture(Scene* scene, int index, int dimension = -1);
    void ViewCloudPictureOfModel(Scene* scene, int index, int dimension = -1);

    void SetShellRenderingOption(bool option);

    FloatArray::Pointer GetRenderPoints();
    void SetRenderPoints(FloatArray::Pointer points);

    void SetPolygonOffsetParameters(float factor, float units);
    void GetPolygonOffsetParameters(float& factor, float& units);

    void SetLineOffsetParameters(float factor, float units);
    void GetLineOffsetParameters(float& factor, float& units);

    void SetPointOffsetParameters(float units);
    void GetPointOffsetParameters(float& units);

    void SetDisplayObject(DataObject::Pointer dataObject);
    DrawObject::Pointer GetDisplayObject();

protected:
    void CreateDrawBuffer();
    void SyncGpuBuffers();

    static void SetPositionBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);
    static void SetColorBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);
    static void SetNormalBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);
    static void SetTextureBufferToVAO(GLVertexArray::Pointer VAO, GLBuffer::Pointer VBO);

    bool m_AutoUpdateDrawData;
    DrawObject::Pointer m_DisplayObject;

    GLVertexArray::Pointer m_PointVAO, m_LineVAO, m_TriangleVAO;
    GLBuffer::Pointer m_PositionVBO, m_ColorVBO, m_NormalVBO, m_TextureVBO;
    GLBuffer::Pointer m_PointEBO, m_LineEBO, m_TriangleEBO;
    GLVertexArray::Pointer m_CellVAO;
    GLBuffer::Pointer m_CellPositionVBO, m_CellColorVBO;
    GLBuffer::Pointer m_CellEBO;

    FloatArray::Pointer m_Positions;
    FloatArray::Pointer m_Colors;
    FloatArray::Pointer m_Normals;
    FloatArray::Pointer m_Textures;

    UnsignedIntArray::Pointer m_PointIndices;
    UnsignedIntArray::Pointer m_LineIndices;
    UnsignedIntArray::Pointer m_TriangleIndices;

    bool m_UseSinglePassWireframeRendering{true};
    UnsignedCharArray::Pointer m_TriangleEdgeMasks;
    GLBuffer::Pointer m_EdgeMaskBuffer;
    GLTextureBuffer::Pointer m_EdgeMaskTexture;

    FloatArray::Pointer m_CellPositions;
    FloatArray::Pointer m_CellColors;
    UnsignedIntArray::Pointer m_CellIndices;

    unsigned int m_ViewStyle;
    bool m_Visibility;

    bool m_Flag;
    bool m_UseColor;
    bool m_UseNormalSmooth;
    bool m_ColorWithCell;
    float m_PointSize;
    float m_LineWidth;
    int m_CellPositionSize;

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

    float m_Transparency;
    bool m_ExecuteShell;
    bool m_ReConvertToDrawableData;

    iGameClipper::Pointer m_Clipper;

    friend class Model;
    friend class Scene;
    friend class UnstructuredMesh;

    template<typename Functor, typename... Args>
    void ProcessSubDataObjects(Functor&& functor, Args&&... args);
};

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