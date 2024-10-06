#include "iGameTensorWidgetBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameTensorWidgetBase::iGameTensorWidgetBase() {
    this->m_TensorManager = iGameTensorRepresentation::New();
    this->m_TensorManager->SetSliceNum(5);
    this->m_DrawGlyphPoints = Points::New();
    this->m_DrawGlyphPointOrders = UnsignedIntArray::New();
    this->m_DrawGlyphColors = FloatArray::New();
    this->m_DrawGlyphColors->SetDimension(3);
}
iGameTensorWidgetBase::~iGameTensorWidgetBase() {}
void iGameTensorWidgetBase::SetPoints(Points::Pointer points) {
    this->m_Points = points;
}
void iGameTensorWidgetBase::SetTensorAttributes(
        ArrayObject::Pointer attributes) {
    this->m_TensorAttributes = attributes;
}
void iGameTensorWidgetBase::ShowTensorField() {
    UpdateGlyphDrawPositionData();
    UpdateGlyphDrawIndexData();
    UpdateGlyphDrawColor();
}
void iGameTensorWidgetBase::UpdateGlyphDrawPositionData() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    Point p;
    if (PointNum == 0 || !this->m_TensorAttributes ||
        this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return;
    }
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    m_DrawGlyphPoints->Resize(PointNum * GlyphPointNum);
    auto GlyphPoints = m_DrawGlyphPoints->RawPointer();
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        p = this->m_Points->GetPoint(i);
        m_TensorManager->SetPosition(p);
        for (int j = 0; j < 9; j++) {
            t[j] = this->m_TensorAttributes->GetValue(9 * i + j);
        }
        m_TensorManager->SetTensor(t);
        auto DrawPoints = m_TensorManager->GetDrawPoints()->RawPointer();
        IGsize st = i * 3 * GlyphPointNum;
        std::copy(DrawPoints, DrawPoints + 3 * GlyphPointNum, GlyphPoints + st);
    }
}
void iGameTensorWidgetBase::UpdateGlyphDrawIndexData() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    Point p;
    if (PointNum == 0 || !this->m_TensorAttributes ||
        this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return;
    }
    auto GlyphPointIndexOrders = m_TensorManager->GetDrawPointIndexOrders();
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    m_DrawGlyphPointOrders->Resize(PointNum *
                                   GlyphPointIndexOrders->GetNumberOfValues());
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        IGsize st = i * GlyphPointIndexOrders->GetNumberOfValues();
        IGsize offset = i * GlyphPointNum;
        for (int j = 0; j < GlyphPointIndexOrders->GetNumberOfValues(); j++) {
            //这里不太好用copy，不能保证两个类型相同
            m_DrawGlyphPointOrders->SetValue(
                    st + j, offset + GlyphPointIndexOrders->GetValue(j));
            //m_DrawGlyphPointOrders->AddValue(st + GlyphPointIndexOrders->GetValue(j));
        }
    }
}
void iGameTensorWidgetBase::UpdateGlyphDrawColor() {
    m_DrawGlyphColors = nullptr;
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    if (PointNum == 0 || this->m_PositionColors == nullptr ||
        this->m_PositionColors->GetNumberOfElements() != PointNum) {
        return;
    }
    int GlyphPointNum = m_TensorManager->GetNumberOfDrawPoints();
    float rgb[16] = {.0};
    m_DrawGlyphColors = FloatArray::New();
    m_DrawGlyphColors->SetDimension(3);
    m_DrawGlyphColors->Resize(PointNum * GlyphPointNum);
    auto GlyphColors = m_DrawGlyphColors->RawPointer();
    IGsize offset = 0;
    for (int i = 0; i < PointNum; i++) {
        m_PositionColors->GetElement(i, rgb);
        //std::cout << rgb[0] << ' ' << rgb[1] << ' ' << rgb[2] << '\n';
        for (int j = 0; j < GlyphPointNum; j++) {
            std::copy(rgb, rgb + 3, GlyphColors + offset);
            offset += 3;
            //m_DrawGlyphColors->AddValue(rgb[0]);
            //m_DrawGlyphColors->AddValue(rgb[1]);
            //m_DrawGlyphColors->AddValue(rgb[2]);
        }
    }
}
void iGameTensorWidgetBase::UpdateGlyphScale(double s) {
    this->m_TensorManager->SetScale(s);
    UpdateGlyphDrawPositionData();
}

void iGameTensorWidgetBase::SetPositionColors(FloatArray::Pointer colors) {
    this->m_PositionColors = colors;
    UpdateGlyphDrawColor();
}

DoubleArray::Pointer iGameTensorWidgetBase::GenerateVectorField() {
    int PointNum = this->m_Points ? this->m_Points->GetNumberOfPoints() : 0;
    if (PointNum == 0 || !this->m_TensorAttributes ||
        this->m_TensorAttributes->GetNumberOfValues() != (9 * PointNum)) {
        return nullptr;
    }
    double vector[3];
    m_EigenVector = DoubleArray::New();
    m_EigenVector->SetDimension(3);
    m_EigenVector->SetName(this->m_TensorAttributes->GetName() +
                           "_PrimaryFeature");
    m_EigenVector->Resize(PointNum);
    double t[9];
    for (int i = 0; i < PointNum; i++) {
        for (int j = 0; j < 9; j++) {
            t[j] = this->m_TensorAttributes->GetValue(9 * i + j);
        }
        m_TensorManager->InitTensorEigenData(t);
        m_TensorManager->GetEigenVector(0, vector);
        m_EigenVector->SetElement(i, vector);
    }
    return m_EigenVector;
}
//void iGameTensorWidgetBase::Draw(Scene* scene)
//{
//	if (!m_Visibility) { return; }
//	// Update uniform buffer
//	if (m_UseColor) {
//		scene->UBO().useColor = true;
//	}
//	else {
//		scene->UBO().useColor = false;
//	}
//	scene->UpdateUniformBuffer();
//
//	if (m_ViewStyle & IG_POINTS) {
//		scene->GetShader(Scene::NOLIGHT)->use();
//		m_PointVAO.bind();
//		//        glPointSize(m_PointSize);
//		glPointSize(9);
//		glad_glDrawArrays(GL_POINTS, 0, m_Positions->GetNumberOfValues() / 3);
//		m_PointVAO.release();
//
//	}
//	//if (m_ViewStyle & IG_WIREFRAME) {
//	//	if (m_UseColor) {
//	//		scene->GetShader(Scene::NOLIGHT)->use();
//	//	}
//	//	else {
//	//		auto shader = scene->GetShader(Scene::PURECOLOR);
//	//		shader->use();
//	//		shader->setUniform(shader->getUniformLocation("inputColor"),
//	//			igm::vec3{ 0.0f, 0.0f, 0.0f });
//	//	}
//
//	//	m_LineVAO.bind();
//	//	glLineWidth(m_LineWidth);
//	//	glad_glDrawElements(GL_LINES, M_LineIndices->GetNumberOfValues(),
//	//		GL_UNSIGNED_INT, 0);
//	//	m_LineVAO.release();
//	//}
//	if (m_ViewStyle & IG_SURFACE) {
//		scene->GetShader(Scene::BLINNPHONG)->use();
//		m_TriangleVAO.bind();
//		glad_glDrawElements(GL_TRIANGLES,
//			M_TriangleIndices->GetNumberOfValues(),
//			GL_UNSIGNED_INT, 0);
//		m_TriangleVAO.release();
//	}
//
//}

void iGameTensorWidgetBase::ConvertToDrawableData() {
    if (!m_Flag) {
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

        m_VertexEBO.create();
        m_VertexEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_LineEBO.create();
        m_LineEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_TriangleEBO.create();
        m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_Flag = true;
    }
    m_Positions = m_DrawGlyphPoints->ConvertToArray();
    m_Positions->Modified();
    //M_TriangleIndices = m_DrawGlyphPointOrders;
    for (int i = 0; i < m_DrawGlyphPointOrders->GetNumberOfValues(); i++) {
        m_TriangleIndices->AddId(static_cast<igIndex>(
                *m_DrawGlyphPointOrders->RawPointer() + i));
    }
    m_Colors = m_DrawGlyphColors;


    GLAllocateGLBuffer(m_PositionVBO,
                       m_Positions->GetNumberOfValues() * sizeof(float),
                       m_Positions->RawPointer());

    //GLAllocateGLBuffer(m_VertexEBO,
    //	M_VertexIndices->GetNumberOfValues() *
    //	sizeof(unsigned int),
    //	M_VertexIndices->RawPointer());

    //GLAllocateGLBuffer(m_LineEBO,
    //	M_LineIndices->GetNumberOfValues() *
    //	sizeof(unsigned int),
    //	M_LineIndices->RawPointer());

    //GLAllocateGLBuffer(m_TriangleEBO,
    //                   M_TriangleIndices->GetNumberOfValues() *
    //                           sizeof(unsigned int),
    //                   M_TriangleIndices->RawPointer());
    GLAllocateGLBuffer(m_TriangleEBO,
                       m_TriangleIndices->GetNumberOfIds() * sizeof(igIndex),
                       m_TriangleIndices->RawPointer());

    m_PointVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);

    //m_VertexVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    //GLSetVertexAttrib(m_VertexVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
    //	GL_FALSE, 0);
    //m_VertexVAO.elementBuffer(m_VertexEBO);

    //m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    //GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
    //	GL_FALSE, 0);
    //m_LineVAO.elementBuffer(m_LineEBO);

    m_TriangleVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                               3 * sizeof(float));
    GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                      GL_FLOAT, GL_FALSE, 0);
    m_TriangleVAO.elementBuffer(m_TriangleEBO);
    m_UseColor = false;
    if (m_Colors != nullptr) {
        m_UseColor = true;
        GLAllocateGLBuffer(m_ColorVBO,
                           m_Colors->GetNumberOfValues() * sizeof(float),
                           m_Colors->RawPointer());

        m_PointVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);

        //m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        //GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
        //	GL_FALSE, 0);

        m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
                                   3 * sizeof(float));
        GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);
    }
}
IGAME_NAMESPACE_END