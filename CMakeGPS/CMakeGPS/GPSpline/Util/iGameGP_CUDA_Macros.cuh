#pragma once

#include <cuda_runtime_api.h>

namespace gpmesh {

#define IS_D_LAMBDA(X) __nv_is_extended_device_lambda_closure_type(X)
#define IS_HD_LAMBDA(X) __nv_is_extended_host_device_lambda_closure_type(X)

    inline void HandleError(cudaError_t err, const char *file, int line) {
        if (err != cudaSuccess) {
#ifdef _WIN32
            system("pause");
#else
            exit(EXIT_FAILURE);
#endif
        }
    }

#define CUDA_ERROR(err) (::gpmesh::HandleError(err, __FILE__, __LINE__))

#define GPU_FREE(ptr)              \
    if (ptr != nullptr) {          \
        CUDA_ERROR(cudaFree(ptr)); \
        ptr = nullptr;             \
    }

#if defined(__CUDACC__)   
#define ALIGN(n) __align__(n)
#elif defined(__GNUC__)    
#define ALIGN(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)   
#define ALIGN(n) __declspec(align(n))
#else
#error "Please provide a definition for MY_ALIGN macro for your host compiler!"
#endif

    const unsigned int warp_size = 32;

}