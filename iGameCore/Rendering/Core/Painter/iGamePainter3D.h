//
// Created by Sumzeek on 10/9/2024.
//

#pragma once

#include "iGamePainterBase.h"

IGAME_NAMESPACE_BEGIN
class Scene;

class Painter3D : public PainterBase {
public:
    I_OBJECT(Painter3D);
    static Pointer New() { return new Painter3D; }

public:
    IGuint DrawPoint(const Point& point);
    IGuint DrawLine(const Point& p1, const Point& p2);
    IGuint DrawTriangle(const Point& p1, const Point& p2, const Point& p3);

    /* draw rectangle
    * p2 +------------+ p3
    *    |            |
    *    |            |
    *    |            |
    * p1 +------------+ p4
    */
    IGuint DrawRect(const Point& p1, const Point& p3);

    /* draw cube
    *     p6+-----------+ p7
    *      /|          /|
    *     / |         / |
    *    /  |        /  |
    * p5+-----------+p8 |
    *   |   |       |   |
    *   | p2+-------|---+ p3
    *   |  /        |  /
    *   | /         | /
    *   |/          |/
    * p1+-----------+ p4
    */
    IGuint DrawCube(const Point& p1, const Point& p7);

protected:
    Painter3D() = default;
    ~Painter3D() override = default;
};

IGAME_NAMESPACE_END