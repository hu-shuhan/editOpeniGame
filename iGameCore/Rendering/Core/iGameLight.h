#pragma once

#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
class Light : public Object {
public:
    I_OBJECT(Light);
    //static Pointer New() { return new Light; }

protected:
    Light();
    ~Light() override;
};

IGAME_NAMESPACE_END