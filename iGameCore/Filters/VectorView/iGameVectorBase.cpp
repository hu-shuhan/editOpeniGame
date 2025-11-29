#include "iGameVectorBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameVectorBase::iGameVectorBase() {
    this->m_Triangles = Points::New();
    this->m_PositionColors = FloatArray::New();
    this->m_PositionColors->SetDimension(3);
    this->index = UnsignedIntArray::New();
    this->index->SetDimension(3);
    this->hL = 0.0;
    this->hR = 0.0;
    this->tL = 0;
    this->tR = 0.0;
    this->count = 0;

    //this->SetTransparency(0.99f);
}
iGameVectorBase::~iGameVectorBase() {}
void iGameVectorBase::SetArrow(float _hR, float _hL, float _tR, float _tL) {
    if (_hR <= 0 || _hL <= 0 || _tR <= 0 || _tL <= 0) {
        std::cout << "Parameter is negative" << std::endl;
        return;
    }
    hR = _hR;
    hL = _hL;
    tR = _tR;
    tL = _tL;
}
std::vector<float> iGameVectorBase::GetArrow() {
    std::vector<float> result;
    result.emplace_back(hR);
    result.emplace_back(hL);
    result.emplace_back(tR);
    result.emplace_back(tL);
    return result;
}
void iGameVectorBase::SetDrawMode(DrawType _mode) {
    drawmode = _mode;
    return;
}
iGameVectorBase::DrawType iGameVectorBase::GetDrawMode() { return drawmode; }
void iGameVectorBase::SetInit(bool init) {
    isInit = init;
    return;
}
bool iGameVectorBase::GetInit() { return isInit; }
int iGameVectorBase::GetNth() { return Nth; }
void iGameVectorBase::SetNth(int _Nth) {
    if (_Nth <= 0) {
        std::cout << "Parameter is negative" << std::endl;
        return;
    }
    Nth = _Nth;
    return;
}

void iGameVectorBase::ComputeBoundingBox() {
    if (m_Bounding.isNull() || m_BoundingHelper->GetMTime() < m_Triangles->GetMTime()) {
        m_Bounding.reset();
        for (int i = 0; i < m_Triangles->GetNumberOfPoints(); i++) { m_Bounding.add(m_Triangles->GetPoint(i)); }
        m_BoundingHelper->Modified();
    }
}
bool iGameVectorBase::addArrow2Draw(iGame::DataObject* obj, std::string VecName) {
    auto _AttributeSet = obj->GetAttributeSet();
    if (!_AttributeSet) return false;
    auto allVectors = _AttributeSet->GetVector(VecName);
    // if (allVectors.isNone() || allVectors.attachmentType != IG_POINT) return;
    if (allVectors.IsNone()) return false;
    if (allVectors.attachmentType == IG_POINT) {
        long long numOfPoint = allVectors.pointer->GetNumberOfElements();
        auto allPoints = DynamicCast<PointSet>(obj)->GetPoints();
        auto mapper = DynamicCast<PointSet>(obj)->GetColorMapper();
        auto array = allVectors.pointer;
        mapper->InitRange(array, -1);
        auto colors = mapper->MapScalars(array, -1);
        auto colorsPtr = colors->RawPointer();
        // Downsampling prevents stuttering
        //int temCount = 0;
        if (drawmode == CellInRange) {
            int minNum = (CellIndexRange.second < numOfPoint) ? CellIndexRange.second : numOfPoint;
            for (int i = CellIndexRange.first; i < minNum; i++) {
                float v[4] = {0.0f};
                allVectors.pointer->GetElement(i, v);
                Vector3f vec(v[0], v[1], v[2]);
                //temCount++;
                //if (temCount % 5 == 0) {
                convertPoint2Arrow(allPoints->GetPoint(i), vec,
                                   Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                // }
            }
        } else if (drawmode == AllCell || Nth == 0) {
            for (int i = 0; i < numOfPoint; i++) {
                float v[4] = {0.0f};
                allVectors.pointer->GetElement(i, v);
                Vector3f vec(v[0], v[1], v[2]);
                //temCount++;
                //if (temCount % 5 == 0) {
                convertPoint2Arrow(allPoints->GetPoint(i), vec,
                                   Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                // }
            }
        } else {
            int temCount = 0;
            for (int i = 0; i < numOfPoint; i++) {
                float v[4] = {0.0f};
                allVectors.pointer->GetElement(i, v);
                Vector3f vec(v[0], v[1], v[2]);
                temCount++;
                if (temCount % Nth == 0) {
                    convertPoint2Arrow(allPoints->GetPoint(i), vec,
                                       Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                    // }
                }
            }
        }
        return true;
    } else if (allVectors.attachmentType == IG_CELL) {
        long long numOfCell = allVectors.pointer->GetNumberOfElements();

        auto volumeMesh = DynamicCast<VolumeMesh>(obj);
        if (volumeMesh == nullptr) {
            std::cout << "not a volumeMesh" << std::endl;
            return false;
        }
        CellCenter centerCul;
        auto mapper = DynamicCast<PointSet>(obj)->GetColorMapper();
        FloatArray::Pointer Vector1 = FloatArray::New();
        Vector1->SetDimension(3);
        Vector1->Resize(0);
        igIndex index = 0;
        if (drawmode == CellInRange) {
            int minNum = (CellIndexRange.second < numOfCell) ? CellIndexRange.second : numOfCell;
            for (int i = CellIndexRange.first; i < minNum; i++) {
                float v[4] = {0.0f};
                allVectors.pointer->GetElement(i, v);
                Vector1->AddElement3(v[0], v[1], v[2]);
            }
        } else {
            for (int i = 0; i < numOfCell; i++) {
                float v[4] = {0.0f};
                allVectors.pointer->GetElement(i, v);
                Vector1->AddElement3(v[0], v[1], v[2]);
            }
        }

        mapper->InitRange(Vector1, -1);
        auto colors = mapper->MapScalars(Vector1, -1);
        //auto array = allVectors.pointer;
        //mapper->InitRange(array, -1);
        //auto colors = mapper->MapScalars(array, -1);
        auto colorsPtr = colors->RawPointer();
        if (!volumeMesh->GetIsPolyhedronType()) {
            if (drawmode == CellInRange) {
                int minNum = (CellIndexRange.second < numOfCell) ? CellIndexRange.second : numOfCell;
                for (int i = CellIndexRange.first; i < minNum; i++) {
                    //      temCount++;
                    // if (temCount % 5 == 0) {
                    float v[4] = {0.0f};
                    allVectors.pointer->GetElement(i, v);
                    auto volume = volumeMesh->GetVolume(i);
                    auto center = centerCul.GetCenter(volume->m_Points);
                    Vector3f vec(v[0], v[1], v[2]);
                    convertPoint2Arrow(center, vec,
                                       Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                    //  }
                }
            } else if (drawmode == AllCell || Nth == 0) {
                for (int i = 0; i < numOfCell; i++) {
                    //      temCount++;
                    // if (temCount % 5 == 0) {
                    float v[4] = {0.0f};
                    allVectors.pointer->GetElement(i, v);
                    auto volume = volumeMesh->GetVolume(i);
                    auto center = centerCul.GetCenter(volume->m_Points);
                    Vector3f vec(v[0], v[1], v[2]);
                    convertPoint2Arrow(center, vec,
                                       Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                    //  }
                }
            } else {
                int temCount = 0;
                for (int i = 0; i < numOfCell; i++) {
                    temCount++;
                    if (temCount % Nth == 0) {
                        float v[4] = {0.0f};
                        allVectors.pointer->GetElement(i, v);
                        auto volume = volumeMesh->GetVolume(i);
                        auto center = centerCul.GetCenter(volume->m_Points);
                        Vector3f vec(v[0], v[1], v[2]);
                        convertPoint2Arrow(center, vec,
                                           Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                        //  }
                    }
                }
            }
            return true;
        } else {
            auto allVolume = volumeMesh->GetVolumes();
            auto allPoints = volumeMesh->GetPoints();
            //   int temCount = 0;
            if (drawmode == CellInRange) {
                for (int i = CellIndexRange.first; i < CellIndexRange.second; i++) {
                    float v[4] = {0.0f};
                    allVectors.pointer->GetElement(i, v);
                    auto center = centerCul.GetCenter(allPoints, allVolume, i);
                    Vector3f vec(v[0], v[1], v[2]);
                    convertPoint2Arrow(center, vec,
                                       Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                }
            } else if (drawmode == AllCell || Nth == 5) {
                for (int i = 0; i < numOfCell; i++) {
                    float v[4] = {0.0f};
                    allVectors.pointer->GetElement(i, v);
                    auto center = centerCul.GetCenter(allPoints, allVolume, i);
                    Vector3f vec(v[0], v[1], v[2]);
                    convertPoint2Arrow(center, vec,
                                       Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                }
            } else {
                int temCount = 0;
                for (int i = 0; i < numOfCell; i++) {
                    temCount++;
                    if (temCount % Nth == 0) {
                        float v[4] = {0.0f};
                        allVectors.pointer->GetElement(i, v);
                        auto center = centerCul.GetCenter(allPoints, allVolume, i);
                        Vector3f vec(v[0], v[1], v[2]);
                        convertPoint2Arrow(center, vec,
                                           Vector3f(colorsPtr[3 * i], colorsPtr[3 * i + 1], colorsPtr[3 * i + 2]));
                    }
                }
            }
            return true;
        }
    } else {
        std::cout << "error attachmentType!" << std::endl;
        return false;
    }
}
void iGameVectorBase::SetCellRange(int min, int max) {
    CellIndexRange.first = min;
    CellIndexRange.second = max;
}
std::pair<int, int> iGameVectorBase::GetCellRange() { return CellIndexRange; }
bool iGameVectorBase::DrawVector(std::string VecName) {
    isUpdate = true;
    if (!isInit) {
        auto sceneManager = iGame::SceneManager::Instance();
        auto scene = sceneManager->GetCurrentScene();
        if (!scene) return false;
        // scene->AddModel(scene->CreateModel(this));
        auto model = scene->GetCurrentModel();
        if (!model) return false;
        obj = model->GetDataObject();
        if (!obj) return false;
        isInit = true;
    }
    if (!obj) {
        isInit = false;
        return false;
    }
    iGame::AttributeSet* _AttributeSet;
    m_Triangles->Reset();
    m_PositionColors->Reset();
    index->Reset();
    count = 0;
    if (obj->HasSubDataObject()) {
        auto it = obj->SubDataObjectIteratorBegin();
        bool canDraw = false;
        for (; it != obj->SubDataObjectIteratorEnd(); it++) {
            auto subObj = it->second;
            if (addArrow2Draw(subObj, VecName)) { canDraw = true; }
        }
        if (canDraw) {
            ConvertToDrawableData();
        } else {
            return false;
        }
    } else {
        if (addArrow2Draw(obj, VecName)) {
            ConvertToDrawableData();
            return true;
        } else {
            return false;
        };
    }
    return true;
    // auto bound = DynamicCast<PointSet>(obj)->GetBoundingBox();
    // maxLength = (bound.max - bound.min).length();
}
bool iGameVectorBase::DrawVector(std::string VecName, iGame::DataObject* _obj) {
    isUpdate = true;
    if (!isInit) {
        obj = _obj;
        if (!obj) return false;
        isInit = true;
    }
    iGame::AttributeSet* _AttributeSet;
    m_Triangles->Reset();
    m_PositionColors->Reset();
    index->Reset();
    count = 0;
    if (obj->HasSubDataObject()) {
        auto it = obj->SubDataObjectIteratorBegin();
        bool canDraw = false;
        for (; it != obj->SubDataObjectIteratorEnd(); it++) {
            auto subObj = it->second;
            if (addArrow2Draw(subObj, VecName)) { canDraw = true; }
        }
        if (canDraw) {
            ConvertToDrawableData();
        } else {
            return false;
        }
    } else {
        if (addArrow2Draw(obj, VecName)) {
            ConvertToDrawableData();
            return true;
        } else {
            return false;
        };
    }

    // auto bound = DynamicCast<PointSet>(obj)->GetBoundingBox();
    // maxLength = (bound.max - bound.min).length();
    return true;
}
void iGameVectorBase::convertPoint2Arrow(Vector3f coord, Vector3f normal, Vector3f RGB) {
    Vector3f L;
    if (normal.length() == 0) {
        L = Vector3f(1.0, 0.0, 0.0);
    } else {
        L = normal.normalized();
    }
    Vector3f normal1 = Vector3f(0, 1, 0).cross(L);
    Vector3f normal2 = normal1.cross(L);
    Vector3f centerHigh = coord + L * (tL + hL);
    // Vector3f centerHigh = coord + L * (tL + hL) * maxLength;
    std::vector<Vector3f> vertices(7);
    std::vector<Vector3f> verticesMid(7);
    std::vector<Vector3f> verticesHigh(7);
    for (int i = 0; i < 6; i++) {
        float angle = igm::radians(60.0 * float(i));
        Vector3f tem = (normal1 * cos(angle) + normal2 * sin(angle)).normalized();
        //   Vector3f vertex = coord + tem * tR * maxLength;
        Vector3f vertex = coord + tem * tR;
        vertices[i] = vertex;
        //verticesMid[i] = vertex + L * tL * maxLength;
        verticesMid[i] = vertex + L * tL;
        verticesMid[i] = vertex + L * tL;
        //verticesHigh[i] = coord + tem * hR * maxLength + L * tL * maxLength;
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
        index->AddElement3(count, count + 1, count + 2);
        count += 3;
    }

}

void iGameVectorBase::ConvertToDrawableData() {
    if (!isUpdate) { return; }
    m_Positions = m_Triangles->ConvertToArray();
    m_Positions->Modified();

    std::cout << "VectorBase:" << m_Positions->GetMTime() << std::endl;
    std::cout << this->GetMTime() << std::endl << std::endl;

    m_TriangleIndices = index;
    m_TriangleIndices->Modified();

    m_Colors = m_PositionColors;
    m_Colors->Modified();

    if (m_Colors != nullptr) { m_UseColor = true; }
    isUpdate = false;
}
IGAME_NAMESPACE_END