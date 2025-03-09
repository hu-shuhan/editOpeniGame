//
// Created by Sumzeek on 9/13/2024.
//

#include "iGamePen.h"
#include "iGameRenderingLogger.h"

IGAME_NAMESPACE_BEGIN

Pen::Pen() {
    m_PenColor = Vector3f{-1.0f, -1.0f, -1.0f};
    m_PenWidth = 1;
    m_PenStyle = Style::SolidLine;
    m_PenOpacity = 1.0f;
    SetColor(Color::Black);
}

Pen::~Pen() {}

void Pen::SetColor(const Color& color) {
    auto c = ColorUtils::Map(color);
    if (c.x == m_PenColor[0] && c.y == m_PenColor[1] && c.z == m_PenColor[2]) {
        return;
    }

    m_PenColor = Vector3f{c.x, c.y, c.z};
    this->Modified();
}

void Pen::SetColor(float red, float green, float blue) {
    if (!ColorUtils::IsValid(red, green, blue)) {
        IGAME_RENDERING_ERROR(
                "Color values must be in the range of 0.0f to 1.0f");
    }

    if (red == m_PenColor[0] && green == m_PenColor[1] &&
        blue == m_PenColor[2]) {
        return;
    }

    m_PenColor = Vector3f{red, green, blue};
    this->Modified();
}

void Pen::SetColor(int red, int green, int blue) {
    if (!ColorUtils::IsValid(red, green, blue)) {
        IGAME_RENDERING_ERROR("Color values must be in the range of 0 to 255");
    }

    float r = static_cast<float>(red) / 255.0f;
    float g = static_cast<float>(green) / 255.0f;
    float b = static_cast<float>(blue) / 255.0f;

    if (r == m_PenColor[0] && g == m_PenColor[1] && b == m_PenColor[2]) {
        return;
    }

    m_PenColor = Vector3f{r, g, b};
    this->Modified();
}

Vector3f Pen::GetColor() const { return m_PenColor; }

void Pen::SetWidth(float width) {
    if (width == m_PenWidth) { return; }

    m_PenWidth = width;
    this->Modified();
}

float Pen::GetWidth() const { return m_PenWidth; }

void Pen::SetStyle(Pen::Style style) {
    if (style == m_PenStyle) { return; }

    m_PenStyle = style;
    this->Modified();
}

Pen::Style Pen::GetStyle() const { return m_PenStyle; }

//void Pen::SetOpacity(float opacity) { m_PenOpacity = opacity; }

//float Pen::GetOpacity() const { return m_PenOpacity; }

IGAME_NAMESPACE_END