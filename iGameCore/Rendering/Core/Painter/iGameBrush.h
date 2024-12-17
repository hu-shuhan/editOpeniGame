//
// Created by Sumzeek on 9/13/2024.
//

#pragma once

#include "iGameColorUtils.h"
#include "iGameObject.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN

class Brush : public Object {
public:
    I_OBJECT(Brush);
    static Pointer New() { return new Brush; }

    // Only SolidPattern has been implemented
    enum class Style {
        NoBrush,
        SolidPattern,
        //Dense1Pattern,
        //Dense2Pattern,
        //Dense3Pattern,
        //Dense4Pattern,
        //Dense5Pattern,
        //Dense6Pattern,
        //Dense7Pattern,
        //HorPattern,
        //VerPattern,
        //CrossPattern,
        //BDiagPattern,
        //FDiagPattern,
        //DiagCrossPattern,
        //LinearGradientPattern,
        //RadialGradientPattern,
        //ConicalGradientPattern,
        //TexturePattern = 24
    };

    void SetColor(const Color& color);
    void SetColor(int red, int green, int blue);
    void SetColor(float red, float green, float blue);
    Vector3f GetColor() const;

    void SetStyle(Brush::Style style);
    Brush::Style GetStyle() const;

    //void SetOpacity(float opacity);
    //float GetOpacity() const;

protected:
    Brush();
    ~Brush() override;

    Vector3f m_BrushColor;
    Brush::Style m_BrushStyle;
    float m_BrushOpacity;
};

IGAME_NAMESPACE_END