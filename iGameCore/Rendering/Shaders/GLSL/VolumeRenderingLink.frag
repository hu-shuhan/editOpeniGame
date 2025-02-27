#version 420 core

layout (early_fragment_tests) in;

layout(std140, binding = 0) uniform CameraDataBlock {
    vec3 viewPos;
    int isOrtho;
    mat4 view;
    mat4 proj;
    mat4 proj_view;// proj * view
} cameraData;

layout(std140, binding = 1) uniform ObjectDataBLock {
    float transparent;
    mat4 model;
    mat4 normal;// transpose(inverse(model))
    vec4 sphereBounds;
} objectData;

layout(std140, binding = 2) uniform UniformBufferObjectBlock {
    int useColor;
    int useNormalSmooth;
} ubo;

// Atomic counter, used to allocate data to the linked list
layout(binding = 0, offset = 0) uniform atomic_uint indexCounter;

layout(binding = 0, r32ui) uniform uimage2D headPointerImage;
layout(binding = 1, rgba32ui) uniform uimageBuffer listBuffer;

layout(location = 0) in vec3 in_MCPosition;
layout(location = 1) in vec3 in_VCPosition;
layout(location = 2) in vec3 in_Color;
layout(location = 3) in vec3 in_Normal;
layout(location = 4) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

#define MAX_FRAGMENTS 256

void main() {
    vec4 fragColor = vec4(in_Color, objectData.transparent);

    uint newHead = atomicCounterIncrement(indexCounter);
    uint oldHead = imageAtomicExchange(headPointerImage, ivec2(gl_FragCoord.xy), newHead);

    // in_VCPosition.length() will return wrong value
    float dist = length(in_VCPosition);
    if (in_VCPosition.z > 0.0f || dist < 0.01f) {
        return;
    }

    uvec4 item = uvec4(0.0f);// 初始化 item
    item.x = oldHead;
    item.y = packUnorm4x8(fragColor);
    item.z = floatBitsToUint(dist);

    // 计算当前片段的数量
    uint fragmentCount = 1;
    if (oldHead != 0xFFFFFFFF) {
        uvec4 oldItem = imageLoad(listBuffer, int(oldHead));
        fragmentCount = oldItem.w + 1;
    }

    // 如果片段数未超过 MAX_FRAGMENTS，直接插入
    if (fragmentCount <= MAX_FRAGMENTS) {
        item.w = fragmentCount;
        imageStore(listBuffer, int(newHead), item);
    } else {
        // 超过 MAX_FRAGMENTS，找到最远距离片段
        float maxDist = 0.0f;// 设定一个足够小的初始值
        uint maxDistIndex = newHead;// 初始化为当前片段
        uint head = oldHead;

        for (uint i = 0; i < MAX_FRAGMENTS; i++) {
            uvec4 fragment = imageLoad(listBuffer, int(head));
            float dist = uintBitsToFloat(fragment.z);
            if (dist > maxDist) {
                maxDist = dist;
                maxDistIndex = head;
            }
            head = fragment.x;// 前往下一个片段
            if (head == 0xFFFFFFFF) break;// 终止条件
        }

        // 如果当前片段距离值比最大距离小，替换最大距离片段
        float currentDist = uintBitsToFloat(item.z);
        if (currentDist < maxDist) {
            item.w = MAX_FRAGMENTS;// 设置片段数量为 MAX_FRAGMENTS
            imageStore(listBuffer, int(maxDistIndex), item);// 替换最大距离片段
        }
    }
}
