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

// 0:blinnPhong shading, 1:no light shading, 2:pure color shading
uniform int colorMode;
uniform vec3 inputColor = vec3(1.0f, 1.0f, 1.0f);
uniform int uUseLighting = 1;

// Atomic counter, used to allocate data to the linked list
layout(binding = 0, offset = 0) uniform atomic_uint indexCounter;

layout(binding = 0, r32ui) uniform uimage2D headPointerImage;
layout(binding = 1, rgba32ui) uniform writeonly uimageBuffer listBuffer;

layout(location = 0) in vec3 in_MCPosition;
layout(location = 1) in vec3 in_VCPosition;
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec3 in_Normal;
layout(location = 4) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

vec3 ambient = vec3(0.4f, 0.4f, 0.4f);
struct Light {
    vec3 direction;
    vec3 color;
};
Light light = Light(
vec3(0.0f, 0.0f, -1.0f),
vec3(1.0f, 1.0f, 1.0f)
);

vec3 BlinnPhong(vec3 normal, vec3 fragPos, Light light)
{
    // diffuse
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(lightDir, normal), 0.0f);
    vec3 diffuse = diff * light.color * 0.5f;
    // specular
    vec3 viewDir = normalize(cameraData.viewPos - fragPos);
    float spec = 0.0f;
    //    vec3 halfwayDir = normalize(lightDir + viewDir);
    //    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
    vec3 specular = spec * light.color * 0.5f;

    //return diffuse + specular;
    return diffuse;
}

vec3 ShadeFragment() {
    vec3 color = vec3(0.0f, 0.0f, 0.0f);

    vec3 normal = vec3(0.0f, 0.0f, 0.0f);
    if (ubo.useNormalSmooth == 1) {
        // continuous patch
        normal = normalize(in_Normal);
    } else {
        // discrete patch
        float scale = 1.0f / length(fwidth(in_VCPosition));
        vec3 fdx = dFdx(in_VCPosition) * scale;
        vec3 fdy = dFdy(in_VCPosition) * scale;
        normal = normalize(cross(fdx, fdy));
    }
    // correct normal orientation
    if (cameraData.isOrtho == 1 && normal.z < 0.0f) {
        normal = -1.0f * normal;
    }
    if (cameraData.isOrtho == 0 && dot(normal, in_VCPosition) > 0.0f) {
        normal = -1.0f * normal;
    }

    // ambient
        color += ambient * in_Color.rgb;
    // lighting
    vec3 lighting = BlinnPhong(normal, in_MCPosition, light);
        color += lighting * in_Color.rgb;

    return color;
}

void main() {
    vec4 fragColor;
    if (colorMode == 0) {
        vec3 color = uUseLighting == 1 ? ShadeFragment() : in_Color.rgb;
        fragColor = vec4(color, objectData.transparent);
    } else if (colorMode == 1) {
        fragColor = vec4(in_Color.rgb, in_Color.a * objectData.transparent);
    } else if (colorMode == 2) {
        fragColor = vec4(inputColor, objectData.transparent);
    }

    uint newHead = atomicCounterIncrement(indexCounter);
    uint oldHead = imageAtomicExchange(headPointerImage, ivec2(gl_FragCoord.xy), newHead);

    uvec4 item;
    item.x = oldHead;
    item.y = packUnorm4x8(fragColor);
    item.z = floatBitsToUint(gl_FragCoord.z);
    item.w = 0;

    imageStore(listBuffer, int(newHead), item);
}
