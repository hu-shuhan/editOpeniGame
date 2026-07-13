#pragma once

#include <cuda_runtime_api.h>
#include <algorithm>
#include <cmath>

#include "iGameGPPatchData.h"
#include "iGameGPSurfaceControlPoints.h"

#include <thrust/reduce.h>
#include <thrust/extrema.h>
#include <thrust/execution_policy.h>

#include "iGamewmma_extension_util.h"

template<size_t MAX_P, size_t MAX_Q>
__device__ static size_t get_local_index(size_t lp, size_t lq)
{
    assert(lp < MAX_P);
    assert(lq < MAX_Q);
    assert(lp + lq * MAX_P < MAX_P * MAX_Q);

    return lp + lq * MAX_P;
}

template<size_t MAX_P, size_t MAX_Q, size_t N, size_t M>
__global__ void scene_evaluation_HHprime_p_kernel(gpmesh::GPPatchData **device_surface_patch_data_arr,
                                                  uint32_t num_surface,
                                                  gpmesh::real_t * delta_u_arr,
                                                  gpmesh::real_t * delta_v_arr
                                                  ) {
    uint32_t surfaceId = blockIdx.x;
    if (surfaceId >= num_surface) {
        return;
    }

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if (p * q >= MAX_P * MAX_Q) {
        return;
    }

    auto get_local_index_lambda = [max_p=MAX_P, max_q=MAX_Q](size_t lp, size_t lq) -> size_t {

        assert(lp + lq * max_p < max_p * max_q);

        return lp + lq * max_p;
    };

    gpmesh::real_t delta_u = delta_u_arr[surfaceId];
    gpmesh::real_t delta_v = delta_v_arr[surfaceId];

    uint32_t tu = std::llround(std::ceil(static_cast<gpmesh::real_t>(1.0) / delta_u));
    uint32_t tv = std::llround(std::ceil(static_cast<gpmesh::real_t>(1.0) / delta_v));

    uint32_t cur_p = 1;
    uint32_t cur_q = 1;

    if(tu > 1)
    {
        cur_p = (tu - 1) / 7;

        if((tu - 1) % 7 != 0)
        {
            cur_p += 1;
        }
    }

    if(tv > 1)
    {
        cur_q = (tv - 1) / 7;

        if((tv - 1) % 7 != 0)
        {
            cur_q += 1;
        }
    }

    cur_p = std::min<uint32_t>(cur_p, MAX_P);
    cur_p = std::max<uint32_t>(cur_p, 1);

    cur_q = std::min<uint32_t>(cur_q, MAX_Q);
    cur_q = std::max<uint32_t>(cur_q, 1);

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }

    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData &patchData = device_surface_patch_data_arr[surfaceId][local_id];


    gpmesh::real_t du = 1.0f / static_cast<gpmesh::real_t>(cur_p);
    gpmesh::real_t dv = 1.0f / static_cast<gpmesh::real_t>(cur_q);

    gpmesh::real_t u_begin = du * static_cast<gpmesh::real_t>(p);
    gpmesh::real_t u_step =  du / (N - 1);

    gpmesh::real_t v_begin = dv * static_cast<gpmesh::real_t>(q);
    gpmesh::real_t v_step =  dv / (M - 1);

    if (threadIdx.x >= N) {
        return;
    }

    if(threadIdx.x == 0)
    {
    }

    uint16_t id = threadIdx.x;

    gpmesh::real_t u = u_begin + u_step * id;

    const int m_p = 3;
    const int p1 = m_p + 1;

    gpmesh::real_t left[p1] = {0};
    gpmesh::real_t right[p1] = {0};

    gpmesh::real_t ndu[p1 * p1] = {0};
    gpmesh::real_t a[2 * p1] = {0};

    gpmesh::real_t* H = patchData.m_HHprime_p;
    gpmesh::real_t* Hprime = &patchData.m_HHprime_p[8 * N];

    ndu[0] = static_cast<gpmesh::real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = u - 0;
        right[j] = 1.0 - u;
        gpmesh::real_t saved = static_cast<gpmesh::real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const gpmesh::real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] = saved + right[r + 1] * temp;      
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 8 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert((start_id + j) + 4 < 8 * N);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        gpmesh::real_t *a1 = &a[0];
        gpmesh::real_t *a2 = &a[p1];

        a1[0] = static_cast<gpmesh::real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            gpmesh::real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r + 4 < 8 * N);
            Hprime[start_id + r] = d * m_p;
        }
    }
}

template<size_t MAX_P, size_t MAX_Q, size_t N, size_t M>
__global__ void scene_evaluation_HHprime_p_kernel_with_stream(gpmesh::GPPatchData **device_surface_patch_data_arr,
                                                  uint32_t num_surface,
                                                  size_t * p_arr,
                                                  size_t * q_arr
) {
    uint32_t surfaceId = blockIdx.x;

    if (surfaceId >= num_surface) {
        return;
    }

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if (p * q >= MAX_P * MAX_Q) {
        return;
    }

    auto get_local_index_lambda = [max_p=MAX_P, max_q=MAX_Q](size_t lp, size_t lq) -> size_t {

        assert(lp + lq * max_p < max_p * max_q);

        return lp + lq * max_p;
    };

    size_t cur_p = p_arr[surfaceId];

    if(p >= cur_p)
    {
        return;
    }

    size_t cur_q = q_arr[surfaceId];

    if(q >= cur_q)
    {
        return;
    }

    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData &patchData = device_surface_patch_data_arr[surfaceId][local_id];


    gpmesh::real_t du = 1.0f / static_cast<gpmesh::real_t>(cur_p);
    gpmesh::real_t dv = 1.0f / static_cast<gpmesh::real_t>(cur_q);

    gpmesh::real_t u_begin = du * static_cast<gpmesh::real_t>(p);
    gpmesh::real_t u_step =  du / (N - 1);

    gpmesh::real_t v_begin = dv * static_cast<gpmesh::real_t>(q);
    gpmesh::real_t v_step =  dv / (M - 1);

    if (threadIdx.x >= N) {
        return;
    }

    if(threadIdx.x == 0)
    {
    }

    uint16_t id = threadIdx.x;

    gpmesh::real_t u = u_begin + u_step * id;

    const int m_p = 3;
    const int p1 = m_p + 1;

    gpmesh::real_t left[p1] = {0};
    gpmesh::real_t right[p1] = {0};

    gpmesh::real_t ndu[p1 * p1] = {0};
    gpmesh::real_t a[2 * p1] = {0};

    gpmesh::real_t* H = patchData.m_HHprime_p;
    gpmesh::real_t* Hprime = &patchData.m_HHprime_p[8 * N];

    ndu[0] = static_cast<gpmesh::real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = u - 0;
        right[j] = 1.0 - u;
        gpmesh::real_t saved = static_cast<gpmesh::real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const gpmesh::real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] = saved + right[r + 1] * temp;      
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 8 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert((start_id + j) + 4 < 8 * N);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        gpmesh::real_t *a1 = &a[0];
        gpmesh::real_t *a2 = &a[p1];

        a1[0] = static_cast<gpmesh::real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            gpmesh::real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r + 4 < 8 * N);
            Hprime[start_id + r] = d * m_p;
        }
    }
}

template<size_t MAX_P, size_t MAX_Q, size_t N, size_t M>
__global__ void scene_evaluation_H_q_extend_and_Hprime_q_extend_kernel(gpmesh::GPPatchData **device_surface_patch_data_arr,
                                                  uint32_t num_surface,
                                                  gpmesh::real_t * delta_u_arr,
                                                  gpmesh::real_t * delta_v_arr
) {
    uint32_t surfaceId = blockIdx.x;
    if (surfaceId >= num_surface) {
        return;
    }

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if (p * q >= MAX_P * MAX_Q) {
        return;
    }

    auto get_local_index = [max_p=MAX_P, max_q=MAX_Q](size_t lp, size_t lq) -> size_t {

        assert(lp + lq * max_p < max_p * max_q);

        return lp + lq * max_p;
    };

    gpmesh::real_t delta_u = delta_u_arr[surfaceId];
    gpmesh::real_t delta_v = delta_v_arr[surfaceId];

    uint32_t tu = std::llround(std::ceil(static_cast<gpmesh::real_t>(1.0) / delta_u));
    uint32_t tv = std::llround(std::ceil(static_cast<gpmesh::real_t>(1.0) / delta_v));

    uint32_t cur_p = 1;
    uint32_t cur_q = 1;

    if(tu > 1)
    {
        cur_p = (tu - 1) / 7;

        if((tu - 1) % 7 != 0)
        {
            cur_p += 1;
        }
    }

    if(tv > 1)
    {
        cur_q = (tv - 1) / 7;

        if((tv - 1) % 7 != 0)
        {
            cur_q += 1;
        }
    }

    cur_p = std::min<uint32_t>(cur_p, MAX_P);
    cur_p = std::max<uint32_t>(cur_p, 1);

    cur_q = std::min<uint32_t>(cur_q, MAX_Q);
    cur_q = std::max<uint32_t>(cur_q, 1);

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }

    size_t local_id = get_local_index(p, q);
    gpmesh::GPPatchData &patchData = device_surface_patch_data_arr[surfaceId][local_id];


    gpmesh::real_t du = 1.0f / static_cast<gpmesh::real_t>(cur_p);
    gpmesh::real_t dv = 1.0f / static_cast<gpmesh::real_t>(cur_q);

    gpmesh::real_t u_begin = du * static_cast<gpmesh::real_t>(p);
    gpmesh::real_t u_step =  du / (N - 1);

    gpmesh::real_t v_begin = dv * static_cast<gpmesh::real_t>(q);
    gpmesh::real_t v_step =  dv / (M - 1);

    if (threadIdx.x >= N) {
        return;
    }

    if(threadIdx.x == 0)
    {
    }

    uint16_t id = threadIdx.x;

    gpmesh::real_t v = v_begin + v_step * id;

    const int m_p = 3;
    const int p1 = m_p + 1;

    gpmesh::real_t left[p1] = {0};
    gpmesh::real_t right[p1] = {0};

    gpmesh::real_t ndu[p1 * p1] = {0};
    gpmesh::real_t a[2 * p1] = {0};

    gpmesh::real_t* H = patchData.m_H_q_extended;
    gpmesh::real_t* Hprime = patchData.m_Hprime_q_extended;

    ndu[0] = static_cast<gpmesh::real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = v - 0;
        right[j] = 1.0 - v;
        gpmesh::real_t saved = static_cast<gpmesh::real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const gpmesh::real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] = saved + right[r + 1] * temp;      
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 8 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert((start_id + j) + 4 < 8 * N);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        gpmesh::real_t *a1 = &a[0];
        gpmesh::real_t *a2 = &a[p1];

        a1[0] = static_cast<gpmesh::real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            gpmesh::real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r + 4 < 8 * N);
            Hprime[start_id + r] = d * m_p;
        }
    }
}

template<size_t MAX_P, size_t MAX_Q, size_t N, size_t M>
__global__ void scene_evaluation_H_q_extend_and_Hprime_q_extend_kernel_with_stream(gpmesh::GPPatchData **device_surface_patch_data_arr,
                                                                       uint32_t num_surface,
                                                                       size_t * p_arr,
                                                                       size_t * q_arr
) {
    uint32_t surfaceId = blockIdx.x;
    if (surfaceId >= num_surface) {
        return;
    }

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if (p * q >= MAX_P * MAX_Q) {
        return;
    }

    auto get_local_index = [max_p=MAX_P, max_q=MAX_Q](size_t lp, size_t lq) -> size_t {

        assert(lp + lq * max_p < max_p * max_q);

        return lp + lq * max_p;
    };


    size_t cur_p = p_arr[surfaceId];
    size_t cur_q = q_arr[surfaceId];

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }

    size_t local_id = get_local_index(p, q);
    gpmesh::GPPatchData &patchData = device_surface_patch_data_arr[surfaceId][local_id];


    gpmesh::real_t du = 1.0f / static_cast<gpmesh::real_t>(cur_p);
    gpmesh::real_t dv = 1.0f / static_cast<gpmesh::real_t>(cur_q);

    gpmesh::real_t u_begin = du * static_cast<gpmesh::real_t>(p);
    gpmesh::real_t u_step =  du / (N - 1);

    gpmesh::real_t v_begin = dv * static_cast<gpmesh::real_t>(q);
    gpmesh::real_t v_step =  dv / (M - 1);

    if (threadIdx.x >= N) {
        return;
    }

    if(threadIdx.x == 0)
    {
    }

    uint16_t id = threadIdx.x;

    gpmesh::real_t v = v_begin + v_step * id;

    const int m_p = 3;
    const int p1 = m_p + 1;

    gpmesh::real_t left[p1] = {0};
    gpmesh::real_t right[p1] = {0};

    gpmesh::real_t ndu[p1 * p1] = {0};
    gpmesh::real_t a[2 * p1] = {0};

    gpmesh::real_t* H = patchData.m_H_q_extended;
    gpmesh::real_t* Hprime = patchData.m_Hprime_q_extended;

    ndu[0] = static_cast<gpmesh::real_t>(1);

    for (int j = 1; j <= m_p; ++j) {
        left[j] = v - 0;
        right[j] = 1.0 - v;
        gpmesh::real_t saved = static_cast<gpmesh::real_t>(0);

        for (int r = 0; r < j; ++r) {
            ndu[j * p1 + r] = right[r + 1] + left[j - r];
            const gpmesh::real_t temp = ndu[r * p1 + j - 1] / ndu[j * p1 + r];
            ndu[r * p1 + j] = saved + right[r + 1] * temp;      
            saved = left[j - r] * temp;
        }

        ndu[j * p1 + j] = saved;
    }

    size_t start_id = 8 * id;
    for (int j = 0; j <= m_p; ++j) {
        assert((start_id + j) + 4 < 8 * N);
        H[start_id + j] = ndu[j * p1 + m_p];
    }

    const int n = 1;

    for (int r = 0; r <= m_p; ++r) {
        gpmesh::real_t *a1 = &a[0];
        gpmesh::real_t *a2 = &a[p1];

        a1[0] = static_cast<gpmesh::real_t>(1);

        for (int k = 1; k <= n; ++k) {
            int rk, pk, j1, j2;
            gpmesh::real_t d(0);
            rk = r - k;
            pk = m_p - k;

            if (r >= k) {
                a2[0] = a1[0] / ndu[(pk + 1) * p1 + rk];
                d = a2[0] * ndu[rk * p1 + pk];
            }

            j1 = (rk >= -1 ? 1 : -rk);
            j2 = (r - 1 <= pk ? k - 1 : m_p - r);

            for (int j = j1; j <= j2; j++) {
                a2[j] = (a1[j] - a1[j - 1]) / ndu[(pk + 1) * p1 + rk + j];
                d += a2[j] * ndu[(rk + j) * p1 + pk];
            }

            if (r <= pk) {
                a2[k] = -a1[k - 1] / ndu[(pk + 1) * p1 + r];
                d += a2[k] * ndu[r * p1 + pk];
            }

            assert(start_id + r + 4 < 8 * N);
            Hprime[start_id + r] = d * m_p;
        }
    }
}

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_HHprime_p_C_x(
                                             gpmesh::GPPatchData **device_surface_patch_data_arr,
                                             gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
                                             uint32_t num_surface,
                                             size_t* p_arr,
                                             size_t* q_arr,
                                             const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_HHprime_p_C_x;
    float* const a_ptr = patchData.m_HHprime_p;
    float* const b_ptr = surfaceCPointsExtended.m_cpoints_x;

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

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_HHprime_p_C_y(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_HHprime_p_C_y;
    float* const a_ptr = patchData.m_HHprime_p;
    float* const b_ptr = surfaceCPointsExtended.m_cpoints_y;

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

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_HHprime_p_C_z(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_HHprime_p_C_z;
    float* const a_ptr = patchData.m_HHprime_p;
    float* const b_ptr = surfaceCPointsExtended.m_cpoints_z;

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


template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_result_SSv_x(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_result_SSv_x;
    float* const a_ptr = patchData.m_HHprime_p_C_x;
    float* const b_ptr = patchData.m_H_q_extended;

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

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_result_SSv_y(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_result_SSv_y;
    float* const a_ptr = patchData.m_HHprime_p_C_y;
    float* const b_ptr = patchData.m_H_q_extended;

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

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_result_SSv_z(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_result_SSv_z;
    float* const a_ptr = patchData.m_HHprime_p_C_z;
    float* const b_ptr = patchData.m_H_q_extended;

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

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_result_Su0_x(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_result_Su0_x;
    float* const a_ptr = patchData.m_HHprime_p_C_x;
    float* const b_ptr = patchData.m_Hprime_q_extended;

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

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_result_Su0_y(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_result_Su0_y;
    float* const a_ptr = patchData.m_HHprime_p_C_y;
    float* const b_ptr = patchData.m_Hprime_q_extended;

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

template <size_t MAX_P, size_t MAX_Q, unsigned M, unsigned N, unsigned K,class T, class A_Layout, class B_Layout, class MEM_A_Layout, class MEM_B_Layout, class Policy>
__global__ void scene_evaluation_result_Su0_z(
        gpmesh::GPPatchData **device_surface_patch_data_arr,
        gpmesh::GPSurfaceControlPoint* device_surface_control_points_extended_arr,
        uint32_t num_surface,
        size_t* p_arr,
        size_t* q_arr,
        const nvcuda::wmma::layout_t c_layout)
{

    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];
    gpmesh::GPSurfaceControlPoint& surfaceCPointsExtended = device_surface_control_points_extended_arr[surface_id];

    float* const d_ptr = patchData.m_result_Su0_z;
    float* const a_ptr = patchData.m_HHprime_p_C_z;
    float* const b_ptr = patchData.m_Hprime_q_extended;

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


template<size_t MAX_P, size_t MAX_Q>
__global__ void scene_write_position(gpmesh::GPPatchData **device_surface_patch_data_arr,
                                     uint32_t num_surface,
                                     size_t* p_arr,
                                     size_t* q_arr)
{
    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];

    gpmesh::real_t* position_ptr = patchData.m_position_ptr;
    size_t offset = patchData.m_position_offset;
    size_t size = patchData.m_position_size;

    gpmesh::real_t* SSv_x = patchData.m_result_SSv_x;
    gpmesh::real_t* SSv_y = patchData.m_result_SSv_y;
    gpmesh::real_t* SSv_z = patchData.m_result_SSv_z;

    assert(offset % sizeof(gpmesh::real_t) == 0);
    assert(size % sizeof(gpmesh::real_t) == 0);

    size_t start_index = offset / sizeof(gpmesh::real_t);
    size_t length = size / sizeof(gpmesh::real_t);

    assert(length = 8 * 8 * 3);

    size_t limit_index = start_index + length;

    uint32_t ids[2] = {threadIdx.x, threadIdx.x + 32};

    for (int i = 0; i < 2; ++i)
    {
        uint32_t id = ids[i];

        if(id >= length) continue;

        uint32_t id_x = id * 3 + 0;
        uint32_t id_y = id * 3 + 1;
        uint32_t id_z = id * 3 + 2;
        assert(start_index + id_x < limit_index && id < 64);
        position_ptr[start_index + id_x] = SSv_x[id];
        assert(start_index + id_y < limit_index);
        position_ptr[start_index + id_y] = SSv_y[id];
        assert(start_index + id_z < limit_index);
        position_ptr[start_index + id_z] = SSv_z[id];
    }
}

template <typename real_t>
__device__ static void CrossProduct1D(real_t a[3] ,real_t b[3], real_t c[3])
{
    c[0] = a[1]*b[2]-a[2]*b[1];
    c[1] = a[2]*b[0]-a[0]*b[2];
    c[2] = a[0]*b[1]-a[1]*b[0];
}

template <typename real_t>
__device__ static void NormalizeInPlace(real_t norm[3])
{
    real_t length = sqrt(norm[0] * norm[0] + norm[1] * norm[1] + norm[2] * norm[2]);

    norm[0] /= length;
    norm[1] /= length;
    norm[2] /= length;
}

template<size_t MAX_P, size_t MAX_Q>
__global__ void scene_write_normal(gpmesh::GPPatchData **device_surface_patch_data_arr,
                                     uint32_t num_surface,
                                     size_t* p_arr,
                                     size_t* q_arr,
                                     bool bCCW = true)
{
    uint32_t surface_id = blockIdx.x;

    if(surface_id >= num_surface)
    {
        return;
    }

    size_t cur_p = p_arr[surface_id];
    size_t cur_q = q_arr[surface_id];

    uint32_t p = blockIdx.y;
    uint32_t q = blockIdx.z;

    if(p >= cur_p)
    {
        return;
    }

    if(q >= cur_q)
    {
        return;
    }


    size_t local_id = get_local_index<MAX_P,MAX_Q>(p, q);
    gpmesh::GPPatchData& patchData = device_surface_patch_data_arr[surface_id][local_id];

    gpmesh::real_t* normal_ptr = patchData.m_normal_ptr;
    size_t offset = patchData.m_normal_offset;
    size_t size = patchData.m_normal_size;

    gpmesh::real_t* SSv_x = patchData.m_result_SSv_x;
    gpmesh::real_t* SSv_y = patchData.m_result_SSv_y;
    gpmesh::real_t* SSv_z = patchData.m_result_SSv_z;

    gpmesh::real_t* Su0_x = patchData.m_result_Su0_x;
    gpmesh::real_t* Su0_y = patchData.m_result_Su0_y;
    gpmesh::real_t* Su0_z = patchData.m_result_Su0_z;

    assert(offset % sizeof(gpmesh::real_t) == 0);
    assert(size % sizeof(gpmesh::real_t) == 0);

    size_t start_index = offset / sizeof(gpmesh::real_t);
    size_t length = size / sizeof(gpmesh::real_t);

    assert(length = 8 * 8 * 3);

    size_t limit_index = start_index + length;

    uint32_t ids[2] = {threadIdx.x, threadIdx.x + 32};

    for (int i = 0; i < 2; ++i)
    {
        uint32_t id = ids[i];
        assert(id < 64);
        if(id >= length) continue;

        gpmesh::real_t dv[3] = { SSv_x[id + 64],
                         SSv_y[id + 64],
                         SSv_z[id + 64] };
        gpmesh::real_t du[3] = { Su0_x[id],
                         Su0_y[id],
                         Su0_z[id] };

        gpmesh::real_t norm[3];

        CrossProduct1D<gpmesh::real_t>(du, dv, norm);
        NormalizeInPlace<gpmesh::real_t>(norm);

        if(!bCCW)
        {
            norm[0] *= -1;
            norm[1] *= -1;
            norm[2] *= -1;
        }

        uint32_t id_x = id * 3 + 0;
        uint32_t id_y = id * 3 + 1;
        uint32_t id_z = id * 3 + 2;
        assert(start_index + id_z < limit_index);
        normal_ptr[start_index + id_x] = norm[0];
        normal_ptr[start_index + id_y] = norm[1];
        normal_ptr[start_index + id_z] = norm[2];
    }
}



