#pragma once

#include "iGameCommon.h"


template<typename real_t>
void NativeKernelCPU(const real_t *A, const real_t *B, real_t *C, size_t M,
                     size_t N, size_t K) {

    for (size_t row = 0; row < M; ++row) {
        for (size_t col = 0; col < N; ++col) {

            float tmp = 0;

            for (size_t i = 0; i < K; ++i) {

                tmp += A[row * K + i] * B[i + col * K];
            }

            C[row * N + col] = tmp;
        }
    }
}


template<typename real_t>
void
mma_kernel_abd_cpu(real_t *const d_ptr, const real_t *const a_ptr, const real_t *const b_ptr, MatrixLayout d_layout) {
    auto get_col_major_index_d = [ROW = 16, COL = 8](int row_id, int col_id) -> int {
        int index = row_id + col_id * ROW;
        assert(index < ROW * COL);
        return index;
    };

    auto get_row_major_index_d = [ROW = 16, COL = 8](int row_id, int col_id) -> int {
        int index = row_id * COL + col_id;
        assert(index < ROW * COL);
        return index;
    };

    auto get_row_major_index_a = [ROW = 16, COL = 8](int row_id, int col_id) -> int {
        int index = row_id * COL + col_id;
        assert(index < ROW * COL);
        return index;
    };

    auto get_col_major_index_b = [ROW = 8, COL = 8](int row_id, int col_id) -> int {
        int index = row_id + col_id * ROW;
        assert(index < ROW * COL);
        return index;
    };

    for (int row = 0; row < 16; ++row) {
        for (int col = 0; col < 8; ++col) {

            int d_index = (d_layout == MatrixLayout::ColMajor) ? get_col_major_index_d(row, col) :
                          get_row_major_index_d(row, col);

            real_t local_sum = 0;

            for (int k = 0; k < 8; ++k) {
                local_sum  += a_ptr[get_row_major_index_a(row, k)] * b_ptr[get_col_major_index_b(k, col)];
            }

            d_ptr[d_index] = local_sum;
        }
    }

}