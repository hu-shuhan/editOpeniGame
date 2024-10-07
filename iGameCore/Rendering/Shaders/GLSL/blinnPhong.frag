#version 330 core

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

//#include "BlinnPhong.h"

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

//layout(binding = 3) uniform sampler2D texSampler;
uniform sampler2D texSampler;

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

void main() {
    //out_ScreenColor = texture(texSampler, in_UV);
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
    out_ScreenColor = vec4(color, 1.0f);

    //out_ScreenColor = vec4(in_Color, 1.0);
}