#version 460

#extension GL_ARB_shading_language_include : enable

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

in PerVertexData {
    vec3 mcPosition;
    vec3 vcPosition;
    vec3 color;
    vec3 normal;
    vec2 uv;
} fragIn;

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

void main()
{
    //    out_ScreenColor = fragIn.color;
    //out_ScreenColor = texture(texSampler, in_UV);
    vec3 color = vec3(0.0, 0.0, 0.0);

    vec3 normal = vec3(0.0, 0.0, 0.0);
    if (ubo.useNormalSmooth == 1) {
        // continuous patch
        normal = normalize(fragIn.normal);
    } else {
        // discrete patch
        float scale = 1.0 / length(fwidth(fragIn.vcPosition));
        vec3 fdx = dFdx(fragIn.vcPosition) * scale;
        vec3 fdy = dFdy(fragIn.vcPosition) * scale;
        normal = normalize(cross(fdx, fdy));
    }
    // correct normal orientation
    if (cameraData.isOrtho == 1 && normal.z < 0.0f) {
        normal = -1.0 * normal;
    }
    if (cameraData.isOrtho == 0 && dot(normal, fragIn.vcPosition) > 0.0f) {
        normal = -1.0 * normal;
    }

    // ambient
    color += ambient * fragIn.color;
    // lighting
    vec3 lighting = BlinnPhong(normal, fragIn.mcPosition, light);
    color += lighting * fragIn.color;

    out_ScreenColor = vec4(color, 1.0f);
}