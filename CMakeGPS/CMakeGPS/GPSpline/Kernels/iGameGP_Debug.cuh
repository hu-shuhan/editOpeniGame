#pragma once

#include <stdint.h>
#include <stdio.h>
#include "cuda_runtime.h"

namespace gpmesh {

    template<typename T>
    __device__ void print_arr_uint(char msg[],
                                   uint32_t size,
                                   T *arr,
                                   uint32_t block_id = 0,
                                   uint32_t thread_id = 0) {
        if (blockIdx.x == block_id && threadIdx.x == thread_id) {
            printf("\n %s \n", msg);
            for (uint32_t i = 0; i < size; i++) {
                printf("\n arr[%u]=%u", i, arr[i]);
            }
            printf("\n");
        }
    }

    template<typename T>
    __device__ void print_arr_float(T size, float *arr) {
        if (blockIdx.x == 0 && threadIdx.x == 0) {
            for (uint32_t i = 0; i < size; i++) {
                printf("\n arr[%u]=%f", i, arr[i]);
            }
        }
    }

    template<typename T>
    __device__ void print_arr_double(T size, double *arr) {
        if (blockIdx.x == 0 && threadIdx.x == 0) {
            for (uint32_t i = 0; i < size; i++) {
                printf("double arr[%u]=%f \n", i, arr[i]);
            }
        }
    }

    template<typename T>
    __device__ void print_2D_arr_double(T rsize, T csize, double **data) {
        if (blockIdx.x == 0 && threadIdx.x == 0) {
            printf("cuda %u x %u 2D double array", rsize, csize);
            for (uint32_t i = 0; i < rsize; ++i) {
                for (uint32_t j = 0; j < csize; ++j) {
                    printf("arr[%u][%u] = {%f}", i, j, data[i][j]);
                }
                if (i < rsize - 1) {
                    printf(" - ");
                }
            }
            printf("\n");
        }
    }

    __device__ void print_3x3_double_array(double data[3][3]);

    inline __device__ void print_double(uint16_t geometry_id, double data) {
        if (blockIdx.x == geometry_id && blockIdx.y == 0 && threadIdx.x == 0 && threadIdx.y == 0) {
            printf("data is {%f}", data);
        }
    }

    template<typename T>
     __device__ inline void print_3x3_double_array(T geometry_id, double data[3][3]) {
        if (blockIdx.x == geometry_id && blockIdx.y == 0 && threadIdx.x == 0 && threadIdx.y == 0) {
            printf("cuda 3x3 double array\n");
            for (uint32_t i = 0; i < 3; ++i) {
                for (uint32_t j = 0; j < 3; ++j) {
                    printf("arr[%u][%u] = {%f}", i, j, data[i][j]);
                    if (j < 2) {
                        printf(" - ");
                    }
                }
                printf("\n");
            }
        }
    }

    template<typename T, size_t Row, size_t Col>
    __device__ void print_RowxCol_double_array(T geometry_id, double data[Row][Col]) {
        if (blockIdx.x == geometry_id && blockIdx.y == 0 && threadIdx.x == 0 && threadIdx.y == 0) {
            printf("cuda (%u)x(%u) double array\n", Row, Col);
            for (uint32_t i = 0; i < Row; ++i) {
                for (uint32_t j = 0; j < Col; ++j) {
                    printf("arr[%u][%u] = {%f}", i, j, data[i][j]);
                    if (j < 2) {
                        printf(" - ");
                    }
                }
                printf("\n");
            }

            printf("----------------\n");
        }
    }


    __device__ __forceinline__ unsigned total_smem_size() {
        unsigned ret;
        asm volatile("mov.u32 %0, %total_smem_size;" : "=r"(ret));
        return ret;
    }
}    