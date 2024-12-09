//
// Created by Sumzeek on 9/13/2024.
//

#include "iGamePen.h"

IGAME_NAMESPACE_BEGIN

Pen::Pen() {
    m_PenWidth = 1;
    m_PenStyle = Style::SolidLine;
    m_PenOpacity = 1.0f;
    SetColor(Color::Black);
}

Pen::~Pen() {}

void Pen::SetColor(const Color& color) {
    if (color == Color::None) {
        m_PenColor = Vector3f{-1.0f, -1.0f, -1.0f};
        return;
    }
    auto c = ColorUtils::Map(color);
    m_PenColor = Vector3f{c.x, c.y, c.z};
};

void Pen::SetColor(float red, float green, float blue) {
    if (ColorUtils::IsValid(red, green, blue)) {
        m_PenColor = Vector3f{red, green, blue};
    } else {
        igError("Color values must be in the range of 0.0 to 1.0");
        throw std::runtime_error(
                "Color values must be in the range of 0.0 to 1.0");
    }
};

void Pen::SetColor(int red, int green, int blue) {
    if (ColorUtils::IsValid(red, green, blue)) {
        m_PenColor = Vector3f{static_cast<float>(red) / 255.0f,
                              static_cast<float>(green) / 255.0f,
                              static_cast<float>(blue) / 255.0f};
    } else {
        igError("Color values must be in the range of 0 to 255");
        throw std::runtime_error(
                "Color values must be in the range of 0 to 255");
    }
}

Vector3f Pen::GetColor() const { return m_PenColor; }

void Pen::SetWidth(float width) { m_PenWidth = width; }

int Pen::GetWidth() const { return m_PenWidth; }

void Pen::SetStyle(Style style) { m_PenStyle = style; }

Pen::Style Pen::GetStyle() const { return m_PenStyle; }

//void Pen::SetOpacity(float opacity) { m_PenOpacity = opacity; }

//float Pen::GetOpacity() const { return m_PenOpacity; }

IGAME_NAMESPACE_END