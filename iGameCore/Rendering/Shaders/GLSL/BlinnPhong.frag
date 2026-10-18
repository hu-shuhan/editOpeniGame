#version 330 core

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

//#include "BlinnPhong.h"

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

layout(std140, binding = 1) uniform ObjectDataBlock {
    float transparent;
    mat4 model;
    mat4 normal;// transpose(inverse(model))
    vec4 sphereBounds;
} objectData;

layout(std140, binding = 2) uniform UniformBufferObjectBlock {
    int useColor;
    int useNormalSmooth;
} ubo;

// Set per draw; independent of the shared smoothing flag.
uniform bool uHasNormal = false;

//layout(binding = 3) uniform sampler2D texSampler;
uniform sampler2D texSampler;
uniform vec3 inputColor = vec3(1.0f, 1.0f, 1.0f);
// 高光参数：桌面端由 C++ 侧按 Scene 上保存的值写入。默认高光强度为 0，
// 即默认观感与历史上的 Blinn-Phong（只出漫反射）保持一致。
uniform float uSpecularStrength = 0.0f;
uniform float uShininess = 32.0f;

uniform bool uEnvironmentEnabled = false;
uniform samplerCube uEnvironmentDiffuse;
uniform samplerCube uEnvironmentSpecular;
uniform float uEnvironmentIntensity = 1.0;
uniform float uEnvironmentRotation = 0.0;

vec3 EnvironmentDirection(vec3 directionView) {
    vec3 d = transpose(mat3(cameraData.view)) * directionView;
    float c = cos(uEnvironmentRotation), s = sin(uEnvironmentRotation);
    return vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
}

layout(location = 0) in vec3 in_MCPosition;
layout(location = 1) in vec3 in_VCPosition;
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec3 in_Normal;
layout(location = 4) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

// 环境光与漫反射强度保持历史（原项目）取值 0.4 / 0.5：Blinn-Phong 档位就是
// 原版观感，用于与 PBR 做 A/B 对比，因此不随 PBR 的默认亮度做对齐调整。
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
    vec3 viewDir = cameraData.isOrtho == 1
            ? vec3(0.0, 0.0, 1.0) : normalize(-fragPos);
    float spec = 0.0f;
    //    vec3 halfwayDir = normalize(lightDir + viewDir);
    //    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(dot(viewDir, reflectDir), 0.0f), uShininess);
    vec3 specular = spec * light.color * uSpecularStrength;

    return diffuse + specular;
}

// Both normal sources are resolved in view space. Evaluate derivatives
// before the per-fragment validity branch so fallback remains well-defined.
vec3 ResolveNormal() {
    float scale = 1.0 / max(length(fwidth(in_VCPosition)), 1e-8);
    vec3 faceNormal = cross(dFdx(in_VCPosition) * scale,
                            dFdy(in_VCPosition) * scale);
    float faceLength2 = dot(faceNormal, faceNormal);
    vec3 n = vec3(0.0, 0.0, 1.0);
    if (!any(isnan(faceNormal)) && !any(isinf(faceNormal)) &&
        faceLength2 > 1e-20 && !isinf(faceLength2)) {
        n = faceNormal * inversesqrt(faceLength2);
    }
    if (ubo.useNormalSmooth == 1 || uHasNormal) {
        vec3 pointNormal = mat3(cameraData.view) * in_Normal;
        float pointLength2 = dot(pointNormal, pointNormal);
        if (!any(isnan(pointNormal)) && !any(isinf(pointNormal)) &&
            pointLength2 > 1e-20 && !isinf(pointLength2)) {
            n = pointNormal * inversesqrt(pointLength2);
        }
    }
    if (cameraData.isOrtho == 1 && n.z < 0.0) { n = -n; }
    if (cameraData.isOrtho == 0 && dot(n, in_VCPosition) > 0.0) { n = -n; }
    return n;
}

void main() {
    //out_ScreenColor = texture(texSampler, in_UV);
    vec3 color = vec3(0.0f, 0.0f, 0.0f);

    vec3 normal = ResolveNormal();

    //use in_Color if exist, if not ,use default color
    vec3 baseColor = (ubo.useColor == 1) ? in_Color.rgb : inputColor;

    // Preserve the original cloud/disabled/failure path byte-for-byte below.
    if (uEnvironmentEnabled && ubo.useColor != 1) {
        vec3 albedo = pow(max(baseColor, vec3(0.0)), vec3(2.2));
        vec3 v = cameraData.isOrtho == 1 ? vec3(0, 0, 1) : normalize(-in_VCPosition);
        vec3 r = reflect(-v, normal);
        // Match a Phong lobe width approximately to the GGX roughness convention.
        float roughness = pow(2.0 / (max(uShininess, 1.0) + 2.0), 0.25);
        vec3 diffuseEnv = texture(uEnvironmentDiffuse, EnvironmentDirection(normal)).rgb;
        vec3 specularEnv = textureLod(uEnvironmentSpecular, EnvironmentDirection(r), roughness * 7.0).rgb;
        color = albedo * BlinnPhong(normal, in_VCPosition, light);
        color += uEnvironmentIntensity * (albedo * diffuseEnv + uSpecularStrength * specularEnv);
        color = max(color, vec3(0.0));
        color = pow(color / (vec3(1.0) + color), vec3(1.0 / 2.2));
        out_ScreenColor = vec4(color, 1.0);
        return;
    }

    // ambient
    color += ambient * baseColor;
    // lighting
    vec3 lighting = BlinnPhong(normal, in_VCPosition, light);
    color += lighting * baseColor;

    out_ScreenColor = vec4(color, 1.0f);
    //out_ScreenColor = vec4(in_Color, 1.0);
}
