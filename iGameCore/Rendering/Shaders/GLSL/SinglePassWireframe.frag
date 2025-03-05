#version 330 core

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform CameraDataBlock {
    vec3 viewPos;
    int isOrtho;
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

in vec3 gs_OutMCPosition;
in vec3 gs_OutVCPosition;
in vec3 gs_OutColor;
in vec3 gs_OutNormal;
in vec2 gs_OutUV;

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

vec3 BlinnPhong(vec3 normal, vec3 fragPos, Light light);
vec3 ShadeFragment();

// single-pass wireframe rendering
flat in vec4 edgeEqn[3];
uniform float lineWidth;
// 0 : no light shading wireframe(use gs_OutColor)
// 1 : pure color shading wireframe(use uniform edgeColor)
uniform int edgeColorMode;
uniform vec3 edgeColor = vec3(0.0f, 0.0f, 0.0f);

void main()
{
    // shading patch
    vec3 fragColor = ShadeFragment();

    float edist[3];
    edist[0] = abs(dot(edgeEqn[0].xy, gl_FragCoord.xy) + edgeEqn[0].z);
    edist[1] = abs(dot(edgeEqn[1].xy, gl_FragCoord.xy) + edgeEqn[1].z);
    edist[2] = abs(dot(edgeEqn[2].xy, gl_FragCoord.xy) + edgeEqn[2].z);

    edist[0] += edgeEqn[0].w;
    edist[1] += edgeEqn[1].w;
    edist[2] += edgeEqn[2].w;

    // float emix = clamp(0.5 + 0.5 * lineWidth - min(min(edist[0], edist[1]), edist[2]), 0.0, 1.0);
    float minDist = min(min(edist[0], edist[1]), edist[2]);
    float halfWidth = lineWidth * 0.5;
    float edgeFactor = smoothstep(0.0f, halfWidth + 0.5, minDist);

    vec3 color = vec3(0.0f);
    if (edgeFactor == 1.0f) {
        color = fragColor;
    } else {
        if (edgeColorMode == 0) {
            color = mix(gs_OutColor, fragColor, edgeFactor);
        } else if (edgeColorMode == 1) {
            color = mix(edgeColor, fragColor, edgeFactor);
        }
    }
    out_ScreenColor = vec4(color, 1.0f);
}

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
    vec3 color = vec3(0.0f, 0.0f, 0.0f);

    vec3 normal = vec3(0.0f, 0.0f, 0.0f);
    if (ubo.useNormalSmooth == 1) {
        // continuous patch
        normal = normalize(gs_OutNormal);
    } else {
        // discrete patch
        float scale = 1.0f / length(fwidth(gs_OutVCPosition));
        vec3 fdx = dFdx(gs_OutVCPosition) * scale;
        vec3 fdy = dFdy(gs_OutVCPosition) * scale;
        normal = normalize(cross(fdx, fdy));
    }
    // correct normal orientation
    if (cameraData.isOrtho == 1 && normal.z < 0.0f) {
        normal = -1.0f * normal;
    }
    if (cameraData.isOrtho == 0 && dot(normal, gs_OutVCPosition) > 0.0f) {
        normal = -1.0f * normal;
    }

    // ambient
    color += ambient * gs_OutColor;
    // lighting
    vec3 lighting = BlinnPhong(normal, gs_OutMCPosition, light);
    color += lighting * gs_OutColor;

    return color;
}
