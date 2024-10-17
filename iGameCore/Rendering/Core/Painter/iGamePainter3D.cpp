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
    auto& indexes = primitive.indices;

    auto color = m_Pen->GetColor();
    points.insert(points.end(), {point});
    colors.insert(colors.end(), {color});

    std::vector<int> index = {0};
    indexes[0].insert(indexes[0].end(), index.begin(), index.end());

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawLine(const Point& p1, const Point& p2) {
    if (!ColorUtils::IsValid(m_Pen->GetColor())) { return 0; }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indexes = primitive.indices;

    auto color = m_Pen->GetColor();
    points.insert(points.end(), {p1, p2});
    colors.insert(colors.end(), {color, color});

    std::vector<int> index = {0, 1};
    indexes[1].insert(indexes[1].end(), index.begin(), index.end());

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
    auto& indexes = primitive.indices;


    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();
        points.insert(points.end(), {p1, p2, p3});
        colors.insert(colors.end(), {color, color, color});

        std::vector<int> index = {0, 1, 1, 2, 2, 0};
        indexes[1].insert(indexes[1].end(), index.begin(), index.end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();

        auto color = m_Brush->GetColor();
        points.insert(points.end(), {p1, p2, p3});
        colors.insert(colors.end(), {color, color, color});

        std::vector<int> index = {0, 1, 2};
        std::for_each(index.begin(), index.end(),
                      [offset](int& value) { value += offset; });
        indexes[2].insert(indexes[2].end(), index.begin(), index.end());
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
    auto& indexes = primitive.indices;

    auto p2 = Point{p1[0], p3[1], p1[2]};
    auto p4 = Point{p3[0], p1[1], p3[2]};

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();
        points.insert(points.end(), {p1, p2, p3, p4});
        colors.insert(colors.end(), {color, color, color, color});

        std::vector<int> index = {0, 1, 1, 2, 2, 3, 3, 0};
        indexes[1].insert(indexes[1].end(), index.begin(), index.end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();

        auto color = m_Brush->GetColor();
        points.insert(points.end(), {p1, p2, p3, p4});
        colors.insert(colors.end(), {color, color, color, color});

        std::vector<int> index = {0, 1, 2, 2, 3, 0};
        std::for_each(index.begin(), index.end(),
                      [offset](int& value) { value += offset; });
        indexes[2].insert(indexes[2].end(), index.begin(), index.end());
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
    auto& indexes = primitive.indices;

    auto p2 = Point{p1[0], p1[1], p7[2]};
    auto p3 = Point{p7[0], p1[1], p7[2]};
    auto p4 = Point{p7[0], p1[1], p1[2]};
    auto p5 = Point{p1[0], p7[1], p1[2]};
    auto p6 = Point{p1[0], p7[1], p7[2]};
    auto p8 = Point{p7[0], p7[1], p1[2]};

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();
        points.insert(points.end(), {p1, p2, p3, p4, p5, p6, p7, p8});
        colors.insert(colors.end(),
                      {color, color, color, color, color, color, color, color});

        std::vector<int> index = {0, 1, 1, 2, 2, 3, 3, 0, 0, 4, 1, 5,
                                  2, 6, 3, 7, 4, 5, 5, 6, 6, 7, 7, 4};
        indexes[1].insert(indexes[1].end(), index.begin(), index.end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();

        auto color = m_Brush->GetColor();
        points.insert(points.end(), {p1, p2, p3, p4, p5, p6, p7, p8});
        colors.insert(colors.end(),
                      {color, color, color, color, color, color, color, color});

        std::vector<int> index = {0, 1, 2, 2, 3, 0, 0, 1, 5, 5, 4, 0,
                                  0, 3, 7, 7, 4, 0, 3, 7, 6, 6, 2, 3,
                                  1, 2, 6, 6, 5, 1, 4, 5, 6, 6, 7, 4};
        std::for_each(index.begin(), index.end(),
                      [offset](int& value) { value += offset; });
        indexes[2].insert(indexes[2].end(), index.begin(), index.end());
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawCircle(const Point& center, const Vector3f& normal,
                             double radius, int num) 
{
    if (!ColorUtils::IsValid(m_Pen->GetColor()) &&
        !ColorUtils::IsValid(m_Brush->GetColor())) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indexes = primitive.indices;

    normal.normalized();

    Vector3f up(0, 0, 1); 
    if (std::abs(normal.dot(up)) > 0.999f) {
        up = Vector3f(0, 1, 0);
    }

    Vector3f u = (normal.cross(up)).normalized();
    Vector3f v = (normal.cross(u)).normalized();

    std::vector<Point> ps;
    for (int i = 0; i < num; ++i) {
        double angle = 2.0 * M_PI * i / num;
        float x_offset = radius * std::cos(angle);
        float y_offset = radius * std::sin(angle);

        Point p = center + u * x_offset + v * y_offset;
        ps.push_back(p);
    }

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();

        points.insert(points.end(), ps.begin(), ps.end());
        points.insert(points.end(), ps[0]);

        for (int i = 0; i <= num; i++) colors.push_back(color);
        for (int i = 0; i < num; i++) { 
            indexes[1].push_back(i);
            indexes[1].push_back(i + 1);
        }
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();

        auto color = m_Brush->GetColor();
        points.insert(points.end(), center);
        points.insert(points.end(), ps.begin(), ps.end());
        points.insert(points.end(), ps[0]);
        
        for (int i = 0; i <= num + 1; i++) colors.push_back(color);

        for (int i = 0; i < num; i++) {
            indexes[2].push_back(offset);
            indexes[2].push_back(offset + i + 1);
            indexes[2].push_back(offset + i + 2);
        }
    }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawSphere(const Point& center, double radius, int num) {
    // TODO:
    return IGuint();
}
IGAME_NAMESPACE_END