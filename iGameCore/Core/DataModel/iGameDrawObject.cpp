#include "iGameDrawObject.h"

IGAME_NAMESPACE_BEGIN

void DrawObject::CreateDrawBuffer() {
    if (m_Flag) {
        m_PointVAO.create();
        m_LineVAO.create();
        m_TriangleVAO.create();

        m_PositionVBO.create();
        m_PositionVBO.target(GL_ARRAY_BUFFER);
        m_ColorVBO.create();
        m_ColorVBO.target(GL_ARRAY_BUFFER);
        m_NormalVBO.create();
        m_NormalVBO.target(GL_ARRAY_BUFFER);
        m_TextureVBO.create();
        m_TextureVBO.target(GL_ARRAY_BUFFER);

        m_PointEBO.create();
        m_PointEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_LineEBO.create();
        m_LineEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_TriangleEBO.create();
        m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);

        m_CellVAO.create();
        m_CellPositionVBO.create();
        m_CellPositionVBO.target(GL_ARRAY_BUFFER);
        m_CellColorVBO.create();
        m_CellColorVBO.target(GL_ARRAY_BUFFER);

#ifdef IGAME_OPENGL_VERSION_460
        m_Meshlets->CreateBuffer();
#endif

        m_Flag = false;
    }
}

void DrawObject::ConvertToDrawableData() {
    ProcessSubDataObjects(&DrawObject::ConvertToDrawableData);
}

IGenum DrawObject::GetDataObjectType() const { return IG_DRAW_OBJECT; }

IGsize DrawObject::GetRealMemorySize() { return 0; };

void DrawObject::SetVisibility(bool f) {
    this->m_Visibility = f;
    ProcessSubDataObjects(&DrawObject::SetVisibility, f);
}

bool DrawObject::GetVisibility() { return m_Visibility; }

void DrawObject::SetViewStyle(IGenum mode) {
    /*
     * e.g. mode = IG_WIREFRAME | IG_SURFACE, means that the model shows the wireframe and surface.
     * */
    m_ViewStyle = mode;
    ProcessSubDataObjects(&DrawObject::SetViewStyle, mode);
}

void DrawObject::AddViewStyle(IGenum mode) {
    m_ViewStyle |= mode;
    ProcessSubDataObjects(&DrawObject::AddViewStyle, mode);
}

void DrawObject::RemoveViewStyle(IGenum mode) {
    m_ViewStyle &= ~mode;
    ProcessSubDataObjects(&DrawObject::RemoveViewStyle, mode);
}

unsigned int DrawObject::GetViewStyle() { return m_ViewStyle; }

void DrawObject::AddViewStyleOfModel(IGenum mode) {
    //auto* parent = FindParent();
    auto* parentDrawObject = DynamicCast<DrawObject>(FindParent());
    if (parentDrawObject != this) {
        parentDrawObject->AddViewStyle(mode);
    } else {
        this->AddViewStyle(mode);
    }
}

unsigned int DrawObject::GetViewStyleOfModel() {
    //auto* parent = FindParent();
    auto* parentDrawObject = DynamicCast<DrawObject>(FindParent());
    if (parentDrawObject != this) {
        return parentDrawObject->GetViewStyle();
    } else {
        return this->GetViewStyle();
    }
}

bool DrawObject::GetClipped() {
    return false;
}; // Gets whether this can be clipped.

void DrawObject::SetTransparency(float transparency) {
    if (transparency < 0.0f || transparency > 1.0f) {
        throw std::runtime_error("Transparency must be between 0-1");
    }
    m_Transparency = transparency;
}

float DrawObject::GetTransparency() { return m_Transparency; }

void DrawObject::ViewCloudPicture(Scene* scene, int index, int dimension) {
    m_AttributeIndex = index;
    m_AttributeDimension = dimension;
    ProcessSubDataObjects(&DrawObject::ViewCloudPicture, scene, index,
                          dimension);
}

void DrawObject::ViewCloudPictureOfModel(Scene* scene, int index,
                                         int dimension) {
    auto* parent = dynamic_cast<DrawObject*>(FindParent());
    if (parent != nullptr && parent != this) {
        parent->ViewCloudPicture(scene, index, dimension);
    } else {
        this->ViewCloudPicture(scene, index, dimension);
    }
}

//void DrawObject::Draw(Scene *scene) {
//    ProcessSubDataObjects(&DrawObject::Draw, scene);
//}

IGAME_NAMESPACE_END