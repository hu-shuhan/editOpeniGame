#include "iGameVectorBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameVectorBase::iGameVectorBase() {
    this->m_Triangles = Points::New();
    this->m_PositionColors = FloatArray::New();
    this->m_PositionColors->SetDimension(3);
    this->index = IdArray::New();
}
iGameVectorBase::~iGameVectorBase() {}
void iGameVectorBase::SetArrow(float _hR, float _hL, float _tR, float _tL) {
    hR = _hR;
    hL = _hL;
    tR = _tR;
    tL = _tL;
    return;
}
void iGameVectorBase::DrawVector(std::string VecName) {
    if (!isInit) {
        auto sceneManager = iGame::SceneManager::Instance();
        auto scene = sceneManager->GetCurrentScene();
        if (!scene) return;
         model = scene->GetCurrentModel();
        if (!model) return;
        isInit = true;
    }
    auto obj = model->GetDataObject();
    if (!obj) return;
    auto AttributeSet = obj->GetAttributeSet();
    if (!AttributeSet) return;
    auto allVectors = AttributeSet->GetVector(VecName);
    if (allVectors.isNone() || allVectors.attachmentType != IG_POINT) return;
    m_Triangles->Reset();
    m_PositionColors->Reset();
    index->Reset();
    count = 0;
    long long numOfPoint = allVectors.pointer->GetNumberOfElements();
    auto allPoints =DynamicCast<PointSet>(model->GetDataObject())->GetPoints();
    auto mapper = ScalarsToColors::New();
    auto array = allVectors.pointer;
    mapper->InitRange(array, -1); 
    auto colors = mapper->MapScalars(array, -1);
    auto colorsPtr = colors->RawPointer();
    //m_Triangles->AddPoint(Vector3f(0.0, 1.0, 0.0));
    //m_PositionColors->AddElement3(1.0, 0.2, 0.3);
    //m_Triangles->AddPoint(Vector3f(1.0, 0.0, 0.0));
    //m_PositionColors->AddElement3(1.0, 0.2, 0.3);
    //m_Triangles->AddPoint(Vector3f(0.0, 0.0, 1.0));
    //m_PositionColors->AddElement3(1.0, 0.2, 0.3);
    //index->AddId(0);
    return;
    for (int i = 0; i < numOfPoint; i++) {
        float v[4] = {0.0f};
        allVectors.pointer->GetElement(i, v);
        Vector3f vec(v[0], v[1], v[2]);     
        convertPoint2Arrow(allPoints->GetPoint(i), vec,Vector3f(colorsPtr[3 * i], colorsPtr[3 * i+1], colorsPtr[3 * i+2]));
    }
    return;
}
void iGameVectorBase::convertPoint2Arrow(Vector3f coord,Vector3f normal,Vector3f RGB) {
    Vector3f L = normal.normalized();
    Vector3f normal1 = Vector3f(0, 1, 0).cross(L);
    Vector3f normal2 = normal1.cross(L);
    Vector3f centerHigh = coord + normal * (tL + hL);
    std::vector<Vector3f> vertices(7);
    std::vector<Vector3f> verticesMid(7);
    std::vector<Vector3f> verticesHigh(7);
    for (int i = 0; i < 6; i++) {
        float angle = igm::radians(60.0 * float(i));
        Vector3f tem =
                (normal1 * cos(angle) + normal2 * sin(angle)).normalized();
        Vector3f vertex = coord + tem * tR;
        vertices[i] = vertex;
        verticesMid[i] = vertex + L * tL;
        verticesHigh[i] = coord + tem * hR + L * tL;
    }
    //tail
    for (int i = 1; i < 5; i++) {
        m_Triangles->AddPoint(vertices[0]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(vertices[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(vertices[i + 1]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        index->AddId(count++);

        m_Triangles->AddPoint(verticesMid[0]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesMid[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesMid[i + 1]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        index->AddId(count++);
    }
    for (int i = 0; i < 6; i++) {
        m_Triangles->AddPoint(vertices[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(vertices[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesMid[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        index->AddId(count++);

        m_Triangles->AddPoint(vertices[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesMid[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesMid[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        index->AddId(count++);
    }
    //head
    for (int i = 1; i < 5; i++) {
        m_Triangles->AddPoint(verticesHigh[0]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesHigh[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesHigh[i + 1]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        index->AddId(count++);
    }

    for (int i = 0; i < 6; i++) {
        m_Triangles->AddPoint(verticesHigh[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(verticesHigh[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        m_Triangles->AddPoint(centerHigh);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        index->AddId(count++);
    }
    return ;
}

//void iGameVectorBase::Draw(Scene* scene) {
//    if (!m_Visibility) { return; }
//    // Update uniform buffer
//    if (m_UseColor) {
//        scene->UBO().useColor = true;
//    } else {
//        scene->UBO().useColor = false;
//    }
//    scene->UpdateUniformBuffer();
//
//    if (m_ViewStyle & IG_POINTS) {
//        scene->GetShader(Scene::NOLIGHT)->use();
//        m_PointVAO.bind();
//        //        glPointSize(m_PointSize);
//        glPointSize(9);
//        glad_glDrawArrays(GL_POINTS, 0, m_Positions->GetNumberOfValues() / 3);
//        m_PointVAO.release();
//    }
//    if (m_ViewStyle & IG_WIREFRAME) {
//        if (m_UseColor) {
//            scene->GetShader(Scene::NOLIGHT)->use();
//        } else {
//            auto shader = scene->GetShader(Scene::PURECOLOR);
//            shader->use();
//            shader->setUniform(shader->getUniformLocation("inputColor"),
//                               igm::vec3{0.0f, 0.0f, 0.0f});
//        }
//
//        m_LineVAO.bind();
//        glLineWidth(m_LineWidth);
//        //glad_glDrawElements(GL_LINES, M_LineIndices->GetNumberOfValues(),
//        //	GL_UNSIGNED_INT, 0);
//        glad_glDrawArrays(GL_LINES, 0, m_Positions->GetNumberOfValues());
//        m_LineVAO.release();
//    }
//}

void iGameVectorBase::ConvertToDrawableData() {
    if (!m_Flag) {
        //m_PointVAO.create();
        //m_VertexVAO.create();
       // m_LineVAO.create();
        m_TriangleVAO.create();

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
        m_TriangleEBO.create();
        m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_Flag = true;
    }
    //m_Points->Reset();
    //for (int i = 0; i < m_StreamLine.size(); i++) {
    //    IdArray::Pointer line = IdArray::New();
    //    for (int j = 0; j + 1 < m_StreamLine[i].size() / 3; j++) {
    //        m_Points->AddPoint(Point(m_StreamLine[i][j * 3],
    //                                 m_StreamLine[i][j * 3 + 1],
    //                                 m_StreamLine[i][j * 3 + 2]));
    //        m_Points->AddPoint(Point(m_StreamLine[i][j * 3 + 3],
    //                                 m_StreamLine[i][j * 3 + 4],
    //                                 m_StreamLine[i][j * 3 + 5]));
    //    }
    //}


    m_Positions = m_Triangles->ConvertToArray();
    m_Positions->Modified();
    m_TriangleIndices = index;
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

    GLAllocateGLBuffer(m_TriangleEBO,
    	m_TriangleIndices->GetNumberOfIds() *
    	sizeof(unsigned int),
    	m_TriangleIndices->RawPointer());

    //m_PointVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    //GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
    //	GL_FALSE, 0);

    //m_VertexVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    //GLSetVertexAttrib(m_VertexVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
    //	GL_FALSE, 0);
    //m_VertexVAO.elementBuffer(m_VertexEBO);

    //m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
    //GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
    //                  GL_FALSE, 0);
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

        //m_PointVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        //GLSetVertexAttrib(m_PointVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
        //	GL_FALSE, 0);

        //m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        //GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        //                  GL_FLOAT, GL_FALSE, 0);

        m_TriangleVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));
        GLSetVertexAttrib(m_TriangleVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
        	GL_FLOAT, GL_FALSE, 0);
    }
}
IGAME_NAMESPACE_END