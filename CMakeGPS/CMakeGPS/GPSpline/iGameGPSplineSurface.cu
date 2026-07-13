#include "iGameGPSplineSurface.h"

#include <algorithm>
#include <cuda_runtime_api.h>
#include <functional>

#include "Util/iGameGP_CUDA_Macros.cuh"
#include "Util/iGameGP_Macros.h"

#include "iGameGPSplineKernel.cuh"
#include "iGameGPSplineKernelCPU.h"
#include "iGameGemmKernel.cuh"
#include "iGameGemmKernelCPU.h"

GPSTART
void GPSplineSurface::init(CBSplineSurface& surface) {
    init_knot_vector(surface);
    init_cpoints(surface);
    init_memory_pools(surface);

    build_tessellation(1, 1);
    build_tessellation_cpu(1, 1);

    compare_cpu_and_gpu_data();

    build_tessellation(3, 3);
    build_tessellation_cpu(3, 3);

    compare_cpu_and_gpu_data();

    build_tessellation(MAX_P, MAX_Q);
    build_tessellation_cpu(MAX_P, MAX_Q);

    compare_cpu_and_gpu_data();
}

void GPSplineSurface::init_knot_vector(CBSplineSurface& surface) {
    host_uKnots = new real_t[uSize]{0};

    for (int i = 4; i < 8; ++i) { host_uKnots[i] = 1.0; }

    host_vKnots = new real_t[vSize]{0};

    for (int i = 4; i < 8; ++i) { host_vKnots[i] = 1.0; }

    size_t uDataSize = uSize * sizeof(real_t);
    CUDA_ERROR(cudaMalloc(&device_uKnots, uDataSize));
    CUDA_ERROR(cudaMemcpyAsync(device_uKnots, host_uKnots, uDataSize,
                               cudaMemcpyHostToDevice));

    size_t vDataSize = vSize * sizeof(real_t);
    CUDA_ERROR(cudaMalloc(&device_vKnots, vDataSize));
    CUDA_ERROR(cudaMemcpyAsync(device_vKnots, host_vKnots, vDataSize,
                               cudaMemcpyHostToDevice));
}

void GPSplineSurface::init_cpoints(CBSplineSurface& surface) {
    const size_t CPOINT_SIZE = 4 * 4;
    const int ROW_NUM = 4;
    const int COL_NUM = 4;
    host_cpoints_x = new real_t[CPOINT_SIZE]{0};
    host_cpoints_y = new real_t[CPOINT_SIZE]{0};
    host_cpoints_z = new real_t[CPOINT_SIZE]{0};

    auto get_cpoint_index_RowMajor = [COL_NUM](int i, int j) -> int {
        int res = i * COL_NUM + j;

        assert(res < 16 && res >= 0);

        return res;
    };

    auto get_cpoint_index_ColMajor = [ROW_NUM](int i, int j) -> int {
        int res = i + j * ROW_NUM;

        assert(res < 16 && res >= 0);

        return res;
    };


    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            host_cpoints_x[get_cpoint_index_ColMajor(i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getX());
            host_cpoints_y[get_cpoint_index_ColMajor(i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getY());
            host_cpoints_z[get_cpoint_index_ColMajor(i, j)] =
                    static_cast<float>(surface.getControlPoints()[i][j].getZ());
        }
    }

    CUDA_ERROR(cudaMalloc(&device_cpoints_x, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(device_cpoints_x, host_cpoints_x,
                               CPOINT_SIZE * sizeof(real_t),
                               cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&device_cpoints_y, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(device_cpoints_y, host_cpoints_y,
                               CPOINT_SIZE * sizeof(real_t),
                               cudaMemcpyHostToDevice));

    CUDA_ERROR(cudaMalloc(&device_cpoints_z, CPOINT_SIZE * sizeof(real_t)));
    CUDA_ERROR(cudaMemcpyAsync(device_cpoints_z, host_cpoints_z,
                               CPOINT_SIZE * sizeof(real_t),
                               cudaMemcpyHostToDevice));
}

void GPSplineSurface::init_memory_pools(CBSplineSurface& surface) {

    size_t n = 7 * (MAX_P + 0) + 1;
    size_t m = 7 * (MAX_Q + 0) + 1;
    size_t nm = n * m;

    host_memPool_H_p = new real_t[4 * n];
    host_memPool_H_prime_p = new real_t[4 * n];

    host_memPool_H_q = new real_t[4 * m];
    host_memPool_H_prime_q = new real_t[4 * m];

    host_memPool_HnP_tmp = new real_t[4 * n];
    host_memPool_HmP_tmp = new real_t[4 * m];

    host_memPool_result_x = new real_t[nm];
    host_memPool_result_y = new real_t[nm];
    host_memPool_result_z = new real_t[nm];

    CUDA_ERROR(cudaMalloc(&device_memPool_H_p, 4 * n * sizeof(real_t)));

    CUDA_ERROR(cudaMalloc(&device_memPool_H_prime_p, 4 * n * sizeof(real_t)));

    CUDA_ERROR(cudaMalloc(&device_memPool_H_q, 4 * m * sizeof(real_t)));

    CUDA_ERROR(cudaMalloc(&device_memPool_H_prime_q, 4 * m * sizeof(real_t)));

    CUDA_ERROR(cudaMalloc(&device_memPool_HnP_tmp, 4 * n * sizeof(real_t)));
    CUDA_ERROR(cudaMalloc(&device_memPool_HmP_tmp, 4 * m * sizeof(real_t)));

    CUDA_ERROR(cudaMalloc(&device_memPool_result_x, nm * sizeof(real_t)));
    CUDA_ERROR(cudaMalloc(&device_memPool_result_y, nm * sizeof(real_t)));
    CUDA_ERROR(cudaMalloc(&device_memPool_result_z, nm * sizeof(real_t)));
}

void GPSplineSurface::release() {
    release_knot_vector();
    release_cpoints();
    release_memory_pools();
}

void GPSplineSurface::release_knot_vector() {
    if (host_uKnots != nullptr) {
        delete[] host_uKnots;
        host_uKnots = nullptr;
    }

    if (host_vKnots != nullptr) {
        delete[] host_vKnots;
        host_uKnots = nullptr;
    }

    GPU_FREE(device_uKnots);
    GPU_FREE(device_vKnots);
}

void GPSplineSurface::release_cpoints() {
    if (host_cpoints_x != nullptr) {
        delete[] host_cpoints_x;
        host_cpoints_x = nullptr;
    }

    if (host_cpoints_y != nullptr) {
        delete[] host_cpoints_y;
        host_cpoints_y = nullptr;
    }

    if (host_cpoints_z != nullptr) {
        delete[] host_cpoints_z;
        host_cpoints_z = nullptr;
    }

    GPU_FREE(device_cpoints_x);
    GPU_FREE(device_cpoints_y);
    GPU_FREE(device_cpoints_z);
}

void GPSplineSurface::release_memory_pools() {
    if (host_memPool_H_p != nullptr) {
        delete[] host_memPool_H_p;
        host_memPool_H_p = nullptr;
    }
    if (host_memPool_H_prime_p != nullptr) {
        delete[] host_memPool_H_prime_p;
        host_memPool_H_prime_p = nullptr;
    }

    if (host_memPool_H_q != nullptr) {
        delete[] host_memPool_H_q;
        host_memPool_H_q = nullptr;
    }
    if (host_memPool_H_prime_q != nullptr) {
        delete[] host_memPool_H_prime_q;
        host_memPool_H_prime_q = nullptr;
    }

    if (host_memPool_HnP_tmp != nullptr) {
        delete[] host_memPool_HnP_tmp;
        host_memPool_HnP_tmp = nullptr;
    }

    if (host_memPool_HmP_tmp != nullptr) {
        delete[] host_memPool_HmP_tmp;
        host_memPool_HmP_tmp = nullptr;
    }

    if (host_memPool_result_x != nullptr) {
        delete[] host_memPool_result_x;
        host_memPool_result_x = nullptr;
    }

    if (host_memPool_result_y != nullptr) {
        delete[] host_memPool_result_y;
        host_memPool_result_y = nullptr;
    }

    if (host_memPool_result_z != nullptr) {
        delete[] host_memPool_result_z;
        host_memPool_result_z = nullptr;
    }

    GPU_FREE(device_memPool_H_p);
    GPU_FREE(device_memPool_H_prime_p);
    GPU_FREE(device_memPool_HnP_tmp);

    GPU_FREE(device_memPool_H_q);
    GPU_FREE(device_memPool_HmP_tmp);
    GPU_FREE(device_memPool_H_prime_q);

    GPU_FREE(device_memPool_result_x);
    GPU_FREE(device_memPool_result_y);
    GPU_FREE(device_memPool_result_z);
}

void GPSplineSurface::build_tessellation(uint16_t new_p, uint16_t new_q) {

    CUDA_ERROR(cudaDeviceSynchronize());
    size_t n = 7 * new_p + 1;
    size_t m = 7 * new_q + 1;

    if (new_p != cur_p) {

        cur_p = new_p;

        dim3 threadsPerBlock;

        threadsPerBlock.x = static_cast<uint32_t>(n);
        threadsPerBlock.y = 1;
        threadsPerBlock.z = 1;
        evaluation_D01_GISMO<real_t><<<1, threadsPerBlock>>>(
                new_p, device_memPool_H_p, device_memPool_H_prime_p);

        CUDA_ERROR(cudaGetLastError());
        CUDA_ERROR(cudaDeviceSynchronize());

        threadsPerBlock.y = static_cast<uint32_t>(4);

        simtNaiveKernel<<<1, threadsPerBlock>>>(
                device_memPool_H_p, device_cpoints_x, device_memPool_HnP_tmp, n,
                4, 4);

        CUDA_ERROR(cudaGetLastError());
        CUDA_ERROR(cudaDeviceSynchronize());
    }

    if (new_q != cur_q) {

        cur_q = new_q;

        dim3 threadsPerBlock;

        threadsPerBlock.x = static_cast<uint32_t>(m);
        threadsPerBlock.y = 1;
        threadsPerBlock.z = 1;

        evaluation_D01_GISMO<real_t><<<1, threadsPerBlock>>>(
                new_q, device_memPool_H_q, device_memPool_H_prime_q);

        CUDA_ERROR(cudaGetLastError());
        CUDA_ERROR(cudaDeviceSynchronize());

        threadsPerBlock.y = static_cast<uint32_t>(4);

        simtNaiveKernel<<<1, threadsPerBlock>>>(
                device_memPool_H_q, device_cpoints_x, device_memPool_HmP_tmp, m,
                4, 4);

        CUDA_ERROR(cudaGetLastError());
        CUDA_ERROR(cudaDeviceSynchronize());
    }

    CUDA_ERROR(cudaGetLastError());

    dim3 threadsPerBlock(16, 16, 1);
    dim3 grid;
    grid.x = static_cast<uint32_t>(DIVIDE_UP(n, 16));
    grid.y = static_cast<uint32_t>(DIVIDE_UP(m, 16));
    grid.z = 1;

    simtNaiveKernel_BigMN_GridBlock<<<grid, threadsPerBlock>>>(
            device_memPool_HnP_tmp, device_memPool_H_q, device_memPool_result_x,
            n, m, 4);

    CUDA_ERROR(cudaGetLastError());
    CUDA_ERROR(cudaDeviceSynchronize());
}

void GPSplineSurface::build_tessellation_cpu(uint16_t new_p, uint16_t new_q) {

    size_t n = 7 * new_p + 1;

    size_t m = 7 * new_q + 1;

    {
        cur_p = new_p;

        evaluation_D01_CPU<real_t>(new_p, host_memPool_H_p,
                                   host_memPool_H_prime_p);

        NativeKernelCPU<real_t>(host_memPool_H_p, host_cpoints_x,
                                host_memPool_HnP_tmp, n, 4, 4);
    }

    {
        cur_q = new_q;

        evaluation_D01_CPU<real_t>(new_q, host_memPool_H_q,
                                   host_memPool_H_prime_q);

        NativeKernelCPU<real_t>(host_memPool_H_q, host_cpoints_x,
                                host_memPool_HmP_tmp, m, 4, 4);
    }

    {
        NativeKernelCPU<real_t>(host_memPool_HnP_tmp, host_memPool_H_q,
                                host_memPool_result_x, n, m, 4);
    }
}

void GPSplineSurface::compare_cpu_and_gpu_data() const {
    real_t tol = 1e-10;

    int n = 7 * cur_p + 1;
    int m = 7 * cur_q + 1;

    real_t* mapped_device_data = nullptr;

    bool bCheckHn = true;
    bool bCheckHnPrime = true;
    bool bCheckHm = true;
    bool bCheckHmPrime = true;

    bool bCheckHnP = true;
    bool bCheckHmP = true;

    bool bCheckResultX = true;

    if (bCheckHn) {
        mapped_device_data = new real_t[4 * n];

        CUDA_ERROR(cudaMemcpy(mapped_device_data, device_memPool_H_p,
                              4 * n * sizeof(real_t), cudaMemcpyDeviceToHost));

        real_t errors(0);
        real_t norm(0);
        for (int i = 0; i < 4 * n; ++i) {
            errors += std::abs(mapped_device_data[i] - host_memPool_H_p[i]);
            norm += mapped_device_data[i] * mapped_device_data[i];
        }

        if (std::abs(errors) > tol) {
        } else {
        }

        delete[] mapped_device_data;
        mapped_device_data = nullptr;
    }

    if (bCheckHnPrime) {
        mapped_device_data = new real_t[4 * n];

        CUDA_ERROR(cudaMemcpy(mapped_device_data, device_memPool_H_prime_p,
                              4 * n * sizeof(real_t), cudaMemcpyDeviceToHost));

        real_t errors(0);
        real_t norm(0);
        for (int i = 0; i < 4 * n; ++i) {
            errors +=
                    std::abs(mapped_device_data[i] - host_memPool_H_prime_p[i]);
            norm += mapped_device_data[i] * mapped_device_data[i];
        }

        if (std::abs(errors) > tol) {
        } else {
        }

        delete[] mapped_device_data;
        mapped_device_data = nullptr;
    }


    if (bCheckHm) {
        mapped_device_data = new real_t[4 * m];

        CUDA_ERROR(cudaMemcpy(mapped_device_data, device_memPool_H_q,
                              4 * m * sizeof(real_t), cudaMemcpyDeviceToHost));

        real_t errors(0);
        real_t norm(0);
        for (int i = 0; i < 4 * n; ++i) {
            errors += std::abs(mapped_device_data[i] - host_memPool_H_q[i]);
            norm += mapped_device_data[i] * mapped_device_data[i];
        }

        if (std::abs(errors) > tol) {
        } else {
        }

        delete[] mapped_device_data;
        mapped_device_data = nullptr;
    }


    if (bCheckHmPrime) {
        mapped_device_data = new real_t[4 * m];

        CUDA_ERROR(cudaMemcpy(mapped_device_data, device_memPool_H_prime_q,
                              4 * m * sizeof(real_t), cudaMemcpyDeviceToHost));

        real_t errors(0);
        real_t norm(0);
        for (int i = 0; i < 4 * n; ++i) {
            errors +=
                    std::abs(mapped_device_data[i] - host_memPool_H_prime_q[i]);
            norm += mapped_device_data[i] * mapped_device_data[i];
        }

        if (std::abs(errors) > tol) {
        } else {
        }

        delete[] mapped_device_data;
        mapped_device_data = nullptr;
    }

    if (bCheckHnP) {
        mapped_device_data = new real_t[4 * n];

        CUDA_ERROR(cudaMemcpy(mapped_device_data, device_memPool_HnP_tmp,
                              4 * n * sizeof(real_t), cudaMemcpyDeviceToHost));

        real_t errors(0);
        real_t norm(0);
        for (int i = 0; i < 4 * n; ++i) {
            errors += std::abs(mapped_device_data[i] - host_memPool_HnP_tmp[i]);
            norm += mapped_device_data[i] * mapped_device_data[i];
        }

        if (std::abs(errors) > tol) {
        } else {
        }

        delete[] mapped_device_data;
        mapped_device_data = nullptr;
    }


    if (bCheckHmP) {
        mapped_device_data = new real_t[4 * m];

        CUDA_ERROR(cudaMemcpy(mapped_device_data, device_memPool_HmP_tmp,
                              4 * m * sizeof(real_t), cudaMemcpyDeviceToHost));

        real_t errors(0);
        real_t norm(0);
        for (int i = 0; i < 4 * m; ++i) {
            errors += std::abs(mapped_device_data[i] - host_memPool_HmP_tmp[i]);
            norm += mapped_device_data[i] * mapped_device_data[i];
        }

        if (std::abs(errors) > tol) {
        } else {
        }

        delete[] mapped_device_data;
        mapped_device_data = nullptr;
    }

    if (bCheckResultX) {
        size_t local_size = n * m;
        real_t* host_data = host_memPool_result_x;
        string name = "result X";

        mapped_device_data = new real_t[local_size];

        CUDA_ERROR(cudaMemcpy(mapped_device_data, device_memPool_result_x,
                              local_size * sizeof(real_t),
                              cudaMemcpyDeviceToHost));

        real_t errors(0);
        real_t norm(0);
        for (int i = 0; i < local_size; ++i) {
            errors += std::abs(mapped_device_data[i] - host_data[i]);
            norm += mapped_device_data[i] * mapped_device_data[i];
        }

        if (std::abs(errors) > tol) {
        } else {
        }

        delete[] mapped_device_data;
        mapped_device_data = nullptr;
    }
}


GPEND
