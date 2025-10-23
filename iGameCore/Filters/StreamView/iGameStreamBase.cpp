#include "iGameStreamBase.h"
#include "iGameScene.h"
IGAME_NAMESPACE_BEGIN
iGameStreamBase::iGameStreamBase() {
    this->m_Points = Points::New();
    this->m_PositionColors = FloatArray::New();
    this->m_PositionColors->SetDimension(3);
    this->index = UnsignedIntArray::New();
    this->index->SetDimension(2);
    DynamicCast<iGame::DrawObject>(this)->AddViewStyle(IG_WIREFRAME);
    streamFilter = new iGameStreamTracer;
}
iGameStreamBase::~iGameStreamBase() {}

void iGameStreamBase::ComputeBoundingBox() {
    if (m_Bounding.isNull() || m_BoundingHelper->GetMTime() < m_Points->GetMTime()) {
        m_Bounding.reset();
        for (int i = 0; i < m_Points->GetNumberOfPoints(); i++) { m_Bounding.add(m_Points->GetPoint(i)); }
        m_BoundingHelper->Modified();
    }
}

void iGameStreamBase::ConvertToDrawableData() {
    if (!isUpdate) { return; }
    m_Points->Reset();
    m_PositionColors->Reset();
    index->Reset();
    int count = 0;
    for (int i = 0; i < m_StreamLine.size(); i++) {
        IdArray::Pointer line = IdArray::New();
        for (int j = 0; j + 1 < m_StreamLine[i].size() / 3; j += 2) {
            m_Points->AddPoint(Point(m_StreamLine[i][j * 3], m_StreamLine[i][j * 3 + 1], m_StreamLine[i][j * 3 + 2]));
            m_PositionColors->AddElement3(m_StreamLineColor[i][j * 3], m_StreamLineColor[i][j * 3 + 1],
                                          m_StreamLineColor[i][j * 3 + 2]);

            m_Points->AddPoint(
                    Point(m_StreamLine[i][j * 3 + 3], m_StreamLine[i][j * 3 + 4], m_StreamLine[i][j * 3 + 5]));
            m_PositionColors->AddElement3(m_StreamLineColor[i][j * 3 + 3], m_StreamLineColor[i][j * 3 + 4],
                                          m_StreamLineColor[i][j * 3 + 5]);
            index->AddElement2(count, count + 1);
            count += 2;
        }
    }
    this->m_ColorMapper->InitRange(m_PositionColors, -1);
    auto colors = this->m_ColorMapper->MapScalars(m_PositionColors, -1);
    m_Positions = m_Points->ConvertToArray();
    m_Positions->Modified();

    m_LineIndices = index;
    m_LineIndices->Modified();

    m_Colors = colors;
    m_Colors->Modified();

    if (m_Colors != nullptr) { m_UseColor = true; }
    isUpdate = false;
}

bool iGameStreamBase::IsUseSinglePassWireframeRendering() { return false; }

IGAME_NAMESPACE_END