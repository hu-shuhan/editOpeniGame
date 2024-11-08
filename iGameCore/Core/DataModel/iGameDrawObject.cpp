#include "iGameDrawObject.h"

#include "iGameScene.h"
#include <utility>

IGAME_NAMESPACE_BEGIN
DrawObject::DrawObject() {
    m_Clipper = iGameClipper::New();

    m_Positions = FloatArray::New();
    m_Positions->SetDimension(3);

    m_Colors = FloatArray::New();
    m_Colors->SetDimension(3);

    m_Normals = FloatArray::New();
    m_Normals->SetDimension(3);

    m_Textures = FloatArray::New();
    m_Textures->SetDimension(2);

    m_PointIndices = UnsignedIntArray::New();
    m_PointIndices->SetDimension(1);

    m_LineIndices = UnsignedIntArray::New();
    m_LineIndices->SetDimension(2);

    m_TriangleIndices = UnsignedIntArray::New();
    m_TriangleIndices->SetDimension(3);

    m_CellPositions = FloatArray::New();
    m_CellPositions->SetDimension(3);

    m_CellColors = FloatArray::New();
    m_CellColors->SetDimension(3);

    m_CellIndices = UnsignedIntArray::New();
    m_CellIndices->SetDimension(3);
}

void DrawObject::CreateDrawBuffer() {
    if (!m_Flag) {
        m_PointVAO = GLVertexArray::New();
        m_PointVAO->Create();

        m_LineVAO = GLVertexArray::New();
        m_LineVAO->Create();

        m_TriangleVAO = GLVertexArray::New();
        m_TriangleVAO->Create();

        m_PositionVBO = GLBuffer::New();
        m_PositionVBO->Create();
        m_PositionVBO->Target(GL_ARRAY_BUFFER);

        m_ColorVBO = GLBuffer::New();
        m_ColorVBO->Create();
        m_ColorVBO->Target(GL_ARRAY_BUFFER);

        m_NormalVBO = GLBuffer::New();
        m_NormalVBO->Create();
        m_NormalVBO->Target(GL_ARRAY_BUFFER);

        m_TextureVBO = GLBuffer::New();
        m_TextureVBO->Create();
        m_TextureVBO->Target(GL_ARRAY_BUFFER);

        m_PointEBO = GLBuffer::New();
        m_PointEBO->Create();
        m_PointEBO->Target(GL_ELEMENT_ARRAY_BUFFER);

        m_LineEBO = GLBuffer::New();
        m_LineEBO->Create();
        m_LineEBO->Target(GL_ELEMENT_ARRAY_BUFFER);

        m_TriangleEBO = GLBuffer::New();
        m_TriangleEBO->Create();
        m_TriangleEBO->Target(GL_ELEMENT_ARRAY_BUFFER);

        m_CellVAO = GLVertexArray::New();
        m_CellVAO->Create();

        m_CellPositionVBO = GLBuffer::New();
        m_CellPositionVBO->Create();
        m_CellPositionVBO->Target(GL_ARRAY_BUFFER);

        m_CellColorVBO = GLBuffer::New();
        m_CellColorVBO->Create();
        m_CellColorVBO->Target(GL_ARRAY_BUFFER);

        m_CellEBO = GLBuffer::New();
        m_CellEBO->Create();
        m_CellEBO->Target(GL_ELEMENT_ARRAY_BUFFER);

#ifdef IGAME_OPENGL_VERSION_460
        m_Meshlets->CreateBuffer();
#endif

        //// set point drawing format
        //{
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
        //                            3 * sizeof(float));
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
        //                            3 * sizeof(float));
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_2, m_NormalVBO, 0,
        //                            3 * sizeof(float));
        //    m_PointVAO.vertexBuffer(GL_VBO_IDX_3, m_TextureVBO, 0,
        //                            2 * sizeof(float));
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_PointVAO.elementBuffer(m_PointEBO);
        //}
        //
        //// set line drawing format
        //{
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
        //                           3 * sizeof(float));
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
        //                           3 * sizeof(float));
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_2, m_NormalVBO, 0,
        //                           3 * sizeof(float));
        //    m_LineVAO.vertexBuffer(GL_VBO_IDX_3, m_TextureVBO, 0,
        //                           2 * sizeof(float));
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_LineVAO.elementBuffer(m_LineEBO);
        //}
        //
        //// set triangle drawing format
        //{
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
        //                               3 * sizeof(float));
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
        //                               3 * sizeof(float));
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_2, m_NormalVBO, 0,
        //                               3 * sizeof(float));
        //    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_3, m_TextureVBO, 0,
        //                               2 * sizeof(float));
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_TriangleVAO.elementBuffer(m_TriangleEBO);
        //}
        //
        //// set cell drawing format
        //{
        //    m_CellVAO.vertexBuffer(GL_VBO_IDX_0, m_CellPositionVBO, 0,
        //                           3 * sizeof(float));
        //    m_CellVAO.vertexBuffer(GL_VBO_IDX_1, m_CellColorVBO, 0,
        //                           3 * sizeof(float));
        //    GLSetVertexAttrib(m_CellVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    GLSetVertexAttrib(m_CellVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                      GL_FLOAT, GL_FALSE, 0);
        //    m_CellVAO.elementBuffer(m_CellEBO);
        //}

        m_Flag = true;
    }
}

void DrawObject::ConvertToDrawableData() {
    // process display object
    if (m_DisplayObject) {
        m_DisplayObject->ConvertToDrawableData();
        return;
    }

    // process this object
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::ConvertToDrawableData);
    }
}

void DrawObject::ReAllocateDisplayBuffer() {
    // process display object
    if (m_DisplayObject) {
        m_DisplayObject->ReAllocateDisplayBuffer();
        return;
    }

    // process this object
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::ReAllocateDisplayBuffer);
    }
    this->CreateDrawBuffer();

    if (m_AutoUpdateDrawData) {
        ConvertToDrawableData();
        if (m_DisplayObject) { m_DisplayObject->ReAllocateDisplayBuffer(); }
    }

    if (m_Positions->GetMTime() > m_PositionVBO->GetMTime()) {
        GLAllocateGLBuffer(m_PositionVBO,
                           m_Positions->GetNumberOfValues() * sizeof(float),
                           m_Positions->RawPointer());
        m_PositionVBO->Modified();

        SetPositionBufferToVAO(m_PointVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_LineVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_TriangleVAO, m_PositionVBO);
    }

    if (m_Colors->GetMTime() > m_ColorVBO->GetMTime()) {
        GLAllocateGLBuffer(m_ColorVBO,
                           m_Colors->GetNumberOfValues() * sizeof(float),
                           m_Colors->RawPointer());
        m_ColorVBO->Modified();

        SetColorBufferToVAO(m_PointVAO, m_ColorVBO);
        SetColorBufferToVAO(m_LineVAO, m_ColorVBO);
        SetColorBufferToVAO(m_TriangleVAO, m_ColorVBO);
    }

    if (m_Normals->GetMTime() > m_NormalVBO->GetMTime()) {
        GLAllocateGLBuffer(m_NormalVBO,
                           m_Normals->GetNumberOfValues() * sizeof(float),
                           m_Normals->RawPointer());
        m_NormalVBO->Modified();

        SetNormalBufferToVAO(m_PointVAO, m_NormalVBO);
        SetNormalBufferToVAO(m_LineVAO, m_NormalVBO);
        SetNormalBufferToVAO(m_TriangleVAO, m_NormalVBO);
    }

    if (m_Textures->GetMTime() > m_TextureVBO->GetMTime()) {
        GLAllocateGLBuffer(m_TextureVBO,
                           m_Textures->GetNumberOfValues() * sizeof(float),
                           m_Textures->RawPointer());
        m_TextureVBO->Modified();

        SetTextureBufferToVAO(m_PointVAO, m_TextureVBO);
        SetTextureBufferToVAO(m_LineVAO, m_TextureVBO);
        SetTextureBufferToVAO(m_TriangleVAO, m_TextureVBO);
    }

    if (m_PointIndices->GetMTime() > m_PointEBO->GetMTime()) {
        GLAllocateGLBuffer(m_PointEBO,
                           m_PointIndices->GetNumberOfValues() *
                                   sizeof(igIndex),
                           m_PointIndices->RawPointer());
        m_PointEBO->Modified();

        m_PointVAO->ElementBuffer(m_PointEBO);
    }

    if (m_LineIndices->GetMTime() > m_LineEBO->GetMTime()) {
        GLAllocateGLBuffer(m_LineEBO,
                           m_LineIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_LineIndices->RawPointer());
        m_LineEBO->Modified();

        m_LineVAO->ElementBuffer(m_LineEBO);
    }

    if (m_TriangleIndices->GetMTime() > m_TriangleEBO->GetMTime()) {
        GLAllocateGLBuffer(m_TriangleEBO,
                           m_TriangleIndices->GetNumberOfValues() *
                                   sizeof(igIndex),
                           m_TriangleIndices->RawPointer());
        m_TriangleEBO->Modified();

        m_TriangleVAO->ElementBuffer(m_TriangleEBO);
    }

    if (m_CellPositions->GetMTime() > m_CellPositionVBO->GetMTime()) {
        GLAllocateGLBuffer(m_CellPositionVBO,
                           m_CellPositions->GetNumberOfValues() * sizeof(float),
                           m_CellPositions->RawPointer());
        m_CellPositionVBO->Modified();

        SetPositionBufferToVAO(m_CellVAO, m_CellPositionVBO);
    }

    if (m_CellColors->GetMTime() > m_CellColorVBO->GetMTime()) {
        GLAllocateGLBuffer(m_CellColorVBO,
                           m_CellColors->GetNumberOfValues() * sizeof(float),
                           m_CellColors->RawPointer());
        m_CellColorVBO->Modified();

        SetColorBufferToVAO(m_CellVAO, m_CellColorVBO);
    }

    if (m_CellIndices->GetMTime() > m_CellEBO->GetMTime()) {
        GLAllocateGLBuffer(m_CellEBO,
                           m_CellIndices->GetNumberOfValues() * sizeof(float),
                           m_CellIndices->RawPointer());
        m_CellEBO->Modified();

        m_CellVAO->ElementBuffer(m_CellEBO);
    }
}

IGenum DrawObject::GetDataObjectType() const { return IG_DRAW_OBJECT; }

IGsize DrawObject::GetRealMemorySize() {
    IGsize res = this->DataObject::GetRealMemorySize();
    return res;
};

void DrawObject::SetVisibility(bool f) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->SetVisibility(f); }

    // process this object
    this->m_Visibility = f;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::SetVisibility, f);
    }
}

bool DrawObject::GetVisibility() { return m_Visibility; }

void DrawObject::SetViewStyle(IGenum mode) {
    /*
     * e.g. mode = IG_WIREFRAME | IG_SURFACE, means that the model shows the wireframe and surface.
     * */

    // process display object
    if (m_DisplayObject) { m_DisplayObject->SetViewStyle(mode); }

    // process this object
    m_ViewStyle = mode;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::SetViewStyle, mode);
    }
}

void DrawObject::AddViewStyle(IGenum mode) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->AddViewStyle(mode); }

    // process this object
    m_ViewStyle |= mode;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::AddViewStyle, mode);
    }
}

void DrawObject::RemoveViewStyle(IGenum mode) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->RemoveViewStyle(mode); }

    // process this object
    m_ViewStyle &= ~mode;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::RemoveViewStyle, mode);
    }
}

unsigned int DrawObject::GetViewStyle() { return m_ViewStyle; }

void DrawObject::AddViewStyleOfModel(IGenum mode) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->AddViewStyleOfModel(mode); }

    // process this object
    auto* parentDrawObject = DynamicCast<DrawObject>(FindParent());
    if (parentDrawObject != this) {
        parentDrawObject->AddViewStyle(mode);
    } else {
        this->AddViewStyle(mode);
    }
}

unsigned int DrawObject::GetViewStyleOfModel() {
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

void DrawObject::SetPointSize(int size) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->SetPointSize(size); }

    // process this object
    m_PointSize = size;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::SetPointSize, size);
    }
}

int DrawObject::GetPointSize() { return m_PointSize; }

void DrawObject::SetLineWidth(int size) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->SetLineWidth(size); }

    // process this object
    m_LineWidth = size;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::SetLineWidth, size);
    }
}

int DrawObject::GetLineWidth() { return m_LineWidth; }

void DrawObject::SetTransparency(float transparency) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->SetTransparency(transparency); }

    // process this object
    if (transparency < 0.0f || transparency > 1.0f) {
        throw std::runtime_error("Transparency must be between 0-1");
    }
    m_Transparency = transparency;

    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::SetTransparency, transparency);
    }
}

float DrawObject::GetTransparency() { return m_Transparency; }

void DrawObject::ViewCloudPicture(Scene* scene, int index, int dimension) {
    // process display object
    if (m_DisplayObject) {
        m_DisplayObject->ViewCloudPicture(scene, index, dimension);
    }

    // process this object
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::ViewCloudPicture, scene, index,
                              dimension);
    }

    if (index == -1) {
        m_AttributeIndex = -1;
        m_AttributeDimension = -1;
    } else {
        m_AttributeIndex = index;
        m_AttributeDimension = dimension;
    }
    m_AttributeHelper->Modified();


    this->Modified();

    scene->Update();
}

void DrawObject::ViewCloudPictureOfModel(Scene* scene, int index,
                                         int dimension) {
    // process display object
    if (m_DisplayObject) {
        m_DisplayObject->ViewCloudPictureOfModel(scene, index, dimension);
    }

    // process this object
    auto* parent = dynamic_cast<DrawObject*>(FindParent());
    if (parent != nullptr && parent != this) {
        parent->ViewCloudPicture(scene, index, dimension);
    } else {
        this->ViewCloudPicture(scene, index, dimension);
    }
}

FloatArray::Pointer DrawObject::GetRenderPoints() {
    // return display object
    if (m_DisplayObject) { return m_DisplayObject->m_Positions; }

    // return this object
    return m_Positions;
}
void DrawObject::SetRenderPoints(FloatArray::Pointer points) {
    m_Positions = std::move(points);
}
void DrawObject::SetPolygonOffsetParameters(float factor, float units) {
    // process display object
    if (m_DisplayObject) {
        m_DisplayObject->SetPolygonOffsetParameters(factor, units);
    }

    // process this object
    this->m_PolygonFactor = factor;
    this->m_PolygonOffset = units;
    this->Modified();
}

void DrawObject::GetPolygonOffsetParameters(float& factor, float& units) {
    factor = this->m_PolygonFactor;
    units = this->m_PolygonOffset;
}

void DrawObject::SetLineOffsetParameters(float factor, float units) {
    // process display object
    if (m_DisplayObject) {
        m_DisplayObject->SetLineOffsetParameters(factor, units);
    }

    // process this object
    this->m_LineFactor = factor;
    this->m_LineOffset = units;
    this->Modified();
}

void DrawObject::GetLineOffsetParameters(float& factor, float& units) {
    factor = this->m_LineFactor;
    units = this->m_LineOffset;
}

void DrawObject::SetPointOffsetParameters(float units) {
    // process display object
    if (m_DisplayObject) { m_DisplayObject->SetPointOffsetParameters(units); }

    // process this object
    this->m_PointOffset = units;
    this->Modified();
}

void DrawObject::GetPointOffsetParameters(float& units) {
    units = this->m_PointOffset;
}

void DrawObject::SetDisplayObject(DataObject::Pointer dataObject) {
    m_DisplayObject = DynamicCast<DrawObject>(dataObject);
    // Copy object status
    m_DisplayObject->m_ViewStyle = this->m_ViewStyle;
    m_DisplayObject->m_Visibility = this->m_Visibility;
    m_DisplayObject->m_UseColor = this->m_UseColor;
    m_DisplayObject->m_UseNormalSmooth = this->m_UseNormalSmooth;
    m_DisplayObject->m_ColorWithCell = this->m_ColorWithCell;
    m_DisplayObject->m_PointSize = this->m_PointSize;
    m_DisplayObject->m_LineWidth = this->m_LineWidth;
    m_DisplayObject->m_Transparency = this->m_Transparency;
    // The original should be invisible, and it should remain invisible after conversion.
    m_DisplayObject->SetVisibility(this->GetVisibility());
    m_DisplayObject->m_UseColor = this->m_UseColor;
    // After the first extraction, there is no data for rendering "m_Positions"
    m_DisplayObject->ConvertToDrawableData();
    // After the first extraction, if the "m_Positions" is not updated, the shell will be extracted repeatedly
    m_Positions->Modified();
    m_DisplayObject->SetColorMapper(this->GetColorMapper());
}

DrawObject::Pointer DrawObject::GetDisplayObject() { return m_DisplayObject; }

void DrawObject::SetPositionBufferToVAO(GLVertexArray::Pointer VAO,
                                        GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_0, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
}
void DrawObject::SetColorBufferToVAO(GLVertexArray::Pointer VAO,
                                     GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_1, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
                      GL_FALSE, 0);
}
void DrawObject::SetNormalBufferToVAO(GLVertexArray::Pointer VAO,
                                      GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_2, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3, GL_FLOAT,
                      GL_FALSE, 0);
}
void DrawObject::SetTextureBufferToVAO(GLVertexArray::Pointer VAO,
                                       GLBuffer::Pointer VBO) {
    VAO->VertexBuffer(GL_VBO_IDX_3, VBO, 0, 2 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2, GL_FLOAT,
                      GL_FALSE, 0);
}

IGAME_NAMESPACE_END