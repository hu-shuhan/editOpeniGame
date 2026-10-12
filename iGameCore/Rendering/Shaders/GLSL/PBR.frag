#version 330 core

#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

/*
 * PBR（metallic-roughness，glTF 风格）表面着色器。
 *
 * 与工程内数据流的约定：
 *   - Vertex.vert 输出的 in_MCPosition 已经乘过 objectData.model，因此它是
 *     【世界坐标】；in_Normal 同样已是世界法线。但本文件的法线默认走"离散面片"
 *     分支，由 dFdx/dFdy(in_VCPosition) 得到，那是【视空间】法线；而灯方向、世界
 *     向上轴都是世界空间常量。两边空间不一致，点乘在几何上就没有意义，因此本文件
 *     统一在【视空间】做光照：main 里先用 mat3(cameraData.view) 把灯方向与 +Y 轴
 *     旋进视空间，视线方向也改成视空间写法，保证参与点乘的向量同处一个坐标系。
 *   - HDR 环境按世界方向采样，不需要网格 UV 或切线。普通表面使用预计算漫反射
 *     和 GGX 预滤波环境，BRDF 使用 EnvBRDFApprox；云图、关闭或加载失败时
 *     保留「天空/地面梯度 + 太阳斑」的原有解析路径。
 *   - 世界 +Y 是默认视角的上方（ResetCameraView 把相机放在包围球中心 + (0, 0, 3r)），
 *     天空梯度沿世界 +Y、主光来自世界右上前方；两者都在 main 里被旋进视空间，所以
 *     旋转视角时"灯和天空"仍然固定在世界里，不会跟着相机转。
 *   - 反照率来源与 BlinnPhong.frag 保持一致：ubo.useColor == 1 时用顶点色
 *     （标量场云图/色带），否则用 inputColor（模型默认颜色）。
 *
 * 参考 HarmonyOS 3D 引擎 core3d_dm_fw（glTF 前向 PBR）的 pbrBasic 结构，沿用两点：
 *   - 环境遮蔽（AO）只作用于间接项，直接光不受影响；
 *   - 间接高光额外乘一个环境镜面遮蔽项，避免掠射角漏光。
 *
 * 本文件当前的 AO 是【半球近似】，不是屏幕空间 AO（SSAO）：
 *   - 它以世界 +Y 为参考，按「法线朝上/朝下」压暗间接光，不反映真实遮挡——
 *     同一个凹槽换个角度不会变暗，也不会因为周围有别的部件而变暗；
 *   - 强度由 uAoStrength 线性缩放，0 = 关闭（画面与未引入本项时逐像素一致）；
 *   - 色带（云图）模型在 main 里强制关闭，避免影响颜色判读基准。
 */

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

layout(location = 0) in vec3 in_MCPosition;
layout(location = 1) in vec3 in_VCPosition;
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec3 in_Normal;
layout(location = 4) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

// ---- 材质参数。全部带默认值，C++ 侧不设置也能正常渲染。
uniform vec3 inputColor = vec3(1.0, 1.0, 1.0);
uniform float uMetallic = 0.0;
uniform float uRoughness = 0.35;
// 环境遮蔽强度（简易半球近似，见 HemisphereAo 的注释）。语义与旧版不同：旧
// uAo 是「AO 系数」（1 = 不遮蔽），这里改成「强度」（1 = 完全启用）；旧的那个
// C++ 侧从未写入过，恒为 1，等价于关闭，所以换成强度语义不会影响历史画面。
// 默认值与 Scene::m_SurfaceAo 保持一致。
uniform float uAoStrength = 1.0;

// ---- 环境与灯组。下列常量都定义在【世界空间】且固定不动（旋转模型时光照不动，
// 因此高光会随模型移动）。main 里统一乘 mat3(cameraData.view) 旋进视空间后才参与
// 计算，这里的数值本身仍然按世界空间写。
// 强度默认值取「压暗版」与「原始版」的中间值：亮面与 Blinn-Phong 基本同亮度，
// 又不会像原始默认那样在直出（uTonemap == 0）时大面积顶到 1.0 被截断。
// 地面色比原先略亮（0.16 → 0.22）是为了把背光面从约 0.07 抬到约 0.15，
// 缩小与 Blinn-Phong 那层常量环境光（0.35）的差距。
uniform float uEnvIntensity = 0.8;
uniform vec3 uSkyColor = vec3(0.58, 0.63, 0.72);
uniform vec3 uGroundColor = vec3(0.22, 0.22, 0.25);
uniform float uSunIntensity = 2.5;
uniform vec3 uKeyDir = vec3(0.42, 0.62, 0.66);
uniform vec3 uKeyColor = vec3(1.00, 0.98, 0.94);
uniform float uKeyIntensity = 1.85;
uniform vec3 uFillDir = vec3(-0.65, 0.10, 0.52);
uniform vec3 uFillColor = vec3(0.30, 0.34, 0.42);
uniform float uFillIntensity = 0.43;
uniform vec3 uRimDir = vec3(-0.20, -0.72, -0.52);
uniform vec3 uRimColor = vec3(0.34, 0.38, 0.46);
uniform float uRimIntensity = 0.38;
uniform float uExposure = 0.92;
// Legacy/cloud output selection; ordinary surfaces always share the display transform.
uniform int uTonemap = 0;        // 0=直出，1=Reinhard + gamma
uniform int uLegacyColorMap = 0; // 1=色带模型退回旧的 Blinn-Phong 观感

uniform bool uEnvironmentEnabled = false;
uniform samplerCube uEnvironmentDiffuse;
uniform samplerCube uEnvironmentSpecular;
uniform float uEnvironmentIntensity = 1.0;
uniform float uEnvironmentRotation = 0.0;

// The inverse rigid view rotation maps normals/reflections into the shared world.
vec3 EnvironmentDirection(vec3 directionView) {
    vec3 d = transpose(mat3(cameraData.view)) * directionView;
    float c = cos(uEnvironmentRotation), s = sin(uEnvironmentRotation);
    return vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
}

const float PI = 3.14159265359;
// 粗糙度不再设下限：UI 的 0~1 与着色器一一对应（0 = 最光滑），与 ParaView 的语义一致。
// 代价：roughness 落在 (0, 0.045] 时 GGX 峰值 1/(π·roughness⁴) 极大，高光会退化成
// 亚像素级白点（静帧基本看不出来，交互浏览时可能出现闪烁）；而恰好为 0 时 a2 = 0、
// D_GGX 返回 0，即镜面高光完全消失（分子为 0，分母有 max(..., 1e-8) 保护，不会出 NaN）。
const float DIELECTRIC_F0 = 0.04;

// ---------------------------------------------------------------------------
// 环境：解析近似，代替 cubemap
// ---------------------------------------------------------------------------

// 漫反射环境（辐照度）。沿世界 +Y 做天空/地面梯度，平均值约等于旧的 0.4 环境光。
// upView 是"世界 +Y"在视空间中的方向，由调用方传入；n / r 也是视空间向量，两边
// 同空间点乘才是"法线与世界上方"的真实夹角，梯度才不会跟着相机翻。
vec3 SkyIrradiance(vec3 n, vec3 upView) {
    float t = clamp(dot(n, upView) * 0.5 + 0.5, 0.0, 1.0);
    return mix(uGroundColor, uSkyColor, t);
}

// 镜面环境（辐射度）。roughness 控制太阳斑的扩散范围，起到类似环境贴图 LOD 的作用。
// sunDirView 是主光方向在视空间中的方向，与 main 里传给直接光的是同一个向量。
vec3 EnvRadiance(vec3 r, float roughness, vec3 sunDirView, vec3 upView) {
    vec3 env = SkyIrradiance(r, upView);

    float sunCos = max(dot(normalize(r), sunDirView), 0.0);
    float width = mix(0.03, 0.55, roughness * roughness);
    env += uKeyColor * (uSunIntensity * smoothstep(1.0 - width, 1.0, sunCos));
    return env;
}

// 环境遮蔽（简易半球近似）：以世界 +Y（旋进视空间后的 upView，与 SkyIrradiance
// 同轴）为参考，越朝下的面间接光越少，用来在不引入屏幕空间 AO 的前提下补一点
// 立体感。它【不是】真实遮挡，实现代价是两次乘加。
//
//   t   = 0 表示正朝上，1 表示正朝下；
//   曲线取二次，让压暗集中在下半球，水平面只轻微变暗：
//     朝上 dot=+1 → t=0   → ao = 1.00    （不变）
//     水平 dot= 0 → t=0.5 → ao ≈ 0.875   （间接光暗 12.5%）
//     朝下 dot=-1 → t=1   → ao = 0.50    （间接光暗 50%）
//   若直接写成 saturate(dot(n, up))，所有竖直面都会被砍掉一半间接光，整个场景
//   会像降了曝光一样一起变暗，不再像局部遮蔽，所以这里用「最大暗化 50% + 二次」。
//   strength 为 0 时返回 1.0，与未引入该项时的分支完全一致。
float HemisphereAo(vec3 n, vec3 upView, float strength) {
    float t = clamp(0.5 - 0.5 * dot(n, upView), 0.0, 1.0);
    return mix(1.0, 1.0 - 0.5 * t * t, strength);
}

// ---------------------------------------------------------------------------
// BRDF
// ---------------------------------------------------------------------------

float D_GGX(float NoH, float a2) {
    float d = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / max(PI * d * d, 1e-8);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a2) {
    float ggxV = NoL * sqrt(max(NoV * NoV * (1.0 - a2) + a2, 1e-8));
    float ggxL = NoV * sqrt(max(NoL * NoL * (1.0 - a2) + a2, 1e-8));
    return 0.5 / max(ggxV + ggxL, 1e-6);
}

vec3 F_Schlick(vec3 f0, float cosTheta) {
    float f = pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    return f0 + (vec3(1.0) - f0) * f;
}

// Karis 的解析 split-sum 近似，返回环境高光的 A/B 系数（不需要预计算 LUT）。
vec3 EnvBRDFApprox(vec3 f0, float roughness, float NoV) {
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.040, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
    return f0 * ab.x + ab.y;
}

// 环境镜面遮蔽（Frostbite 形式）。按 NoV 与 AO 衰减间接高光，环境贴图缺失时也能
// 得到合理的掠射角表现；这里没有采用依赖高度场地平线的写法，避免正常入射时把
// 高光整体抹掉。
float EnvSpecularAo(float ao, float NoV, float roughness) {
    float t = pow(clamp(NoV + ao, 0.0, 1.0), exp2(-16.0 * roughness - 1.0));
    return clamp(t - 1.0 + ao, 0.0, 1.0);
}

// 单个方向光的直接光照。diffuseColor 已经折入 (1 - metallic)。
vec3 DirectLight(vec3 n, vec3 v, vec3 l, vec3 radiance, vec3 diffuseColor,
                 vec3 f0, float a2) {
    float NoL = dot(n, l);
    if (NoL <= 0.0) { return vec3(0.0); }

    vec3 h = normalize(v + l);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float NoV = clamp(dot(n, v), 1e-4, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    vec3 f = F_Schlick(f0, LoH);
    vec3 specular = D_GGX(NoH, a2) * V_SmithGGXCorrelated(NoV, NoL, a2) * f;
    vec3 diffuse = diffuseColor * (vec3(1.0) - f) / PI;

    return (diffuse + specular) * radiance * NoL;
}

// ---------------------------------------------------------------------------
// 旧光照路径：色带模型需要与历史渲染完全一致时可切换过来
// ---------------------------------------------------------------------------

// 注意：这条路径【故意】不复用上面的视空间约定——它拿世界空间的光方向去点乘视空间
// 法线，正是老版本"明暗跟着相机转"的观感来源。它的定位就是复刻历史画面，改了就不再
// 是"与历史渲染完全一致"，所以本次统一视空间的改动不涉及这里。
vec3 LegacyBlinnPhong(vec3 n, vec3 albedo) {
    const vec3 ambient = vec3(0.4);
    const vec3 lightDir = vec3(0.0, 0.0, 1.0); // 旧 shader 里 light.direction = (0,0,-1)
    float diff = max(dot(lightDir, n), 0.0);
    return albedo * (ambient + diff * 0.5);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

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
    vec3 n = ResolveNormal();

    bool colorMapped = (ubo.useColor == 1);
    vec3 albedo = colorMapped ? in_Color.rgb : inputColor;
    bool useHdr = uEnvironmentEnabled && !colorMapped;

    if (colorMapped && uLegacyColorMap == 1) {
        out_ScreenColor = vec4(LegacyBlinnPhong(n, albedo), 1.0);
        return;
    }

    float metallic = clamp(uMetallic, 0.0, 1.0);
    float roughness = clamp(uRoughness, 0.0, 1.0);
    float envIntensity = uEnvIntensity;
    if (!colorMapped) {
        // HDR is a light source, not a switch for the material's color space.
        albedo = pow(max(albedo, vec3(0.0)), vec3(2.2));
    }

    if (colorMapped) {
        // 云图颜色代表数据本身：强制哑光，并把强度配到与旧 Blinn-Phong
        // （环境 0.4 + 漫反射 0.5）大致等亮，避免颜色判读基准漂移。
        metallic = 0.0;
        roughness = max(roughness, 0.62);
        envIntensity = uEnvIntensity * 0.95;
    }

    vec3 f0 = mix(vec3(DIELECTRIC_F0), albedo, metallic);
    vec3 diffuseColor = albedo * (1.0 - metallic);

    // ---- 统一到视空间 ----
    // mat3(cameraData.view) 取 view 矩阵的左上 3x3。lookAtRH 生成的是刚体变换
    // （纯旋转 + 平移，无缩放），所以这一块就是正交的旋转矩阵，方向向量直接乘即可。
    mat3 viewRot = mat3(cameraData.view);

    // 视线方向 V（表面指向相机）。视空间里相机恒在原点、朝 -Z：
    //   - 透视：逐像素由顶点视空间位置反推，normalize(-in_VCPosition)；
    //   - 正交：所有视线平行，方向恒为 +Z。这里不能再用 viewPos - in_MCPosition
    //     （世界空间），那样每个像素会得到略有差异的方向，与平行投影的语义不符。
    vec3 v = (cameraData.isOrtho == 1) ? vec3(0.0, 0.0, 1.0)
                                       : normalize(-in_VCPosition);
    float NoV = clamp(dot(n, v), 1e-4, 1.0);

    // 灯方向与"世界向上"从世界空间旋进视空间。常量数值不变（仍是世界空间定义），
    // 所以旋转视角时灯在世界里不动，明暗分布固定在世界上，不会跟着相机转。
    vec3 keyDir = normalize(viewRot * uKeyDir);
    vec3 fillDir = normalize(viewRot * uFillDir);
    vec3 rimDir = normalize(viewRot * uRimDir);
    vec3 upView = viewRot * vec3(0.0, 1.0, 0.0);

    float a2 = roughness * roughness;
    a2 = a2 * a2;

    vec3 color = vec3(0.0);
    color += DirectLight(n, v, keyDir, uKeyColor * uKeyIntensity,
                         diffuseColor, f0, a2);
    color += DirectLight(n, v, fillDir,
                         uFillColor * uFillIntensity, diffuseColor,
                         f0, a2);
    color += DirectLight(n, v, rimDir,
                         uRimColor * uRimIntensity, diffuseColor,
                         f0, a2);

    // 间接光：AO 只作用于这里。
    // 色带（云图）模型强制关闭：颜色代表数据本身，任何明暗调制都会影响判读基准
    // （与上面「强制哑光」是同一个理由）。
    float aoStrength = colorMapped ? 0.0 : clamp(uAoStrength, 0.0, 1.0);
    float ao = HemisphereAo(n, upView, aoStrength);
    vec3 r = reflect(-v, n);
    vec3 irradiance = SkyIrradiance(n, upView) * diffuseColor;
    vec3 radiance = EnvRadiance(r, roughness, keyDir, upView) *
                    EnvBRDFApprox(f0, roughness, NoV) *
                    EnvSpecularAo(ao, NoV, roughness);
    if (useHdr) {
        // The analytic fit can undershoot zero for very dark, rough metals.
        // Radiance must remain nonnegative before display gamma is applied.
        vec3 envBrdf = max(EnvBRDFApprox(f0, roughness, NoV), vec3(0.0));
        vec3 diffuseWeight = vec3(1.0) - F_Schlick(f0, NoV);
        vec3 environmentDiffuse = texture(uEnvironmentDiffuse, EnvironmentDirection(n)).rgb;
        // Neutral diffuse illumination protects the chosen engineering color.
        // Preserve colored environment reflections and the existing PBR BRDF.
        float diffuseLuminance = dot(environmentDiffuse, vec3(0.2126, 0.7152, 0.0722));
        irradiance = vec3(diffuseLuminance) * diffuseColor * diffuseWeight;
        radiance = textureLod(uEnvironmentSpecular, EnvironmentDirection(r),
                              roughness * 7.0).rgb * envBrdf
                   * EnvSpecularAo(ao, NoV, roughness);
        color += (irradiance * ao + radiance) * uEnvironmentIntensity;
    } else {
        if (!colorMapped) {
            float diffuseLuminance = dot(SkyIrradiance(n, upView), vec3(0.2126, 0.7152, 0.0722));
            irradiance = vec3(diffuseLuminance) * diffuseColor * (vec3(1.0) - F_Schlick(f0, NoV));
            radiance = max(radiance, vec3(0.0));
        }
        color += (irradiance * ao + radiance) * envIntensity;
    }

    color *= uExposure;
    if (!colorMapped) {
        // Same display transform on either environment path. Luminance mapping
        // scales RGB together, preserving hue as illumination strength changes.
        color = max(color, vec3(0.0));
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color /= 1.0 + luminance;
        color /= max(1.0, max(color.r, max(color.g, color.b)));
        color = pow(color, vec3(1.0 / 2.2));
    } else if (uTonemap == 1) {
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));
    }

    out_ScreenColor = vec4(color, 1.0);
}
