/**
 * @class   iGameAttribute
 * @brief   iGameAttribute's brief
 */

#pragma once

#include <iGameObject.h>
#include <iGameArrayObject.h>

IGAME_NAMESPACE_BEGIN
class Attribute{
public:
    ArrayObject::Pointer pointer{};
    IGenum type{ IG_NONE };
    IGenum attachmentType{ IG_NONE };
    bool isDeleted{ false };

    std::pair<float, float> dataRange {0.f, 0.f};

    static Attribute None();

    bool isNone() const ;

};
IGAME_NAMESPACE_END