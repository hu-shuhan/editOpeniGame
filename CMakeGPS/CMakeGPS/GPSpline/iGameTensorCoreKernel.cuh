#pragma once

#include "iGamewmma_extension_util.h"


template <unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void mma_kernel_abd(float* const d_ptr, const float* const a_ptr, const float* const b_ptr, const nvcuda::wmma::layout_t c_layout) {
    constexpr unsigned LD = N;

    mtk::wmma::tcec::fragment<nvcuda::wmma::matrix_a   , M, N, K, T, A_Layout, Policy> frag_a;
    mtk::wmma::tcec::fragment<nvcuda::wmma::matrix_b   , M, N, K, T, B_Layout, Policy> frag_b;
    mtk::wmma::tcec::fragment<nvcuda::wmma::accumulator, M, N, K, T, void    , Policy> frag_d;

    const unsigned lda = std::is_same<A_Layout, nvcuda::wmma::col_major>::value ? M : K;
    const unsigned ldb = std::is_same<B_Layout, nvcuda::wmma::col_major>::value ? K : N;

    mtk::wmma::tcec::load_matrix_sync<MEM_A_Layout>(frag_a, a_ptr, lda);

    mtk::wmma::tcec::load_matrix_sync<MEM_B_Layout>(frag_b, b_ptr, ldb);

    unsigned ldc = c_layout == nvcuda::wmma::mem_col_major ? M : N;

    mtk::wmma::tcec::mma_sync(frag_d, frag_a, frag_b);

    mtk::wmma::tcec::store_matrix_sync(d_ptr, frag_d, ldc, c_layout);
}

template <unsigned M, unsigned N, unsigned K, class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void mma_kernel_abcd(float* const d_ptr, const float* const a_ptr, const float* const b_ptr, const float* const c_ptr, const nvcuda::wmma::layout_t cd_layout) {

    mtk::wmma::tcec::fragment<nvcuda::wmma::matrix_a   , M, N, K, T, A_Layout, Policy> frag_a;
    mtk::wmma::tcec::fragment<nvcuda::wmma::matrix_b   , M, N, K, T, B_Layout, Policy> frag_b;
    mtk::wmma::tcec::fragment<nvcuda::wmma::accumulator, M, N, K, T, void    , Policy> frag_c;
    mtk::wmma::tcec::fragment<nvcuda::wmma::accumulator, M, N, K, T, void    , Policy> frag_d;

    const unsigned lda = std::is_same<A_Layout, nvcuda::wmma::col_major>::value ? M : K;
    const unsigned ldb = std::is_same<B_Layout, nvcuda::wmma::col_major>::value ? K : N;

    mtk::wmma::tcec::load_matrix_sync<MEM_A_Layout>(frag_a, a_ptr, lda);

    mtk::wmma::tcec::load_matrix_sync<MEM_B_Layout>(frag_b, b_ptr, ldb);

    unsigned ldcd = cd_layout == nvcuda::wmma::mem_col_major ? M : N;

    mtk::wmma::tcec::load_matrix_sync(frag_c, c_ptr, ldcd, cd_layout);

    mtk::wmma::tcec::mma_sync(frag_d, frag_a, frag_b, frag_c);

    mtk::wmma::tcec::store_matrix_sync(d_ptr, frag_d, ldcd, cd_layout);

    mtk::wmma::tcec::fill_zero(frag_d);
}