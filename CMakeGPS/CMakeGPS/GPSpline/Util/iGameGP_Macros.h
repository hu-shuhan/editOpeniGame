#pragma once

#include <stdint.h>
#include "iGameGP_Log.h"

#define GPSTART namespace gpmesh {
#define GPEND }  

GPSTART

    typedef uint8_t flag_t;

    constexpr uint32_t TRANSPOSE_ITEM_PER_THREAD = 13;

#define DIVIDE_UP(num, divisor) (num + divisor - 1) / (divisor)

#define INVALID64 0xFFFFFFFFFFFFFFFFu

#define INVALID32 0xFFFFFFFFu

#define INVALID16 0xFFFFu

#define INVALID8 0xFFu


#define STRINGIFY(x) TOSTRING(x)
#define TOSTRING(x) #x


GPEND

#define SWAP(a, b, c) do {c t; t = a; a = b; b=t; } while(0)