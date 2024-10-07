//
// Created by Sumzeek on 9/13/2024.
//

#pragma once

#include "iGameColorUtils.h"
#include "iGameObject.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN

// Only SolidLine has been implemented
enum class PenStyle {
    NoPen,
    SolidLine,
    //DashLine,
    //DotLine,
    //DashDotLine,
    //DashDotDotLine,
    //CustomDashLine
};

class Pen : public Object {
public:
    I_OBJECT(Pen);
    static Pointer New() { return new Pen; }

    void SetColor(const Color& color);
    void SetColor(int red, int green, int blue);
    void SetColor(float red, float green, float blue);
    Vector3f GetColor() const;

    void SetWidth(float width);
    int GetWidth() const;

    void SetStyle(PenStyle style);
    PenStyle GetStyle() const;

    //void SetOpacity(float opacity);
    //float GetOpacity() const;

protected:
    Pen();
    ~Pen() override;

    Vector3f m_PenColor;
    float m_PenWidth;
    PenStyle m_PenStyle;
    float m_PenOpacity;
};

IGAME_NAMESPACE_END