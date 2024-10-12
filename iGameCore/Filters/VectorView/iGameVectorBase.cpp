#include "iGameVectorBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameVectorBase::iGameVectorBase() {
    this->m_Triangles = Points::New();
    this->m_PositionColors = FloatArray::New();
    this->m_PositionColors->SetDimension(3);

    this->index = UnsignedIntArray::New();
    this->index->SetDimension(3);
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
    auto allPoints = DynamicCast<PointSet>(model->GetDataObject())->GetPoints();
    auto mapper = ScalarsToColors::New();
    auto array = allVectors.pointer;
    mapper->InitRange(array, -1);
    auto colors = mapper->MapScalars(array, -1);
    auto colorsPtr = colors->RawPointer();
    //m_Triangles->AddPoint(Vector3f(0.0, 1.0, 0.0));
    //m_PositionColors->AddElement3(0.0, 1.0, 0.0);
    //index->AddId(0);
    //m_Triangles->AddPoint(Vector3f(1.0, 0.0, 0.0));
    //m_PositionColors->AddElement3(1.0, 0.0, 0.0);
    //index->AddId(1);
    //m_Triangles->AddPoint(Vector3f(0.0, 0.0, 1.0));
    //m_PositionColors->AddElement3(0.0, 0.0, 1.0);
    //index->AddId(2);
    //return;
    for (int i = 0; i < numOfPoint; i++) {
        float v[4] = {0.0f};
        allVectors.pointer->GetElement(i, v);
        Vector3f vec(v[0], v[1], v[2]);
        convertPoint2Arrow(allPoints->GetPoint(i), vec,
                           Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1],
                                    colorsPtr[3 * i + 2]));
    }
    return;
}
void iGameVectorBase::convertPoint2Arrow(Vector3f coord, Vector3f normal,
                                         Vector3f RGB) {
    Vector3f L = normal.normalized();
    Vector3f normal1 = Vector3f(0, 1, 0).cross(L);
    Vector3f normal2 = normal1.cross(L);
    Vector3f centerHigh = coord + L * (tL + hL);
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
        //index->AddId(count++);
        m_Triangles->AddPoint(vertices[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(vertices[i + 1]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        index->AddElement3(count, count + 1, count + 2);
        count += 3;

        m_Triangles->AddPoint(verticesMid[0]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesMid[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesMid[i + 1]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        index->AddElement3(count, count + 1, count + 2);
        count += 3;
    }
    for (int i = 0; i < 6; i++) {
        m_Triangles->AddPoint(vertices[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(vertices[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesMid[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        index->AddElement3(count, count + 1, count + 2);
        count += 3;

        m_Triangles->AddPoint(vertices[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesMid[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesMid[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        index->AddElement3(count, count + 1, count + 2);
        count += 3;
    }
    //head
    for (int i = 1; i < 5; i++) {
        m_Triangles->AddPoint(verticesHigh[0]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesHigh[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesHigh[i + 1]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        index->AddElement3(count, count + 1, count + 2);
        count += 3;
    }

    for (int i = 0; i < 6; i++) {
        m_Triangles->AddPoint(verticesHigh[i]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(verticesHigh[(i + 1) % 6]);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
        m_Triangles->AddPoint(centerHigh);
        m_PositionColors->AddElement3(RGB[0], RGB[1], RGB[2]);
        //index->AddId(count++);
    }
    return;
}

void iGameVectorBase::ConvertToDrawableData() {
    m_Positions = m_Triangles->ConvertToArray();
    m_Positions->Modified();

    m_TriangleIndices = index;
    m_TriangleIndices->Modified();

    m_Colors = m_PositionColors;
    m_Colors->Modified();

    if (m_Colors != nullptr) { m_UseColor = true; }
}
IGAME_NAMESPACE_END