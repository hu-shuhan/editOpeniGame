#include "iGamePainter3D.h"
//
// Created by Sumzeek on 10/9/2024.
//

#include "iGamePainter3D.h"

IGAME_NAMESPACE_BEGIN

IGuint Painter3D::DrawPoint(const Point& point) {
    if (!ColorUtils::IsValid(m_Pen->GetColor())) { return 0; }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    auto color = m_Pen->GetColor();

    auto source = DataSource::RequestPoint(point);
    points.insert(points.end(), source.points.begin(), source.points.end());
    colors.insert(colors.end(), source.points.size(), color);
    indices[0].insert(indices[0].end(), source.indices[0].begin(),
                      source.indices[0].end());

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawLine(const Point& p1, const Point& p2) {
    if (!ColorUtils::IsValid(m_Pen->GetColor())) { return 0; }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    auto color = m_Pen->GetColor();

    auto source = DataSource::RequestLine(p1, p2);
    points.insert(points.end(), source.points.begin(), source.points.end());
    colors.insert(colors.end(), source.points.size(), color);
    indices[1].insert(indices[1].end(), source.indices[1].begin(),
                      source.indices[1].end());

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawTriangle(const Point& p1, const Point& p2,
                               const Point& p3) {
    if (!ColorUtils::IsValid(m_Pen->GetColor()) &&
        !ColorUtils::IsValid(m_Brush->GetColor())) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();

        auto source = DataSource::RequestTriangle(p1, p2, p3);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource::RequestTriangle(p1, p2, p3, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawRect(const Point& p1, const Point& p3) {
    if (!ColorUtils::IsValid(m_Pen->GetColor()) &&
        !ColorUtils::IsValid(m_Brush->GetColor())) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    auto p2 = Point{p1[0], p3[1], p1[2]};
    auto p4 = Point{p3[0], p1[1], p3[2]};

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();

        auto source = DataSource::RequestRect(p1, p3);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource::RequestRect(p1, p3, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawCube(const Point& p1, const Point& p7) {
    if (!ColorUtils::IsValid(m_Pen->GetColor()) &&
        !ColorUtils::IsValid(m_Brush->GetColor())) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();

        auto source = DataSource::RequestCube(p1, p7);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource::RequestCube(p1, p7, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawCircle(const Point& center, const Vector3f& normal,
                             double radius, int resolution) {
    if (!ColorUtils::IsValid(m_Pen->GetColor()) &&
        !ColorUtils::IsValid(m_Brush->GetColor())) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();

        auto source =
                DataSource::RequestCircle(center, normal, radius, resolution);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource::RequestCircle(center, normal, radius,
                                                resolution, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}
IGAME_NAMESPACE_END