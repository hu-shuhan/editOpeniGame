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

    virtual void Draw(Scene*);
    void PackDrawableData();
    void Clear();

protected:
    struct Primitive {
        float penWidth;
        std::vector<Vector3f> points;
        std::vector<Vector3f> colors;
        //std::vector<Vector3f> normals;
        std::array<std::vector<iguIndex>, 3> indices;
        bool visible = true;
    };

protected:
    PainterBase();
    ~PainterBase() override;

    Pen::Pointer m_Pen{};
    Brush::Pointer m_Brush{};

    Object::Pointer m_PrimitivesUpdateHelper{};
    HandlePool<Primitive>::Pointer m_PrimitivesPool{};

    std::unordered_map<float, GLVertexArray::Pointer> m_VAOs;
    std::unordered_map<float, GLBuffer::Pointer> m_PositionVBOs;
    std::unordered_map<float, GLBuffer::Pointer> m_ColorVBOs;
    //std::unordered_map<float, GLBuffer::Pointer> m_NormalVBOs;
    std::unordered_map<float, GLBuffer::Pointer> m_PointEBOs;
    std::unordered_map<float, GLBuffer::Pointer> m_LineEBOs;
    std::unordered_map<float, GLBuffer::Pointer> m_TriangleEBOs;

    std::unordered_map<float, IGsize> m_PointEBOSizes;
    std::unordered_map<float, IGsize> m_LineEBOSizes;
    std::unordered_map<float, IGsize> m_TriangleEBOSizes;

private:
    void CreateDrawBuffer(float penWidth);
};

IGAME_NAMESPACE_END