#pragma once

#include "iGameCommon.h"
#include <assert.h>

__global__ void simtNaiveKernel(const float * A,  const float * B, float * C, size_t M,
                                size_t N, size_t K) {
    size_t row = threadIdx.x;
    size_t col = threadIdx.y;

    if (row >= M || col >= N) {
        return;
    }

    float tmp = 0.0;

#pragma unroll
    for (size_t i = 0; i < K; ++i) {

        size_t id_A = row * K + i;
        size_t id_B = i + col * K;
        assert(id_A < M * K && id_A >= 0);
        assert(id_B < N * K && id_B >= 0);

        tmp += A[id_A] * B[id_B];
    }

    size_t id_C = row * N + col;
    assert(id_C >= 0 && id_C < M * N);
    C[id_C] = tmp;
}


__global__ void simtNaiveKernel_BigMN(const float * A,  const float * B, float * C, size_t M,
                                size_t N, size_t K) {
    size_t row = threadIdx.x;
    size_t col = threadIdx.y;

    if (row >= M || col >= N) {
        return;
    }

    for(size_t h = row; h < M; h += blockDim.x)
    {
        for(size_t l = col; l < N; l += blockDim.y)
        {
            float tmp = 0.0;

#pragma unroll
            for (size_t i = 0; i < K; ++i) {

                tmp += A[h * K + i] * B[i + l * K];
            }

            C[h * N + l] = tmp;
        }
    }
}

__global__ void simtNaiveKernel_BigMN_GridBlock(const float * A,  const float * B, float * C, size_t M,
                                      size_t N, size_t K) {
    size_t row = threadIdx.x + blockDim.x * blockIdx.x;
    size_t col = threadIdx.y + blockDim.y * blockIdx.y;

    if (row >= M || col >= N) {
        return;
    }

    float tmp = 0.0;

#pragma unroll
    for (size_t i = 0; i < K; ++i) {

        size_t id_A = row * K + i;
        size_t id_B = i + col * K;
        assert(id_A < M * K && id_A >= 0);
        assert(id_B < N * K && id_B >= 0);

        tmp += A[id_A] * B[id_B];
    }

    size_t id_C = row * N + col;
    assert(id_C >= 0 && id_C < M * N);
    C[id_C] = tmp;
}