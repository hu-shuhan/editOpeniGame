//
// Created by Sumzeek on 10/9/2024.
//

#pragma once

#include "DataSource/iGameDataSource2D.h"
#include "iGamePainterBase.h"

IGAME_NAMESPACE_BEGIN
class Scene;
/**
 * @class Painter2D
 * @brief A 2D drawing utility class for rendering graphical elements in a scene.
 *
 * **Coordinate System**:
 * - The origin `(0, 0)` is located at the **bottom-left corner** of the screen.
 * - All coordinates are specified in **pixels**, representing absolute positions on the screen.
 * - Positive x-axis points to the **right**.
 * - Positive y-axis points **upward**.
 * - The valid range for coordinates depends on the screen's resolution.
 *
 * **Coordinate Axis Diagram**:
 *
 *    +Y (0, screenHeight)
 *    ^
 *    |
 *    |
 *    |
 *    +----------> +X (screenWidth, 0)
 *  (0, 0)
 */
class Painter2D : public PainterBase {
public:
    I_OBJECT(Painter2D);
    static Pointer New() { return new Painter2D; }

    void Draw(Scene*) override;

    IGuint DrawPoint(const Vector2ui& point);
    IGuint DrawLine(const Vector2ui& p1, const Vector2ui& p2);
    IGuint DrawTriangle(const Vector2ui& p1, const Vector2ui& p2,
                        const Vector2ui& p3);
    IGuint DrawRect(const Vector2ui& p1, const Vector2ui& p3);
    IGuint DrawCircle(const Vector2ui& center, double radius, int resolution);

protected:
    Painter2D();
    ~Painter2D() override;
};

IGAME_NAMESPACE_END