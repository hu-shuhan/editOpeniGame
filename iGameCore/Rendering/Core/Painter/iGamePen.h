//
// Created by Sumzeek on 9/13/2024.
//

#pragma once

#include "iGameColorUtils.h"
#include "iGameObject.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN

class Pen : public Object {
public:
    I_OBJECT(Pen);
    static Pointer New() { return new Pen; }

    // Only SolidLine has been implemented
    enum class Style {
        NoPen,
        SolidLine,
        //DashLine,
        //DotLine,
        //DashDotLine,
        //DashDotDotLine,
        //CustomDashLine
    };

    void SetColor(const Color& color);
    void SetColor(int red, int green, int blue);
    void SetColor(float red, float green, float blue);
    Vector3f GetColor() const;

    void SetWidth(float width);
    float GetWidth() const;

    void SetStyle(Style style);
    Style GetStyle() const;

    //void SetOpacity(float opacity);
    //float GetOpacity() const;

protected:
    Pen();
    ~Pen() override;

    Vector3f m_PenColor;
    float m_PenWidth;
    Pen::Style m_PenStyle;
    float m_PenOpacity;
};

IGAME_NAMESPACE_END