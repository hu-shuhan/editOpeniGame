#version 420 core

layout (early_fragment_tests) in;

layout(std140, binding = 0) uniform CameraDataBlock {
    vec3 viewPos;
    int isOrtho;
    vec4 orthoBounds;
    float zNear;
    float zFar;
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
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec3 in_Normal;
layout(location = 4) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

#define MAX_FRAGMENTS 512

void main() {
    vec4 fragColor = vec4(in_Color.rgb, in_Color.a * objectData.transparent);


    // in_VCPosition.length() will return wrong value
    float dist = length(in_VCPosition);
    if (in_VCPosition.z > 0.0f || dist < 0.01f) {
        return;
    }

    // check whether this pixel's linked list is already full; if so, discard the fragment
    // without allocating a node or touching the head pointer (avoids out-of-bounds writes)
    uint currentHead = imageLoad(headPointerImage, ivec2(gl_FragCoord.xy)).x;
    uint currentCount = 1;
    if (currentHead != 0xFFFFFFFFu) {
        uvec4 currentItem = imageLoad(listBuffer, int(currentHead));
        currentCount = currentItem.w + 1u;
    }
    if (currentCount > MAX_FRAGMENTS) {
        return;
    }

    uint newHead = atomicCounterIncrement(indexCounter);
    uint oldHead = imageAtomicExchange(headPointerImage, ivec2(gl_FragCoord.xy), newHead);

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
        // race fallback: the list filled up between check and insert; roll back head and drop this fragment
        imageAtomicExchange(headPointerImage, ivec2(gl_FragCoord.xy), oldHead);
    }
}
