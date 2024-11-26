//
// Created by Sumzeek on 10/9/2024.
//

#pragma once

#include "DataSource/iGameConeSource.h"
#include "DataSource/iGameCylinderSource.h"
#include "DataSource/iGameDataSource3D.h"
#include "DataSource/iGameSphereSource.h"
#include "iGamePainterBase.h"

IGAME_NAMESPACE_BEGIN
class Scene;

/**
 * @class Painter3D
 * @brief A utility class for rendering 3D graphical elements in a scene.
 *
 * This class provides methods to draw various 3D primitives such as points, lines, triangles,
 * rectangles, cubes, and circles. It operates in conjunction with the Scene object and assumes
 * the use of an OpenGL-like rendering backend.
 */
class Painter3D : public PainterBase {
public:
    I_OBJECT(Painter3D);
    static Pointer New() { return new Painter3D; }

    void Draw(Scene*) override;

    IGuint DrawPoint(const Point& point);
    IGuint DrawLine(const Point& p1, const Point& p2);
    IGuint DrawTriangle(const Point& p1, const Point& p2, const Point& p3);
    IGuint DrawRect(const Point& p1, const Point& p3);
    IGuint DrawCube(const Point& p1, const Point& p7);
    IGuint DrawCircle(const Point& center, const Vector3f& normal, float radius,
                      int resolution);
    IGuint DrawSphere(const Point& center, float radius,
                      unsigned int stackCount, unsigned int sectorCount);
    IGuint DrawIcoSphere(const Point& center, float radius,
                         unsigned int subdivision);
    IGuint DrawCubeSphere(const Point& center, float radius,
                          unsigned int vertexCountPerRow);
    IGuint DrawCylinder(const Point& center, const Vector3f& normal,
                        float height, float radius, unsigned int resolution);
    IGuint DrawCone(const Point& center, const Vector3f& normal, float height,
                    float radius, unsigned int resolution);
    IGuint DrawPyramid(const Point& center, const Vector3f& normal,
                       float height, float radius, unsigned int stackCount,
                       unsigned int sectorCount);
    IGuint DrawFrustum(const Point& center, const Vector3f& normal,
                       float height, float baseRadius, float topRadius,
                       unsigned int resolution);

protected:
    Painter3D() = default;
    ~Painter3D() override = default;
};

IGAME_NAMESPACE_END