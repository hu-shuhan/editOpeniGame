#include "Util/iGameGPSplineDefine.h"
#include "Util/iGameGP_Macros.h"
#include "iGameGPCadscene.h"

#include "Util/iGameGPSceneDefine.h"

#include "Util/iGameGP_CPU_Timer.h"
#include "Util/iGameGP_CUDA_Macros.cuh"
#include "Util/iGameGP_Mem_Monitor.cuh"

#include <GPHelperIO/iGameGP_Surface_Convert.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "iGameGPSceneEvaluationKernel.cuh"
#include "iGameGPTessellationFactorEstimationKernel.cuh"
#include "iGameGPTessellationFactorEstimationKernelCPU.h"

#include <omp.h>
#include <chrono>
#include <fstream>
#include <iomanip>
GPSTART

static inline void safeCudaFree(void*& p) {
    if (p) {
        cudaFree(p);
        p = nullptr;
    }
}
void CadSceneGP::release() noexcept {
    cudaDeviceSynchronize();
    if (m_device_surface_patch_data_arr) {
        cudaFree(m_device_surface_patch_data_arr);
        m_device_surface_patch_data_arr = nullptr;
    }

    safeCudaFree(reinterpret_cast<void*&>(m_device_surface_control_points_arr));
    safeCudaFree(reinterpret_cast<void*&>(m_device_surface_control_points_extended_arr));
    safeCudaFree(reinterpret_cast<void*&>(m_device_surface_scalar_points_arr));
    safeCudaFree(reinterpret_cast<void*&>(m_device_surface_scalar_points_extended_arr));

    safeCudaFree(reinterpret_cast<void*&>(m_device_delta_u));
    safeCudaFree(reinterpret_cast<void*&>(m_device_delta_v));
    safeCudaFree(reinterpret_cast<void*&>(m_device_p));
    safeCudaFree(reinterpret_cast<void*&>(m_device_q));

    safeCudaFree(reinterpret_cast<void*&>(device_ptr_gpsurfaces));

    m_device_positionx_ptr = nullptr;
    m_device_normalx_ptr = nullptr;
    m_device_scalarx_ptr = nullptr;

    delete[] m_host_mapping_surface_control_points_arr;
    m_host_mapping_surface_control_points_arr = nullptr;

    delete[] m_host_mapping_surface_control_points_extended_arr;
    m_host_mapping_surface_control_points_extended_arr = nullptr;

    delete[] m_host_mapping_surface_scalar_points_arr;
    m_host_mapping_surface_scalar_points_arr = nullptr;

    delete[] m_host_mapping_surface_scalar_points_extended_arr;
    m_host_mapping_surface_scalar_points_extended_arr = nullptr;

    delete[] m_host_delta_u;
    m_host_delta_u = nullptr;
    delete[] m_host_delta_v;
    m_host_delta_v = nullptr;
    delete[] m_host_p;
    m_host_p = nullptr;
    delete[] m_host_q;
    m_host_q = nullptr;
    m_host_positionx_ptr = nullptr;
    m_host_normalx_ptr = nullptr;
    m_host_scalarx_ptr = nullptr;


    for (auto* p: m_cuda_ptr_arr) delete[] p;
    for (auto* p: m_cuda_normal_arr) delete[] p;
    for (auto* p: m_cuda_scalar_arr) delete[] p;
    m_cuda_ptr_arr.clear();
    m_cuda_normal_arr.clear();
    m_cuda_scalar_arr.clear();

    for (void* s: m_cuda_streams) cudaStreamDestroy(reinterpret_cast<cudaStream_t>(s));
    for (void* e: m_cuda_events) cudaEventDestroy(reinterpret_cast<cudaEvent_t>(e));
    m_cuda_streams.clear();
    m_cuda_events.clear();

    for (void* m: m_cuda_extern_mem_arr) cudaDestroyExternalMemory(reinterpret_cast<cudaExternalMemory_t>(m));
    m_cuda_extern_mem_arr.clear();
    host_gpsurfaces.clear();
    host_patchsurfaces.clear();
    m_host_mapping_surface_patch_data_arr.clear();

    iboDataCCW.clear();
    iboDataCW.clear();
    m_materials.clear();
    m_matrices.clear();
    m_objects.clear();

    m_num_surface = 0;
}

void CadSceneGP::init_cuda(uint8_t* vkDeviceUUID, size_t UUID_SIZE) {
    int current_device = 0;
    int device_count = 0;
    int devices_prohibited = 0;

    cudaDeviceProp deviceProp;
    CUDA_ERROR(cudaGetDeviceCount(&device_count));

    if (device_count == 0) {
        fprintf(stderr, "CUDA error: no devices supporting CUDA.\n");
        exit(EXIT_FAILURE);
    }

    while (current_device < device_count) {
        cudaGetDeviceProperties(&deviceProp, current_device);

        if ((deviceProp.computeMode != cudaComputeModeProhibited)) {
            int ret = memcmp((void*) &deviceProp.uuid, vkDeviceUUID, UUID_SIZE);
            if (ret == 0) {
                CUDA_ERROR(cudaSetDevice(current_device));
                CUDA_ERROR(cudaGetDeviceProperties(&deviceProp, current_device));
                break;
            }

        } else {
            devices_prohibited++;
        }

        current_device++;
    }

    if (devices_prohibited == device_count) {
        fprintf(stderr, "CUDA error:"
                        " No Vulkan-CUDA Interop capable GPU found.\n");
        printf("Error: No CUDA-Vulkan interop capable device found\n");
        exit(EXIT_FAILURE);
    }


    cudaDeviceProp prop = {};
    CUDA_ERROR(cudaSetDevice(current_device));
    CUDA_ERROR(cudaGetDeviceProperties(&prop, current_device));

    m_threads = prop.warpSize;
}

glm::vec4 randomVector(float from, float to) {
    glm::vec4 vec;
    float width = to - from;
    for (int i = 0; i < 4; i++) { vec[i] = from + (float(rand()) / float(RAND_MAX)) * width; }
    return vec;
}

inline float frand() { return float(rand() % RAND_MAX) / float(RAND_MAX); }

std::vector<gpmesh::GPSplinePatchSurface>& CadSceneGP::init_scene(gpbezier::SurfaceConvertHelper& helper, int pq) {
    release();

    //auto start_time = std::chrono::high_resolution_clock::now();
    //std::ofstream time_file("F:\\Program\\editOpeniGame2\\log\\log1.txt", std::ios::app);


    int maxpq = pq;
    int maxpq2 = maxpq * maxpq;

    size_t surface_num = helper.getSurfaces().size();

    m_num_surface = surface_num;

    m_host_mapping_surface_control_points_arr = new GPSurfaceControlPoint[m_num_surface];

    m_host_mapping_surface_control_points_extended_arr = new GPSurfaceControlPoint[m_num_surface];

    m_host_mapping_surface_scalar_points_arr = new GPSurfaceControlPoint[m_num_surface];

    m_host_mapping_surface_scalar_points_extended_arr = new GPSurfaceControlPoint[m_num_surface];

    m_host_mapping_surface_patch_data_arr.resize(m_num_surface);

    bool bUseSplineSurface = true;

    //auto end_time = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration<double>(end_time - start_time);

    //if (time_file.is_open()) {
    //    time_file << "GPU Processing Time1: " << std::fixed << std::setprecision(3) << duration.count() << " s"
    //              << std::endl;
    //}

    bool bUsePatchSurface = true;
    if (bUsePatchSurface) {

        host_patchsurfaces.resize(surface_num);

        for (size_t i = 0; i < surface_num; ++i) {
            auto& patchsurface = host_patchsurfaces[i];
            patchsurface.init_surface(helper.getSurfaces()[i]);
            m_host_mapping_surface_control_points_arr[i] = patchsurface.get_device_surface_control_points();

            m_host_mapping_surface_control_points_extended_arr[i] =
                    patchsurface.get_device_surface_control_points_extended();

            m_host_mapping_surface_scalar_points_arr[i] = patchsurface.get_device_surface_scalar_points();

            patchsurface.bCompactCUDAMapBuffer = this->bCompactCUDAMapBuffer;
            patchsurface.init_patches();

            m_host_mapping_surface_patch_data_arr[i] = patchsurface.get_device_patch_data_arr();
        }




        CUDA_ERROR(cudaMalloc(&m_device_surface_patch_data_arr, m_num_surface * sizeof(GPPatchData*)));
        CUDA_ERROR(cudaMemcpy(m_device_surface_patch_data_arr, m_host_mapping_surface_patch_data_arr.data(),
                              m_num_surface * sizeof(GPPatchData*), cudaMemcpyHostToDevice));

        GPUMemMonitor monitor;

        monitor.start();


        real_t* d_pos = nullptr;
        real_t* d_norm = nullptr;
        real_t* d_scalar = nullptr;
        CUDA_ERROR(cudaMalloc(&d_pos, maxpq2 * 192 * sizeof(real_t)));
        CUDA_ERROR(cudaMalloc(&d_norm, maxpq2 * 192 * sizeof(real_t)));
        CUDA_ERROR(cudaMalloc(&d_scalar, maxpq2 * 192 * sizeof(real_t)));
        //end_time = std::chrono::high_resolution_clock::now();
        //duration = std::chrono::duration<double>(end_time - start_time);
        //if (time_file.is_open()) {
        //    time_file << "GPU Processing Time2: " << std::fixed << std::setprecision(3) << duration.count() << " s"
        //              << std::endl;
        //}
        size_t now_i = 0;
        for (auto& patchsurface: host_patchsurfaces) {
            patchsurface.m_host_positions_ptr = d_pos;
            patchsurface.m_host_normal_ptr = d_norm;
            patchsurface.m_host_scalar_ptr = d_scalar;

            patchsurface.build_tessellation(maxpq, maxpq);

            if (m_cuda_ptr_arr.size() <= now_i) {
                m_cuda_ptr_arr.push_back(new real_t[maxpq2 * 192]{0});
                m_cuda_normal_arr.push_back(new real_t[maxpq2 * 192]{0});
                m_cuda_scalar_arr.push_back(new real_t[maxpq2 * 192]{0});
            }

            CUDA_ERROR(cudaMemcpy(m_cuda_ptr_arr[now_i], d_pos, maxpq2 * 192 * sizeof(real_t), cudaMemcpyDeviceToHost));
            CUDA_ERROR(cudaMemcpy(m_cuda_normal_arr[now_i], d_norm, maxpq2 * 192 * sizeof(real_t),
                                  cudaMemcpyDeviceToHost));
            CUDA_ERROR(cudaMemcpy(m_cuda_scalar_arr[now_i], d_scalar, maxpq2 * 192 * sizeof(real_t),
                                  cudaMemcpyDeviceToHost));

            now_i++;
        }
        //end_time = std::chrono::high_resolution_clock::now();
        //duration = std::chrono::duration<double>(end_time - start_time);

        //if (time_file.is_open()) {
        //    time_file << "GPU Processing Time3: " << std::fixed << std::setprecision(3) << duration.count() << " s"
        //              << std::endl;
        //    time_file.close();
        //}

        cudaFree(d_pos);
        cudaFree(d_norm);
        cudaFree(d_scalar);
    }

    auto csf = helper.getCSFile();


    return host_patchsurfaces;
}


GPEND