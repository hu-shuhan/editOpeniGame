//
// Created by Sumzeek on 9/13/2024.
//

#include "iGameBrush.h"
#include "iGameRenderingLogger.h"

IGAME_NAMESPACE_BEGIN

Brush::Brush() {
    m_BrushStyle = Brush::Style::SolidPattern;
    m_BrushOpacity = 1.0f;
    SetColor(Color::White);
}

Brush::~Brush() {}

void Brush::SetColor(const Color& color) {
    auto c = ColorUtils::Map(color);
    if (c.x == m_BrushColor[0] && c.y == m_BrushColor[1] &&
        c.z == m_BrushColor[2]) {
        return;
    }

    m_BrushColor = Vector3f{c.x, c.y, c.z};
    this->Modified();
}

void Brush::SetColor(float red, float green, float blue) {
    if (!ColorUtils::IsValid(red, green, blue)) {
        IGAME_RENDERING_ERROR("Color values must be in the range of 0.0f to 1.0f");
    }

    if (red == m_BrushColor[0] && green == m_BrushColor[1] &&
        blue == m_BrushColor[2]) {
        return;
    }

    m_BrushColor = Vector3f{red, green, blue};
    this->Modified();
}

void Brush::SetColor(int red, int green, int blue) {
    if (!ColorUtils::IsValid(red, green, blue)) {
        IGAME_RENDERING_ERROR("Color values must be in the range of 0 to 255");
    }

    float r = static_cast<float>(red) / 255.0f;
    float g = static_cast<float>(green) / 255.0f;
    float b = static_cast<float>(blue) / 255.0f;

    if (r == m_BrushColor[0] && g == m_BrushColor[1] && b == m_BrushColor[2]) {
        return;
    }

    m_BrushColor = Vector3f{r, g, b};
    this->Modified();
}

Vector3f Brush::GetColor() const { return m_BrushColor; }

void Brush::SetStyle(Brush::Style style) {
    if (style == m_BrushStyle) { return; }

    m_BrushStyle = style;
    this->Modified();
}

Brush::Style Brush::GetStyle() const { return m_BrushStyle; }

//void Brush::SetOpacity(float opacity) { m_BrushOpacity = opacity; }

//float Brush::GetOpacity() const { return m_BrushOpacity; }

IGAME_NAMESPACE_END