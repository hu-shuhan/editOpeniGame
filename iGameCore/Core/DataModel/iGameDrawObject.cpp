#include "iGameDrawObject.h"

IGAME_NAMESPACE_BEGIN

DrawObject::DrawObject() {
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

    m_Clipper = iGameClipper::New();
}

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
        m_CellEBO.create();
        m_CellEBO.target(GL_ELEMENT_ARRAY_BUFFER);

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

        m_Flag = false;
    }
}

void DrawObject::ConvertToDrawableData() {
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::ConvertToDrawableData);
        return;
    }
}

void DrawObject::ReAllocateDisplayBuffer() {
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::ReAllocateDisplayBuffer);
        return;
    }

    this->CreateDrawBuffer();

        std::cout << "DrawObject:" << m_Positions->GetMTime() << std::endl;
    std::cout << this->GetMTime() << std::endl ;
          
    if (m_Positions->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_PositionVBO,
                           m_Positions->GetNumberOfValues() * sizeof(float),
                           m_Positions->RawPointer());

        SetPositionBufferToVAO(m_PointVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_LineVAO, m_PositionVBO);
        SetPositionBufferToVAO(m_TriangleVAO, m_PositionVBO);
    }

    if (m_Colors->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_ColorVBO,
                           m_Colors->GetNumberOfValues() * sizeof(float),
                           m_Colors->RawPointer());

        SetColorBufferToVAO(m_PointVAO, m_ColorVBO);
        SetColorBufferToVAO(m_LineVAO, m_ColorVBO);
        SetColorBufferToVAO(m_TriangleVAO, m_ColorVBO);
    }

    if (m_Normals->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_NormalVBO,
                           m_Normals->GetNumberOfValues() * sizeof(float),
                           m_Normals->RawPointer());

        SetNormalBufferToVAO(m_PointVAO, m_NormalVBO);
        SetNormalBufferToVAO(m_LineVAO, m_NormalVBO);
        SetNormalBufferToVAO(m_TriangleVAO, m_NormalVBO);
    }

    if (m_Textures->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_TextureVBO,
                           m_Textures->GetNumberOfValues() * sizeof(float),
                           m_Textures->RawPointer());

        SetTextureBufferToVAO(m_PointVAO, m_TextureVBO);
        SetTextureBufferToVAO(m_LineVAO, m_TextureVBO);
        SetTextureBufferToVAO(m_TriangleVAO, m_TextureVBO);
    }

    if (m_PointIndices->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_PointEBO,
                           m_PointIndices->GetNumberOfValues() *
                                   sizeof(igIndex),
                           m_PointIndices->RawPointer());

        m_PointVAO.elementBuffer(m_PointEBO);
    }

    if (m_LineIndices->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_LineEBO,
                           m_LineIndices->GetNumberOfValues() * sizeof(igIndex),
                           m_LineIndices->RawPointer());

        m_LineVAO.elementBuffer(m_LineEBO);
    }

    if (m_TriangleIndices->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_TriangleEBO,
                           m_TriangleIndices->GetNumberOfValues() *
                                   sizeof(igIndex),
                           m_TriangleIndices->RawPointer());

        m_TriangleVAO.elementBuffer(m_TriangleEBO);
    }

    if (m_CellPositions->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_CellPositionVBO,
                           m_CellPositions->GetNumberOfValues() * sizeof(float),
                           m_CellPositions->RawPointer());

        SetPositionBufferToVAO(m_CellVAO, m_CellPositionVBO);
    }

    if (m_CellColors->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_CellColorVBO,
                           m_CellColors->GetNumberOfValues() * sizeof(float),
                           m_CellColors->RawPointer());

        SetColorBufferToVAO(m_CellVAO, m_CellColorVBO);
    }

    if (m_CellIndices->GetMTime() > this->GetMTime()) {
        GLAllocateGLBuffer(m_CellEBO,
                           m_CellIndices->GetNumberOfValues() * sizeof(float),
                           m_CellIndices->RawPointer());

        m_CellVAO.elementBuffer(m_CellEBO);
    }
}

IGenum DrawObject::GetDataObjectType() const { return IG_DRAW_OBJECT; }

IGsize DrawObject::GetRealMemorySize() { return 0; };

void DrawObject::SetVisibility(bool f) {
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
    m_ViewStyle = mode;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::SetViewStyle, mode);
    }
}

void DrawObject::AddViewStyle(IGenum mode) {
    m_ViewStyle |= mode;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::AddViewStyle, mode);
    }
}

void DrawObject::RemoveViewStyle(IGenum mode) {
    m_ViewStyle &= ~mode;
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::RemoveViewStyle, mode);
    }
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
    if (this->HasSubDataObject()) {
        ProcessSubDataObjects(&DrawObject::ViewCloudPicture, scene, index,
                              dimension);
    }
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

void DrawObject::SetPositionBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO) {
    VAO.vertexBuffer(GL_VBO_IDX_0, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
}
void DrawObject::SetColorBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO) {
    VAO.vertexBuffer(GL_VBO_IDX_1, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
                      GL_FALSE, 0);
}
void DrawObject::SetNormalBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO) {
    VAO.vertexBuffer(GL_VBO_IDX_2, VBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3, GL_FLOAT,
                      GL_FALSE, 0);
}
void DrawObject::SetTextureBufferToVAO(GLVertexArray& VAO, GLBuffer& VBO) {
    VAO.vertexBuffer(GL_VBO_IDX_3, VBO, 0, 2 * sizeof(float));
    GLSetVertexAttrib(VAO, GL_LOCATION_IDX_3, GL_VBO_IDX_3, 2, GL_FLOAT,
                      GL_FALSE, 0);
}

IGAME_NAMESPACE_END