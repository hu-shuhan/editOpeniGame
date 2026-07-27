#extension GL_OES_standard_derivatives : enable
precision highp float;
precision highp int;

uniform mat4 uView;
uniform vec3 uViewPos;
uniform int uIsOrtho;
uniform int uUseNormalSmooth;
uniform int uUseColor;
uniform vec3 inputColor;

varying vec3 in_MCPosition;
varying vec3 in_VCPosition;
varying vec3 in_ColorV;
varying vec3 in_NormalV;

vec3 ambientColor = vec3(0.32, 0.32, 0.34);
vec3 lightDirection = normalize(vec3(0.25, 0.35, -1.0));
vec3 lightColor = vec3(1.0, 1.0, 1.0);

vec3 SafeNormalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);
    if (len2 < 1e-10) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

vec3 GetFaceNormal() {
    vec3 dx = dFdx(in_VCPosition);
    vec3 dy = dFdy(in_VCPosition);
    return SafeNormalize(cross(dx, dy), vec3(0.0, 0.0, 1.0));
}

vec3 GetSmoothNormal() {
    vec3 viewNormal = mat3(uView) * in_NormalV;
    return SafeNormalize(viewNormal, GetFaceNormal());
}

vec3 OrientNormal(vec3 normal) {
    if (uIsOrtho == 1) {
        return normal.z < 0.0 ? -normal : normal;
    }
    return dot(normal, in_VCPosition) > 0.0 ? -normal : normal;
}

vec3 BlinnPhong(vec3 normal) {
    vec3 lightDir = SafeNormalize(-(mat3(uView) * lightDirection), vec3(0.0, 0.0, 1.0));
    vec3 viewDir = SafeNormalize(-in_VCPosition, vec3(0.0, 0.0, 1.0));
    vec3 halfDir = SafeNormalize(lightDir + viewDir, viewDir);

    float diffuseTerm = max(dot(normal, lightDir), 0.0);
    float specularTerm = 0.0;
    if (diffuseTerm > 0.0) {
        specularTerm = pow(max(dot(normal, halfDir), 0.0), 32.0);
    }

    vec3 diffuse = diffuseTerm * lightColor * 0.62;
    vec3 specular = specularTerm * lightColor * 0.26;
    return ambientColor + diffuse + specular;
}

void main() {
    vec3 normal = uUseNormalSmooth == 1 ? GetSmoothNormal() : GetFaceNormal();
    normal = OrientNormal(normal);
    vec3 baseColor = (uUseColor == 1) ? in_ColorV : inputColor;
    vec3 color = baseColor * BlinnPhong(normal);
    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
