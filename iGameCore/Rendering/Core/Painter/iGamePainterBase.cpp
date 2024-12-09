//
// Created by Sumzeek on 9/12/2024.
//

#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

PainterBase::PainterBase() {
    m_Pen = Pen::New();
    m_Brush = Brush::New();

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
}
void PainterBase::HideAll() {
    for (auto it = m_PrimitivesPool->Begin(); it != m_PrimitivesPool->End();
         ++it) {
        auto& primitive = it->second;
        primitive.visible = false;
    }
    m_PrimitivesPool->Modified();
}
void PainterBase::Show(IGuint handle) {
    m_PrimitivesPool->CheckHandle(handle);

    auto primitive = m_PrimitivesPool->GetObject(handle);
    if (primitive) {
        primitive->visible = true;
        m_PrimitivesPool->Modified();
    } else {
        igDebug("handle is invalid.");
    }
}
void PainterBase::Hide(IGuint handle) {
    m_PrimitivesPool->CheckHandle(handle);

    auto primitive = m_PrimitivesPool->GetObject(handle);
    if (primitive) {
        primitive->visible = false;
        m_PrimitivesPool->Modified();
    } else {
        igDebug("handle is invalid.");
    }
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

void PainterBase::SetPen(const Pen::Style& style) { m_Pen->SetStyle(style); }

void PainterBase::SetPen(float width) { m_Pen->SetWidth(width); }

void PainterBase::SetBrush(const Color& color) { m_Brush->SetColor(color); }

void PainterBase::SetBrush(const Brush::Pointer& brush) { m_Brush = brush; }

void PainterBase::SetBrush(int red, int green, int blue) {
    m_Brush->SetColor(red, green, blue);
}

void PainterBase::SetBrush(float red, float green, float blue) {
    m_Brush->SetColor(red, green, blue);
}

void PainterBase::SetBrush(const Brush::Style& style) {
    m_Brush->SetStyle(style);
}

void PainterBase::Draw(Scene* scene) {
    this->PackDrawableData();

    for (const auto& pair: m_VAOs) {
        float penWidth = pair.first;

        scene->UBO().useColor = true;
        scene->UpdateUniformBuffer();
        scene->GetShader(Scene::NOLIGHT)->Use();

        m_VAOs[penWidth]->Bind();
        {
            // draw point
            m_VAOs[penWidth]->ElementBuffer(m_PointEBOs[penWidth]);
            glad_glPointSize(penWidth);
            glad_glDepthRange(0.000001, 1);
            glad_glDrawElements(GL_POINTS, m_PointEBOSizes[penWidth],
                                GL_UNSIGNED_INT, 0);
            glad_glDepthRange(0, 1);

            // draw line
            m_VAOs[penWidth]->ElementBuffer(m_LineEBOs[penWidth]);
            glad_glLineWidth(penWidth);
            glad_glDepthRange(0.000001, 1);
            glad_glDrawElements(GL_LINES, m_LineEBOSizes[penWidth],
                                GL_UNSIGNED_INT, 0);
            glad_glDepthRange(0, 1);

            // draw triangle
            m_VAOs[penWidth]->ElementBuffer(m_TriangleEBOs[penWidth]);
            glad_glDrawElements(GL_TRIANGLES, m_TriangleEBOSizes[penWidth],
                                GL_UNSIGNED_INT, 0);
        }
        m_VAOs[penWidth]->Release();
    }
}

void PainterBase::PackDrawableData() {
    if (m_PrimitivesPool->GetMTime() < m_PrimitivesUpdateHelper->GetMTime()) {
        return;
    }
    m_PrimitivesUpdateHelper->Modified();

    std::unordered_map<float, FloatArray::Pointer> packPositions;
    std::unordered_map<float, FloatArray::Pointer> packColors;
    //std::unordered_map<float, FloatArray::Pointer> packNormals;
    std::unordered_map<float, UnsignedIntArray::Pointer> packPointIndices;
    std::unordered_map<float, UnsignedIntArray::Pointer> packLineIndices;
    std::unordered_map<float, UnsignedIntArray::Pointer> packTriangleIndices;

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

        for (auto point: primitive.points) {
            packPositions[penWidth]->AddElement3(point[0], point[1], point[2]);
        }
        for (auto color: primitive.colors) {
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
}

void PainterBase::Clear() { m_PrimitivesPool->Clear(); }

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