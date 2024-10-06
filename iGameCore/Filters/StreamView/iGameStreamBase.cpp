#include "iGameStreamBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameStreamBase::iGameStreamBase() {
    this->m_Points = Points::New();
    this->m_PositionColors = FloatArray::New();
    streamFilter = new iGameStreamTracer;
}
iGameStreamBase::~iGameStreamBase() {}


//void iGameStreamBase::Draw(Scene* scene)
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
//	if (m_ViewStyle & IG_WIREFRAME) {
//		if (m_UseColor) {
//			scene->GetShader(Scene::NOLIGHT)->use();
//		}
//		else {
//			auto shader = scene->GetShader(Scene::PURECOLOR);
//			shader->use();
//			shader->setUniform(shader->getUniformLocation("inputColor"),
//				igm::vec3{ 0.0f, 0.0f, 0.0f });
//		}
//
//		m_LineVAO.bind();
//		glLineWidth(m_LineWidth);
//		//glad_glDrawElements(GL_LINES, M_LineIndices->GetNumberOfValues(),
//		//	GL_UNSIGNED_INT, 0);
//		glad_glDrawArrays(GL_LINES, 0, m_Positions->GetNumberOfValues());
//		m_LineVAO.release();
//	}
//
//}

void iGameStreamBase::ConvertToDrawableData() {
    if (!m_Flag) {
        //m_PointVAO.create();
        //m_VertexVAO.create();
        m_LineVAO.create();
        //m_TriangleVAO.create();

        m_PositionVBO.create();
        m_PositionVBO.target(GL_ARRAY_BUFFER);
        m_ColorVBO.create();
        m_ColorVBO.target(GL_ARRAY_BUFFER);
        //m_NormalVBO.create();
        //m_NormalVBO.target(GL_ARRAY_BUFFER);
        //m_TextureVBO.create();
        //m_TextureVBO.target(GL_ARRAY_BUFFER);

        //m_VertexEBO.create();
        //m_VertexEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        //m_LineEBO.create();
        //m_LineEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        //m_TriangleEBO.create();
        //m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_Flag = true;
    }
    m_Points->Reset();
    for (int i = 0; i < m_StreamLine.size(); i++) {
        IdArray::Pointer line = IdArray::New();
        for (int j = 0; j + 1 < m_StreamLine[i].size() / 3; j++) {
            m_Points->AddPoint(Point(m_StreamLine[i][j * 3],
                                     m_StreamLine[i][j * 3 + 1],
                                     m_StreamLine[i][j * 3 + 2]));
            m_Points->AddPoint(Point(m_StreamLine[i][j * 3 + 3],
                                     m_StreamLine[i][j * 3 + 4],
                                     m_StreamLine[i][j * 3 + 5]));
        }
    }


    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();
    //M_TriangleIndices = m_DrawGlyphPointOrders;
    m_Colors = m_PositionColors;


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
    //	M_TriangleIndices->GetNumberOfValues() *
    //	sizeof(unsigned int),
    //	M_TriangleIndices->RawPointer());

    //m_PointVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    //GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
    //	GL_FALSE, 0);

    //m_VertexVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    //GLSetVertexAttrib(m_VertexVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
    //	GL_FALSE, 0);
    //m_VertexVAO.elementBuffer(m_VertexEBO);

    m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                      GL_FALSE, 0);
    //m_LineVAO.elementBuffer(m_LineEBO);

    //m_TriangleVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
    //	3 * sizeof(float));
    //GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
    //	GL_FLOAT, GL_FALSE, 0);
    //m_TriangleVAO.elementBuffer(m_TriangleEBO);

    m_UseColor = false;
    if (m_Colors != nullptr) {
        m_UseColor = true;
        GLAllocateGLBuffer(m_ColorVBO,
                           m_Colors->GetNumberOfValues() * sizeof(float),
                           m_Colors->RawPointer());

        //m_PointVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        //GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
        //	GL_FALSE, 0);

        m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                          GL_FLOAT, GL_FALSE, 0);

        //m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        //GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //	GL_FLOAT, GL_FALSE, 0);
    }
}
IGAME_NAMESPACE_END