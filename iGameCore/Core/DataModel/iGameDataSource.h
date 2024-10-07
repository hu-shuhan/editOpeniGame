#ifndef iGameDataSource_h
#define iGameDataSource_h

#include "iGameDrawObject.h"
#include "iGamePoints.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN
class DataSource : public DrawObject {
public:
    I_OBJECT(DataSource);
    static Pointer New() { return new DataSource; }

protected:
    DataSource() {}
    ~DataSource() override = default;


public:
    //void Draw(Scene*) override {}
    void ConvertToDrawableData() override {}
    bool IsDrawable() override { return true; }
};

class LineSource : public DataSource {
public:
    I_OBJECT(LineSource);
    static Pointer New() { return new LineSource; }

    void SetPoints(Points::Pointer arr) {
        if (m_Points != arr) {
            m_Points = arr;
            this->Modified();
        }
    }
    void SetColors(FloatArray::Pointer arr) {
        if (m_ColorValues != arr) {
            m_ColorValues = arr;
            if (m_ColorValues == nullptr) {
                m_UseColor = false;
            } else {
                m_UseColor = true;
            }
            this->Modified();
        }
    }
    void SetLines(CellArray::Pointer arr) {
        if (m_Lines != arr) {
            m_Lines = arr;
            this->Modified();
        }
    }
    void SetPolyLines(CellArray::Pointer arr) {
        if (m_PolyLines != arr) {
            m_PolyLines = arr;
            this->Modified();
        }
    }

    //IGsize AddPoint(const Point& p) {
    //    return m_Points->AddPoint(p);
    //}

    //IGsize AddLine(const IGsize ptId1, const IGsize ptId2) {
    //    return m_Lines->AddCellId2(ptId1, ptId2);
    //}

    //IGsize AddPolyLine(igIndex* ptIds, int size) {
    //    return m_PolyLines->AddCellIds(ptIds, size);
    //}

    //IGsize AddLine(const Vector3f& startPos, const Vector3f& endPos)
    //{
    //    IGuint startOffset = GetStartOffset(m_NumberOfLines);
    //    IGuint endOffset = startOffset + 2;
    //    if (endOffset > m_Buffer->GetNumberOfElements()) {
    //        const IGsize newSize = std::max(endOffset, (startOffset * 2));
    //        m_Buffer->Resize(newSize);
    //    }

    //    m_Buffer->SetElement(startOffset, startPos);
    //    m_Buffer->SetElement(static_cast<IGsize>(startOffset) + 1, endPos);
    //    m_Offsets->AddValue(endOffset);
    //    return m_NumberOfLines++;
    //}

    //IGsize AddLine(const Vector3f& startPos, const Vector3f& endPos,
    //               float startColor, float endColor) {
    //    IGuint startOffset = GetStartOffset(m_NumberOfLines);
    //    IGuint endOffset = startOffset + 2;
    //    if (endOffset > m_Buffer->GetNumberOfElements()) {
    //        const IGsize newSize = std::max(endOffset, (startOffset * 2));
    //        m_Buffer->Resize(newSize);
    //    }

    //    m_Buffer->SetElement(startOffset, startPos);
    //    m_Buffer->SetElement(static_cast<IGsize>(startOffset) + 1, endPos);
    //    m_ColorValues->AddValue(startColor);
    //    m_ColorValues->AddValue(endColor);
    //    m_Offsets->AddValue(endOffset);
    //    m_UseColor = true;
    //    return m_NumberOfLines++;
    //}

    //IGsize AddPolyLine(ElementArray<Vector3f>::Pointer processPos)
    //{
    //    IGuint startOffset = GetStartOffset(m_NumberOfLines);
    //    IGuint endOffset = startOffset + processPos->GetNumberOfElements();
    //    if (endOffset > m_Buffer->GetNumberOfElements()) {
    //        const IGsize newSize = std::max(endOffset, (startOffset * 2));
    //        m_Buffer->Resize(newSize);
    //    }

    //    IGuint pos = startOffset;
    //    //m_Buffer->SetElement(startOffset, startPos);
    //    for (int i = 0; i < processPos->GetNumberOfElements(); i++) {
    //        m_Buffer->SetElement(pos,
    //                           processPos->GetElement(i));
    //        pos++;
    //    }
    //    //m_Buffer->SetElement(endOffset - 1, endPos);
    //    m_Offsets->AddValue(endOffset);
    //    return m_NumberOfLines++;
    //}

    //IGsize AddPolyLine(ElementArray<Vector3f>::Pointer processPos, FloatArray::Pointer processColor) {
    //    IGuint startOffset = GetStartOffset(m_NumberOfLines);
    //    IGuint endOffset = startOffset + processPos->GetNumberOfElements();
    //    if (endOffset > m_Buffer->GetNumberOfElements()) {
    //        const IGsize newSize = std::max(endOffset, (startOffset * 2));
    //        m_Buffer->Resize(newSize);
    //    }

    //    IGuint pos = startOffset;
    //    //m_Buffer->SetElement(startOffset, startPos);
    //    for (int i = 0; i < processPos->GetNumberOfElements(); i++) {
    //        m_Buffer->SetElement(pos, processPos->GetElement(i));
    //        m_ColorValues->AddValue(processColor->GetValue(i));
    //        pos++;
    //    }
    //    //m_Buffer->SetElement(endOffset - 1, endPos);
    //    m_Offsets->AddValue(endOffset);
    //    m_UseColor = true;
    //    return m_NumberOfLines++;
    //}

protected:
    LineSource() {
        m_Points = Points::New();
        m_ColorValues = FloatArray::New();
        m_Lines = CellArray::New();
        m_PolyLines = CellArray::New();
    }
    ~LineSource() override = default;

    void ComputeBoundingBox() override {
        if (m_Bounding.isNull() ||
            m_BoundingHelper->GetMTime() < m_Points->GetMTime()) {
            m_Bounding.reset();
            for (int i = 0; i < m_Points->GetNumberOfPoints(); i++) {
                m_Bounding.add(m_Points->GetPoint(i));
            }
            m_BoundingHelper->Modified();
        }
    }

    Points::Pointer m_Points{};
    FloatArray::Pointer m_ColorValues{};

    CellArray::Pointer m_Lines{};
    CellArray::Pointer m_PolyLines{};

public:
    //void Draw(Scene* scene) override {
    //    if (!m_Visibility) { return; }
    //
    //    // update uniform buffer
    //    if (m_UseColor) {
    //        scene->UBO().useColor = true;
    //    } else {
    //        scene->UBO().useColor = false;
    //    }
    //    scene->UpdateUniformBuffer();
    //
    //    if (m_UseColor) {
    //        scene->GetShader(Scene::NOLIGHT)->use();
    //    } else {
    //        auto shader = scene->GetShader(Scene::PURECOLOR);
    //        shader->use();
    //        shader->setUniform(shader->getUniformLocation("inputColor"),
    //                           igm::vec3{0.0f, 0.0f, 0.0f});
    //    }
    //
    //    m_LineVAO.bind();
    //    glLineWidth(2);
    //    glad_glDrawElements(GL_LINES, m_Indices->GetNumberOfIds(),
    //                        GL_UNSIGNED_INT, 0);
    //    m_LineVAO.release();
    //}
    void ConvertToDrawableData() override {
        this->Create();

        if (m_Positions && m_Positions->GetMTime() > this->GetMTime()) {
            return;
        }

        m_Positions = m_Points->ConvertToArray();
        m_Positions->Modified();

        m_Indices = IdArray::New();
        IdArray::Pointer line = IdArray::New();
        for (int i = 0; i < m_Lines->GetNumberOfCells(); i++) {
            m_Lines->GetCellIds(i, line);
            m_Indices->AddId(line->GetId(0));
            m_Indices->AddId(line->GetId(1));
        }
        for (int i = 0; i < m_PolyLines->GetNumberOfCells(); i++) {
            m_PolyLines->GetCellIds(i, line);
            for (int j = 1; j < line->GetNumberOfIds(); j++) {
                m_Indices->AddId(line->GetId(static_cast<IGsize>(j) - 1));
                m_Indices->AddId(line->GetId(j));
            }
        }

        if (m_UseColor) {
            ScalarsToColors::Pointer mapper = ScalarsToColors::New();
            mapper->InitRange(m_ColorValues);
            m_Colors = mapper->MapScalars(m_ColorValues);
            if (m_Colors == nullptr) { return; }
        }

        GLAllocateGLBuffer(m_PositionVBO,
                           m_Positions->GetNumberOfValues() * sizeof(float),
                           m_Positions->RawPointer());

        GLAllocateGLBuffer(m_LineEBO,
                           m_Indices->GetNumberOfIds() * sizeof(igIndex),
                           m_Indices->RawPointer());

        m_LineVAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0,
                               3 * sizeof(float));
        GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                          GL_FLOAT, GL_FALSE, 0);

        if (m_UseColor) {
            m_LineVAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0,
                                   3 * sizeof(float));
            GLSetVertexAttrib(m_LineVAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                              GL_FLOAT, GL_FALSE, 0);
        }
        m_LineVAO.elementBuffer(m_LineEBO);


        if (m_UseColor) {
            GLAllocateGLBuffer(m_ColorVBO,
                               m_Colors->GetNumberOfValues() * sizeof(float),
                               m_Colors->RawPointer());
        }
    }
    bool IsDrawable() override { return true; }

private:
    GLVertexArray m_LineVAO;
    GLBuffer m_PositionVBO, m_ColorVBO;
    GLBuffer m_LineEBO;

    FloatArray::Pointer m_Positions{};
    FloatArray::Pointer m_Colors{};
    IdArray::Pointer m_Indices{};
    bool m_UseColor{false};
    bool m_Flag{false};
};
IGAME_NAMESPACE_END
#endif