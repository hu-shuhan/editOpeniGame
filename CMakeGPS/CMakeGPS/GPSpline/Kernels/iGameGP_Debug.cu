#include "iGameGP_Debug.cuh"

__device__ void print_3x3_double_array(double data[3][3]) {
    if (blockIdx.x == 0 && blockIdx.y == 0 && threadIdx.x == 0 && threadIdx.y == 0) {
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