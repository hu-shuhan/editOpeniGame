#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_UV;
layout(location = 0) out vec4 out_Color;
uniform samplerCube uEnvironment;
uniform mat4 uView;
uniform mat4 uInverseProjection;
uniform float uRotation;

void main() {
    vec2 ndc = in_UV * 2.0 - 1.0;
    // Reconstruct the same reversed-Z camera rays as the ground grid. A midpoint
    // avoids an infinite far point; orthographic pixels naturally yield parallel rays.
    vec4 nearPoint = uInverseProjection * vec4(ndc, 1.0, 1.0);
    vec4 middlePoint = uInverseProjection * vec4(ndc, 0.5, 1.0);
    vec3 ray = middlePoint.xyz / middlePoint.w - nearPoint.xyz / nearPoint.w;
    vec3 d = transpose(mat3(uView)) * ray;
    float c = cos(uRotation), s = sin(uRotation);
    d = vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
    vec3 color = max(textureLod(uEnvironment, d, 0.0).rgb, vec3(0.0));
    // Compensate the lighting-normalized cubemap for background display only.
    const float backgroundExposureEV = 4.5;
    color *= exp2(backgroundExposureEV);
    // The cubemap is linear HDR; write display color just as the surface shaders do.
    color = color / (color + vec3(1.0));
    out_Color = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
