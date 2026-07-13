#pragma once

#include "GPSplineSurface.h"

GPSTART
    class GPSplineSurfaceCPU :  GPSplineSurface
    {

    public:

        using real_t = float;

        void init_knot_vector(CBSplineSurface& surface) override;

        void init_cpoints(CBSplineSurface& surface) override;

        void init_memory_pools(CBSplineSurface& surface) override;

        void release_knot_vector() override;

        void release_cpoints() override;

        void release_memory_pools() override;

        void build_tessellation(uint16_t  new_p, uint16_t new_q) override;
    };

GPEND