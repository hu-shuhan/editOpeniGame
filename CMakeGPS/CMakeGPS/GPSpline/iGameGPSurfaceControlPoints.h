#pragma once

#include "Util/iGameGP_Macros.h"

GPSTART

    struct GPSurfaceControlPoint {
        real_t *m_cpoints_x = nullptr;
        real_t *m_cpoints_y = nullptr;
        real_t *m_cpoints_z = nullptr;
    };

    struct GPSurfaceScalarPoint {
        real_t* m_cpoints_x = nullptr;
        real_t* m_cpoints_y = nullptr;
        real_t* m_cpoints_z = nullptr;
    };

GPEND