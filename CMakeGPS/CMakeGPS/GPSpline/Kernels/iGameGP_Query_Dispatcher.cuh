#pragma once
#include <assert.h>
#include <stdint.h>
#include <cub/block/block_discontinuity.cuh>

#include "../iGameGP_Context.h"
#include "../iGameGP_Handle.h"
#include "../iGameGP_Iterator.cuh"
#include "iGameGP_Collective.cuh"
#include "iGameGP_Debug.cuh"
#include "iGameGP_Loader.cuh"
#include "iGameGP_Mesh_Queries.cuh"
#include "../iGameGP_Types.h"
#include "../Util/iGameGP_Meta.h"

namespace gpmesh {

namespace detail {

template <Op op, uint32_t blockThreads, typename activeSetT>
__device__ __inline__ void query_block_dispatcher(const PatchInfo& patch_info,
                                                  activeSetT compute_active_set,
                                                  const bool oriented,
                                                  uint32_t&  num_src_in_patch,
                                                  uint16_t*& s_output_offset,
                                                  uint16_t*& s_output_value,
                                                  uint16_t&  num_owned,
                                                  uint32_t*& not_owned_patch,
                                                  uint16_t*& not_owned_local_id)
{
    static_assert(op != Op::EE, "Op::EE is not supported!");

    num_src_in_patch = 0;
    if constexpr (op == Op::VV || op == Op::VE || op == Op::VF) {
        num_src_in_patch = patch_info.num_owned_vertices;
    }
    if constexpr (op == Op::EV || op == Op::EF) {
        num_src_in_patch = patch_info.num_owned_edges;
    }
    if constexpr (op == Op::FV || op == Op::FE || op == Op::FF) {
        num_src_in_patch = patch_info.num_owned_faces;
    }
    if constexpr (op == Op::V_SURROUND_V || op == Op::V_V_SURROUND_UNCLOSED)
    {
        num_src_in_patch = patch_info.num_owned_vertices;
    }

    bool     is_active = false;
    uint16_t local_id  = threadIdx.x;
    while (local_id < num_src_in_patch) {
        is_active =
            is_active || compute_active_set({patch_info.patch_id, local_id});
        local_id += blockThreads;
    }

    if (__syncthreads_or(is_active) == 0) {
        num_src_in_patch = 0;
        return;
    }

    extern __shared__ uint16_t shrd_mem[];
    uint16_t*                  s_ev = shrd_mem;
    uint16_t*                  s_fe = shrd_mem;
    load_mesh_async<op>(patch_info, s_ev, s_fe, true);

    not_owned_patch    = reinterpret_cast<uint32_t*>(shrd_mem);
    not_owned_local_id = shrd_mem;
    num_owned          = 0;

    __syncthreads();

    if (oriented) {
        assert(op == Op::VV || op == Op::V_SURROUND_V || op == Op::V_V_SURROUND_UNCLOSED);
        if constexpr (op == Op::VV) {
            v_v_oreinted<blockThreads>(
                patch_info, s_output_offset, s_output_value, s_ev);
        }
        else if constexpr (op == Op::V_SURROUND_V)
        {
            v_surround_v<blockThreads>(patch_info, s_output_offset, s_output_value, s_ev);
        }
        else if constexpr (op == Op::V_V_SURROUND_UNCLOSED)
        {
            v_v_surround_unclosed<blockThreads>(patch_info, s_output_offset, s_output_value, s_ev);
        }
    } else {
        if constexpr (!(op == Op::VV || op == Op::FV || op == Op::FF)) {
            load_not_owned_async<op>(patch_info,
                                     not_owned_local_id,
                                     not_owned_patch,
                                     num_owned,
                                     true);
        }

        query<blockThreads, op>(s_output_offset,
                                s_output_value,
                                s_ev,
                                s_fe,
                                patch_info.num_vertices,
                                patch_info.num_edges,
                                patch_info.num_faces);
    }

    if constexpr (op == Op::VV || op == Op::FV || op == Op::FF || op == Op::V_SURROUND_V || op == Op::V_V_SURROUND_UNCLOSED) {
        __syncthreads();
        load_not_owned_async<op>(
            patch_info, not_owned_local_id, not_owned_patch, num_owned, true);
    }

    __syncthreads();
}


template <Op op, uint32_t blockThreads, typename computeT, typename activeSetT>
__device__ __inline__ void query_block_dispatcher(const Context& context,
                                                  const uint32_t patch_id,
                                                  computeT       compute_op,
                                                  activeSetT compute_active_set,
                                                  const bool oriented = false)
{
    using ComputeTraits    = detail::FunctionTraits<computeT>;
    using ComputeHandleT   = typename ComputeTraits::template arg<0>::type;
    using ComputeIteratorT = typename ComputeTraits::template arg<1>::type;
    using LocalT           = typename ComputeIteratorT::LocalT;

    using ActiveSetTraits  = detail::FunctionTraits<activeSetT>;
    using ActiveSetHandleT = typename ActiveSetTraits::template arg<0>::type;
    static_assert(
        std::is_same_v<ActiveSetHandleT, ComputeHandleT>,
        "First argument of compute_op lambda function should match the first "
        "argument of active_set lambda function ");

    static_assert(op != Op::EE, "Op::EE is not supported!");


    assert(patch_id < context.get_num_patches());

    uint32_t  num_src_in_patch = 0;
    uint16_t* s_output_offset(nullptr);
    uint16_t* s_output_value(nullptr);
    uint16_t  num_owned;
    uint32_t* not_owned_patch(nullptr);
    uint16_t* not_owned_local_id(nullptr);

    query_block_dispatcher<op, blockThreads>(
        context.get_patches_info()[patch_id],
        compute_active_set,
        oriented,
        num_src_in_patch,
        s_output_offset,
        s_output_value,
        num_owned,
        not_owned_patch,
        not_owned_local_id);

    uint16_t local_id = threadIdx.x;
    while (local_id < num_src_in_patch) {

        assert(s_output_value);

        if (compute_active_set({patch_id, local_id})) {
            constexpr uint32_t fixed_offset =
                ((op == Op::EV)                 ? 2 :
                 (op == Op::FV || op == Op::FE) ? 4 :
                                                  0);


            ComputeHandleT   handle(patch_id, local_id);
            ComputeIteratorT iter(local_id,
                                  reinterpret_cast<LocalT*>(s_output_value),
                                  s_output_offset,
                                  fixed_offset,
                                  patch_id,
                                  num_owned,
                                  not_owned_patch,
                                  not_owned_local_id,
                                  int(op == Op::FE));

            compute_op(handle, iter);
        }

        local_id += blockThreads;
    }
}

}    
template <Op op, uint32_t blockThreads, typename computeT, typename activeSetT>
__device__ __inline__ void  query_block_dispatcher(const Context& context,
                                                  computeT       compute_op,
                                                  activeSetT compute_active_set,
                                                  const bool oriented = false)
{
    if (blockIdx.x >= context.get_num_patches()) {
        return;
    }

    detail::query_block_dispatcher<op, blockThreads>(
        context, blockIdx.x, compute_op, compute_active_set, oriented);
}


template <Op op, uint32_t blockThreads, typename computeT>
__device__ __inline__ void query_block_dispatcher(const Context& context,
                                                  computeT       compute_op,
                                                  const bool oriented = false)
{
    using ComputeTraits  = detail::FunctionTraits<computeT>;
    using ComputeHandleT = typename ComputeTraits::template arg<0>::type;

    query_block_dispatcher<op, blockThreads>(
        context, compute_op, [](ComputeHandleT) { return true; }, oriented);
}


template <Op op, uint32_t blockThreads, typename computeT, typename HandleT>
__device__ __inline__ void higher_query_block_dispatcher(
    const Context& context,
    const HandleT  src_id,
    computeT       compute_op,
    const bool     oriented = false)
{
    using ComputeTraits    = detail::FunctionTraits<computeT>;
    using ComputeIteratorT = typename ComputeTraits::template arg<1>::type;

    auto compute_active_set = [](HandleT) { return true; };

    std::pair<uint32_t, uint16_t> pl = src_id.unpack();

    __shared__ uint32_t s_block_patches[blockThreads];
    __shared__ uint32_t s_num_patches;
    if (threadIdx.x == 0) {
        s_num_patches = 0;
    }
    typedef cub::BlockRadixSort<uint32_t, blockThreads, 1>  BlockRadixSort;
    typedef cub::BlockDiscontinuity<uint32_t, blockThreads> BlockDiscontinuity;
    union TempStorage
    {
        typename BlockRadixSort::TempStorage     sort_storage;
        typename BlockDiscontinuity::TempStorage discont_storage;
    };
    __shared__ TempStorage all_temp_storage;
    uint32_t               thread_data[1], thread_head_flags[1];
    thread_data[0]       = pl.first;
    thread_head_flags[0] = 0;
    BlockRadixSort(all_temp_storage.sort_storage).Sort(thread_data);
    BlockDiscontinuity(all_temp_storage.discont_storage)
        .FlagHeads(thread_head_flags, thread_data, cub::Inequality());

    if (thread_head_flags[0] == 1 && thread_data[0] != INVALID32) {
        uint32_t id         = ::atomicAdd(&s_num_patches, uint32_t(1));
        s_block_patches[id] = thread_data[0];
    }

    __syncthreads();


    for (uint32_t p = 0; p < s_num_patches; ++p) {

        uint32_t patch_id = s_block_patches[p];

        assert(patch_id < context.get_num_patches());

        uint32_t  num_src_in_patch = 0;
        uint16_t *s_output_offset(nullptr), *s_output_value(nullptr);
        uint16_t  num_owned = 0;
        uint16_t* not_owned_local_id(nullptr);
        uint32_t* not_owned_patch(nullptr);

        detail::template query_block_dispatcher<op, blockThreads>(
            context.get_patches_info()[patch_id],
            compute_active_set,
            oriented,
            num_src_in_patch,
            s_output_offset,
            s_output_value,
            num_owned,
            not_owned_patch,
            not_owned_local_id);


        if (pl.first == patch_id) {

            constexpr uint32_t fixed_offset =
                ((op == Op::EV)                 ? 2 :
                 (op == Op::FV || op == Op::FE) ? 3 :
                                                  0);

            ComputeIteratorT iter(
                pl.second,
                reinterpret_cast<typename ComputeIteratorT::LocalT*>(
                    s_output_value),
                s_output_offset,
                fixed_offset,
                patch_id,
                num_owned,
                not_owned_patch,
                not_owned_local_id,
                int(op == Op::FE));

            compute_op(src_id, iter);
        }
        __syncthreads();
    }
}


}    
