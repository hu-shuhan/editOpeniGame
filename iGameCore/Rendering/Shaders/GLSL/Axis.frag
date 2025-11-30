#version 330 core

#extension GL_ARB_separate_shader_objects : enable

uniform vec3 viewPos;
uniform bool isDrawFont;
uniform sampler2D fontSampler;
uniform vec3 textColor;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;
layout(location = 2) in vec3 in_Normal;
layout(location = 3) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

vec3 ambient = vec3(0.1f, 0.1f, 0.1f);
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
    vec3 diffuse = diff * light.color;
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    float spec = 0.0f;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0f), 32.0f);
    vec3 specular = spec * light.color;
    return diffuse + specular;
}

void main() {
    if (isDrawFont) {
        float alpha = texture(fontSampler, in_UV).r;
        if (alpha < 0.1f) discard;
        out_ScreenColor = vec4(textColor, alpha);
    } else {
        vec3 color = vec3(0.0f, 0.0f, 0.0f);

        // discrete patch
        vec3 normal = normalize(cross(dFdx(in_Position), dFdy(in_Position)));

        // ambient
        color += ambient * in_Color;

        // lighting
        vec3 lighting = BlinnPhong(normal, in_Position, light);
        color += lighting * in_Color;

        out_ScreenColor = vec4(color, 1.0f);
    }
}
