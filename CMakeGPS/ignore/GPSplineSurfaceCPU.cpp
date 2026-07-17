#include "GPSplineSurfaceCPU.h"

GPSTART
    void GPSplineSurfaceCPU::init_knot_vector(CBSplineSurface &surface) {
        host_uKnots = new real_t[uSize]{0};

        for (int i = 4; i < 8; ++i) {
            host_uKnots[i] = 1.0;
        }

        host_vKnots = new real_t[vSize]{0};

        for (int i = 4; i < 8; ++i) {
            host_vKnots[i] = 1.0;
        }

    }

    void GPSplineSurfaceCPU::init_cpoints(CBSplineSurface &surface) {
        const size_t CPOINT_SIZE = 4 * 4;
        const int ROW_NUM = 4;
        const int COL_NUM = 4;
        host_cpoints_x = new real_t[CPOINT_SIZE]{0};
        host_cpoints_y = new real_t[CPOINT_SIZE]{0};
        host_cpoints_z = new real_t[CPOINT_SIZE]{0};


        auto get_cpoint_index_ColMajor = [COL_NUM](int i, int j) -> int {
            int res = i * COL_NUM + j;

            return res;
        };

        auto get_cpoint_index_RowMajor = [ROW_NUM](int i, int j) -> int {
            int res = i + j * ROW_NUM;

            return res;
        };


        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                host_cpoints_x[get_cpoint_index_ColMajor(i,
                                                         j)] = static_cast<float>(surface.getControlPoints()[i][j].getX());
                host_cpoints_y[get_cpoint_index_ColMajor(i,
                                                         j)] = static_cast<float>(surface.getControlPoints()[i][j].getY());
                host_cpoints_z[get_cpoint_index_ColMajor(i,
                                                         j)] = static_cast<float>(surface.getControlPoints()[i][j].getZ());
            }
        }
    }

    void GPSplineSurfaceCPU::init_memory_pools(CBSplineSurface &surface) {
        size_t n = 7 * MAX_P + 1;
        size_t m = 7 * MAX_Q + 1;
        size_t nm = n * m;

        host_memPool_H_p = new real_t[n];
        host_memPool_H_prime_p = new real_t[n];

        host_memPool_H_q = new real_t[n];
        host_memPool_H_prime_q = new real_t[n];

        host_memPool_result_x = new real_t[nm];
        host_memPool_result_y = new real_t[nm];
        host_memPool_result_z = new real_t[nm];
    }

    void GPSplineSurfaceCPU::release_knot_vector() {
        if (host_uKnots != nullptr) {
            delete[] host_uKnots;
            host_uKnots = nullptr;
        }

        if (host_vKnots != nullptr) {
            delete[] host_vKnots;
            host_uKnots = nullptr;
        }
    }

    void GPSplineSurfaceCPU::release_cpoints() {
        if (host_cpoints_x != nullptr) {
            delete[] host_cpoints_x;
            host_cpoints_x = nullptr;
        }

        if (host_cpoints_y != nullptr) {
            delete[] host_cpoints_y;
            host_cpoints_y = nullptr;
        }

        if (host_cpoints_z != nullptr) {
            delete[] host_cpoints_z;
            host_cpoints_z = nullptr;
        }
    }

    void GPSplineSurfaceCPU::release_memory_pools() {
        if(host_memPool_H_p != nullptr)
        {
            delete[] host_memPool_H_p;
            host_memPool_H_p = nullptr;
        }
        if(host_memPool_H_prime_p != nullptr)
        {
            delete[] host_memPool_H_prime_p;
            host_memPool_H_prime_p = nullptr;
        }

        if(host_memPool_H_q != nullptr)
        {
            delete[] host_memPool_H_q;
            host_memPool_H_q = nullptr;
        }
        if(host_memPool_H_prime_q != nullptr)
        {
            delete[] host_memPool_H_prime_q;
            host_memPool_H_prime_q = nullptr;
        }

        if(host_memPool_result_x != nullptr)
        {
            delete[] host_memPool_result_x;
            host_memPool_result_x = nullptr;
        }

        if(host_memPool_result_y != nullptr)
        {
            delete[] host_memPool_result_y;
            host_memPool_result_y = nullptr;
        }

        if(host_memPool_result_z != nullptr)
        {
            delete[] host_memPool_result_z;
            host_memPool_result_z = nullptr;
        }
    }

    void GPSplineSurfaceCPU::build_tessellation(uint16_t new_p, uint16_t new_q) {

    }


GPEND


