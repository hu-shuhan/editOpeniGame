#include "iGameDrawObject.h"

IGAME_NAMESPACE_BEGIN

void DrawObject::Create() {
    if (m_Flag) {
        m_PointVAO.create();
        m_VertexVAO.create();
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
        m_VertexEBO.create();
        m_VertexEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_LineEBO.create();
        m_LineEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_TriangleEBO.create();
        m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);

        m_CellVAO.create();
        m_CellPositionVBO.create();
        m_CellPositionVBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_CellColorVBO.create();
        m_CellColorVBO.target(GL_ELEMENT_ARRAY_BUFFER);

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

void DrawObject::SetExtentClipping(bool _in) {
    if (GetClipped()) this->m_Clip.m_Extent.m_Use = _in;
}

void DrawObject::SetPlaneClipping(bool _in) {
    if (GetClipped()) this->m_Clip.m_Plane.m_Use = _in;
}

void DrawObject::SetExtent(double xMin, double xMax, double yMin, double yMax,
                           double zMin, double zMax, bool flip) {
    m_Clip.m_Extent.m_bmin[0] = xMin;
    m_Clip.m_Extent.m_bmin[1] = yMin;
    m_Clip.m_Extent.m_bmin[2] = zMin;
    m_Clip.m_Extent.m_bmax[0] = xMax;
    m_Clip.m_Extent.m_bmax[1] = yMax;
    m_Clip.m_Extent.m_bmax[2] = zMax;
    m_Clip.m_Extent.m_flip = flip;
    SetExtentClipping(true);
}

void DrawObject::SetPlane(double ox, double oy, double oz, double nx, double ny,
                          double nz, bool flip) {
    m_Clip.m_Plane.m_origin[0] = ox;
    m_Clip.m_Plane.m_origin[1] = oy;
    m_Clip.m_Plane.m_origin[2] = oz;
    m_Clip.m_Plane.m_normal[0] = nx;
    m_Clip.m_Plane.m_normal[1] = ny;
    m_Clip.m_Plane.m_normal[2] = nz;
    m_Clip.m_Plane.m_flip = flip;
    SetPlaneClipping(true);
}

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

void DrawObject::ViewCloudPictureOfModel(Scene *scene, int index, int dimension){
    auto* parent = dynamic_cast<DrawObject*>(FindParent());
    if (parent != nullptr && parent != this) {
        parent->ViewCloudPicture(scene, index, dimension);
    } else {
        this->ViewCloudPicture(scene, index, dimension);
    }
}

void DrawObject::Draw(Scene *scene) {
    ProcessSubDataObjects(&DrawObject::Draw, scene);
}

IGAME_NAMESPACE_END