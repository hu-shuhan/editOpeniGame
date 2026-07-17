#pragma once

#include <assert.h>
#include <stdint.h>

#include "../iGameGP_Context.h"
#include "iGameGP_Collective.cuh"
#include "iGameGP_Loader.cuh"
#include "iGameGP_CUDA_Util.cuh"
#include "../iGameGP_Types.h"

namespace gpmesh {
namespace detail {

template <uint32_t rowOffset,
          uint32_t blockThreads,
          uint32_t itemPerThread = TRANSPOSE_ITEM_PER_THREAD>
__device__ __forceinline__ void block_mat_transpose(const uint32_t num_rows,
                                                    const uint32_t num_cols,
                                                    uint16_t*      mat,
                                                    uint16_t*      output,
                                                    int            shift = 0)
{
    uint16_t thread_data[itemPerThread];
    uint16_t local_offset[itemPerThread];
    uint32_t nnz = num_rows * rowOffset;

    for (uint32_t i = 0; i < itemPerThread; ++i) {
        uint32_t index = itemPerThread * threadIdx.x + i;
        if (index < nnz) {
            thread_data[i] = mat[index] >> shift;
            mat[index]     = 0;
        } else {
            thread_data[i] = INVALID16;
        }
    }

    uint32_t m = max(nnz, num_cols);
    __syncthreads();
    for (uint32_t i = threadIdx.x; i < m; i += blockThreads) {
        mat[i] = 0;
    }
    __syncthreads();

#if __CUDA_ARCH__ >= 700
    __half* mat_half = (__half*)(mat);
    for (uint32_t i = 0; i < itemPerThread; ++i) {
        if (thread_data[i] != INVALID16) {
            local_offset[i] = ::atomicAdd(&mat_half[thread_data[i]], 1);
        }
    }
    __syncthreads();
    for (uint32_t i = threadIdx.x; i < num_cols; i += blockThreads) {
        uint16_t val = uint16_t(mat_half[i]);
        mat[i]       = val;
    }
#else
    for (uint32_t i = 0; i < itemPerThread; ++i) {
        if (thread_data[i] != INVALID16) {
            local_offset[i] = atomicAdd(&mat[thread_data[i]], 1u);
        } else {
            break;
        }
    }
    __syncthreads();
#endif


    cub_block_exclusive_sum<uint16_t, blockThreads>(mat, num_cols);

    for (uint32_t i = 0; i < itemPerThread; ++i) {
        uint16_t item = thread_data[i];
        if (item != INVALID16) {
            uint16_t offset = mat[item] + local_offset[i];
            uint16_t row    = (itemPerThread * threadIdx.x + i) / rowOffset;
            output[offset]  = row;
        } else {
            break;
        }
    }
}

template <uint32_t blockThreads>
__device__ __forceinline__ void e_f_manifold(const uint16_t  num_edges,
                                             const uint16_t  num_faces,
                                             const uint16_t* s_fe,
                                             uint16_t*       s_ef)
{
    for (uint16_t e = threadIdx.x; e < 4 * num_faces; e += blockThreads) {
        uint16_t edge    = s_fe[e] >> 1;
        uint16_t face_id = e / 4;

        auto ret = atomicCAS(s_ef + 2 * edge, INVALID16, face_id);
        if (ret != INVALID16) {
            ret = atomicCAS(s_ef + 2 * edge + 1, INVALID16, face_id);
            assert(ret == INVALID16);
        }
    }
}

template <uint32_t blockThreads>
__device__ __forceinline__ void e_f_unclosed(const uint16_t  num_edges,
                                             const uint16_t  num_faces,
                                             const uint16_t* s_fe,
                                             uint16_t*       s_ef)
{
    for (uint16_t e = threadIdx.x; e < 4 * num_faces; e += blockThreads) {
        uint16_t edge    = s_fe[e] >> 1;
        uint16_t face_id = e / 4;

        auto ret = atomicCAS(s_ef + 2 * edge, INVALID16, face_id);
        if (ret != INVALID16) {
            ret = atomicCAS(s_ef + 2 * edge + 1, INVALID16, face_id);
            assert(ret == INVALID16);
        }
    }
}

template <uint32_t blockThreads>
__device__ __forceinline__ void load_inc(const uint16_t num_elements,
                                         const uint16_t* from,
                                         uint16_t* to)
{
    for (uint16_t e = threadIdx.x; e < num_elements; e += blockThreads)
    {
        to[e] = from[e] - e;
    }
}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_v_oreinted(const PatchInfo& patch_info,
                                             uint16_t*&       s_output_offset,
                                             uint16_t*&       s_output_value,
                                             uint16_t*        s_ev)
{

    const uint16_t num_edges          = patch_info.num_edges;
    const uint16_t num_faces          = patch_info.num_faces;
    const uint16_t num_vertices       = patch_info.num_vertices;
    const uint16_t num_owned_vertices = patch_info.num_owned_vertices;

    s_output_offset = &s_ev[0];
    s_output_value  = &s_ev[num_vertices + 1 + (num_vertices + 1) % 2];

    uint16_t*   s_fe    = &s_output_value[2 * num_edges];
    uint16_t*   s_ef    = &s_fe[4 * num_faces + (4 * num_faces) % 2];
    LocalEdgeT* temp_fe = reinterpret_cast<LocalEdgeT*>(s_fe);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.fe),
               num_faces * 4,
               reinterpret_cast<uint16_t*>(temp_fe),
               true);

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_ef[i] = INVALID16;
    }

    block_mat_transpose<2u, blockThreads>(
        num_edges, num_vertices, s_output_offset, s_output_value);

    e_f_manifold<blockThreads>(num_edges, num_faces, s_fe, s_ef);

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockDim.x) {

        uint16_t start = s_output_offset[v];
        uint16_t end   = s_output_offset[v + 1];


        for (uint16_t e_id = start; e_id < end - 1; ++e_id) {
            uint16_t e_0 = s_output_value[e_id];
            uint16_t f0(s_ef[2 * e_0]), f1(s_ef[2 * e_0 + 1]);

            assert(f0 != INVALID16 && f1 != INVALID16 && f0 < num_faces &&
                   f1 < num_faces);


            uint16_t e_candid_0, e_candid_1;

            if ((s_fe[4 * f0 + 0] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 3] >> 1;
            }
            if ((s_fe[4 * f0 + 1] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 0] >> 1;
            }
            if ((s_fe[4 * f0 + 2] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 1] >> 1;
            }
            if ((s_fe[4 * f0 + 3] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 2] >> 1;
            }


            if ((s_fe[4 * f1 + 0] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 3] >> 1;
            }
            if ((s_fe[4 * f1 + 1] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 0] >> 1;
            }
            if ((s_fe[4 * f1 + 2] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 1] >> 1;
            }
            if ((s_fe[4 * f1 + 3] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 2] >> 1;
            }

            for (uint16_t vn = e_id + 1; vn < end; ++vn) {
                uint16_t e_winning_candid = s_output_value[vn];
                if (e_candid_0 == e_winning_candid ||
                    e_candid_1 == e_winning_candid) {
                    uint16_t temp            = s_output_value[e_id + 1];
                    s_output_value[e_id + 1] = e_winning_candid;
                    s_output_value[vn]       = temp;
                    break;
                }
            }
        }
    }

    __syncthreads();

    s_ev                  = s_ef;
    LocalVertexT* temp_ev = reinterpret_cast<LocalVertexT*>(s_ef);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.ev),
               num_edges * 2,
               reinterpret_cast<uint16_t*>(temp_ev),
               true);

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {
            uint16_t edge = s_output_value[e];
            uint16_t v0   = s_ev[2 * edge];
            uint16_t v1   = s_ev[2 * edge + 1];

            assert(v0 == v || v1 == v);
            s_output_value[e] = (v0 == v) * v1 + (v1 == v) * v0;
        }
    }
}


template<uint32_t blockThreads>
__device__ void v_surround_v_ver2(const PatchInfo& patch_info,
                                  uint16_t*& s_output_offset,
                                  uint16_t*& s_output_value,
                                  uint16_t*& s_ev)
{
    const uint16_t num_edges          = patch_info.num_edges;
    const uint16_t num_faces          = patch_info.num_faces;
    const uint16_t num_vertices       = patch_info.num_vertices;
    const uint16_t num_owned_vertices = patch_info.num_owned_vertices;

    s_output_offset = &s_ev[0];
    s_output_value  = &s_ev[num_vertices + 1];

    uint16_t*   s_fe    = &s_output_value[2 * num_edges];
    uint16_t*   s_ef    = &s_fe[4 * num_faces + (4 * num_faces) % 2];
    LocalEdgeT* temp_fe = reinterpret_cast<LocalEdgeT*>(s_fe);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.fe),
               num_faces * 4,
               reinterpret_cast<uint16_t*>(temp_fe),
               true);

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_ef[i] = INVALID16;
    }

    block_mat_transpose<2u, blockThreads>(
        num_edges, num_vertices, s_output_offset, s_output_value);

    e_f_manifold<blockThreads>(num_edges, num_faces, s_fe, s_ef);

    __syncthreads();


    uint16_t* s_opposite_offset = &s_fe[4 * num_edges];

    load_async(s_output_offset, num_vertices+1, s_opposite_offset, false);

    uint16_t opposite_size = s_opposite_offset[num_vertices];
    assert(opposite_size == 2 * num_edges);

    uint16_t* s_opposite_value = &s_opposite_offset[num_vertices + 1 + (num_vertices + 1) % 2];

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_opposite_value[i] = INVALID16;
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockDim.x) {

        uint16_t start = s_output_offset[v];
        uint16_t end   = s_output_offset[v + 1];

        for (uint16_t e_id = start; e_id < end; ++e_id) {
            uint16_t e_0 = s_output_value[e_id];
            uint16_t f0(s_ef[2 * e_0]), f1(s_ef[2 * e_0 + 1]);

            assert(f0 != INVALID16 && f1 != INVALID16 && f0 < num_faces &&
                   f1 < num_faces);

            uint16_t e_candid_0, e_candid_1;
            uint16_t e_pre_candid_0, e_pre_candid_1;

            if ((s_fe[4 * f0 + 0] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 3] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 2] >> 1;
            }
            if ((s_fe[4 * f0 + 1] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 0] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 3] >> 1;
            }
            if ((s_fe[4 * f0 + 2] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 1] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 0] >> 1;
            }
            if ((s_fe[4 * f0 + 3] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 2] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 1] >> 1;
            }

            if ((s_fe[4 * f1 + 0] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 3] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 2] >> 1;
            }
            if ((s_fe[4 * f1 + 1] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 0] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 3] >> 1;
            }
            if ((s_fe[4 * f1 + 2] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 1] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 0] >> 1;
            }
            if ((s_fe[4 * f1 + 3] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 2] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 1] >> 1;
            }

            bool isFind = false;
            for (uint16_t vn = start; vn < end; ++vn) {

                uint16_t e_winning_candid = s_output_value[vn];

                if (e_candid_0 == e_winning_candid ||
                    e_candid_1 == e_winning_candid) {

                    isFind = true;

                    uint16_t e_prev_winning = (e_candid_0 == e_winning_candid) ? e_pre_candid_0 : e_pre_candid_1;
                    s_opposite_value[e_id] = e_prev_winning;

                    if(e_id + 1  == end) break;

                    uint16_t temp            = s_output_value[e_id + 1];
                    s_output_value[e_id + 1] = e_winning_candid;
                    s_output_value[vn]       = temp;
                    break;
                }
            }
            assert(isFind==true);
            assert(s_opposite_value[e_id] != INVALID16);
        }
    }

    __syncthreads();


    s_ev                  = s_ef;
    LocalVertexT* temp_ev = reinterpret_cast<LocalVertexT*>(s_ef);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.ev),
               num_edges * 2,
               reinterpret_cast<uint16_t*>(temp_ev),
               true);

    __syncthreads();


    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {

            uint32_t prev_e = (e == end - 1) ? start : e + 1;

            uint16_t prev_edge = s_output_value[prev_e];
            uint16_t pp_edge = s_opposite_value[e];

            assert(pp_edge != INVALID16);

            uint16_t p_v0 = s_ev[2 * prev_edge];
            uint16_t p_v1 = s_ev[2 * prev_edge+1];

            assert(p_v0 == v || p_v1 == v);

            uint16_t p_v = (p_v0 == v) * p_v1 + (p_v1 == v) * p_v0;

            uint16_t pp_v0 = s_ev[2 * pp_edge];
            uint16_t pp_v1 = s_ev[2 * pp_edge + 1];

            assert(pp_v0 == p_v || pp_v1 == p_v);

            uint16_t pp_v = (pp_v0 == p_v) * pp_v1 + (pp_v1 == p_v) * pp_v0;

            s_opposite_value[e] = pp_v;
        }
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {
            uint16_t edge = s_output_value[e];
            uint16_t v0   = s_ev[2 * edge];
            uint16_t v1   = s_ev[2 * edge + 1];

            assert(v0 == v || v1 == v);
            s_output_value[e] = (v0 == v) * v1 + (v1 == v) * v0;
        }
    }

    __syncthreads();


}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_surround_v_ver3(const PatchInfo& patch_info,
                                             uint16_t*& s_output_offset,
                                             uint16_t*& s_output_value,
                                             uint16_t*& s_ev)
{
    const uint16_t num_edges          = patch_info.num_edges;
    const uint16_t num_faces          = patch_info.num_faces;
    const uint16_t num_vertices       = patch_info.num_vertices;
    const uint16_t num_owned_vertices = patch_info.num_owned_vertices;

    s_output_offset = &s_ev[0];
    s_output_value  = &s_ev[num_vertices + 1];

    uint16_t*   s_fe    = &s_output_value[2 * num_edges];
    uint16_t*   s_ef    = &s_fe[4 * num_faces + (4 * num_faces) % 2];
    LocalEdgeT* temp_fe = reinterpret_cast<LocalEdgeT*>(s_fe);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.fe),
               num_faces * 4,
               reinterpret_cast<uint16_t*>(temp_fe),
               true);

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_ef[i] = INVALID16;
    }

    block_mat_transpose<2u, blockThreads>(
        num_edges, num_vertices, s_output_offset, s_output_value);

    e_f_manifold<blockThreads>(num_edges, num_faces, s_fe, s_ef);

    __syncthreads();


    uint16_t* s_opposite_offset = &s_fe[4 * num_edges];

    load_async(s_output_offset, num_vertices+1, s_opposite_offset, false);

    uint16_t opposite_size = s_opposite_offset[num_vertices];
    assert(opposite_size == 2 * num_edges);

    uint16_t* s_opposite_value = &s_opposite_offset[num_vertices + 1 + (num_vertices + 1) % 2];

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_opposite_value[i] = INVALID16;
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockDim.x) {

        uint16_t start = s_output_offset[v];
        uint16_t end   = s_output_offset[v + 1];

        for (uint16_t e_id = start; e_id < end; ++e_id) {
            uint16_t e_0 = s_output_value[e_id];
            uint16_t f0(s_ef[2 * e_0]), f1(s_ef[2 * e_0 + 1]);

            assert(f0 != INVALID16 && f1 != INVALID16 && f0 < num_faces &&
                   f1 < num_faces);

            uint16_t e_candid_0, e_candid_1;
            uint16_t e_pre_candid_0, e_pre_candid_1;

            if ((s_fe[4 * f0 + 0] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 3] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 2] >> 1;
            }
            if ((s_fe[4 * f0 + 1] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 0] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 3] >> 1;
            }
            if ((s_fe[4 * f0 + 2] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 1] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 0] >> 1;
            }
            if ((s_fe[4 * f0 + 3] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 2] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 1] >> 1;
            }

            if ((s_fe[4 * f1 + 0] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 3] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 2] >> 1;
            }
            if ((s_fe[4 * f1 + 1] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 0] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 3] >> 1;
            }
            if ((s_fe[4 * f1 + 2] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 1] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 0] >> 1;
            }
            if ((s_fe[4 * f1 + 3] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 2] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 1] >> 1;
            }

            bool isFind = false;
            for (uint16_t vn = start; vn < end; ++vn) {

                uint16_t e_winning_candid = s_output_value[vn];

                if (e_candid_0 == e_winning_candid ||
                    e_candid_1 == e_winning_candid) {

                    isFind = true;

                    uint16_t e_prev_winning = (e_candid_0 == e_winning_candid) ? e_pre_candid_0 : e_pre_candid_1;
                    s_opposite_value[e_id] = e_prev_winning;

                    if(e_id + 1  == end) break;

                    uint16_t temp            = s_output_value[e_id + 1];
                    s_output_value[e_id + 1] = e_winning_candid;
                    s_output_value[vn]       = temp;
                    break;
                }
            }
            assert(isFind==true);
            assert(s_opposite_value[e_id] != INVALID16);
        }
    }

    __syncthreads();


    __syncthreads();

    s_ev                  = s_ef;
    LocalVertexT* temp_ev = reinterpret_cast<LocalVertexT*>(s_ef);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.ev),
               num_edges * 2,
               reinterpret_cast<uint16_t*>(temp_ev),
               true);

    __syncthreads();


    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {

            uint32_t prev_e = (e == end - 1) ? start : e + 1;

            uint16_t prev_edge = s_output_value[prev_e];
            uint16_t pp_edge = s_opposite_value[e];

            assert(pp_edge != INVALID16);

            uint16_t p_v0 = s_ev[2 * prev_edge];
            uint16_t p_v1 = s_ev[2 * prev_edge+1];

            assert(p_v0 == v || p_v1 == v);

            uint16_t p_v = (p_v0 == v) * p_v1 + (p_v1 == v) * p_v0;

            uint16_t pp_v0 = s_ev[2 * pp_edge];
            uint16_t pp_v1 = s_ev[2 * pp_edge + 1];

            assert(pp_v0 == p_v || pp_v1 == p_v);

            uint16_t pp_v = (pp_v0 == p_v) * pp_v1 + (pp_v1 == p_v) * pp_v0;

            s_opposite_value[e] = pp_v;
        }
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {
            uint16_t edge = s_output_value[e];
            uint16_t v0   = s_ev[2 * edge];
            uint16_t v1   = s_ev[2 * edge + 1];

            assert(v0 == v || v1 == v);
            s_output_value[e] = (v0 == v) * v1 + (v1 == v) * v0;
        }
    }

    __syncthreads();

    uint16_t* s_new_output_value = &s_output_value[2 * num_edges];

    for (uint32_t i = threadIdx.x; i < 4 * num_edges; i += blockThreads)
    {
        s_new_output_value[i] = INVALID16;
    }

    __syncthreads();

    assert(s_opposite_offset[0] != INVALID16);

    for (uint32_t i = threadIdx.x; i <= num_vertices; i += blockThreads)
    {
        s_output_offset[i] = 2 * s_output_offset[i];
    }

    for (uint32_t i = threadIdx.x; i < num_owned_vertices; i += blockThreads)
    {
        uint32_t new_start = s_output_offset[i];
        uint32_t new_end = s_output_offset[i+1];

        uint32_t old_start =  s_opposite_offset[i];
        uint32_t old_end = s_opposite_offset[i+1];

        assert(new_end - new_start == 2 * (old_end - old_start));

        uint32_t iter = new_start;

        for (uint32_t e = old_start; e < old_end; ++e)
        {
            s_new_output_value[iter] = s_output_value[e];
            iter++;
            s_new_output_value[iter] = s_opposite_value[e];
            iter++;
        }

        assert(iter == new_end);
    }

    __syncthreads();

    uint16_t last_index = s_output_offset[num_owned_vertices - 1];
    uint16_t last_tmp = s_new_output_value[last_index];

    assert(s_opposite_offset[0] != INVALID16);


    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[i] = INVALID16;
    }

    assert(s_new_output_value[0] != INVALID16);
    __syncthreads();

    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[i] = s_new_output_value[i];
    }

    __syncthreads();

    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[2 * num_edges + i] = s_new_output_value[2 * num_edges + i];
    }

    __syncthreads();

    assert(last_tmp == s_output_value[last_index]);
}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_surround_v_ver4(const PatchInfo& patch_info,
                                                  uint16_t*& s_output_offset,
                                                  uint16_t*& s_output_value,
                                                  uint16_t*& s_ev)
{
    const uint16_t num_edges          = patch_info.num_edges;
    const uint16_t num_faces          = patch_info.num_faces;
    const uint16_t num_vertices       = patch_info.num_vertices;
    const uint16_t num_owned_vertices = patch_info.num_owned_vertices;

    s_output_offset = &s_ev[0];
    s_output_value  = &s_ev[num_vertices + 1];

    uint16_t*   s_fe    = &s_output_value[2 * num_edges];
    uint16_t*   s_ef    = &s_fe[4 * num_faces + (4 * num_faces) % 2];
    LocalEdgeT* temp_fe = reinterpret_cast<LocalEdgeT*>(s_fe);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.fe),
               num_faces * 4,
               reinterpret_cast<uint16_t*>(temp_fe),
               true);

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_ef[i] = INVALID16;
    }

    block_mat_transpose<2u, blockThreads>(
        num_edges, num_vertices, s_output_offset, s_output_value);

    e_f_manifold<blockThreads>(num_edges, num_faces, s_fe, s_ef);

    __syncthreads();


    uint16_t* s_opposite_offset = &s_fe[4 * num_edges];

    load_async(s_output_offset, num_vertices+1, s_opposite_offset, false);

    uint16_t opposite_size = s_opposite_offset[num_vertices];
    assert(opposite_size == 2 * num_edges);

    uint16_t* s_opposite_value = &s_opposite_offset[num_vertices + 1 + (num_vertices + 1) % 2];

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_opposite_value[i] = INVALID16;
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockDim.x) {

        uint16_t start = s_output_offset[v];
        uint16_t end   = s_output_offset[v + 1];

        for (uint16_t e_id = start; e_id < end; ++e_id) {
            uint16_t e_0 = s_output_value[e_id];
            uint16_t f0(s_ef[2 * e_0]), f1(s_ef[2 * e_0 + 1]);

            assert(f0 != INVALID16 && f1 != INVALID16 && f0 < num_faces &&
                   f1 < num_faces);

            uint16_t e_candid_0, e_candid_1;
            uint16_t e_pre_candid_0, e_pre_candid_1;

            if ((s_fe[4 * f0 + 0] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 3] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 2] >> 1;
            }
            if ((s_fe[4 * f0 + 1] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 0] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 3] >> 1;
            }
            if ((s_fe[4 * f0 + 2] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 1] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 0] >> 1;
            }
            if ((s_fe[4 * f0 + 3] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 2] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 1] >> 1;
            }

            if ((s_fe[4 * f1 + 0] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 3] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 2] >> 1;
            }
            if ((s_fe[4 * f1 + 1] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 0] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 3] >> 1;
            }
            if ((s_fe[4 * f1 + 2] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 1] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 0] >> 1;
            }
            if ((s_fe[4 * f1 + 3] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 2] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 1] >> 1;
            }

            bool isFind = false;
            for (uint16_t vn = start; vn < end; ++vn) {

                uint16_t e_winning_candid = s_output_value[vn];

                if (e_candid_0 == e_winning_candid ||
                    e_candid_1 == e_winning_candid) {

                    isFind = true;

                    uint16_t e_prev_winning = (e_candid_0 == e_winning_candid) ? e_pre_candid_0 : e_pre_candid_1;
                    s_opposite_value[e_id] = e_prev_winning;

                    uint16_t exchange_pos = (e_id + 1 == end) ? vn : e_id + 1;

                    uint16_t temp            = s_output_value[exchange_pos];
                    s_output_value[exchange_pos] = e_winning_candid;
                    s_output_value[vn]       = temp;
                    break;

                }
            }
            assert(isFind==true);
            assert(s_opposite_value[e_id] != INVALID16);
        }
    }

    __syncthreads();

    s_ev                  = s_ef;
    LocalVertexT* temp_ev = reinterpret_cast<LocalVertexT*>(s_ef);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.ev),
               num_edges * 2,
               reinterpret_cast<uint16_t*>(temp_ev),
               true);

    __syncthreads();


    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {

            uint32_t prev_e = (e == end - 1) ? start : e + 1;

            uint16_t prev_edge = s_output_value[prev_e];
            uint16_t pp_edge = s_opposite_value[e];

            assert(pp_edge != INVALID16);

            uint16_t p_v0 = s_ev[2 * prev_edge];
            uint16_t p_v1 = s_ev[2 * prev_edge+1];

            assert(p_v0 == v || p_v1 == v);

            uint16_t p_v = (p_v0 == v) * p_v1 + (p_v1 == v) * p_v0;

            uint16_t pp_v0 = s_ev[2 * pp_edge];
            uint16_t pp_v1 = s_ev[2 * pp_edge + 1];

            assert(pp_v0 == p_v || pp_v1 == p_v);

            uint16_t pp_v = (pp_v0 == p_v) * pp_v1 + (pp_v1 == p_v) * pp_v0;

            s_opposite_value[e] = pp_v;
        }
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {
            uint16_t edge = s_output_value[e];
            uint16_t v0   = s_ev[2 * edge];
            uint16_t v1   = s_ev[2 * edge + 1];

            assert(v0 == v || v1 == v);
            s_output_value[e] = (v0 == v) * v1 + (v1 == v) * v0;
        }
    }

    __syncthreads();

    uint16_t* s_new_output_value = &s_output_value[2 * num_edges];

    for (uint32_t i = threadIdx.x; i < 4 * num_edges; i += blockThreads)
    {
        s_new_output_value[i] = INVALID16;
    }

    __syncthreads();

    assert(s_opposite_offset[0] != INVALID16);

    for (uint32_t i = threadIdx.x; i <= num_vertices; i += blockThreads)
    {
        s_output_offset[i] = 2 * s_output_offset[i];
    }

    for (uint32_t i = threadIdx.x; i < num_owned_vertices; i += blockThreads)
    {
        uint32_t new_start = s_output_offset[i];
        uint32_t new_end = s_output_offset[i+1];

        uint32_t old_start =  s_opposite_offset[i];
        uint32_t old_end = s_opposite_offset[i+1];

        assert(new_end - new_start == 2 * (old_end - old_start));

        uint32_t iter = new_start;

        for (uint32_t e = old_start; e < old_end; ++e)
        {
            s_new_output_value[iter] = s_output_value[e];
            iter++;
            s_new_output_value[iter] = s_opposite_value[e];
            iter++;
        }

        assert(iter == new_end);
    }

    __syncthreads();

    uint16_t last_index = s_output_offset[num_owned_vertices - 1];
    uint16_t last_tmp = s_new_output_value[last_index];

    assert(s_opposite_offset[0] != INVALID16);

    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[i] = INVALID16;
    }

    assert(s_new_output_value[0] != INVALID16);
    __syncthreads();

    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[i] = s_new_output_value[i];
    }

    __syncthreads();

    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[2 * num_edges + i] = s_new_output_value[2 * num_edges + i];
    }
    __syncthreads();

    assert(last_tmp == s_output_value[last_index]);
}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_surround_v_ver5(const PatchInfo& patch_info,
                                                  uint16_t*& s_output_offset,
                                                  uint16_t*& s_output_value,
                                                  uint16_t*& s_ev)
{
    const uint16_t num_edges          = patch_info.num_edges;
    const uint16_t num_faces          = patch_info.num_faces;
    const uint16_t num_vertices       = patch_info.num_vertices;
    const uint16_t num_owned_vertices = patch_info.num_owned_vertices;

    s_output_offset = &s_ev[0];
    assert(s_output_offset == s_ev);
    uint16_t*   s_fe    = &s_output_offset[num_vertices+1];
    uint16_t*   s_ef    = &s_fe[4 * num_faces];

    assert(2 * num_edges >= 4 * num_faces);

    s_output_value  = &s_fe[4 * num_edges];

    block_mat_transpose<2u, blockThreads>(
        num_edges, num_vertices, s_output_offset, s_output_value);

    __syncthreads();

    LocalEdgeT* temp_fe = reinterpret_cast<LocalEdgeT*>(s_fe);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.fe),
               num_faces * 4,
               reinterpret_cast<uint16_t*>(temp_fe),
               true);

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_ef[i] = INVALID16;
    }

    e_f_manifold<blockThreads>(num_edges, num_faces, s_fe, s_ef);

    __syncthreads();

    uint16_t* s_opposite_offset = &s_output_value[2 * num_edges];

    load_async(s_output_offset, num_vertices+1, s_opposite_offset, false);

    uint16_t* s_opposite_value = &s_opposite_offset[num_vertices + 1];

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_opposite_value[i] = INVALID16;
    }

    uint16_t opposite_size = s_opposite_offset[num_vertices];
    assert(opposite_size == 2 * num_edges);

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockDim.x) {

        uint16_t start = s_output_offset[v];
        uint16_t end   = s_output_offset[v + 1];

        for (uint16_t e_id = start; e_id < end; ++e_id) {
            uint16_t e_0 = s_output_value[e_id];
            uint16_t f0(s_ef[2 * e_0]), f1(s_ef[2 * e_0 + 1]);

            assert(f0 != INVALID16 && f1 != INVALID16 && f0 < num_faces &&
                   f1 < num_faces);

            uint16_t e_candid_0, e_candid_1;
            uint16_t e_pre_candid_0, e_pre_candid_1;

            if ((s_fe[4 * f0 + 0] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 3] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 2] >> 1;
            }
            if ((s_fe[4 * f0 + 1] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 0] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 3] >> 1;
            }
            if ((s_fe[4 * f0 + 2] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 1] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 0] >> 1;
            }
            if ((s_fe[4 * f0 + 3] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 2] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 1] >> 1;
            }

            if ((s_fe[4 * f1 + 0] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 3] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 2] >> 1;
            }
            if ((s_fe[4 * f1 + 1] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 0] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 3] >> 1;
            }
            if ((s_fe[4 * f1 + 2] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 1] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 0] >> 1;
            }
            if ((s_fe[4 * f1 + 3] >> 1) == e_0) {
                e_candid_1 = s_fe[4 * f1 + 2] >> 1;
                e_pre_candid_1 = s_fe[4 * f1 + 1] >> 1;
            }

            bool isFind = false;
            for (uint16_t vn = start; vn < end; ++vn) {

                uint16_t e_winning_candid = s_output_value[vn];

                if (e_candid_0 == e_winning_candid ||
                    e_candid_1 == e_winning_candid) {

                    isFind = true;

                    uint16_t e_prev_winning = (e_candid_0 == e_winning_candid) ? e_pre_candid_0 : e_pre_candid_1;
                    s_opposite_value[e_id] = e_prev_winning;

                    uint16_t exchange_pos = (e_id + 1 == end) ? vn : e_id + 1;

                    uint16_t temp            = s_output_value[exchange_pos];
                    s_output_value[exchange_pos] = e_winning_candid;
                    s_output_value[vn]       = temp;
                    break;

                }
            }
            assert(isFind==true);
            assert(s_opposite_value[e_id] != INVALID16);
        }
    }

    __syncthreads();

    s_ev                  = s_ef;
    LocalVertexT* temp_ev = reinterpret_cast<LocalVertexT*>(s_ef);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.ev),
               num_edges * 2,
               reinterpret_cast<uint16_t*>(temp_ev),
               true);

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {

            uint32_t prev_e = (e == end - 1) ? start : e + 1;

            uint16_t prev_edge = s_output_value[prev_e];
            uint16_t pp_edge = s_opposite_value[e];

            assert(pp_edge != INVALID16);

            uint16_t p_v0 = s_ev[2 * prev_edge];
            uint16_t p_v1 = s_ev[2 * prev_edge+1];

            assert(p_v0 == v || p_v1 == v);

            uint16_t p_v = (p_v0 == v) * p_v1 + (p_v1 == v) * p_v0;

            uint16_t pp_v0 = s_ev[2 * pp_edge];
            uint16_t pp_v1 = s_ev[2 * pp_edge + 1];

            assert(pp_v0 == p_v || pp_v1 == p_v);

            uint16_t pp_v = (pp_v0 == p_v) * pp_v1 + (pp_v1 == p_v) * pp_v0;

            s_opposite_value[e] = pp_v;
        }
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {
            uint16_t edge = s_output_value[e];
            uint16_t v0   = s_ev[2 * edge];
            uint16_t v1   = s_ev[2 * edge + 1];

            assert(v0 == v || v1 == v);
            s_output_value[e] = (v0 == v) * v1 + (v1 == v) * v0;
        }
    }

    __syncthreads();

    uint16_t* s_new_output_value = s_fe;

    for (uint32_t i = threadIdx.x; i < 4 * num_edges; i += blockThreads)
    {
        s_new_output_value[i] = INVALID16;
    }

    __syncthreads();

    assert(s_opposite_offset[0] != INVALID16);

    for (uint32_t i = threadIdx.x; i <= num_vertices; i += blockThreads)
    {
        s_output_offset[i] = 2 * s_output_offset[i];
    }

    for (uint32_t i = threadIdx.x; i < num_owned_vertices; i += blockThreads)
    {
        uint32_t new_start = s_output_offset[i];
        uint32_t new_end = s_output_offset[i+1];

        uint32_t old_start =  s_opposite_offset[i];
        uint32_t old_end = s_opposite_offset[i+1];

        assert(new_end - new_start == 2 * (old_end - old_start));

        uint32_t iter = new_start;

        for (uint32_t e = old_start; e < old_end; ++e)
        {
            s_new_output_value[iter] = s_output_value[e];
            iter++;
            s_new_output_value[iter] = s_opposite_value[e];
            iter++;
        }

        assert(iter == new_end);
    }

    __syncthreads();

    s_output_value = s_new_output_value;
}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_surround_v(const PatchInfo& patch_info,
                                             uint16_t*& s_output_offset,
                                             uint16_t*& s_output_value,
                                             uint16_t*& s_ev)
{
    v_surround_v_ver5<blockThreads>(patch_info,
                      s_output_offset,
                      s_output_value,
                      s_ev);
}


template<uint32_t blockThreads>
__device__ __forceinline__ void v_v_surround_unclosed_ver0(const PatchInfo& patch_info,
                                                      uint16_t*& s_output_offset,
                                                      uint16_t*& s_output_value,
                                                      uint16_t*& s_ev)
{
    const uint16_t num_edges          = patch_info.num_edges;
    const uint16_t num_faces          = patch_info.num_faces;
    const uint16_t num_vertices       = patch_info.num_vertices;
    const uint16_t num_owned_vertices = patch_info.num_owned_vertices;

    s_output_offset = &s_ev[0];
    s_output_value  = &s_ev[num_vertices + 1];

    uint16_t*   s_fe    = &s_output_value[2 * num_edges];
    uint16_t*   s_ef    = &s_fe[4 * num_faces];
    LocalEdgeT* temp_fe = reinterpret_cast<LocalEdgeT*>(s_fe);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.fe),
               num_faces * 4,
               reinterpret_cast<uint16_t*>(temp_fe),
               true);


    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_ef[i] = INVALID16;
    }

    block_mat_transpose<2u, blockThreads>(
        num_edges, num_vertices, s_output_offset, s_output_value);

    e_f_manifold<blockThreads>(num_edges, num_faces, s_fe, s_ef);

    __syncthreads();


    uint16_t* s_opposite_offset = &s_fe[4 * num_edges];

    load_async(s_output_offset, num_vertices+1, s_opposite_offset, false);

    uint16_t opposite_size = s_opposite_offset[num_vertices];
    assert(opposite_size == 2 * num_edges);

    uint16_t* s_opposite_value = &s_opposite_offset[num_vertices + 1 + (num_vertices + 1) % 2];

    for (uint32_t i = threadIdx.x; i < num_edges * 2; i += blockThreads) {
        s_opposite_value[i] = INVALID16;
    }

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockDim.x) {

        uint16_t start = s_output_offset[v];
        uint16_t end   = s_output_offset[v + 1];


        uint16_t count = 0;

        uint16_t e_bd_start_id = INVALID16;
        uint16_t e_bd_end_id   = INVALID16;

        for (uint16_t e_id = start; e_id < end; ++e_id) {

            uint16_t e_0 = s_output_value[e_id];
            uint16_t f0(s_ef[2 * e_0]), f1(s_ef[2 * e_0 + 1]);

            if (f0 == INVALID16 || f1 == INVALID16) {
                count++;
                uint16_t f = (f0 == INVALID16) * f1 + (f1 == INVALID16) * f0;

                assert(f == f0);

                uint16_t e_candid;

                if ((s_fe[4 * f + 0] >> 1) == e_0) {
                    e_candid = s_fe[4 * f + 3] >> 1;
                }
                if ((s_fe[4 * f + 1] >> 1) == e_0) {
                    e_candid = s_fe[4 * f + 0] >> 1;
                }
                if ((s_fe[4 * f + 2] >> 1) == e_0) {
                    e_candid = s_fe[4 * f + 1] >> 1;
                }
                if ((s_fe[4 * f + 3] >> 1) == e_0) {
                    e_candid = s_fe[4 * f + 2] >> 1;
                }

                bool isFind = false;

                for (uint16_t search_id = start; search_id < end; ++search_id) {
                    if (e_candid == s_output_value[search_id]) {
                        isFind = true;
                        break;
                    }
                }

                if (isFind) {
                    e_bd_start_id = e_id;
                } else {
                    e_bd_end_id = e_id;
                }
            }
        }

        assert(count == 0 || count == 2);


        assert(count != 2 || e_bd_end_id != e_bd_start_id);
        assert(count != 2 || e_bd_start_id != INVALID16);
        assert(count != 2 || e_bd_end_id != INVALID16);


        e_bd_start_id = (count == 2) ? e_bd_start_id : start;
        e_bd_end_id = (count == 2) ? e_bd_end_id : end - 1;

        {
            uint16_t tmp = s_output_value[start];
            uint16_t tmp_bd_end = s_output_value[e_bd_end_id];

            s_output_value[start] = s_output_value[e_bd_start_id];
            s_output_value[e_bd_start_id] = tmp;

            tmp = s_output_value[end - 1];
            if(tmp != tmp_bd_end)
            {
                s_output_value[end - 1] = s_output_value[e_bd_end_id];
                s_output_value[e_bd_end_id] = tmp;
            }

        }

        if(count == 2)
        {
            uint16_t e_0 = s_output_value[start];

            uint16_t f0 = s_ef[2 * e_0];

            assert(s_ef[2 * e_0 + 1] == INVALID16);

            uint16_t e_candid_0;
            uint16_t e_pre_candid_0;

            if ((s_fe[4 * f0 + 0] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 3] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 2] >> 1;
            }
            if ((s_fe[4 * f0 + 1] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 0] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 3] >> 1;
            }
            if ((s_fe[4 * f0 + 2] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 1] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 0] >> 1;
            }
            if ((s_fe[4 * f0 + 3] >> 1) == e_0) {
                e_candid_0 = s_fe[4 * f0 + 2] >> 1;
                e_pre_candid_0 = s_fe[4 * f0 + 1] >> 1;
            }

            bool isFind = false;
            for (uint16_t vn = start + 1; vn < end; ++vn)
            {
                uint16_t e_winning_candid = s_output_value[vn];
                if (e_candid_0 == e_winning_candid) {
                    isFind = true;
                    uint16_t temp            = s_output_value[start + 1];
                    s_output_value[start + 1] = e_winning_candid;
                    s_output_value[vn]       = temp;
                    break;
                }
            }
            assert(isFind);
            s_opposite_value[start] = e_pre_candid_0;
        }

        uint16_t iter_start = (count == 2) * 1 + start;
        uint16_t iter_end = (count == 2) * (-1) + end;

        {
            for (uint16_t e_id = iter_start; e_id < iter_end; ++e_id)
            {
                uint16_t e_0 = s_output_value[e_id];
                uint16_t f0(s_ef[2 * e_0]), f1(s_ef[2 * e_0 + 1]);

                assert(f0 != INVALID16 && f1 != INVALID16 && f0 < num_faces &&
                       f1 < num_faces);

                uint16_t e_candid_0, e_candid_1;
                uint16_t e_pre_candid_0, e_pre_candid_1;

                if ((s_fe[4 * f0 + 0] >> 1) == e_0) {
                    e_candid_0 = s_fe[4 * f0 + 3] >> 1;
                    e_pre_candid_0 = s_fe[4 * f0 + 2] >> 1;
                }
                if ((s_fe[4 * f0 + 1] >> 1) == e_0) {
                    e_candid_0 = s_fe[4 * f0 + 0] >> 1;
                    e_pre_candid_0 = s_fe[4 * f0 + 3] >> 1;
                }
                if ((s_fe[4 * f0 + 2] >> 1) == e_0) {
                    e_candid_0 = s_fe[4 * f0 + 1] >> 1;
                    e_pre_candid_0 = s_fe[4 * f0 + 0] >> 1;
                }
                if ((s_fe[4 * f0 + 3] >> 1) == e_0) {
                    e_candid_0 = s_fe[4 * f0 + 2] >> 1;
                    e_pre_candid_0 = s_fe[4 * f0 + 1] >> 1;
                }

                if ((s_fe[4 * f1 + 0] >> 1) == e_0) {
                    e_candid_1 = s_fe[4 * f1 + 3] >> 1;
                    e_pre_candid_1 = s_fe[4 * f1 + 2] >> 1;
                }
                if ((s_fe[4 * f1 + 1] >> 1) == e_0) {
                    e_candid_1 = s_fe[4 * f1 + 0] >> 1;
                    e_pre_candid_1 = s_fe[4 * f1 + 3] >> 1;
                }
                if ((s_fe[4 * f1 + 2] >> 1) == e_0) {
                    e_candid_1 = s_fe[4 * f1 + 1] >> 1;
                    e_pre_candid_1 = s_fe[4 * f1 + 0] >> 1;
                }
                if ((s_fe[4 * f1 + 3] >> 1) == e_0) {
                    e_candid_1 = s_fe[4 * f1 + 2] >> 1;
                    e_pre_candid_1 = s_fe[4 * f1 + 1] >> 1;
                }

                bool isFind = false;
                for (uint16_t vn = start; vn < end; ++vn) {

                    uint16_t e_winning_candid = s_output_value[vn];

                    if (e_candid_0 == e_winning_candid ||
                        e_candid_1 == e_winning_candid) {

                        isFind = true;

                        uint16_t e_prev_winning = (e_candid_0 == e_winning_candid) ? e_pre_candid_0 : e_pre_candid_1;

                        s_opposite_value[e_id] = e_prev_winning;

                        uint16_t exchange_pos = (e_id + 1 == end) ? vn : e_id + 1;

                        uint16_t temp            = s_output_value[exchange_pos];
                        s_output_value[exchange_pos] = e_winning_candid;
                        s_output_value[vn]       = temp;
                        break;

                    }
                }
                assert(isFind==true);
                assert(s_opposite_value[e_id] != INVALID16);
            }
        }

        if (count == 2)
        {
            s_opposite_value[end-1] = INVALID16;
        }
    }

    __syncthreads();

    s_ev                  = s_ef;
    LocalVertexT* temp_ev = reinterpret_cast<LocalVertexT*>(s_ef);
    load_async(reinterpret_cast<const uint16_t*>(patch_info.ev),
               num_edges * 2,
               reinterpret_cast<uint16_t*>(temp_ev),
               true);

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_owned_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e_id = start; e_id < end; ++e_id) {

            uint16_t pp_edge = s_opposite_value[e_id];

            if(pp_edge == INVALID16)
            {
                assert(e_id == end - 1);

                break;
            }

            uint32_t prev_e_id = (e_id + 1 == end) ? start : e_id + 1;

            uint16_t prev_edge = s_output_value[prev_e_id];

            assert(pp_edge != INVALID16);
            assert(prev_edge != INVALID16);

            uint16_t p_v0 = s_ev[2 * prev_edge];
            uint16_t p_v1 = s_ev[2 * prev_edge+1];

            assert(p_v0 == v || p_v1 == v);

            uint16_t p_v = (p_v0 == v) * p_v1 + (p_v1 == v) * p_v0;

            uint16_t pp_v0 = s_ev[2 * pp_edge];
            uint16_t pp_v1 = s_ev[2 * pp_edge + 1];

            assert(pp_v0 == p_v || pp_v1 == p_v);

            uint16_t pp_v = (pp_v0 == p_v) * pp_v1 + (pp_v1 == p_v) * pp_v0;

            s_opposite_value[e_id] = pp_v;
        }
    }

    __syncthreads();


    for (uint32_t v = threadIdx.x; v < num_vertices; v += blockThreads) {
        uint32_t start = s_output_offset[v];
        uint32_t end   = s_output_offset[v + 1];

        for (uint32_t e = start; e < end; ++e) {
            uint16_t edge = s_output_value[e];
            uint16_t v0   = s_ev[2 * edge];
            uint16_t v1   = s_ev[2 * edge + 1];

            assert(v0 == v || v1 == v);
            s_output_value[e] = (v0 == v) * v1 + (v1 == v) * v0;
        }
    }

    __syncthreads();

    uint16_t* s_new_output_value = &s_output_value[2 * num_edges];

    for (uint32_t i = threadIdx.x; i < 4 * num_edges; i += blockThreads)
    {
        s_new_output_value[i] = INVALID16;
    }

    __syncthreads();

    assert(s_opposite_offset[0] != INVALID16);

    for (uint32_t i = threadIdx.x; i <= num_vertices; i += blockThreads)
    {
        s_output_offset[i] = 2 * s_output_offset[i];
    }

    for (uint32_t i = threadIdx.x; i < num_owned_vertices; i += blockThreads)
    {
        uint32_t new_start = s_output_offset[i];
        uint32_t new_end = s_output_offset[i+1];

        uint32_t old_start =  s_opposite_offset[i];
        uint32_t old_end = s_opposite_offset[i+1];

        assert(new_end - new_start == 2 * (old_end - old_start));

        uint32_t iter = new_start;

        for (uint32_t e = old_start; e < old_end; ++e)
        {
            s_new_output_value[iter] = s_output_value[e];
            iter++;
            s_new_output_value[iter] = s_opposite_value[e];
            iter++;
        }

        assert(iter == new_end);
    }

    __syncthreads();

    uint16_t last_index = s_output_offset[num_owned_vertices - 1];
    uint16_t last_tmp = s_new_output_value[last_index];

    assert(s_opposite_offset[0] != INVALID16);


    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[i] = INVALID16;
    }

    assert(s_new_output_value[0] != INVALID16);
    __syncthreads();

    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[i] = s_new_output_value[i];
    }

    __syncthreads();

    for (uint32_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads)
    {
        s_output_value[2 * num_edges + i] = s_new_output_value[2 * num_edges + i];
    }
    __syncthreads();
}

template<uint32_t blockThreads>
__device__ __forceinline__ void v_v_surround_unclosed(const PatchInfo& patch_info,
                                                      uint16_t*& s_output_offset,
                                                      uint16_t*& s_output_value,
                                                      uint16_t*& s_ev)
{
    v_v_surround_unclosed_ver0<blockThreads>(patch_info,
                               s_output_offset,
                               s_output_value,
                               s_ev);
}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_e(const uint16_t num_vertices,
                                    const uint16_t num_edges,
                                    uint16_t*      d_edges,
                                    uint16_t*      d_output)
{
    block_mat_transpose<2u, blockThreads>(
        num_edges, num_vertices, d_edges, d_output);
}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_v(const uint16_t num_vertices,
                                    const uint16_t num_edges,
                                    uint16_t*      d_edges,
                                    uint16_t*      d_output)
{
    uint16_t* s_edges_duplicate = &d_edges[2 * 2 * num_edges];

    assert(2 * 2 * num_edges >= num_vertices + 1 + 2 * num_edges);

    for (uint16_t i = threadIdx.x; i < 2 * num_edges; i += blockThreads) {
        s_edges_duplicate[i] = d_edges[i];
    }

    __syncthreads();

    v_e<blockThreads>(num_vertices, num_edges, d_edges, d_output);

    __syncthreads();

    for (uint32_t v = threadIdx.x; v < num_vertices; v += blockThreads) {
        uint32_t start = d_edges[v];
        uint32_t end   = d_edges[v + 1];

        for (uint32_t e = start; e < end; ++e) {
            uint16_t edge = d_output[e];
            uint16_t v0   = s_edges_duplicate[2 * edge];
            uint16_t v1   = s_edges_duplicate[2 * edge + 1];

            assert(v0 == v || v1 == v);
            d_output[e] = (v0 == v) * v1 + (v1 == v) * v0;
        }
    }
}

template <uint32_t blockThreads>
__device__ __forceinline__ void f_v(const uint16_t  num_edges,
                                    const uint16_t* d_edges,
                                    const uint16_t  num_faces,
                                    uint16_t*       d_faces)
{
    for (uint32_t f = threadIdx.x; f < num_faces; f += blockThreads) {
        uint16_t f_v[4];
        uint32_t f_id = 4 * f;
        for (uint32_t i = 0; i < 4; i++) {
            uint16_t e = d_faces[f_id + i];
            flag_t   e_dir(0);
            Context::unpack_edge_dir(e, e, e_dir);
            uint16_t e_id = (2 * e) + (1 * e_dir);
            if( e_id >= 2*num_edges)
            {
                printf("CUDA Debug: num_edges=%d, e_id=%d\n", num_edges, e_id);
            }
            assert(e_id < 2 * num_edges);
            f_v[i] = d_edges[e_id];
        }
        for (uint32_t i = 0; i < 4; i++) {
            d_faces[f * 4 + i] = f_v[i];
        }
    }
}

template <uint32_t blockThreads>
__device__ __forceinline__ void v_f(const uint16_t num_faces,
                                    const uint16_t num_edges,
                                    const uint16_t num_vertices,
                                    uint16_t*      d_edges,
                                    uint16_t*      d_faces)
{
    f_v<blockThreads>(num_edges, d_edges, num_faces, d_faces);
    __syncthreads();

    block_mat_transpose<3u, blockThreads>(
        num_faces, num_vertices, d_faces, d_edges);
}

template <uint32_t blockThreads>
__device__ __forceinline__ void e_f(const uint16_t num_edges,
                                    const uint16_t num_faces,
                                    uint16_t*      d_faces,
                                    uint16_t*      d_output,
                                    int            shift = 1)
{
    block_mat_transpose<3u, blockThreads>(
        num_faces, num_edges, d_faces, d_output, shift);
}

template <uint32_t blockThreads>
__device__ __forceinline__ void f_f(const uint16_t num_edges,
                                    const uint16_t num_faces,
                                    uint16_t*      s_FE,
                                    uint16_t*      s_FF_offset,
                                    uint16_t*      s_FF_output)
{
    uint16_t* s_EF_offset = &s_FE[num_faces * 3];
    uint16_t* s_EF_output = &s_EF_offset[num_edges + 1];

    for (uint16_t i = threadIdx.x; i < num_faces * 3; i += blockThreads) {
        flag_t   dir(0);
        uint16_t e     = s_FE[i] >> 1;
        s_EF_offset[i] = e;
        s_FE[i]        = e;
    }
    __syncthreads();

    e_f<blockThreads>(num_edges, num_faces, s_EF_offset, s_EF_output, 0);
    __syncthreads();

    for (uint16_t f = threadIdx.x; f < num_faces; f += blockThreads) {
        uint16_t num_neighbour_faces = 0;
        for (uint16_t e = 0; e < 3; ++e) {
            uint16_t edge = s_FE[3 * f + e];
            assert(s_EF_offset[edge + 1] >= s_EF_offset[edge]);

            num_neighbour_faces +=
                s_EF_offset[edge + 1] - s_EF_offset[edge] - 1;
        }
        s_FF_offset[f] = num_neighbour_faces;
    }
    __syncthreads();

    cub_block_exclusive_sum<uint16_t, blockThreads>(s_FF_offset, num_faces);

    for (uint16_t f = threadIdx.x; f < num_faces; f += blockThreads) {
        uint16_t offset = s_FF_offset[f];
        for (uint16_t e = 0; e < 3; ++e) {
            uint16_t edge = s_FE[3 * f + e];
            for (uint16_t ef = s_EF_offset[edge]; ef < s_EF_offset[edge + 1];
                 ++ef) {
                uint16_t n_face = s_EF_output[ef];
                if (n_face != f) {
                    s_FF_output[offset] = n_face;
                    ++offset;
                }
            }
        }
        assert(offset == s_FF_offset[f + 1]);
    }

}

template <uint32_t blockThreads, Op op>
__device__ __forceinline__ void query(uint16_t*&     s_output_offset,
                                      uint16_t*&     s_output_value,
                                      uint16_t*      s_ev,
                                      uint16_t*      s_fe,
                                      const uint16_t num_vertices,
                                      const uint16_t num_edges,
                                      const uint16_t num_faces)
{


    switch (op) {
        case Op::VV: {
            assert(num_vertices <= 2 * num_edges);
            s_output_offset = &s_ev[0];
            s_output_value  = &s_ev[num_vertices + 1];
            v_v<blockThreads>(num_vertices, num_edges, s_ev, s_output_value);
            break;
        }
        case Op::VE: {
            assert(num_vertices <= 2 * num_edges);
            s_output_offset = &s_ev[0];
            s_output_value  = &s_ev[num_vertices + 1];
            v_e<blockThreads>(num_vertices, num_edges, s_ev, s_output_value);
            break;
        }
        case Op::VF: {
            assert(num_vertices <= 2 * num_edges);
            s_output_offset = &s_fe[0];
            s_output_value  = &s_ev[0];
            v_f<blockThreads>(num_faces, num_edges, num_vertices, s_ev, s_fe);
            break;
        }
        case Op::EV: {
            s_output_value = s_ev;
            break;
        }
        case Op::EF: {
            assert(num_edges <= 3 * num_faces);
            s_output_offset = &s_fe[0];
            s_output_value  = &s_fe[num_edges + 1];
            e_f<blockThreads>(num_edges, num_faces, s_fe, s_output_value);
            break;
        }
        case Op::FV: {
            s_output_value = s_fe;
            f_v<blockThreads>(num_edges, s_ev, num_faces, s_fe);
            break;
        }
        case Op::FE: {
            s_output_value = s_fe;
            break;
        }
        case Op::FF: {
            assert(num_edges <= 3 * num_faces);
            s_output_offset = &s_fe[3 * num_faces + 2 * 3 * num_faces];
            s_output_value = &s_output_offset[num_faces + 1];
            f_f<blockThreads>(
                num_edges, num_faces, s_fe, s_output_offset, s_output_value);

            break;
        }
        default:
            assert(1 != 1);
            break;
    }
}
}    
}    
