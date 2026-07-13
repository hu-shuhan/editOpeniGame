#pragma once

#include "Util/iGameGP_Macros.h"
#include "Util/iGameGPSplineDefine.h"

#include "iGameGPPatchData.h"
#include <string>

GPSTART

    class GPPatch {

    public:

        void init_device_cpoints(real_t *device_cpoints_x, real_t *device_cpoints_y, real_t *device_cpoints_z);
        void init_device_scalar_cpoints(real_t* device_cpoints_x, real_t* device_cpoints_y, real_t* device_cpoints_z);

        void init_device_cpoints_extended(real_t* device_cpoints_x_extended, real_t* device_cpoints_y_extended, real_t* device_cpoints_z_extended);
        void init_device_scalar_cpoints_extended(real_t *device_cpoints_x_extended, real_t *device_cpoints_y_extended, real_t *device_cpoints_z_extended);

        void init_host_cpoints(real_t *host_cpoints_x, real_t *host_cpoints_y, real_t *host_cpoints_z);
        void init_host_scalar_cpoints(real_t* host_cpoints_x, real_t* host_cpoints_y, real_t* host_cpoints_z);

        void init_host_cpoints_extended(real_t* host_cpoints_x_extended, real_t* host_cpoints_y_extended, real_t* host_cpoints_z_extended);
        void init_host_scalar_cpoints_extended(real_t* host_cpoints_x_extended, real_t* host_cpoints_y_extended, real_t* host_cpoints_z_extended);


        void init_u(real_t u_begin, real_t u_step);

        void init_v(real_t v_begin, real_t v_step);

        void init_device();
        void bind_device_blob(void* base, size_t bytes, bool zero_init = false);

        static size_t device_blob_stride_bytes();

        void init_stream();

        void evaluation();

        void evaluation_tensor_core(bool bUsingTCEC = true,size_t mm_offset=0);

        void evaluation_HHprime();

        void evaluation_HHprime_p_C();

        void evaluation_SSv();

        void evaluation_Su0();

        void write_data();

        void stream_driven_evaluation();

        void release();

        GPPatchData get_gpu_patch_data()
        {
            return m_gpu_patch_data;
        }

        void debug_check_H_and_H_prime();

        void debug_check_H_and_H_prime_for_tensor_core();

        void debug_check_HHprime_p_C();

        void debug_check_result_SSv();

        void debug_check_cpu_and_gpu_result(real_t* cpu_data, real_t* gpu_data, size_t sz, std::string name, real_t tol);

    public:
        bool bUseDeviceMemoryAllocator = true;

        bool bCompactCUDAMapBuffer = true;

        real_t *m_cuda_position_ptr = nullptr;
        real_t *m_cuda_normal_ptr = nullptr;
        real_t* m_cuda_scalar_ptr = nullptr;


        static const size_t n = _M_N;
        static const size_t m = _M_M;

        bool i_bCCW = true;

        size_t patch_offset;

    private:

        real_t m_u_begin = -1;
        real_t m_u_delta = 0;

        real_t m_v_begin = -1;
        real_t m_v_delta = 0;


        bool bEnableStream = false;
        std::vector<void*> m_xstreams;
        std::vector<void*> m_ystreams;
        std::vector<void*> m_xevents;
        bool m_stream_inited = false;
        real_t *m_device_H_p = nullptr;
        real_t *m_device_H_prime_p = nullptr;

        real_t *m_device_HHprime_p;
        real_t *m_host_HHprime_p;

        real_t* m_device_HHprime_p_s;
        real_t* m_host_HHprime_p_s;

        real_t *m_device_H_q = nullptr;
        real_t *m_device_H_prime_q = nullptr;


        real_t *m_host_H_p = nullptr;
        real_t *m_host_H_prime_p = nullptr;
        real_t *m_host_H_q = nullptr;
        real_t *m_host_H_prime_q = nullptr;

        real_t *m_device_HpP = nullptr;
        real_t *m_device_HqP = nullptr;

        real_t *m_host_HpP = nullptr;
        real_t *m_host_HqP = nullptr;

        real_t *i_device_cpoints_x = nullptr;
        real_t *i_device_cpoints_y = nullptr;
        real_t *i_device_cpoints_z = nullptr;


        real_t *i_host_cpoints_x = nullptr;
        real_t *i_host_cpoints_y = nullptr;
        real_t *i_host_cpoints_z = nullptr;

        real_t* i_device_scalar_cpoints_x = nullptr;
        real_t* i_device_scalar_cpoints_y = nullptr;
        real_t* i_device_scalar_cpoints_z = nullptr;

        real_t* i_host_scalar_cpoints_x = nullptr;
        real_t* i_host_scalar_cpoints_y = nullptr;
        real_t* i_host_scalar_cpoints_z = nullptr;

        real_t *i_device_cpoints_x_extended = nullptr;
        real_t *i_device_cpoints_y_extended = nullptr;
        real_t *i_device_cpoints_z_extended = nullptr;

        real_t *i_host_cpoints_x_extended = nullptr;
        real_t *i_host_cpoints_y_extended = nullptr;
        real_t *i_host_cpoints_z_extended = nullptr;

        real_t* i_device_scalar_cpoints_x_extended = nullptr;
        real_t* i_device_scalar_cpoints_y_extended = nullptr;
        real_t* i_device_scalar_cpoints_z_extended = nullptr;

        real_t* i_host_scalar_cpoints_x_extended = nullptr;
        real_t* i_host_scalar_cpoints_y_extended = nullptr;
        real_t* i_host_scalar_cpoints_z_extended = nullptr;

        real_t *m_device_H_q_extended = nullptr;
        real_t *m_device_Hprime_q_extended = nullptr;

        real_t* m_device_H_q_extended_s = nullptr;
        real_t* m_device_Hprime_q_extended_s = nullptr;

        real_t *m_host_H_q_extended = nullptr;
        real_t *m_host_Hprime_q_extended = nullptr;

        real_t *m_device_HHprime_p_C_x;
        real_t *m_device_HHprime_p_C_y;
        real_t *m_device_HHprime_p_C_z;
        real_t *m_host_HHprime_p_C;

        real_t* m_device_HHprime_p_C_x_s;
        real_t* m_device_HHprime_p_C_y_s;
        real_t* m_device_HHprime_p_C_z_s;
        real_t* m_host_HHprime_p_C_s;

        real_t *m_device_result_SSv_x;
        real_t *m_device_result_SSv_y;
        real_t *m_device_result_SSv_z;

        real_t *m_host_result_SSv_x;
        real_t *m_host_result_SSv_y;
        real_t *m_host_result_SSv_z;

        real_t* m_device_result_SSv_x_s;
        real_t* m_device_result_SSv_y_s;
        real_t* m_device_result_SSv_z_s;

        real_t* m_host_result_SSv_x_s;
        real_t* m_host_result_SSv_y_s;
        real_t* m_host_result_SSv_z_s;

        real_t *m_device_result_Su0_x;
        real_t *m_device_result_Su0_y;
        real_t *m_device_result_Su0_z;

        real_t *m_host_result_Su0_x;
        real_t *m_host_result_Su0_y;
        real_t *m_host_result_Su0_z;

        GPPatchData m_gpu_patch_data;

        void *m_cuda_position_extern_mem = nullptr;
        void *m_cuda_normal_extern_mem = nullptr;

        real_t *m_host_result_position_ptr = nullptr;
        real_t *m_host_result_normal_ptr = nullptr;
        void setup_device_blob_layout_and_ptrs();
        void* m_device_blob = nullptr;
        size_t m_device_blob_bytes = 0;
        bool m_owns_device_blob = true;
        //real_t* m_device_blob = nullptr;
        //size_t m_device_blob_bytes = 0;

    };

GPEND