//
// Created by Sumzeek on 12/9/2024.
//

#include "iGameColorUtils.h"

IGAME_NAMESPACE_BEGIN

ColorUtils::ColorUtils(){};
ColorUtils::~ColorUtils(){};

bool ColorUtils::IsValid(const igm::vec3 color) {
    if (color.r < 0.0f || color.r > 1.0f || color.g < 0.0f || color.g > 1.0f ||
        color.b < 0.0f || color.b > 1.0f) {
        return false;
    }
    return true;
}

bool ColorUtils::IsValid(Vector3f color) {
    if (color[0] < 0.0f || color[0] > 1.0f || color[1] < 0.0f ||
        color[1] > 1.0f || color[2] < 0.0f || color[2] > 1.0f) {
        return false;
    }
    return true;
}

bool ColorUtils::IsValid(float red, float green, float blue) {
    if (red < 0.0f || red > 1.0f || green < 0.0f || green > 1.0f ||
        blue < 0.0f || blue > 1.0f) {
        return false;
    }
    return true;
}

bool ColorUtils::IsValid(int red, int green, int blue) {
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 ||
        blue > 255) {
        return false;
    }
    return true;
}

igm::vec3 ColorUtils::Map(Color color) {
    switch (color) {
        case Color::Red:
            return igm::vec3{1.0f, 0.0f, 0.0f};
        case Color::Green:
            return igm::vec3{0.0f, 1.0f, 0.0f};
        case Color::Blue:
            return igm::vec3{0.0f, 0.0f, 1.0f};
        case Color::LightBlue:
            return igm::vec3{0.678f, 0.847f, 0.902f};
        case Color::White:
            return igm::vec3{1.0f, 1.0f, 1.0f};
        case Color::Black:
            return igm::vec3{0.0f, 0.0f, 0.0f};
        case Color::Gray:
            return igm::vec3{0.5f, 0.5f, 0.5f};
        case Color::DarkGray:
            return igm::vec3{0.25f, 0.25f, 0.25f};
        case Color::LightGray:
            return igm::vec3{0.75f, 0.75f, 0.75f};
        case Color::Yellow:
            return igm::vec3{1.0f, 1.0f, 0.0f};
        case Color::Magenta:
            return igm::vec3{1.0f, 0.0f, 1.0f};
        case Color::Cyan:
            return igm::vec3{0.0f, 1.0f, 1.0f};
        case Color::Orange:
            return igm::vec3{1.0f, 0.647f, 0.0f};
        case Color::Purple:
            return igm::vec3{0.5f, 0.0f, 0.5f};
        case Color::Pink:
            return igm::vec3{1.0f, 0.752f, 0.796f};
        case Color::Brown:
            return igm::vec3{0.647f, 0.165f, 0.165f};
        case Color::Gold:
            return igm::vec3{1.0f, 0.843f, 0.0f};
        default:
            igError("Color not mapped!");
    }
}

IGAME_NAMESPACE_END