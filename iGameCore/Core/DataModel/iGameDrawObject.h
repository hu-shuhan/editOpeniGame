#ifndef iGameDrawObject_h
#define iGameDrawObject_h

#include "iGameClipper.h"
#include "iGameDataObject.h"
#include "iGameIdArray.h"
#include "iGameMarker.h"
#include "iGameMeshlet.h"
#include "iGamePoints.h"

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLTexture2d.h"
#include "OpenGL/GLVertexArray.h"

#include "iGameMeshlet.h"

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
    void CreateDrawBuffer();
    void ReAllocateDisplayBuffer();
    IGenum GetDataObjectType() const override;
    IGsize GetRealMemorySize() override;

    bool IsUseColor() { return m_UseColor; }

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
    iGameClipper::Pointer GetClipper() { return m_Clipper; }

    void SetTransparency(float transparency);
    float GetTransparency();
    void SetPointSize(int size);
    int GetPointSize();

    void ViewCloudPicture(Scene* scene, int index, int dimension = -1);
    void ViewCloudPictureOfModel(Scene* scene, int index, int dimension = -1);

    void SetPolygonOffsetParameters(float factor, float units);
    void GetPolygonOffsetParameters(float& factor, float& units);

    void SetLineOffsetParameters(float factor, float units);
    void GetLineOffsetParameters(float& factor, float& units);

    void SetPointOffsetParameters(float units);
    void GetPointOffsetParameters(float& units);

    void SetDisplayObject(DataObject::Pointer dataObject);

private:
    static void SetPositionBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO);
    static void SetColorBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO);
    static void SetNormalBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO);
    static void SetTextureBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO);

protected:
    bool m_AutoUpdateDrawData{true};
    DrawObject::Pointer m_DisplayObject{nullptr};

    GLVertexArray m_PointVAO, m_LineVAO, m_TriangleVAO;
    GLBuffer m_PositionVBO, m_ColorVBO, m_NormalVBO, m_TextureVBO;
    GLBuffer m_PointEBO, m_LineEBO, m_TriangleEBO;
    GLVertexArray m_CellVAO;
    GLBuffer m_CellPositionVBO, m_CellColorVBO;
    GLBuffer m_CellEBO;

    FloatArray::Pointer m_Positions;
    FloatArray::Pointer m_Colors;
    FloatArray::Pointer m_Normals;
    FloatArray::Pointer m_Textures;

    UnsignedIntArray::Pointer m_PointIndices;
    UnsignedIntArray::Pointer m_LineIndices;
    UnsignedIntArray::Pointer m_TriangleIndices;

    FloatArray::Pointer m_CellPositions;
    FloatArray::Pointer m_CellColors;
    UnsignedIntArray::Pointer m_CellIndices;

    unsigned int m_ViewStyle{0};
    bool m_Visibility{true};

    bool m_Flag{true};
    bool m_UseColor{false};
    bool m_ColorWithCell{false};
    int m_PointSize{8};
    int m_LineWidth{1};
    int m_CellPositionSize{};

    float m_PolygonFactor{-1.0f};
    float m_PolygonOffset{-1.0f};
    float m_LineFactor{-1.0f};
    float m_LineOffset{-1.0f};
    float m_PointOffset{-1.0f};

    float m_Transparency{1.0f};

    iGameClipper::Pointer m_Clipper;

    friend class Model;
    friend class UnstructuredMesh;

#ifdef IGAME_OPENGL_VERSION_460
    Meshlet::Pointer m_Meshlets{Meshlet::New()};
#endif

protected:
    template<typename Functor, typename... Args>
    void ProcessSubDataObjects(Functor&& functor, Args&&... args);
};

template<typename Functor, typename... Args>
inline void DrawObject::ProcessSubDataObjects(Functor&& functor,
                                              Args&&... args) {
    if (HasSubDataObject()) {
        for (auto it = m_SubDataObjectsHelper->Begin();
             it != m_SubDataObjectsHelper->End(); ++it) {
            (DynamicCast<DrawObject>(it->second)->*functor)(
                    std::forward<Args>(args)...);
        }
    }
}
IGAME_NAMESPACE_END
#endif