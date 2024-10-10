//
// Created by Sumzeek on 9/12/2024.
//

#include "iGamePainter3D.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

PainterBase::PainterBase() {
    m_Pen = Pen::New();
    m_Brush = Brush::New();

    m_PrimitivesPool = HandlePool<Primitive>::New();

    Clear();
}

PainterBase::~PainterBase() {}

void PainterBase::ShowAll() {
    for (auto& [handle, primitive]: *m_PrimitivesPool) {
        primitive.visible = true;
    }
}
void PainterBase::HideAll() {
    for (auto& [handle, primitive]: *m_PrimitivesPool) {
        primitive.visible = false;
    }
}
void PainterBase::Show(IGuint handle) {
    m_PrimitivesPool->CheckHandle(handle);

    auto primitive = m_PrimitivesPool->GetObject(handle);
    primitive.visible = true;
}
void PainterBase::Hide(IGuint handle) {
    m_PrimitivesPool->CheckHandle(handle);

    auto& primitive = m_PrimitivesPool->GetObject(handle);
    primitive.visible = false;
}

void PainterBase::Delete(IGuint handle) {
    m_PrimitivesPool->CheckHandle(handle);
    m_PrimitivesPool->ReleaseHandle(handle);
}

void PainterBase::SetPen(const Pen::Pointer& pen) { m_Pen = pen; }
void PainterBase::SetPen(const Color& color) { m_Pen->SetColor(color); }
void PainterBase::SetPen(int red, int green, int blue) {
    m_Pen->SetColor(red, green, blue);
}
void PainterBase::SetPen(float red, float green, float blue) {
    m_Pen->SetColor(red, green, blue);
}
void PainterBase::SetPen(const PenStyle& style) { m_Pen->SetStyle(style); }
void PainterBase::SetPen(float width) { m_Pen->SetWidth(width); }

void PainterBase::SetBrush(const Color& color) { m_Brush->SetColor(color); }
void PainterBase::SetBrush(const Brush::Pointer& brush) { m_Brush = brush; }
void PainterBase::SetBrush(int red, int green, int blue) {
    m_Brush->SetColor(red, green, blue);
}
void PainterBase::SetBrush(float red, float green, float blue) {
    m_Brush->SetColor(red, green, blue);
}
void PainterBase::SetBrush(const BrushStyle& style) {
    m_Brush->SetStyle(style);
}

void PainterBase::Draw(Scene* scene) {
    if (first) {
        m_VAO.create();

        m_PositionVBO.create();
        m_PositionVBO.target(GL_ARRAY_BUFFER);
        m_ColorVBO.create();
        m_ColorVBO.target(GL_ARRAY_BUFFER);
        m_PointEBO.create();
        m_PointEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_LineEBO.create();
        m_LineEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_TriangleEBO.create();
        m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);

        m_VAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
        m_VAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));

        GLSetVertexAttrib(m_VAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                          GL_FALSE, 0);
        GLSetVertexAttrib(m_VAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
                          GL_FALSE, 0);

        first = false;
    }

    for (auto& [handle, primitive]: *m_PrimitivesPool) {
        if (!primitive.visible) { continue; }

        auto& points = primitive.points;
        auto& colors = primitive.colors;
        auto& indexes = primitive.indices;

        m_PositionVBO.allocate(points.size() * sizeof(Vector3f), points.data(),
                               GL_STATIC_DRAW);
        m_ColorVBO.allocate(colors.size() * sizeof(Vector3f), colors.data(),
                            GL_STATIC_DRAW);

        scene->UBO().useColor = true;
        scene->UpdateUniformBuffer();
        scene->GetShader(Scene::NOLIGHT)->use();

        m_VAO.bind();

        if (indexes[0].size() != 0) {
            m_PointEBO.allocate(indexes[0].size() * sizeof(iguIndex),
                                indexes[0].data(), GL_STATIC_DRAW);
            m_VAO.elementBuffer(m_PointEBO);

            m_VAO.bind();
            glad_glPointSize(primitive.penWidth);
            glad_glDepthRange(0.000001, 1);
            glad_glDrawElements(GL_POINTS, indexes[0].size(), GL_UNSIGNED_INT,
                                0);
            glad_glDepthRange(0, 1);
        }
        if (indexes[1].size() != 0) {
            m_LineEBO.allocate(indexes[1].size() * sizeof(iguIndex),
                               indexes[1].data(), GL_STATIC_DRAW);
            m_VAO.elementBuffer(m_LineEBO);

            m_VAO.bind();
            GLCheckError();
            glad_glLineWidth(primitive.penWidth);
            GLCheckError();
            glad_glDrawElements(GL_LINES, indexes[1].size(), GL_UNSIGNED_INT,
                                0);
        }
        if (indexes[2].size() != 0) {
            m_TriangleEBO.allocate(indexes[2].size() * sizeof(iguIndex),
                                   indexes[2].data(), GL_STATIC_DRAW);
            m_VAO.elementBuffer(m_TriangleEBO);

            m_VAO.bind();
            glad_glDrawElements(GL_TRIANGLES, indexes[2].size(),
                                GL_UNSIGNED_INT, 0);
        }

        m_VAO.release();
    }
}

void PainterBase::PackDrawableData() {
    if (first) {
        m_VAO.create();

        m_PositionVBO.create();
        m_PositionVBO.target(GL_ARRAY_BUFFER);
        m_ColorVBO.create();
        m_ColorVBO.target(GL_ARRAY_BUFFER);
        m_PointEBO.create();
        m_PointEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_LineEBO.create();
        m_LineEBO.target(GL_ELEMENT_ARRAY_BUFFER);
        m_TriangleEBO.create();
        m_TriangleEBO.target(GL_ELEMENT_ARRAY_BUFFER);

        m_VAO.vertexBuffer(GL_VBO_IDX_0, m_PositionVBO, 0, 3 * sizeof(float));
        m_VAO.vertexBuffer(GL_VBO_IDX_1, m_ColorVBO, 0, 3 * sizeof(float));

        GLSetVertexAttrib(m_VAO, GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3, GL_FLOAT,
                          GL_FALSE, 0);
        GLSetVertexAttrib(m_VAO, GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3, GL_FLOAT,
                          GL_FALSE, 0);

        first = false;
    }

    FloatArray::Pointer packPositions = FloatArray::New();
    FloatArray::Pointer packColors = FloatArray::New();
    UnsignedIntArray::Pointer packPointIndices = UnsignedIntArray::New();
    UnsignedIntArray::Pointer packLineIndices = UnsignedIntArray::New();
    UnsignedIntArray::Pointer packTriangleIndices = UnsignedIntArray::New();

    packPositions->SetDimension(3);
    packColors->SetDimension(3);
    packPointIndices->SetDimension(1);
    packLineIndices->SetDimension(1);
    packTriangleIndices->SetDimension(1);

    for (auto& [handle, primitive]: *m_PrimitivesPool) {
        if (!primitive.visible) { continue; }

        unsigned int offset = packPositions->GetNumberOfElements();

        for (auto point: primitive.points) {
            packPositions->AddElement3(point[0], point[1], point[2]);
        }
        for (auto color: primitive.colors) {
            packColors->AddElement3(color[0], color[1], color[2]);
        }
        for (auto index: primitive.indices[0]) {
            packPointIndices->AddValue(index + offset);
        }
        for (auto index: primitive.indices[1]) {
            packLineIndices->AddValue(index + offset);
        }
        for (auto index: primitive.indices[2]) {
            packTriangleIndices->AddValue(index + offset);
        }
    }

    GLAllocateGLBuffer(m_PositionVBO,
                       packPositions->GetNumberOfValues() * sizeof(float),
                       packPositions->RawPointer());
    GLAllocateGLBuffer(m_ColorVBO,
                       packColors->GetNumberOfValues() * sizeof(float),
                       packColors->RawPointer());
    GLAllocateGLBuffer(m_PointEBO,
                       packPointIndices->GetNumberOfValues() * sizeof(iguIndex),
                       packPointIndices->RawPointer());
    GLAllocateGLBuffer(m_LineEBO,
                       packLineIndices->GetNumberOfValues() * sizeof(iguIndex),
                       packLineIndices->RawPointer());
    GLAllocateGLBuffer(m_TriangleEBO,
                       packTriangleIndices->GetNumberOfValues() *
                               sizeof(iguIndex),
                       packTriangleIndices->RawPointer());
}

void PainterBase::Clear() { m_PrimitivesPool->Clear(); }

//void Painter::ExpandVBO(GLBuffer& vbo, size_t oldSize, size_t newSize) {
//    GLBuffer tmp;
//    tmp.create();
//    tmp.target(GL_ARRAY_BUFFER);
//    tmp.storage(newSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
//    GLBuffer::copySubData(vbo, tmp, 0, 0, oldSize);
//
//    vbo.destroy();
//    vbo = std::move(tmp);
//}
//
//void Painter::ExpandEBO(GLBuffer& ebo, size_t oldSize, size_t newSize) {
//    GLBuffer tmp;
//    tmp.create();
//    tmp.target(GL_ELEMENT_ARRAY_BUFFER);
//    tmp.storage(newSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
//    GLBuffer::copySubData(ebo, tmp, 0, 0, oldSize);
//
//    ebo.destroy();
//    ebo = std::move(tmp);
//}

IGAME_NAMESPACE_END