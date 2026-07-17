#pragma once

#include <Geom/CBSplineSurface.h>
#include "Util/iGameGP_Macros.h"
#include "Util/iGameGPSplineDefine.h"

GPSTART

    class GPSplineSurface {

        using real_t = float;

    public:

        void init(CBSplineSurface& surface);

        virtual void init_knot_vector(CBSplineSurface& surface);

        virtual void init_cpoints(CBSplineSurface& surface);

        virtual void init_memory_pools(CBSplineSurface& surface);

        void release();

        virtual void release_knot_vector();

        virtual void release_cpoints();

        virtual void release_memory_pools();

        virtual void build_tessellation(uint16_t  new_p, uint16_t new_q);

        void build_tessellation_cpu(uint16_t new_p, uint16_t new_q);

        void compare_cpu_and_gpu_data() const;

    public:

        real_t *device_uKnots = nullptr;
        real_t *device_vKnots = nullptr;

        real_t *host_uKnots = nullptr;
        real_t *host_vKnots = nullptr;

        real_t *device_memPool_H_p = nullptr;
        real_t *device_memPool_H_q = nullptr;

        real_t *device_memPool_H_prime_p = nullptr;

        real_t *device_memPool_H_prime_q = nullptr;

        real_t *host_memPool_H_p = nullptr;
        real_t *host_memPool_H_q = nullptr;

        real_t *host_memPool_H_prime_p = nullptr;
        real_t *host_memPool_H_prime_q = nullptr;


        real_t *device_memPool_HnP_tmp = nullptr;
        real_t *device_memPool_HmP_tmp = nullptr;

        real_t *host_memPool_HnP_tmp = nullptr;
        real_t *host_memPool_HmP_tmp = nullptr;

        real_t *device_cpoints_x = nullptr;
        real_t *device_cpoints_y = nullptr;
        real_t *device_cpoints_z = nullptr;

        real_t *host_cpoints_x = nullptr;
        real_t *host_cpoints_y = nullptr;
        real_t *host_cpoints_z = nullptr;

        real_t *device_memPool_result_x = nullptr;
        real_t *device_memPool_result_y = nullptr;
        real_t *device_memPool_result_z = nullptr;

        real_t *host_memPool_result_x = nullptr;
        real_t *host_memPool_result_y = nullptr;
        real_t *host_memPool_result_z = nullptr;

        const uint16_t MAX_P = _M_MAX_P;
        const uint16_t MAX_Q = _M_MAX_Q;

        uint16_t old_p = INVALID16;
        uint16_t old_q = INVALID16;

        uint16_t cur_p = INVALID16;
        uint16_t cur_q = INVALID16;


        const uint16_t uDegree = 3;
        const uint16_t vDegree = 3;

        const uint16_t uSize = 8;
        const uint16_t vSize = 8;

    };

GPEND