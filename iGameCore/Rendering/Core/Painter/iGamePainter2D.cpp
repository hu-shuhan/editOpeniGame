//
// Created by Sumzeek on 10/9/2024.
//

#include "iGamePainter2D.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

Painter2D::Painter2D() {}

Painter2D::~Painter2D() {}

void Painter2D::Draw() {
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    auto width = static_cast<float>(vp[2]);
    auto height = static_cast<float>(vp[3]);

    igm::mat4 model = igm::mat4{1.0f};
    igm::mat4 view = igm::mat4{1.0f};
    igm::mat4 proj = igm::orthoRH_OZ(0.0f, width, 0.0f, height, -1.0f, 1.0f);

    m_Scene->m_ShaderManager->UpdateCameraBlock(
            {igm::vec3{0.0f}, 1, view, proj, proj * view});
    m_Scene->m_ShaderManager->UpdateObjectBlock(
            {1.0f, model, model.invert().transpose(), igm::vec4{}});
    m_Scene->m_ShaderManager->UpdateUBOBlock({1, 0});

    glDisable(GL_DEPTH_TEST);
    PainterBase::Draw();
    glEnable(GL_DEPTH_TEST);
}

IGuint Painter2D::DrawPoint(const Vector2ui& point) {
    if (m_Pen->GetStyle() == Pen::Style::NoPen) { return 0; }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    auto color = m_Pen->GetColor();

    auto source = DataSource2D::RequestPoint(point);
    points.insert(points.end(), source.points.begin(), source.points.end());
    colors.insert(colors.end(), source.points.size(), color);
    indices[0].insert(indices[0].end(), source.indices[0].begin(),
                      source.indices[0].end());

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    this->Modified();
    return handle;
}

IGuint Painter2D::DrawLine(const Vector2ui& p1, const Vector2ui& p2) {
    if (m_Pen->GetStyle() == Pen::Style::NoPen) { return 0; }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    auto color = m_Pen->GetColor();

    auto source = DataSource2D::RequestLine(p1, p2);
    points.insert(points.end(), source.points.begin(), source.points.end());
    colors.insert(colors.end(), source.points.size(), color);
    indices[1].insert(indices[1].end(), source.indices[1].begin(),
                      source.indices[1].end());

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    this->Modified();
    return handle;
}

IGuint Painter2D::DrawTriangle(const Vector2ui& p1, const Vector2ui& p2,
                               const Vector2ui& p3) {
    if (m_Pen->GetStyle() == Pen::Style::NoPen &&
        m_Brush->GetStyle() == Brush::Style::NoBrush) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    if (m_Pen->GetStyle() != Pen::Style::NoPen) {
        auto color = m_Pen->GetColor();

        auto source = DataSource2D::RequestTriangle(p1, p2, p3);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (m_Brush->GetStyle() != Brush::Style::NoBrush) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource2D::RequestTriangle(p1, p2, p3, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    this->Modified();
    return handle;
}

IGuint Painter2D::DrawRect(const Vector2ui& p1, const Vector2ui& p3) {
    if (m_Pen->GetStyle() == Pen::Style::NoPen &&
        m_Brush->GetStyle() == Brush::Style::NoBrush) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    if (m_Pen->GetStyle() != Pen::Style::NoPen) {
        auto color = m_Pen->GetColor();

        auto source = DataSource2D::RequestRect(p1, p3);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (m_Brush->GetStyle() != Brush::Style::NoBrush) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource2D::RequestRect(p1, p3, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    this->Modified();
    return handle;
}

IGuint Painter2D::DrawCircle(const Vector2ui& center, double radius,
                             int resolution) {
    if (m_Pen->GetStyle() == Pen::Style::NoPen &&
        m_Brush->GetStyle() == Brush::Style::NoBrush) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    if (m_Pen->GetStyle() != Pen::Style::NoPen) {
        auto color = m_Pen->GetColor();

        auto source = DataSource2D::RequestCircle(center, radius, resolution);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (m_Brush->GetStyle() != Brush::Style::NoBrush) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source =
                DataSource2D::RequestCircle(center, radius, resolution, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    this->Modified();
    return handle;
}

IGAME_NAMESPACE_END