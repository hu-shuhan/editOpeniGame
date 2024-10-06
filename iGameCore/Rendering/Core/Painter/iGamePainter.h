//
// Created by Sumzeek on 9/12/2024.
//

#pragma once

#include <unordered_map>

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLVertexArray.h"

#include "iGameBrush.h"
#include "iGameColorUtils.h"
#include "iGameHandlePool.h"
#include "iGameObject.h"
#include "iGamePen.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN
class Scene;

class Painter : public Object {
public:
    I_OBJECT(Painter);
    static Pointer New() { return new Painter; }

public:
    void ShowAll();
    void HideAll();
    void Show(IGuint handle);
    void Hide(IGuint handle);
    void Delete(IGuint handle);

    void SetPen(const Pen::Pointer& pen);
    void SetPen(const Color& color);
    void SetPen(int red, int green, int blue);
    void SetPen(float red, float green, float blue);
    void SetPen(const PenStyle& style);
    void SetPen(float width);

    void SetBrush(const Brush::Pointer& brush);
    void SetBrush(const Color& color);
    void SetBrush(int red, int green, int blue);
    void SetBrush(float red, float green, float blue);
    void SetBrush(const BrushStyle& style);

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

    void Draw(Scene*);
    void Clear();

protected:
    struct Primitive {
        float penWidth;
        std::vector<Vector3f> points;
        std::vector<Vector3f> colors;
        std::array<std::vector<iguIndex>, 3> indexes;
        bool visible = true;
    };

protected:
    Painter();
    ~Painter() override;
    bool first{true};

    Pen::Pointer m_Pen;
    Brush::Pointer m_Brush;

    HandlePool<Primitive> m_PrimitivesPool;

    std::vector<Vector3f> m_Points{};
    std::vector<Vector3f> m_Colors{};
    std::vector<iguIndex> m_PointIndexes{};
    std::vector<iguIndex> m_LineIndexes{};
    std::vector<iguIndex> m_TriangleIndexes{};

    GLVertexArray m_VAO;
    GLBuffer m_PositionVBO, m_ColorVBO;
    GLBuffer m_PointEBO, m_LineEBO, m_TriangleEBO;
};

IGAME_NAMESPACE_END