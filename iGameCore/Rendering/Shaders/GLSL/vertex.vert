#version 330 core

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

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

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;
layout(location = 2) in vec3 in_Normal;
layout(location = 3) in vec2 in_UV;

layout(location = 0) out vec3 out_Position;
layout(location = 1) out vec3 out_Color;
layout(location = 2) out vec3 out_Normal;
layout(location = 3) out vec2 out_UV;

void main() {
    gl_Position = cameraData.proj_view * objectData.model * vec4(in_Position, 1.0);

    out_Position = vec3(objectData.model * vec4(in_Position, 1.0));
    if (ubo.useColor) {
        out_Color = in_Color;
    } else {
        out_Color = vec3(1.0f, 1.0f, 1.0f);
    }
    out_Normal = mat3(objectData.normal) * in_Normal;
    out_UV = in_UV;
}