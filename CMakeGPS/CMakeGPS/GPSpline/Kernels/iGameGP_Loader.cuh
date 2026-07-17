#pragma once

#include <assert.h>
#include <stdint.h>

#include <cooperative_groups.h>
#include <cooperative_groups/memcpy_async.h>
#include "../iGameGP_Context.h"
#include "../iGameGP_Local.h"
#include "../iGameGP_Types.h"
#include "../Util/iGameGP_Util.cuh"

namespace gpmesh {
namespace detail {

template <typename T, typename SizeT>
__device__ __inline__ void load_async(const T*    in,
                                      const SizeT size,
                                      T*          out,
                                      bool        with_wait)
{
    namespace cg           = cooperative_groups;
    cg::thread_block block = cg::this_thread_block();

    cg::memcpy_async(
        block,
        out,
        in,
        sizeof(T) * size);

    if (with_wait) {
        cg::wait(block);
    }
}

template <uint32_t blockThreads>
__device__ __forceinline__ void load_uint16(const uint16_t* in,
                                            const uint16_t  size,
                                            uint16_t*       out)
{
    const uint32_t  size32   = size / 2;
    const uint32_t  reminder = size % 2;
    const uint32_t* in32     = reinterpret_cast<const uint32_t*>(in);
    uint32_t*       out32    = reinterpret_cast<uint32_t*>(out);

    for (uint32_t i = threadIdx.x; i < size32; i += blockThreads) {
        uint32_t a = in32[i];
        out32[i]   = a;
    }

    if (reminder != 0) {
        if (threadIdx.x == 0) {
            out[size - 1] = in[size - 1];
        }
    }
}


template <Op op>
__device__ __forceinline__ void load_mesh_async(const PatchInfo& patch_info,
                                                uint16_t*&       s_ev,
                                                uint16_t*&       s_fe,
                                                bool             with_wait)
{
    assert(s_ev == s_fe);

    switch (op) {
        case Op::VV: {
            load_async(reinterpret_cast<uint16_t*>(patch_info.ev),
                       2 * patch_info.num_edges,
                       s_ev,
                       with_wait);
            break;
        }
        case Op::VE: {
            assert(2 * patch_info.num_edges > patch_info.num_vertices);
            load_async(reinterpret_cast<uint16_t*>(patch_info.ev),
                       2 * patch_info.num_edges,
                       s_ev,
                       with_wait);
            break;
        }
        case Op::VF: {
            assert(4 * patch_info.num_faces > patch_info.num_vertices);
            s_ev = s_fe + 4 * patch_info.num_faces;
            load_async(reinterpret_cast<uint16_t*>(patch_info.fe),
                       4 * patch_info.num_faces,
                       s_fe,
                       false);
            load_async(reinterpret_cast<uint16_t*>(patch_info.ev),
                       2 * patch_info.num_edges,
                       s_ev,
                       with_wait);
            break;
        }
        case Op::FV: {
            s_fe = s_ev + 2 * patch_info.num_edges;
            load_async(reinterpret_cast<uint16_t*>(patch_info.ev),
                       2 * patch_info.num_edges,
                       s_ev,
                       false);

            load_async(reinterpret_cast<uint16_t*>(patch_info.fe),
                       4 * patch_info.num_faces,
                       s_fe,
                       with_wait);
            break;
        }
        case Op::FE: {
            load_async(reinterpret_cast<uint16_t*>(patch_info.fe),
                       4 * patch_info.num_faces,
                       s_fe,
                       with_wait);
            break;
        }
        case Op::FF: {
            load_async(reinterpret_cast<uint16_t*>(patch_info.fe),
                       4 * patch_info.num_faces,
                       s_fe,
                       with_wait);
            break;
        }
        case Op::EV: {
            load_async(reinterpret_cast<uint16_t*>(patch_info.ev),
                       2 * patch_info.num_edges,
                       s_ev,
                       with_wait);
            break;
        }
        case Op::EF: {
            assert(3 * patch_info.num_faces > patch_info.num_edges);
            load_async(reinterpret_cast<uint16_t*>(patch_info.fe),
                       4 * patch_info.num_faces,
                       s_fe,
                       with_wait);
            break;
        }
        case Op::V_SURROUND_V: {
            load_async(reinterpret_cast<uint16_t*>(patch_info.ev),
                       2 * patch_info.num_edges,
                       s_ev,
                       with_wait);
            break;
        }
        case Op::V_V_SURROUND_UNCLOSED: {
            load_async(reinterpret_cast<uint16_t*>(patch_info.ev),
                       2 * patch_info.num_edges,
                       s_ev,
                       with_wait);
            break;
        }
        default: {
            assert(1 != 1);
            break;
        }
    }
}

template <Op op>
__device__ __forceinline__ void load_not_owned_async(
    const PatchInfo& patch_info,
    uint16_t*&       not_owned_local_id,
    uint32_t*&       not_owned_patch,
    uint16_t&        num_owned,
    bool             with_wait)
{
    uint16_t  num_not_owned        = 0;
    uint32_t* g_not_owned_patch    = nullptr;
    uint16_t* g_not_owned_local_id = nullptr;

    switch (op) {
        case Op::VV: {
            num_owned     = patch_info.num_owned_vertices;
            num_not_owned = patch_info.num_vertices - num_owned;

            not_owned_patch = not_owned_patch + 2 * patch_info.num_edges;
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_v;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_v);
            break;
        }
        case Op::VE: {
            num_owned     = patch_info.num_owned_edges;
            num_not_owned = patch_info.num_edges - num_owned;

            not_owned_patch = not_owned_patch + 2 * patch_info.num_edges;
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_e;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_e);
            break;
        }
        case Op::VF: {
            num_owned     = patch_info.num_owned_faces;
            num_not_owned = patch_info.num_faces - num_owned;

            uint32_t shift = DIVIDE_UP(
                3 * patch_info.num_faces + max(3 * patch_info.num_faces,
                                                    2 * patch_info.num_edges),
                2);
            not_owned_patch = not_owned_patch + shift;
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_f;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_f);
            break;
        }
        case Op::FV: {
            num_owned     = patch_info.num_owned_vertices;
            num_not_owned = patch_info.num_vertices - num_owned;

            assert(2 * patch_info.num_edges >= (1 + 2) * num_not_owned);

            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_v;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_v);
            break;
        }
        case Op::FE: {
            num_owned     = patch_info.num_owned_edges;
            num_not_owned = patch_info.num_edges - num_owned;

            not_owned_patch =
                not_owned_patch + DIVIDE_UP(3 * patch_info.num_faces, 2);
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_e;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_e);
            break;
        }
        case Op::FF: {
            num_owned     = patch_info.num_owned_faces;
            num_not_owned = patch_info.num_faces - num_owned;

            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_f;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_f);
            break;
        }
        case Op::EV: {
            num_owned     = patch_info.num_owned_vertices;
            num_not_owned = patch_info.num_vertices - num_owned;

            not_owned_patch = not_owned_patch + patch_info.num_edges;
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_v;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_v);
            break;
        }
        case Op::EF: {
            num_owned     = patch_info.num_owned_faces;
            num_not_owned = patch_info.num_faces - num_owned;

            not_owned_patch = not_owned_patch + 3 * patch_info.num_faces;
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_f;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_f);
            break;
        }
        case Op::V_SURROUND_V: {
            num_owned     = patch_info.num_owned_vertices;
            num_not_owned = patch_info.num_vertices - num_owned;

            not_owned_patch = not_owned_patch + 3 * patch_info.num_edges;
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_v;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_v);
            break;
        }
        case Op::V_V_SURROUND_UNCLOSED: {
            num_owned     = patch_info.num_owned_vertices;
            num_not_owned = patch_info.num_vertices - num_owned;

            not_owned_patch = not_owned_patch + 3 * patch_info.num_edges;
            not_owned_local_id =
                reinterpret_cast<uint16_t*>(not_owned_patch + num_not_owned);

            g_not_owned_patch = patch_info.not_owned_patch_v;
            g_not_owned_local_id =
                reinterpret_cast<uint16_t*>(patch_info.not_owned_id_v);
            break;
        }
        default: {
            assert(1 != 1);
            break;
        }
    }

    load_async(g_not_owned_patch, num_not_owned, not_owned_patch, false);
    load_async(
        g_not_owned_local_id, num_not_owned, not_owned_local_id, with_wait);
}
}    
}    
