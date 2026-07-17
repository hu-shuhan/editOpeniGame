#pragma once

#include <vulkan/vulkan_core.h>

VkExternalMemoryHandleTypeFlagBits getDefaultMemHandleType();

void *getMemHandle(VkDevice m_device,
                   VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagBits handleType);

void importCudaExternalMemory(VkDevice device, void *&cudaMem,
                              const VkDeviceMemory &vkMem, VkDeviceSize size,
                              VkExternalMemoryHandleTypeFlagBits handleType);
