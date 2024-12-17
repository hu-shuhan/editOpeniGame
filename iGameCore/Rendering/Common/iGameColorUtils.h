//
// Created by Sumzeek on 10/4/2024.
//

#pragma once

#include "iGameObject.h"
#include "iGameVector.h"
#include "igm/igm.h"

IGAME_NAMESPACE_BEGIN

class ColorUtils : public Object {
public:
    static bool IsValid(const igm::vec3& color);
    static bool IsValid(Vector3f color);
    static bool IsValid(float red, float green, float blue);
    static bool IsValid(int red, int green, int blue);

    static igm::vec3 Map(Color color);

protected:
    ColorUtils();
    ~ColorUtils() override;
};

IGAME_NAMESPACE_END