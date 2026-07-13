#pragma once

#include "iGameGP_Macros.h"
#include "iGameGP_CUDA_Macros.cuh"
namespace gpmesh {

    struct GPUMemMonitor
    {
        GPUMemMonitor(cudaStream_t stream = NULL) : m_stream(stream)
        {

        }
        ~GPUMemMonitor()
        {

        }

        void start()
        {
            cudaMemGetInfo(&m_start_free, &total);
        }

        void stop()
        {
            cudaMemGetInfo(&m_stop_free, &total);
        }

        void report(string info)
        {
        }

    private:
        size_t  m_start_free, m_stop_free;
        size_t  total;
        cudaStream_t m_stream;
    };
}    