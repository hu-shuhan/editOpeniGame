#pragma once
#include <cuda_runtime_api.h>
#include "../Kernels/iGameGP_Get_Arch.cuh"
#include "iGameGP_Log.h"
#include "iGameGP_Macros.h"

namespace gpmesh {
inline int convert_SMV_to_cores(int major, int minor)
{
    typedef struct
    {
        int SM;             
        int Cores;
    } sSMtoCores;

    sSMtoCores nGpuArchCoresPerSM[] = {
        {0x30, 192},        
        {0x32, 192},        
        {0x35, 192},        
        {0x37, 192},        
        {0x50, 128},        
        {0x52, 128},        
        {0x53, 128},        
        {0x60, 64},         
        {0x61, 128},        
        {0x62, 128},        
        {0x70, 64},         
        {0x72, 64},
        {0x75, 64},
        {0x80, 64},
        {0x86, 128},
        {-1, -1}};

    int index = 0;

    while (nGpuArchCoresPerSM[index].SM != -1) {
        if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor)) {
            return nGpuArchCoresPerSM[index].Cores;
        }
        index++;
    }

    printf(
        "MapSMtoCores for SM %d.%d is undefined.  Default to use %d Cores/SM\n",
        major,
        minor,
        nGpuArchCoresPerSM[index - 1].Cores);
    return nGpuArchCoresPerSM[index - 1].Cores;
}

inline cudaDeviceProp cuda_query(const int dev, bool quite = false)
{

    int deviceCount;
    cudaGetDeviceCount(&deviceCount);

    if (deviceCount == 0) {
        GPMESH_ERROR(
            "cuda_query() device count = 0 i.e., there is not"
            " a CUDA-supported GPU!!!");
    }

    CUDA_ERROR(cudaSetDevice(dev));
    cudaDeviceProp dev_prop;

    CUDA_ERROR(cudaGetDeviceProperties(&dev_prop, dev));

    if (!quite) {

        GPMESH_TRACE("Total number of device: {}", deviceCount);
        GPMESH_TRACE("Using device Number: {}", dev);

        GPMESH_TRACE("Device name: {}", dev_prop.name);
        GPMESH_TRACE("Compute Capability: {}.{}",
                     (int)dev_prop.major,
                     (int)dev_prop.minor);
        GPMESH_TRACE("Total amount of global memory (MB): {0:.1f}",
                     (float)dev_prop.totalGlobalMem / 1048576.0f);
        GPMESH_TRACE("{} Multiprocessors, {} CUDA Cores/MP: {} CUDA Cores",
                     dev_prop.multiProcessorCount,
                     convert_SMV_to_cores(dev_prop.major, dev_prop.minor),
                     convert_SMV_to_cores(dev_prop.major, dev_prop.minor) *
                         dev_prop.multiProcessorCount);
        GPMESH_TRACE("ECC support: {}",
                     (dev_prop.ECCEnabled ? "Enabled" : "Disabled"));
        GPMESH_TRACE("GPU Max Clock rate: {0:.1f} MHz ({1:.2f} GHz)",
                     dev_prop.clockRate * 1e-3f,
                     dev_prop.clockRate * 1e-6f);
        GPMESH_TRACE("Memory Clock rate: {0:.1f} Mhz",
                     dev_prop.memoryClockRate * 1e-3f);
        GPMESH_TRACE("Memory Bus Width:  {}-bit", dev_prop.memoryBusWidth);
        const double maxBW = 2.0 * dev_prop.memoryClockRate *
                             (dev_prop.memoryBusWidth / 8.0) / 1.0E6;
        GPMESH_TRACE("Peak Memory Bandwidth: {0:f}(GB/s)", maxBW);
        GPMESH_TRACE("Kernels compiled for compute capability: {}",
                     cuda_arch());
    }

    if (!dev_prop.managedMemory) {
        GPMESH_ERROR(
            "The selected device does not support CUDA unified memory");
        exit(EXIT_FAILURE);
    }

    return dev_prop;
}
}    