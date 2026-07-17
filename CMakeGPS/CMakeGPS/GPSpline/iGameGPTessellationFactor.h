#pragma once

#include "Util/iGameGP_Macros.h"

GPSTART

    struct GPTessellationFactor
    {
        float A, B;
        float tauX, tauY;

        float* ViewMatrix = nullptr;
    };

GPEND