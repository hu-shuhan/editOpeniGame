//
// Created by Sumzeek on 10/9/2024.
//

#pragma once

#include "iGamePainterBase.h"

IGAME_NAMESPACE_BEGIN
class Scene;

class Painter2D : public PainterBase {
public:
    I_OBJECT(Painter2D);
    //static Pointer New() { return new Painter2D; }

protected:
    Painter2D() = default;
    ~Painter2D() override = default;
};

IGAME_NAMESPACE_END