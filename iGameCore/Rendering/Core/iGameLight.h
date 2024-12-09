#pragma once

#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
class Light : public Object {
public:
    I_OBJECT(Light);

protected:
    Light();
    ~Light() override;

private:
};

IGAME_NAMESPACE_END