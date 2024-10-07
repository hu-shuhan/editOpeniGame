//
// Created by Sumzeek on 9/13/2024.
//

#include "iGameBrush.h"

IGAME_NAMESPACE_BEGIN

Brush::Brush() : m_BrushStyle(BrushStyle::SolidPattern), m_BrushOpacity(1.0f) {
    SetColor(Color::White);
}

Brush::~Brush() {}

void Brush::SetColor(const Color& color) {
    if (color == Color::None) {
        m_BrushColor = Vector3f{-1.0f, -1.0f, -1.0f};
        return;
    }
    auto c = ColorUtils::Map(color);
    m_BrushColor = Vector3f{c.x, c.y, c.z};
}

void Brush::SetColor(float red, float green, float blue) {
    if (ColorUtils::IsValid(red, green, blue)) {
        m_BrushColor = Vector3f{red / 255.0f, green / 255.0f, blue / 255.0f};
    } else {
        throw std::runtime_error(
                "Color values must be in the range of 0.0 to 1.0");
    }
}

void Brush::SetColor(int red, int green, int blue) {
    if (ColorUtils::IsValid(red, green, blue)) {
        m_BrushColor = Vector3f{red / 255.0f, green / 255.0f, blue / 255.0f};
    } else {
        throw std::runtime_error(
                "Color values must be in the range of 0 to 255");
    }
}

Vector3f Brush::GetColor() const { return m_BrushColor; }

void Brush::SetStyle(BrushStyle style) { m_BrushStyle = style; }

BrushStyle Brush::GetStyle() const { return m_BrushStyle; }

//void Brush::SetOpacity(float opacity) { m_BrushOpacity = opacity; }

//float Brush::GetOpacity() const { return m_BrushOpacity; }

IGAME_NAMESPACE_END