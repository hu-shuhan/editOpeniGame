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

class PainterBase : public Object {
public:
    I_OBJECT(PainterBase);

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

    void Draw(Scene*);
    void PackDrawableData();
    void Clear();

protected:
    struct Primitive {
        float penWidth;
        std::vector<Vector3f> points;
        std::vector<Vector3f> colors;
        std::array<std::vector<iguIndex>, 3> indices;
        bool visible = true;
    };

protected:
    PainterBase();
    ~PainterBase() override;

    bool first{true};

    Pen::Pointer m_Pen{};
    Brush::Pointer m_Brush{};

    HandlePool<Primitive>::Pointer m_PrimitivesPool{};

    GLVertexArray::Pointer m_VAO;
    GLBuffer::Pointer m_PositionVBO, m_ColorVBO;
    GLBuffer::Pointer m_PointEBO, m_LineEBO, m_TriangleEBO;
};

IGAME_NAMESPACE_END