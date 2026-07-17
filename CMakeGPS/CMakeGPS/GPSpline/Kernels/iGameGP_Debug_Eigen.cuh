#pragma once
#include <stdint.h>
#include <stdio.h>
#include "cuda_runtime.h"

#include <Eigen/Core>

template<typename T, size_t Row, size_t Col>
__device__ void print_RowxCol_matrix3d(T geometry_id, Eigen::Matrix<double, Row, Col>& data)
{
    if (blockIdx.x == geometry_id && blockIdx.y == 0 && threadIdx.x == 0 && threadIdx.y == 0) {
        printf("cuda (%u)x(%u) EigenMatrix array\n", Row, Col);
        for (uint32_t i = 0; i < Row; ++i) {
            for(uint32_t j = 0; j < Col; ++j)
            {
                printf("arr[%u][%u] = {%f}", i, j, data(i,j));
                if(j < 2)
                {
                    printf(" - ");
                }
            }
            printf("\n");
        }
    }
}