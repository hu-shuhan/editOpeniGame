#include <cuda_device_runtime_api.h>

#include "iGameGPPatch.h"

#include "Util/iGameGP_CUDA_Macros.cuh"

#include "iGameGPSecurityAttributes.h"
#include "iGameGPSplineKernel.cuh"
#include "iGameGPSplineKernelCPU.h"

#include "iGameTensorCoreKernel.cuh"
#include "iGamewmma_extension_util.h"

#include "iGameGemmKernelCPU.h"

GPSTART

void GPPatch::init_device_cpoints(real_t* device_cpoints_x,
	real_t* device_cpoints_y,
	real_t* device_cpoints_z) {
	i_device_cpoints_x = device_cpoints_x;
	i_device_cpoints_y = device_cpoints_y;
	i_device_cpoints_z = device_cpoints_z;
}

void GPPatch::init_device_scalar_cpoints(real_t* device_cpoints_x,
	real_t* device_cpoints_y,
	real_t* device_cpoints_z) {
	i_device_scalar_cpoints_x = device_cpoints_x;
	i_device_scalar_cpoints_y = device_cpoints_y;
	i_device_scalar_cpoints_z = device_cpoints_z;
}

void GPPatch::init_device_cpoints_extended(real_t* device_cpoints_x_extended,
	real_t* device_cpoints_y_extended,
	real_t* device_cpoints_z_extended) {
	i_device_cpoints_x_extended = device_cpoints_x_extended;
	i_device_cpoints_y_extended = device_cpoints_y_extended;
	i_device_cpoints_z_extended = device_cpoints_z_extended;
}

void GPPatch::init_device_scalar_cpoints_extended(real_t* device_cpoints_x_extended,
	real_t* device_cpoints_y_extended,
	real_t* device_cpoints_z_extended) {
	i_device_scalar_cpoints_x_extended = device_cpoints_x_extended;
	i_device_scalar_cpoints_y_extended = device_cpoints_y_extended;
	i_device_scalar_cpoints_z_extended = device_cpoints_z_extended;
}

void GPPatch::init_host_cpoints(real_t* host_cpoints_x, real_t* host_cpoints_y,
	real_t* host_cpoints_z) {
	i_host_cpoints_x = host_cpoints_x;
	i_host_cpoints_y = host_cpoints_y;
	i_host_cpoints_z = host_cpoints_z;
}

void GPPatch::init_host_scalar_cpoints(real_t* host_cpoints_x, real_t* host_cpoints_y,
	real_t* host_cpoints_z) {
	i_host_scalar_cpoints_x = host_cpoints_x;
	i_host_scalar_cpoints_y = host_cpoints_y;
	i_host_scalar_cpoints_z = host_cpoints_z;
}

void GPPatch::init_host_cpoints_extended(real_t* host_cpoints_x_extended,
	real_t* host_cpoints_y_extended,
	real_t* host_cpoints_z_extended) {
	i_host_cpoints_x_extended = host_cpoints_x_extended;
	i_host_cpoints_y_extended = host_cpoints_y_extended;
	i_host_cpoints_z_extended = host_cpoints_z_extended;
}

void GPPatch::init_host_scalar_cpoints_extended(real_t* host_cpoints_x_extended,
	real_t* host_cpoints_y_extended,
	real_t* host_cpoints_z_extended) {
	i_host_scalar_cpoints_x_extended = host_cpoints_x_extended;
	i_host_scalar_cpoints_y_extended = host_cpoints_y_extended;
	i_host_scalar_cpoints_z_extended = host_cpoints_z_extended;
}


void GPPatch::init_u(real_t u_begin, real_t delta) {
	m_u_begin = u_begin;
	m_u_delta = delta;
}

void GPPatch::init_v(real_t v_begin, real_t v_step) {
	m_v_begin = v_begin;
	m_v_delta = v_step;
}

//void GPPatch::init_device() {
//    assert(n == 8 && m == 8);
//
//    auto alloc_host_once = [](real_t*& ptr, size_t count, bool zero_init) {
//        if (ptr) return;
//        ptr = new real_t[count];
//        if (zero_init) std::memset(ptr, 0, count * sizeof(real_t));
//    };
//
//    alloc_host_once(m_host_H_p, 4 * n, false);
//    alloc_host_once(m_host_H_prime_p, 4 * n, false);
//    alloc_host_once(m_host_H_q, 4 * m, false);
//    alloc_host_once(m_host_H_prime_q, 4 * m, false);
//    alloc_host_once(m_host_HpP, 4 * n, false);
//    alloc_host_once(m_host_HqP, 4 * m, false);
//
//    alloc_host_once(m_host_HHprime_p, 16 * 8, true);
//    alloc_host_once(m_host_H_q_extended, 8 * 8, true);
//    alloc_host_once(m_host_Hprime_q_extended, 8 * 8, true);
//    alloc_host_once(m_host_HHprime_p_C, 16 * 8, true);
//
//    alloc_host_once(m_host_result_SSv_x, 16 * 8, true);
//    alloc_host_once(m_host_result_SSv_y, 16 * 8, true);
//    alloc_host_once(m_host_result_SSv_z, 16 * 8, true);
//
//    alloc_host_once(m_host_result_SSv_x_s, 16 * 8, true);
//    alloc_host_once(m_host_result_SSv_y_s, 16 * 8, true);
//    alloc_host_once(m_host_result_SSv_z_s, 16 * 8, true);
//
//    alloc_host_once(m_host_result_Su0_x, 16 * 8, true);
//    alloc_host_once(m_host_result_Su0_y, 16 * 8, true);
//    alloc_host_once(m_host_result_Su0_z, 16 * 8, true);
//
//    if (m_device_blob == nullptr) {
//        auto align_up = [](size_t x, size_t a) { return (x + (a - 1)) & ~(a - 1); };
//
//        constexpr size_t kAlign = 256;
//
//        const size_t B_4n = 4 * n * sizeof(real_t); // 32 * sizeof(real_t)
//        const size_t B_4m = 4 * m * sizeof(real_t);
//        const size_t B_16x8 = 16 * 8 * sizeof(real_t); // 128 * sizeof(real_t)
//        const size_t B_8x8 = 8 * 8 * sizeof(real_t);   // 64  * sizeof(real_t)
//
//        size_t off = 0;
//
//        auto take = [&](size_t bytes) -> size_t {
//            off = align_up(off, kAlign);
//            size_t ret = off;
//            off += bytes;
//            return ret;
//        };
//
//        size_t o_H_p = take(B_4n);
//        size_t o_H_prime_p = take(B_4n);
//        size_t o_H_q = take(B_4m);
//        size_t o_H_prime_q = take(B_4m);
//        size_t o_HpP = take(B_4n);
//        size_t o_HqP = take(B_4m);
//
//        size_t o_HHprime_p = take(B_16x8);
//        size_t o_HHprime_p_s = take(B_16x8);
//
//        size_t o_H_q_extended = take(B_8x8);
//        size_t o_Hprime_q_extended = take(B_8x8);
//        size_t o_H_q_extended_s = take(B_8x8);
//        size_t o_Hprime_q_extended_s = take(B_8x8);
//
//        size_t o_HHprime_p_C_x = take(B_16x8);
//        size_t o_HHprime_p_C_x_s = take(B_16x8);
//        size_t o_HHprime_p_C_y = take(B_16x8);
//        size_t o_HHprime_p_C_z = take(B_16x8);
//
//        size_t o_result_SSv_x = take(B_16x8);
//        size_t o_result_SSv_y = take(B_16x8);
//        size_t o_result_SSv_z = take(B_16x8);
//
//        size_t o_result_SSv_x_s = take(B_16x8);
//        size_t o_result_SSv_y_s = take(B_16x8);
//        size_t o_result_SSv_z_s = take(B_16x8);
//
//        size_t o_result_Su0_x = take(B_16x8);
//        size_t o_result_Su0_y = take(B_16x8);
//        size_t o_result_Su0_z = take(B_16x8);
//
//        m_device_blob_bytes = align_up(off, kAlign);
//
//        CUDA_ERROR(cudaMalloc(&m_device_blob, m_device_blob_bytes));
//
//        CUDA_ERROR(cudaMemset(m_device_blob, 0, m_device_blob_bytes));
//
//        auto ptr_at = [&](size_t offset_bytes) -> real_t* {
//            return reinterpret_cast<real_t*>(reinterpret_cast<unsigned char*>(m_device_blob) + offset_bytes);
//        };
//
//        m_device_H_p = ptr_at(o_H_p);
//        m_device_H_prime_p = ptr_at(o_H_prime_p);
//        m_device_H_q = ptr_at(o_H_q);
//        m_device_H_prime_q = ptr_at(o_H_prime_q);
//        m_device_HpP = ptr_at(o_HpP);
//        m_device_HqP = ptr_at(o_HqP);
//
//        m_device_HHprime_p = ptr_at(o_HHprime_p);
//        m_device_HHprime_p_s = ptr_at(o_HHprime_p_s);
//
//        m_device_H_q_extended = ptr_at(o_H_q_extended);
//        m_device_Hprime_q_extended = ptr_at(o_Hprime_q_extended);
//        m_device_H_q_extended_s = ptr_at(o_H_q_extended_s);
//        m_device_Hprime_q_extended_s = ptr_at(o_Hprime_q_extended_s);
//
//        m_device_HHprime_p_C_x = ptr_at(o_HHprime_p_C_x);
//        m_device_HHprime_p_C_x_s = ptr_at(o_HHprime_p_C_x_s);
//        m_device_HHprime_p_C_y = ptr_at(o_HHprime_p_C_y);
//        m_device_HHprime_p_C_z = ptr_at(o_HHprime_p_C_z);
//
//        m_device_result_SSv_x = ptr_at(o_result_SSv_x);
//        m_device_result_SSv_y = ptr_at(o_result_SSv_y);
//        m_device_result_SSv_z = ptr_at(o_result_SSv_z);
//
//        m_device_result_SSv_x_s = ptr_at(o_result_SSv_x_s);
//        m_device_result_SSv_y_s = ptr_at(o_result_SSv_y_s);
//        m_device_result_SSv_z_s = ptr_at(o_result_SSv_z_s);
//
//        m_device_result_Su0_x = ptr_at(o_result_Su0_x);
//        m_device_result_Su0_y = ptr_at(o_result_Su0_y);
//        m_device_result_Su0_z = ptr_at(o_result_Su0_z);
//    }
//
//    m_gpu_patch_data.m_HHprime_p = m_device_HHprime_p;
//    m_gpu_patch_data.m_H_q_extended = m_device_H_q_extended;
//    m_gpu_patch_data.m_Hprime_q_extended = m_device_Hprime_q_extended;
//
//    m_gpu_patch_data.m_HHprime_p_s = m_device_HHprime_p_s;
//    m_gpu_patch_data.m_H_q_extended_s = m_device_H_q_extended_s;
//    m_gpu_patch_data.m_Hprime_q_extended_s = m_device_Hprime_q_extended_s;
//
//    m_gpu_patch_data.m_HHprime_p_C_x = m_device_HHprime_p_C_x;
//    m_gpu_patch_data.m_HHprime_p_C_y = m_device_HHprime_p_C_y;
//    m_gpu_patch_data.m_HHprime_p_C_z = m_device_HHprime_p_C_z;
//
//    m_gpu_patch_data.m_result_SSv_x = m_device_result_SSv_x;
//    m_gpu_patch_data.m_result_SSv_y = m_device_result_SSv_y;
//    m_gpu_patch_data.m_result_SSv_z = m_device_result_SSv_z;
//
//    m_gpu_patch_data.m_result_SSv_x_s = m_device_result_SSv_x_s;
//    m_gpu_patch_data.m_result_SSv_y_s = m_device_result_SSv_y_s;
//    m_gpu_patch_data.m_result_SSv_z_s = m_device_result_SSv_z_s;
//
//    m_gpu_patch_data.m_result_Su0_x = m_device_result_Su0_x;
//    m_gpu_patch_data.m_result_Su0_y = m_device_result_Su0_y;
//    m_gpu_patch_data.m_result_Su0_z = m_device_result_Su0_z;
//}


size_t GPPatch::device_blob_stride_bytes() {
    auto align_up = [](size_t x, size_t a) { return (x + (a - 1)) & ~(a - 1); };
    constexpr size_t kAlign = 256;
    constexpr int n = 8, m = 8;

    const size_t B_4n = 4 * n * sizeof(real_t);
    const size_t B_4m = 4 * m * sizeof(real_t);
    const size_t B_16x8 = 16 * 8 * sizeof(real_t);
    const size_t B_8x8 = 8 * 8 * sizeof(real_t);

    size_t off = 0;
    auto take = [&](size_t bytes) -> size_t {
        off = align_up(off, kAlign);
        size_t ret = off;
        off += bytes;
        return ret;
    };

    take(B_4n); // H_p
    take(B_4n); // H_prime_p
    take(B_4m); // H_q
    take(B_4m); // H_prime_q
    take(B_4n); // HpP
    take(B_4m); // HqP

    take(B_16x8); // HHprime_p
    take(B_16x8); // HHprime_p_s

    take(B_8x8); // H_q_extended
    take(B_8x8); // Hprime_q_extended
    take(B_8x8); // H_q_extended_s
    take(B_8x8); // Hprime_q_extended_s

    take(B_16x8); // HHprime_p_C_x
    take(B_16x8); // HHprime_p_C_x_s
    take(B_16x8); // HHprime_p_C_y
    take(B_16x8); // HHprime_p_C_z

    take(B_16x8); // result_SSv_x
    take(B_16x8); // result_SSv_y
    take(B_16x8); // result_SSv_z

    take(B_16x8); // result_SSv_x_s
    take(B_16x8); // result_SSv_y_s
    take(B_16x8); // result_SSv_z_s

    take(B_16x8); // result_Su0_x
    take(B_16x8); // result_Su0_y
    take(B_16x8); // result_Su0_z

    return align_up(off, kAlign);
}

void GPPatch::bind_device_blob(void* base, size_t bytes, bool zero_init) {
    assert(base != nullptr);
    assert(bytes >= device_blob_stride_bytes());

    m_device_blob = base;
    m_device_blob_bytes = bytes;
    m_owns_device_blob = false;

    if (zero_init) { CUDA_ERROR(cudaMemset(m_device_blob, 0, m_device_blob_bytes)); }

    setup_device_blob_layout_and_ptrs();
}

void GPPatch::setup_device_blob_layout_and_ptrs() {
    assert(n == 8 && m == 8);
    assert(m_device_blob != nullptr);

    auto align_up = [](size_t x, size_t a) { return (x + (a - 1)) & ~(a - 1); };
    constexpr size_t kAlign = 256;

    const size_t B_4n = 4 * n * sizeof(real_t);
    const size_t B_4m = 4 * m * sizeof(real_t);
    const size_t B_16x8 = 16 * 8 * sizeof(real_t);
    const size_t B_8x8 = 8 * 8 * sizeof(real_t);

    size_t off = 0;
    auto take = [&](size_t bytes) -> size_t {
        off = align_up(off, kAlign);
        size_t ret = off;
        off += bytes;
        return ret;
    };

    size_t o_H_p = take(B_4n);
    size_t o_H_prime_p = take(B_4n);
    size_t o_H_q = take(B_4m);
    size_t o_H_prime_q = take(B_4m);
    size_t o_HpP = take(B_4n);
    size_t o_HqP = take(B_4m);

    size_t o_HHprime_p = take(B_16x8);
    size_t o_HHprime_p_s = take(B_16x8);

    size_t o_H_q_extended = take(B_8x8);
    size_t o_Hprime_q_extended = take(B_8x8);
    size_t o_H_q_extended_s = take(B_8x8);
    size_t o_Hprime_q_extended_s = take(B_8x8);

    size_t o_HHprime_p_C_x = take(B_16x8);
    size_t o_HHprime_p_C_x_s = take(B_16x8);
    size_t o_HHprime_p_C_y = take(B_16x8);
    size_t o_HHprime_p_C_z = take(B_16x8);

    size_t o_result_SSv_x = take(B_16x8);
    size_t o_result_SSv_y = take(B_16x8);
    size_t o_result_SSv_z = take(B_16x8);

    size_t o_result_SSv_x_s = take(B_16x8);
    size_t o_result_SSv_y_s = take(B_16x8);
    size_t o_result_SSv_z_s = take(B_16x8);

    size_t o_result_Su0_x = take(B_16x8);
    size_t o_result_Su0_y = take(B_16x8);
    size_t o_result_Su0_z = take(B_16x8);

    auto ptr_at = [&](size_t offset_bytes) -> real_t* {
        return reinterpret_cast<real_t*>(reinterpret_cast<unsigned char*>(m_device_blob) + offset_bytes);
    };

    m_device_H_p = ptr_at(o_H_p);
    m_device_H_prime_p = ptr_at(o_H_prime_p);
    m_device_H_q = ptr_at(o_H_q);
    m_device_H_prime_q = ptr_at(o_H_prime_q);
    m_device_HpP = ptr_at(o_HpP);
    m_device_HqP = ptr_at(o_HqP);

    m_device_HHprime_p = ptr_at(o_HHprime_p);
    m_device_HHprime_p_s = ptr_at(o_HHprime_p_s);

    m_device_H_q_extended = ptr_at(o_H_q_extended);
    m_device_Hprime_q_extended = ptr_at(o_Hprime_q_extended);
    m_device_H_q_extended_s = ptr_at(o_H_q_extended_s);
    m_device_Hprime_q_extended_s = ptr_at(o_Hprime_q_extended_s);

    m_device_HHprime_p_C_x = ptr_at(o_HHprime_p_C_x);
    m_device_HHprime_p_C_x_s = ptr_at(o_HHprime_p_C_x_s);
    m_device_HHprime_p_C_y = ptr_at(o_HHprime_p_C_y);
    m_device_HHprime_p_C_z = ptr_at(o_HHprime_p_C_z);

    m_device_result_SSv_x = ptr_at(o_result_SSv_x);
    m_device_result_SSv_y = ptr_at(o_result_SSv_y);
    m_device_result_SSv_z = ptr_at(o_result_SSv_z);

    m_device_result_SSv_x_s = ptr_at(o_result_SSv_x_s);
    m_device_result_SSv_y_s = ptr_at(o_result_SSv_y_s);
    m_device_result_SSv_z_s = ptr_at(o_result_SSv_z_s);

    m_device_result_Su0_x = ptr_at(o_result_Su0_x);
    m_device_result_Su0_y = ptr_at(o_result_Su0_y);
    m_device_result_Su0_z = ptr_at(o_result_Su0_z);

    m_gpu_patch_data.m_HHprime_p = m_device_HHprime_p;
    m_gpu_patch_data.m_H_q_extended = m_device_H_q_extended;
    m_gpu_patch_data.m_Hprime_q_extended = m_device_Hprime_q_extended;

    m_gpu_patch_data.m_HHprime_p_s = m_device_HHprime_p_s;
    m_gpu_patch_data.m_H_q_extended_s = m_device_H_q_extended_s;
    m_gpu_patch_data.m_Hprime_q_extended_s = m_device_Hprime_q_extended_s;

    m_gpu_patch_data.m_HHprime_p_C_x = m_device_HHprime_p_C_x;
    m_gpu_patch_data.m_HHprime_p_C_y = m_device_HHprime_p_C_y;
    m_gpu_patch_data.m_HHprime_p_C_z = m_device_HHprime_p_C_z;

    m_gpu_patch_data.m_result_SSv_x = m_device_result_SSv_x;
    m_gpu_patch_data.m_result_SSv_y = m_device_result_SSv_y;
    m_gpu_patch_data.m_result_SSv_z = m_device_result_SSv_z;

    m_gpu_patch_data.m_result_SSv_x_s = m_device_result_SSv_x_s;
    m_gpu_patch_data.m_result_SSv_y_s = m_device_result_SSv_y_s;
    m_gpu_patch_data.m_result_SSv_z_s = m_device_result_SSv_z_s;

    m_gpu_patch_data.m_result_Su0_x = m_device_result_Su0_x;
    m_gpu_patch_data.m_result_Su0_y = m_device_result_Su0_y;
    m_gpu_patch_data.m_result_Su0_z = m_device_result_Su0_z;
}

void GPPatch::init_device() {
    assert(n == 8 && m == 8);

    auto alloc_host_once = [](real_t*& ptr, size_t count, bool zero_init) {
        if (ptr) return;
        ptr = new real_t[count];
        if (zero_init) std::memset(ptr, 0, count * sizeof(real_t));
    };

    alloc_host_once(m_host_H_p, 4 * n, false);
    alloc_host_once(m_host_H_prime_p, 4 * n, false);
    alloc_host_once(m_host_H_q, 4 * m, false);
    alloc_host_once(m_host_H_prime_q, 4 * m, false);
    alloc_host_once(m_host_HpP, 4 * n, false);
    alloc_host_once(m_host_HqP, 4 * m, false);

    alloc_host_once(m_host_HHprime_p, 16 * 8, true);
    alloc_host_once(m_host_H_q_extended, 8 * 8, true);
    alloc_host_once(m_host_Hprime_q_extended, 8 * 8, true);
    alloc_host_once(m_host_HHprime_p_C, 16 * 8, true);

    alloc_host_once(m_host_result_SSv_x, 16 * 8, true);
    alloc_host_once(m_host_result_SSv_y, 16 * 8, true);
    alloc_host_once(m_host_result_SSv_z, 16 * 8, true);

    alloc_host_once(m_host_result_SSv_x_s, 16 * 8, true);
    alloc_host_once(m_host_result_SSv_y_s, 16 * 8, true);
    alloc_host_once(m_host_result_SSv_z_s, 16 * 8, true);

    alloc_host_once(m_host_result_Su0_x, 16 * 8, true);
    alloc_host_once(m_host_result_Su0_y, 16 * 8, true);
    alloc_host_once(m_host_result_Su0_z, 16 * 8, true);

    if (m_device_blob == nullptr) {
        m_device_blob_bytes = device_blob_stride_bytes();
        CUDA_ERROR(cudaMalloc(&m_device_blob, m_device_blob_bytes));
        CUDA_ERROR(cudaMemset(m_device_blob, 0, m_device_blob_bytes));
        m_owns_device_blob = true;
    }

    setup_device_blob_layout_and_ptrs();
}



void GPPatch::init_stream() {
    if (!bEnableStream) return;

    if (m_stream_inited) return;
    m_stream_inited = true;

    m_xstreams.assign(PATCH_STREAM_SIZE, nullptr);
    m_ystreams.assign(PATCH_STREAM_SIZE, nullptr);
    m_xevents.assign(PATCH_STREAM_SIZE, nullptr);

    for (int i = 0; i < PATCH_STREAM_SIZE; ++i) {
        cudaStream_t xs = nullptr;
        cudaStream_t ys = nullptr;
        cudaEvent_t xe = nullptr;

        CUDA_ERROR(cudaStreamCreateWithFlags(&xs, cudaStreamNonBlocking));
        CUDA_ERROR(cudaStreamCreateWithFlags(&ys, cudaStreamNonBlocking));
        CUDA_ERROR(cudaEventCreateWithFlags(&xe, cudaEventDisableTiming));

        // 存进 void*（handle 在 CUDA runtime 中可当作指针大小的值处理）
        m_xstreams[i] = reinterpret_cast<void*>(xs);
        m_ystreams[i] = reinterpret_cast<void*>(ys);
        m_xevents[i] = reinterpret_cast<void*>(xe);
    }
}


void GPPatch::evaluation() {
	evaluation_D01_CPU<real_t>(m_u_begin, m_u_delta, n, m_host_H_p,
		m_host_H_prime_p);
	evaluation_D01_CPU<real_t>(m_v_begin, m_v_delta, m, m_host_H_q,
		m_host_H_prime_q);

	dim3 threadsPerBlock;
	threadsPerBlock.x = static_cast<uint32_t>(8);
	threadsPerBlock.y = 1;
	threadsPerBlock.z = 1;

	evaluation_D01_GISMO << <1, threadsPerBlock >> > (
		m_u_begin, m_u_delta, n, m_device_H_p, m_device_H_prime_p);
	evaluation_D01_GISMO << <1, threadsPerBlock >> > (
		m_v_begin, m_v_delta, m, m_device_H_q, m_device_H_prime_q);

	debug_check_H_and_H_prime();
}

void GPPatch::evaluation_tensor_core(bool bUsingTCEC, size_t mm_offset) {

	bool bCheckCPU = false;

	if (bCheckCPU) {
		evaluation_D01_CPU_HHprime_p<real_t>(m_u_begin, m_u_delta, n,
			m_host_HHprime_p);
		evaluation_D01_CPU_H_q_extended_and_Hprime_q_extended<real_t>(
			m_v_begin, m_v_delta, m, m_host_H_q_extended,
			m_host_Hprime_q_extended);
	}

	dim3 threadsPerBlock;
	threadsPerBlock.x = static_cast<uint32_t>(8);
	threadsPerBlock.y = 1;
	threadsPerBlock.z = 1;


	evaluation_D01_GPU_HHprime_p << <1, threadsPerBlock >> > (m_u_begin, m_u_delta,
		n, m_device_HHprime_p);
	evaluation_D01_GPU_H_q_extended_and_Hprime_q_extended << <1,
		threadsPerBlock >> > (
			m_v_begin, m_v_delta, m, m_device_H_q_extended,
			m_device_Hprime_q_extended);

	if (bCheckCPU) debug_check_H_and_H_prime_for_tensor_core();

	using policy_wec = mtk::wmma::tcec::detail::default_policy<
		half, mtk::wmma::tcec::with_ec, mtk::wmma::tcec::op_mma,
		mtk::wmma::tcec::sm_75>::type;
	using policy_woec = mtk::wmma::tcec::detail::default_policy<
		half, mtk::wmma::tcec::without_ec, mtk::wmma::tcec::op_mma,
		mtk::wmma::tcec::sm_75>::type;

	using policy_current = policy_woec;

	real_t* i_device_cpoints_extended_arr[3] = { i_device_cpoints_x_extended,
												i_device_cpoints_y_extended,
												i_device_cpoints_z_extended };
	real_t* i_host_cpoints_extended_arr[3] = { i_host_cpoints_x_extended,
											  i_host_cpoints_y_extended,
											  i_host_cpoints_z_extended };
	real_t* deivce_result_SSv_arr[3] = { m_device_result_SSv_x,
										m_device_result_SSv_y,
										m_device_result_SSv_z };
	real_t* host_result_SSv_arr[3] = { m_host_result_SSv_x, m_host_result_SSv_y,
									  m_host_result_SSv_z };
	real_t* device_result_Su0_arr[3] = { m_device_result_Su0_x,
										m_device_result_Su0_y,
										m_device_result_Su0_z };
	real_t* host_result_Su0_arr[3] = { m_host_result_Su0_x, m_host_result_Su0_y,
									  m_host_result_Su0_z };



	real_t* i_device_cpoints_extended_arr_s[3] = { i_device_scalar_cpoints_x_extended,
											i_device_scalar_cpoints_y_extended,
											i_device_scalar_cpoints_z_extended };
	real_t* i_host_cpoints_extended_arr_s[3] = { i_host_scalar_cpoints_x_extended,
											  i_host_scalar_cpoints_y_extended,
											  i_host_scalar_cpoints_z_extended };
	real_t* deivce_result_SSv_arr_s[3] = { m_device_result_SSv_x_s,
										m_device_result_SSv_y_s,
										m_device_result_SSv_z_s };
	real_t* host_result_SSv_arr_s[3] = { m_host_result_SSv_x_s, m_host_result_SSv_y_s,
									  m_host_result_SSv_z_s };

	for (int i = 0; i < 3; ++i) {
		if (bUsingTCEC) {
			mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, policy_wec>
				<< <1, warp_size >> > (m_device_HHprime_p_C_x,
					m_device_HHprime_p,
					i_device_cpoints_extended_arr[i],
					nvcuda::wmma::mem_row_major);


			mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
			    nvcuda::wmma::col_major, nvcuda::wmma::row_major,
			    nvcuda::wmma::col_major, policy_wec>
			    << <1, warp_size >> > (m_device_HHprime_p_C_x_s,
			        m_device_HHprime_p,
			        i_device_cpoints_extended_arr_s[i],
			        nvcuda::wmma::mem_row_major);

		}
		else {
			mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, policy_woec>
				<< <1, warp_size >> > (m_device_HHprime_p_C_x,
					m_device_HHprime_p,
					i_device_cpoints_extended_arr[i],
					nvcuda::wmma::mem_row_major);
		}


		if (bCheckCPU) {
			mma_kernel_abd_cpu(m_host_HHprime_p_C, m_host_HHprime_p,
				i_host_cpoints_extended_arr[i],
				MatrixLayout::RowMajor);
			debug_check_cpu_and_gpu_result(m_host_HHprime_p_C,
				m_device_HHprime_p_C_x, 16 * 8,
				"HHprime * C", 1e-5);
		}

		if (bUsingTCEC) {
			mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, policy_wec>
				<< <1, warp_size >> > (
					deivce_result_SSv_arr[i], m_device_HHprime_p_C_x,
					m_device_H_q_extended, nvcuda::wmma::mem_row_major);

			mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
			    nvcuda::wmma::col_major, nvcuda::wmma::row_major,
			    nvcuda::wmma::col_major, policy_wec>
			    << <1, warp_size >> > (
			        deivce_result_SSv_arr_s[i], m_device_HHprime_p_C_x_s,
			        m_device_H_q_extended, nvcuda::wmma::mem_row_major);

		}
		else {
			mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, nvcuda::wmma::row_major,
				nvcuda::wmma::col_major, policy_woec>
				<< <1, warp_size >> > (
					deivce_result_SSv_arr[i], m_device_HHprime_p_C_x,
					m_device_H_q_extended, nvcuda::wmma::mem_row_major);
		}


		if (bCheckCPU) {
			mma_kernel_abd_cpu(host_result_SSv_arr[i], m_host_HHprime_p_C,
				m_host_H_q_extended, MatrixLayout::RowMajor);
			debug_check_cpu_and_gpu_result(host_result_SSv_arr[i],
				deivce_result_SSv_arr[i], 16 * 8,
				"SSv", 1e-5);
		}

		if (bUsingTCEC) {
		    mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		                   nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		                   nvcuda::wmma::col_major, policy_wec>
		            <<<1, warp_size>>>(device_result_Su0_arr[i],
		                               m_device_HHprime_p_C_x,
		                               m_device_Hprime_q_extended,
		                               nvcuda::wmma::mem_row_major);
		} else {
		    mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		                   nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		                   nvcuda::wmma::col_major, policy_woec>
		            <<<1, warp_size>>>(device_result_Su0_arr[i],
		                               m_device_HHprime_p_C_x,
		                               m_device_Hprime_q_extended,
		                               nvcuda::wmma::mem_row_major);
		}


		if (bCheckCPU) {
			mma_kernel_abd_cpu(host_result_Su0_arr[i], m_host_HHprime_p_C,
				m_host_Hprime_q_extended,
				MatrixLayout::RowMajor);
			debug_check_cpu_and_gpu_result(host_result_Su0_arr[i],
				device_result_Su0_arr[i], 16 * 8,
				"Su", 1e-3);
		}
	}

	size_t position_offset = mm_offset; 
	size_t position_size = 192;
	write_position<real_t> << <1, gpmesh::warp_size >> > (
		m_cuda_position_ptr, position_offset, position_size,
		m_device_result_SSv_x, m_device_result_SSv_y,
		m_device_result_SSv_z);
	int x = 1;


	size_t normal_offset = mm_offset;
	size_t normal_size = 192;
	write_normal<real_t> << <1, gpmesh::warp_size >> > (
		m_cuda_normal_ptr, normal_offset, normal_size,
		m_device_result_SSv_x, m_device_result_SSv_y, m_device_result_SSv_z,
		m_device_result_Su0_x, m_device_result_Su0_y,
		m_device_result_Su0_z);

	size_t scalar_offset = mm_offset; 
	size_t scalar_size = 192;
	write_position<real_t> << <1, gpmesh::warp_size >> > (
		m_cuda_scalar_ptr, scalar_offset, scalar_size,
		m_device_result_SSv_x_s, m_device_result_SSv_y_s,
		m_device_result_SSv_z_s);
}

void GPPatch::evaluation_HHprime() {
	dim3 threadsPerBlock;
	threadsPerBlock.x = static_cast<uint32_t>(8);
	threadsPerBlock.y = 1;
	threadsPerBlock.z = 1;


	evaluation_D01_GPU_HHprime_p << <1, threadsPerBlock, 0,
		reinterpret_cast<cudaStream_t>(
			m_xstreams[0]) >> > (
				m_u_begin, m_u_delta, n, m_device_HHprime_p);
	evaluation_D01_GPU_H_q_extended_and_Hprime_q_extended << <
		1, threadsPerBlock, 0,
		reinterpret_cast<cudaStream_t>(m_ystreams[0]) >> > (
			m_v_begin, m_v_delta, m, m_device_H_q_extended,
			m_device_Hprime_q_extended);
}

void GPPatch::evaluation_HHprime_p_C() {
	using policy_wec = mtk::wmma::tcec::detail::default_policy<
		half, mtk::wmma::tcec::with_ec, mtk::wmma::tcec::op_mma,
		mtk::wmma::tcec::sm_75>::type;

	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_xstreams[0]) >> > (
			m_device_HHprime_p_C_x, m_device_HHprime_p,
			i_device_cpoints_x_extended, nvcuda::wmma::mem_row_major);

	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_xstreams[0])));

	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_xstreams[1]) >> > (
			m_device_HHprime_p_C_y, m_device_HHprime_p,
			i_device_cpoints_y_extended, nvcuda::wmma::mem_row_major);

	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_xstreams[2]) >> > (
			m_device_HHprime_p_C_z, m_device_HHprime_p,
			i_device_cpoints_z_extended, nvcuda::wmma::mem_row_major);

	CUDA_ERROR(cudaEventRecord(reinterpret_cast<cudaEvent_t>(m_xevents[0]),
		reinterpret_cast<cudaStream_t>(m_xstreams[0])));
	CUDA_ERROR(cudaEventRecord(reinterpret_cast<cudaEvent_t>(m_xevents[1]),
		reinterpret_cast<cudaStream_t>(m_xstreams[1])));
	CUDA_ERROR(cudaEventRecord(reinterpret_cast<cudaEvent_t>(m_xevents[2]),
		reinterpret_cast<cudaStream_t>(m_xstreams[2])));
}

void GPPatch::evaluation_SSv() {
	using policy_wec = mtk::wmma::tcec::detail::default_policy<
		half, mtk::wmma::tcec::with_ec, mtk::wmma::tcec::op_mma,
		mtk::wmma::tcec::sm_75>::type;
	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_ystreams[0])));

	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_xstreams[0]) >> > (
			m_device_result_SSv_x, m_device_HHprime_p_C_x,
			m_device_H_q_extended, nvcuda::wmma::mem_row_major);

	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_xstreams[1]) >> > (
			m_device_result_SSv_y, m_device_HHprime_p_C_y,
			m_device_H_q_extended, nvcuda::wmma::mem_row_major);

	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_xstreams[1]) >> > (
			m_device_result_SSv_z, m_device_HHprime_p_C_z,
			m_device_H_q_extended, nvcuda::wmma::mem_row_major);
}

void GPPatch::evaluation_Su0() {
	using policy_wec = mtk::wmma::tcec::detail::default_policy<
		half, mtk::wmma::tcec::with_ec, mtk::wmma::tcec::op_mma,
		mtk::wmma::tcec::sm_75>::type;

	CUDA_ERROR(
		cudaEventSynchronize(reinterpret_cast<cudaEvent_t>(m_xevents[0])));
	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_ystreams[0]) >> > (
			m_device_result_Su0_x, m_device_HHprime_p_C_x,
			m_device_Hprime_q_extended, nvcuda::wmma::mem_row_major);

	CUDA_ERROR(
		cudaEventSynchronize(reinterpret_cast<cudaEvent_t>(m_xevents[1])));
	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_ystreams[1]) >> > (
			m_device_result_Su0_y, m_device_HHprime_p_C_y,
			m_device_Hprime_q_extended, nvcuda::wmma::mem_row_major);

	CUDA_ERROR(
		cudaEventSynchronize(reinterpret_cast<cudaEvent_t>(m_xevents[2])));
	mma_kernel_abd<16, 8, 8, half, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, nvcuda::wmma::row_major,
		nvcuda::wmma::col_major, policy_wec>
		<< <1, warp_size, 0,
		reinterpret_cast<cudaStream_t>(m_ystreams[2]) >> > (
			m_device_result_Su0_z, m_device_HHprime_p_C_z,
			m_device_Hprime_q_extended, nvcuda::wmma::mem_row_major);
}

void GPPatch::write_data() {
	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_xstreams[0])));
	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_xstreams[1])));
	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_xstreams[2])));

	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_ystreams[0])));
	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_ystreams[1])));
	CUDA_ERROR(cudaStreamSynchronize(
		reinterpret_cast<cudaStream_t>(m_ystreams[2])));


}

void GPPatch::stream_driven_evaluation() {
	if (bEnableStream) {
		evaluation_HHprime();
		evaluation_HHprime_p_C();
		evaluation_SSv();
		evaluation_Su0();
		write_data();
	}
}

void GPPatch::release() {

	delete[] m_host_H_p;
	m_host_H_p = nullptr;

	delete[] m_host_H_prime_p;
	m_host_H_prime_p = nullptr;

	delete[] m_host_H_q;
	m_host_H_q = nullptr;

	delete[] m_host_H_prime_q;
	m_host_H_prime_q = nullptr;

	delete[] m_host_HpP;
	m_host_HpP = nullptr;

	delete[] m_host_HqP;
	m_host_HqP = nullptr;

	delete[] m_host_result_position_ptr;
	m_host_result_position_ptr = nullptr;

	delete[] m_host_result_normal_ptr;
	m_host_result_normal_ptr = nullptr;

	GPU_FREE(m_device_H_p);
	GPU_FREE(m_device_H_prime_p);
	GPU_FREE(m_device_H_q);
	GPU_FREE(m_device_H_prime_q);
	GPU_FREE(m_device_HpP);
	GPU_FREE(m_device_HqP);

	delete[] m_host_HHprime_p;
	m_host_HHprime_p = nullptr;
	delete[] m_host_H_q_extended;
	m_host_H_q_extended = nullptr;
	delete[] m_host_Hprime_q_extended;
	m_host_Hprime_q_extended = nullptr;
	delete[] m_host_HHprime_p_C;
	m_host_HHprime_p_C = nullptr;

	delete[] m_host_result_SSv_x;
	m_host_result_SSv_x = nullptr;
	delete[] m_host_result_SSv_y;
	m_host_result_SSv_y = nullptr;
	delete[] m_host_result_SSv_z;
	m_host_result_SSv_z = nullptr;

	delete[] m_host_result_Su0_x;
	m_host_result_Su0_x = nullptr;
	delete[] m_host_result_Su0_y;
	m_host_result_Su0_y = nullptr;
	delete[] m_host_result_Su0_z;
	m_host_result_Su0_z = nullptr;

	GPU_FREE(m_device_HHprime_p);
	GPU_FREE(m_device_H_q_extended);
	GPU_FREE(m_device_Hprime_q_extended);
	GPU_FREE(m_device_HHprime_p_C_x);
	GPU_FREE(m_device_HHprime_p_C_y);
	GPU_FREE(m_device_HHprime_p_C_z);

	GPU_FREE(m_device_result_SSv_x);
	GPU_FREE(m_device_result_SSv_y);
	GPU_FREE(m_device_result_SSv_z);

	GPU_FREE(m_device_result_Su0_x);
	GPU_FREE(m_device_result_Su0_y);
	GPU_FREE(m_device_result_Su0_z);

	if (bEnableStream) {
		for (int i = 0; i < PATCH_STREAM_SIZE; ++i) {
			cudaStream_t xstream =
				reinterpret_cast<cudaStream_t>(m_xstreams[i]);
			CUDA_ERROR(cudaStreamDestroy(xstream));
			cudaStream_t ystream =
				reinterpret_cast<cudaStream_t>(m_ystreams[i]);
			CUDA_ERROR(cudaStreamDestroy(ystream));
			cudaEvent_t xevent = reinterpret_cast<cudaEvent_t>(m_xevents[i]);
			CUDA_ERROR(cudaEventDestroy(xevent));
		}
	}

}

void GPPatch::debug_check_H_and_H_prime() {
	real_t tol = 1e-5;

	bool bCheckHn = true;
	bool bCheckHm = true;
	bool bCheckHnPrime = true;
	bool bCheckHmPrime = true;

	real_t* mapped_device_data = nullptr;

	if (bCheckHn) {
		size_t local_size = 4 * n;
		real_t* host_data = m_host_H_p;
		std::string name = "result Hn";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, m_device_H_p,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}

	if (bCheckHm) {
		size_t local_size = 4 * m;
		real_t* host_data = m_host_H_q;
		std::string name = "result Hm";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, m_device_H_q,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}

	if (bCheckHnPrime) {
		size_t local_size = 4 * n;
		real_t* host_data = m_host_H_prime_p;
		std::string name = "result Hn Prime";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, m_device_H_prime_p,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}

	if (bCheckHmPrime) {
		size_t local_size = 4 * m;
		real_t* host_data = m_host_H_prime_q;
		std::string name = "result Hm Prime";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, m_device_H_prime_q,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}
}

void GPPatch::debug_check_H_and_H_prime_for_tensor_core() {
	real_t tol = 1e-5;

	bool bCheckHHprime_p = true;
	bool bCheckH_q = true;
	bool bCheckHprim_q = true;

	real_t* mapped_device_data = nullptr;

	if (bCheckHHprime_p) {
		size_t local_size = 16 * n;
		real_t* host_data = m_host_HHprime_p;
		real_t* device_data = m_device_HHprime_p;
		std::string name = "result HHprime p";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, device_data,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}

	if (bCheckH_q) {
		size_t local_size = 8 * m;
		real_t* host_data = m_host_H_q_extended;
		real_t* device_data = m_device_H_q_extended;
		std::string name = "result H q extended";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, device_data,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}

	if (bCheckHprim_q) {
		size_t local_size = 8 * m;
		real_t* host_data = m_host_Hprime_q_extended;
		real_t* device_data = m_device_Hprime_q_extended;
		std::string name = "result Hn Prime";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, device_data,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}
}

void GPPatch::debug_check_HHprime_p_C() {
	real_t tol = 1e-5;

	bool bCheckHHprime_p_C = true;

	real_t* mapped_device_data = nullptr;

	if (bCheckHHprime_p_C) {
		size_t local_size = 16 * n;
		real_t* host_data = m_host_HHprime_p_C;
		real_t* device_data = m_device_HHprime_p_C_x;
		std::string name = "result HHprime p * C";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, device_data,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}
}

void GPPatch::debug_check_result_SSv() {
	real_t tol = 5 * 1e-4;

	bool bCheckResult_SSv = true;

	real_t* mapped_device_data = nullptr;

	if (bCheckResult_SSv) {
		size_t local_size = 16 * n;
		real_t* host_data = m_host_result_SSv_x;
		real_t* device_data = m_device_result_SSv_x;
		std::string name = "result (S,Sv)";

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, device_data,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (std::abs(errors) / norm > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}
}

void GPPatch::debug_check_cpu_and_gpu_result(real_t* cpu_data, real_t* gpu_data,
	size_t sz, std::string name,
	real_t tol = 1e-6) {

	bool bCheckResult = true;

	real_t* mapped_device_data = nullptr;

	if (bCheckResult) {
		size_t local_size = sz;
		real_t* host_data = cpu_data;
		real_t* device_data = gpu_data;
		std::string outputname = "result " + name;

		mapped_device_data = new real_t[local_size];

		CUDA_ERROR(cudaMemcpy(mapped_device_data, device_data,
			local_size * sizeof(real_t),
			cudaMemcpyDeviceToHost));

		real_t errors(0);
		real_t norm(0);

		for (size_t i = 0; i < local_size; ++i) {
			auto delta = std::abs(mapped_device_data[i] - host_data[i]);
			errors += delta;
			norm += mapped_device_data[i] * mapped_device_data[i];
		}

		if (errors / sqrt(norm) > tol) {
		}
		else {
		}

		delete[] mapped_device_data;
		mapped_device_data = nullptr;
	}
}
GPEND