//
// Created by Sumzeek on 10/9/2024.
//

#include "iGamePainter3D.h"
#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

void Painter3D::Draw(Scene* scene) {
    scene->ObjectData().model = scene->ModelMatrix();
    scene->CameraData().view = scene->GetCamera()->GetViewMatrix();
    scene->CameraData().proj = scene->GetCamera()->GetProjectionMatrix();
    PainterBase::Draw(scene);
}

IGuint Painter3D::DrawPoint(const Point& point) {
    if (!ColorUtils::IsValid(m_Pen->GetColor())) { return 0; }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    auto color = m_Pen->GetColor();

    auto source = DataSource3D::RequestPoint(point);
    points.insert(points.end(), source.points.begin(), source.points.end());
    colors.insert(colors.end(), source.points.size(), color);
    indices[0].insert(indices[0].end(), source.indices[0].begin(),
                      source.indices[0].end());

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

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

    auto source = DataSource3D::RequestLine(p1, p2);
    points.insert(points.end(), source.points.begin(), source.points.end());
    colors.insert(colors.end(), source.points.size(), color);
    indices[1].insert(indices[1].end(), source.indices[1].begin(),
                      source.indices[1].end());

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

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

        auto source = DataSource3D::RequestTriangle(p1, p2, p3);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource3D::RequestTriangle(p1, p2, p3, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

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

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();

        auto source = DataSource3D::RequestRect(p1, p3);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource3D::RequestRect(p1, p3, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

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

        auto source = DataSource3D::RequestCube(p1, p7);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource3D::RequestCube(p1, p7, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawCircle(const Point& center, const Vector3f& normal,
                             float radius, int resolution) {
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
                DataSource3D::RequestCircle(center, normal, radius, resolution);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = DataSource3D::RequestCircle(center, normal, radius,
                                                  resolution, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawSphere(const Point& center, float radius,
                             unsigned int stackCount,
                             unsigned int sectorCount) {
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

        auto source = SphereSource::RequestSphere(center, radius, sectorCount,
                                                  stackCount);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = SphereSource::RequestSphere(center, radius, sectorCount,
                                                  stackCount, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawIcoSphere(const Point& center, float radius,
                                unsigned int subdivision) {
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
                SphereSource::RequestIcoSphere(center, radius, subdivision);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = SphereSource::RequestIcoSphere(center, radius,
                                                     subdivision, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawCubeSphere(const Point& center, float radius,
                                 unsigned int vertexCountPerRow) {
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

        auto source = SphereSource::RequestCubeSphere(center, radius,
                                                      vertexCountPerRow);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = SphereSource::RequestCubeSphere(
                center, radius, vertexCountPerRow, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawCylinder(const Point& center, const Vector3f& normal,
                               float height, float radius,
                               unsigned int resolution) {
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

        auto source = CylinderSource::RequestCylinder(center, normal, height,
                                                      radius, resolution);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = CylinderSource::RequestCylinder(
                center, normal, height, radius, resolution, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawCone(const Point& center, const Vector3f& normal,
                           float height, float radius,
                           unsigned int resolution) {
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

        auto source = ConeSource::RequestCone(center, normal, height, radius,
                                              resolution);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source = ConeSource::RequestCone(center, normal, height, radius,
                                              resolution, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawPyramid(const Point& center, const Vector3f& normal,
                              float height, float radius,
                              unsigned int stackCount,
                              unsigned int sectorCount) {
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

        auto source = ConeSource::RequestPyramid(center, normal, height, radius,
                                                 stackCount, sectorCount);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source =
                ConeSource::RequestPyramid(center, normal, height, radius,
                                           stackCount, sectorCount, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGuint Painter3D::DrawFrustum(const Point& center, const Vector3f& normal,
                              float height, float baseRadius, float topRadius,
                              unsigned int resolution) {
    if (!ColorUtils::IsValid(m_Pen->GetColor()) &&
        !ColorUtils::IsValid(m_Brush->GetColor())) {
        return 0;
    }

    Primitive primitive{};
    primitive.penWidth = m_Pen->GetWidth();

    auto& points = primitive.points;
    auto& colors = primitive.colors;
    auto& indices = primitive.indices;

    auto color = m_Pen->GetColor();

    if (ColorUtils::IsValid(m_Pen->GetColor())) {
        auto color = m_Pen->GetColor();

        auto source = ConeSource::RequestFrustum(
                center, normal, height, baseRadius, topRadius, resolution);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[1].insert(indices[1].end(), source.indices[1].begin(),
                          source.indices[1].end());
    }

    if (ColorUtils::IsValid(m_Brush->GetColor())) {
        int offset = points.size();
        auto color = m_Brush->GetColor();

        auto source =
                ConeSource::RequestFrustum(center, normal, height, baseRadius,
                                           topRadius, resolution, offset);
        points.insert(points.end(), source.points.begin(), source.points.end());
        colors.insert(colors.end(), source.points.size(), color);
        indices[2].insert(indices[2].end(), source.indices[2].begin(),
                          source.indices[2].end());
    }

    primitive.bounding.reset();
    for (const auto& p: points) { primitive.bounding.add(p); }

    auto handle = m_PrimitivesPool->AllocateObject(primitive);
    return handle;
}

IGAME_NAMESPACE_END