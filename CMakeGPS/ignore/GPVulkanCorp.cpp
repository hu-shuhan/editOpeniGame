#include "GPVulkanCorp.h"
#include <cuda_device_runtime_api.h>
#include <cuda_runtime_api.h>
#include "Util/GP_Macros.h"
#include "Util/GP_CUDA_Macros.cuh"

#ifdef _WIN64

#include <windows.h>
#include <VersionHelpers.h>
#include <dxgi1_2.h>
#include <aclapi.h>

#include <vulkan/vulkan_win32.h>

#endif

VkExternalMemoryHandleTypeFlagBits getDefaultMemHandleType() {
#ifdef _WIN64
    return IsWindows8Point1OrGreater()
           ? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
           : VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
#else
    return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif   
}

void *getMemHandle(VkDevice m_device,
                   VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagBits handleType) {
#ifdef _WIN64
    HANDLE handle = 0;

    VkMemoryGetWin32HandleInfoKHR vkMemoryGetWin32HandleInfoKHR = {};
    vkMemoryGetWin32HandleInfoKHR.sType =
            VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    vkMemoryGetWin32HandleInfoKHR.pNext = NULL;
    vkMemoryGetWin32HandleInfoKHR.memory = memory;
    vkMemoryGetWin32HandleInfoKHR.handleType = handleType;

    PFN_vkGetMemoryWin32HandleKHR fpGetMemoryWin32HandleKHR;
    fpGetMemoryWin32HandleKHR =
            (PFN_vkGetMemoryWin32HandleKHR) vkGetDeviceProcAddr(
                    m_device, "vkGetMemoryWin32HandleKHR");
    if (!fpGetMemoryWin32HandleKHR) {
        throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
    }
    if (fpGetMemoryWin32HandleKHR(m_device, &vkMemoryGetWin32HandleInfoKHR,
                                  &handle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to retrieve handle for buffer!");
    }
    return (void *) handle;
#else
    int fd = -1;

        VkMemoryGetFdInfoKHR vkMemoryGetFdInfoKHR = {};
        vkMemoryGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        vkMemoryGetFdInfoKHR.pNext = NULL;
        vkMemoryGetFdInfoKHR.memory = memory;
        vkMemoryGetFdInfoKHR.handleType = handleType;

        PFN_vkGetMemoryFdKHR fpGetMemoryFdKHR;
        fpGetMemoryFdKHR =
          (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(m_device, "vkGetMemoryFdKHR");
        if (!fpGetMemoryFdKHR) {
        throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
        }
        if (fpGetMemoryFdKHR(m_device, &vkMemoryGetFdInfoKHR, &fd) != VK_SUCCESS) {
        throw std::runtime_error("Failed to retrieve handle for buffer!");
        }
        return (void *)(uintptr_t)fd;
#endif   
}


void importCudaExternalMemory(VkDevice device, void *&cudaMem,
                              const VkDeviceMemory &vkMem, VkDeviceSize size,
                              VkExternalMemoryHandleTypeFlagBits handleType) {

    cudaExternalMemoryHandleDesc externalMemoryHandleDesc = {};

    if (handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT) {
        externalMemoryHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
    } else if (handleType &
               VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT) {
        externalMemoryHandleDesc.type =
                cudaExternalMemoryHandleTypeOpaqueWin32Kmt;
    } else if (handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT) {
        externalMemoryHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    } else {
        throw std::runtime_error("Unknown handle type requested!");
    }

    externalMemoryHandleDesc.size = size;

#ifdef _WIN64
    externalMemoryHandleDesc.handle.win32.handle =
            (HANDLE) getMemHandle(device, vkMem, handleType);
#else
    externalMemoryHandleDesc.handle.fd =
        (int)(uintptr_t)getMemHandle(vkMem, handleType);
#endif

    auto cudaExternalMemPtr = reinterpret_cast<cudaExternalMemory_t *>(&cudaMem);

    CUDA_ERROR(
            cudaImportExternalMemory(cudaExternalMemPtr, &externalMemoryHandleDesc));
}