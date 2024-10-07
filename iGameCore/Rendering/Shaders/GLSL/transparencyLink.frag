#version 420 core

layout (early_fragment_tests) in;

layout(std140, binding = 0) uniform CameraDataBlock {
    vec3 viewPos;
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
    bool useColor;
} ubo;

// 0:blinnPhong shading, 1:no light shading, 2:pure color shading
uniform int colorMode;
uniform vec3 inputColor = vec3(1.0, 1.0, 1.0);

// Atomic counter, used to allocate data to the linked list
layout(binding = 0, offset = 0) uniform atomic_uint indexCounter;

layout(binding = 0, r32ui) uniform uimage2D headPointerImage;
layout(binding = 1, rgba32ui) uniform writeonly uimageBuffer listBuffer;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;
layout(location = 2) in vec3 in_Normal;
layout(location = 3) in vec2 in_UV;

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

bool gamma = false;
vec3 BlinnPhong(vec3 normal, vec3 fragPos, Light light)
{
    // diffuse
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.color * 0.5f;
    // specular
    vec3 viewDir = normalize(cameraData.viewPos - fragPos);
    float spec = 0.0;
    //    vec3 halfwayDir = normalize(lightDir + viewDir);
    //    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * light.color * 0.5f;

    //return diffuse + specular;
    return diffuse;
}

vec3 ShadeFragment() {
    vec3 color = vec3(0.0, 0.0, 0.0);

    // continuous patch
    //    vec3 normal = normalize(in_Normal);
    // discrete patch
    vec3 normal = normalize(cross(dFdx(in_Position), dFdy(in_Position)));

    // ambient
    color += ambient * in_Color;
    // lighting
    vec3 lighting = BlinnPhong(normal, in_Position, light);
    color += lighting * in_Color;

    if (gamma) {
        color = pow(color, vec3(1.0 / 2.2));
    }

    return color;
}

void main() {
    vec4 fragColor;
    if (colorMode == 0) {
        fragColor = vec4(ShadeFragment(), objectData.transparent);
    } else if (colorMode == 1) {
        fragColor = vec4(in_Color, objectData.transparent);
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
