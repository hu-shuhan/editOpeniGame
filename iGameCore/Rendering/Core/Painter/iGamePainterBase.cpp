//
// Created by Sumzeek on 9/12/2024.
//

#include "iGameScene.h"
#include <map>
IGAME_NAMESPACE_BEGIN

PainterBase::PainterBase() {
    m_Pen = Pen::New();
    m_Brush = Brush::New();

    m_BoundingHelper = Object::New();

    m_PrimitivesUpdateHelper = Object::New();
    m_PrimitivesPool = HandlePool<Primitive>::New();
}

PainterBase::~PainterBase() {}

void PainterBase::ShowAll() {
    for (auto it = m_PrimitivesPool->Begin(); it != m_PrimitivesPool->End();
         ++it) {
        auto& primitive = it->second;
        primitive.visible = true;
    }
    m_PrimitivesPool->Modified();

    this->Modified();
}
void PainterBase::HideAll() {
    for (auto it = m_PrimitivesPool->Begin(); it != m_PrimitivesPool->End();
         ++it) {
        auto& primitive = it->second;
        primitive.visible = false;
    }
    m_PrimitivesPool->Modified();

    this->Modified();
}
void PainterBase::SetTotallyHide(bool totallyHide) {
    m_TotallyHide = totallyHide;
}
void PainterBase::Show(IGuint handle) {
    if (!m_PrimitivesPool->CheckHandle(handle)) {
        IGAME_RENDERING_INFO("handle is invalid.");
        return;
    }

    auto primitive = m_PrimitivesPool->GetObjectByHandle(handle);

    if (primitive->visible) { return; }

    primitive->visible = true;
    m_PrimitivesPool->Modified();

    this->Modified();
}
void PainterBase::Hide(IGuint handle) {
    if (!m_PrimitivesPool->CheckHandle(handle)) {
        IGAME_RENDERING_INFO("handle is invalid.");
        return;
    }

    auto primitive = m_PrimitivesPool->GetObjectByHandle(handle);

    if (!primitive->visible) { return; }

    primitive->visible = false;
    m_PrimitivesPool->Modified();

    this->Modified();
}

void PainterBase::Delete(IGuint handle) {
    if (!m_PrimitivesPool->CheckHandle(handle)) {
        IGAME_RENDERING_INFO("handle is invalid.");
        return;
    }

    m_PrimitivesPool->ReleaseHandle(handle);
    this->Modified();
}

void PainterBase::SetPen(const SmartPointer<Pen>& pen) {
    if (pen == m_Pen) { return; }

    m_Pen = pen;
    this->Modified();
}

void PainterBase::SetPen(const Color& color) {
    auto c = ColorUtils::Map(color);
    auto penColor = m_Pen->GetColor();

    if (c.x == penColor[0] && c.y == penColor[1] && c.z == penColor[2]) {
        return;
    }

    m_Pen->SetColor(color);
    this->Modified();
}

void PainterBase::SetPen(int red, int green, int blue) {
    float r = static_cast<float>(red) / 255.0f;
    float g = static_cast<float>(green) / 255.0f;
    float b = static_cast<float>(blue) / 255.0f;
    auto penColor = m_Pen->GetColor();

    if (r == penColor[0] && g == penColor[1] && b == penColor[2]) { return; }

    m_Pen->SetColor(red, green, blue);
    this->Modified();
}

void PainterBase::SetPen(float red, float green, float blue) {
    auto penColor = m_Pen->GetColor();

    if (red == penColor[0] && green == penColor[1] && blue == penColor[2]) {
        return;
    }

    m_Pen->SetColor(red, green, blue);
    this->Modified();
}

void PainterBase::SetPen(const Pen::Style& style) {
    if (style == m_Pen->GetStyle()) { return; }

    m_Pen->SetStyle(style);
    this->Modified();
}

void PainterBase::SetPen(float width) {
    if (width == m_Pen->GetWidth()) { return; }

    m_Pen->SetWidth(width);
    this->Modified();
}

void PainterBase::SetBrush(const SmartPointer<Brush>& brush) {
    m_Brush = brush;
}

void PainterBase::SetBrush(const Color& color) {
    auto c = ColorUtils::Map(color);
    auto brushColor = m_Brush->GetColor();

    if (c.x == brushColor[0] && c.y == brushColor[1] && c.z == brushColor[2]) {
        return;
    }

    m_Brush->SetColor(color);
    this->Modified();
}

void PainterBase::SetBrush(int red, int green, int blue) {
    float r = static_cast<float>(red) / 255.0f;
    float g = static_cast<float>(green) / 255.0f;
    float b = static_cast<float>(blue) / 255.0f;
    auto brushColor = m_Brush->GetColor();

    if (r == brushColor[0] && g == brushColor[1] && b == brushColor[2]) {
        return;
    }

    m_Brush->SetColor(red, green, blue);
    this->Modified();
}

void PainterBase::SetBrush(float red, float green, float blue) {
    auto brushColor = m_Brush->GetColor();

    if (red == brushColor[0] && green == brushColor[1] &&
        blue == brushColor[2]) {
        return;
    }

    m_Brush->SetColor(red, green, blue);
    this->Modified();
}

void PainterBase::SetBrush(const Brush::Style& style) {
    if (style == m_Brush->GetStyle()) { return; }

    m_Brush->SetStyle(style);
    this->Modified();
}

const BoundingBox& PainterBase::GetBoundingBox() {
    ComputeBoundingBox();
    return m_Bounding;
}

void PainterBase::SetScene(SmartPointer<Scene> scene) { m_Scene = scene; }

SmartPointer<Scene> PainterBase::GetScene() const { return m_Scene; }

void PainterBase::Draw() {
    if (m_TotallyHide) return;
    this->PackDrawableData();

    for (const auto& pair: m_VAOs) {
        float penWidth = pair.first;

        m_Scene->GetShader(ShaderType::NOLIGHT)->Use();

        // draw points & lines
        glad_glDepthRange(0.000001, 1);
        {
            glad_glPointSize(penWidth);
            m_VAOs[penWidth]->ElementBuffer(m_PointEBOs[penWidth]);
            m_VAOs[penWidth]->DrawElements(GL_POINTS, m_PointEBOSizes[penWidth],
                                           GL_UNSIGNED_INT);

            glad_glLineWidth(penWidth);
            m_VAOs[penWidth]->ElementBuffer(m_LineEBOs[penWidth]);
            m_VAOs[penWidth]->DrawElements(GL_LINES, m_LineEBOSizes[penWidth],
                                           GL_UNSIGNED_INT);
        }
        glad_glDepthRange(0, 1);

        // draw triangle
        m_VAOs[penWidth]->ElementBuffer(m_TriangleEBOs[penWidth]);
        m_VAOs[penWidth]->DrawElements(
                GL_TRIANGLES, m_TriangleEBOSizes[penWidth], GL_UNSIGNED_INT);
    }
}

void PainterBase::PackDrawableData() {
    if (m_PrimitivesPool->GetMTime() < m_PrimitivesUpdateHelper->GetMTime()) {
        return;
    }

    m_PrimitivesUpdateHelper->Modified();

    std::map<float, SmartPointer<FloatArray>> packPositions;
    std::map<float, SmartPointer<FloatArray>> packColors;
    //std::map<float, FloatArray>> packNormals;
    std::map<float, SmartPointer<UnsignedIntArray>> packPointIndices;
    std::map<float, SmartPointer<UnsignedIntArray>> packLineIndices;
    std::map<float, SmartPointer<UnsignedIntArray>>
            packTriangleIndices;

    // create buffer array
    for (auto it = m_PrimitivesPool->Begin(); it != m_PrimitivesPool->End();
         ++it) {
        auto& primitive = it->second;

        if (!primitive.visible) { continue; }

        float penWidth = primitive.penWidth;
        if (packPositions.find(penWidth) == packPositions.end()) {
            packPositions[penWidth] = FloatArray::New();
            packColors[penWidth] = FloatArray::New();
            //packNormals[penWidth] = FloatArray::New();
            packPointIndices[penWidth] = UnsignedIntArray::New();
            packLineIndices[penWidth] = UnsignedIntArray::New();
            packTriangleIndices[penWidth] = UnsignedIntArray::New();

            packPositions[penWidth]->SetDimension(3);
            packColors[penWidth]->SetDimension(3);
            //packNormals[penWidth]->SetDimension(3);
            packPointIndices[penWidth]->SetDimension(1);
            packLineIndices[penWidth]->SetDimension(1);
            packTriangleIndices[penWidth]->SetDimension(1);
        }
    }

    // pack data
    for (auto it = m_PrimitivesPool->Begin(); it != m_PrimitivesPool->End();
         ++it) {
        auto& primitive = it->second;

        if (!primitive.visible) { continue; }

        float penWidth = primitive.penWidth;
        unsigned int offset = packPositions[penWidth]->GetNumberOfElements();

        for (auto& point: primitive.points) {
            packPositions[penWidth]->AddElement3(point[0], point[1], point[2]);
        }
        for (auto& color: primitive.colors) {
            packColors[penWidth]->AddElement3(color[0], color[1], color[2]);
        }
        //for (auto normal: primitive.normals) {
        //    packNormals[penWidth]->AddElement3(normal[0], normal[1], normal[2]);
        //}
        for (auto index: primitive.indices[0]) {
            packPointIndices[penWidth]->AddValue(index + offset);
        }
        for (auto index: primitive.indices[1]) {
            packLineIndices[penWidth]->AddValue(index + offset);
        }
        for (auto index: primitive.indices[2]) {
            packTriangleIndices[penWidth]->AddValue(index + offset);
        }
    }

    // re-allocate buffer
    m_VAOs.clear();
    for (const auto& pair: packPositions) {
        float penWidth = pair.first;
        if (m_VAOs.find(penWidth) == m_VAOs.end()) {
            this->CreateDrawBuffer(penWidth);
        }

        IGsize size = 0;

        size = packPositions[penWidth]->GetNumberOfValues();
        GLAllocateGLBuffer(m_PositionVBOs[penWidth], size * sizeof(float),
                           packPositions[penWidth]->RawPointer());

        size = packColors[penWidth]->GetNumberOfValues();
        GLAllocateGLBuffer(m_ColorVBOs[penWidth], size * sizeof(float),
                           packColors[penWidth]->RawPointer());

        //size = packNormals[penWidth]->GetNumberOfValues();
        //GLAllocateGLBuffer(m_NormalVBOs[penWidth], size * sizeof(float),
        //                   packNormals[penWidth]->RawPointer());

        size = packPointIndices[penWidth]->GetNumberOfValues();
        GLAllocateGLBuffer(m_PointEBOs[penWidth], size * sizeof(iguIndex),
                           packPointIndices[penWidth]->RawPointer());
        m_PointEBOSizes[penWidth] = size;

        size = packLineIndices[penWidth]->GetNumberOfValues();
        GLAllocateGLBuffer(m_LineEBOs[penWidth], size * sizeof(iguIndex),
                           packLineIndices[penWidth]->RawPointer());
        m_LineEBOSizes[penWidth] = size;

        size = packTriangleIndices[penWidth]->GetNumberOfValues();
        GLAllocateGLBuffer(m_TriangleEBOs[penWidth], size * sizeof(iguIndex),
                           packTriangleIndices[penWidth]->RawPointer());
        m_TriangleEBOSizes[penWidth] = size;
    }

    this->Modified();
}

void PainterBase::Clear() {
    m_PrimitivesPool->Clear();
    this->Modified();
}

void PainterBase::ComputeBoundingBox() {
    if (m_BoundingHelper->GetMTime() < m_PrimitivesPool->GetMTime()) {
        m_Bounding.reset();
        for (auto it = m_PrimitivesPool->Begin(); it != m_PrimitivesPool->End();
             ++it) {
            auto& primitive = it->second;
            if (primitive.visible) { m_Bounding.add(primitive.bounding); }
        }
        m_BoundingHelper->Modified();
        this->Modified();
    }
}

void PainterBase::CreateDrawBuffer(float penWidth) {
    m_VAOs[penWidth] = GLVertexArray::New();
    m_VAOs[penWidth]->Create();

    m_PositionVBOs[penWidth] = GLBuffer::New();
    m_PositionVBOs[penWidth]->Create();
    m_PositionVBOs[penWidth]->Target(GL_ARRAY_BUFFER);

    m_ColorVBOs[penWidth] = GLBuffer::New();
    m_ColorVBOs[penWidth]->Create();
    m_ColorVBOs[penWidth]->Target(GL_ARRAY_BUFFER);

    //m_NormalVBOs[penWidth] = GLBuffer::New();
    //m_NormalVBOs[penWidth]->Create();
    //m_NormalVBOs[penWidth]->Target(GL_ARRAY_BUFFER);

    m_PointEBOs[penWidth] = GLBuffer::New();
    m_PointEBOs[penWidth]->Create();
    m_PointEBOs[penWidth]->Target(GL_ELEMENT_ARRAY_BUFFER);

    m_LineEBOs[penWidth] = GLBuffer::New();
    m_LineEBOs[penWidth]->Create();
    m_LineEBOs[penWidth]->Target(GL_ELEMENT_ARRAY_BUFFER);

    m_TriangleEBOs[penWidth] = GLBuffer::New();
    m_TriangleEBOs[penWidth]->Create();
    m_TriangleEBOs[penWidth]->Target(GL_ELEMENT_ARRAY_BUFFER);

    m_VAOs[penWidth]->VertexBuffer(GL_VBO_IDX_0, m_PositionVBOs[penWidth], 0,
                                   3 * sizeof(float));
    m_VAOs[penWidth]->VertexBuffer(GL_VBO_IDX_1, m_ColorVBOs[penWidth], 0,
                                   3 * sizeof(float));
    //m_VAOs[penWidth]->VertexBuffer(GL_VBO_IDX_2, m_NormalVBOs[penWidth], 0,
    //                               3 * sizeof(float));

    GLSetVertexAttrib(m_VAOs[penWidth], GL_LOCATION_IDX_0, GL_VBO_IDX_0, 3,
                      GL_FLOAT, GL_FALSE, 0);
    GLSetVertexAttrib(m_VAOs[penWidth], GL_LOCATION_IDX_1, GL_VBO_IDX_1, 3,
                      GL_FLOAT, GL_FALSE, 0);
    //GLSetVertexAttrib(m_VAOs[penWidth], GL_LOCATION_IDX_2, GL_VBO_IDX_2, 3,
    //                  GL_FLOAT, GL_FALSE, 0);

    this->Modified();
}

//void Painter::ExpandVBO(GLBuffer& vbo, size_t oldSize, size_t newSize) {
//    GLBuffer tmp;
//    tmp->Create();
//    tmp->Target(GL_ARRAY_BUFFER);
//    tmp.storage(newSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
//    GLBuffer::copySubData(vbo, tmp, 0, 0, oldSize);
//
//    vbo.destroy();
//    vbo = std::move(tmp);
//}
//
//void Painter::ExpandEBO(GLBuffer& ebo, size_t oldSize, size_t newSize) {
//    GLBuffer tmp;
//    tmp->Create();
//    tmp->Target(GL_ELEMENT_ARRAY_BUFFER);
//    tmp.storage(newSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
//    GLBuffer::copySubData(ebo, tmp, 0, 0, oldSize);
//
//    ebo.destroy();
//    ebo = std::move(tmp);
//}

IGAME_NAMESPACE_END