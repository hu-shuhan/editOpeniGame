#version 330 core

#extension GL_ARB_separate_shader_objects : enable

// 底板（地面网格）——参考 Blender 的解析式无限网格。
//
// 这里不使用任何几何：顶点着色器只画一个覆盖全屏的三角形，片元着色器对每个
// 像素反向投射出"模型空间的射线"，与水平面 y = uPlaneY 求交，拿到交点后再由
// fwidth 算出该像素跨过多少格距，再按像素覆盖率绘制等宽的抗锯齿网格线。
// 因为没有几何边界，网格在视觉上就是无限延伸的；远处的线在间距过小时提前
// 淡出，再因距离超过 uFadeRadius 而淡出，抑制摩尔纹与地平线附近的亮带。
//
// 深度：把交点反投影回裁剪空间后写入 gl_FragDepth。本引擎调用过
// glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)（见 Scene::InitOpenGL），
// 裁剪空间深度是 [0,1] 而不是 OpenGL 默认的 [-1,1]；配合反向 Z（近平面 1、
// 远平面 0），窗口深度就等于 clip.z / clip.w。这里不能再做 (z + 1) / 2 的
// 映射，否则整体前移半个深度范围，网格会盖到模型上面。

layout(location = 0) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

uniform mat4 uInvModelProjView; // inverse(proj * view * model)
uniform mat4 uModelProjView;    // proj * view * model
uniform vec3 uCameraPosModel;   // 相机位置（模型空间）
uniform float uPlaneY;          // 地平面高度（模型空间）
uniform float uBaseCell;        // 参考格距（模型空间）
uniform float uFadeRadius;      // 距离渐隐半径（模型空间）

uniform vec3 uLineColor;        // 细网格线
uniform vec3 uMajorColor;       // 每 10 格的线

// 一层十进位网格的"线覆盖率"。
// coordWidth 在任何 discard 之前计算，避免片元组内分支退出影响导数。
// 按像素区间与线条区间的重叠长度计算覆盖率，减少线条移动时的亮度跳变。
float GridLine(vec2 coord, vec2 coordWidth, float cell, float halfWidthPixels) {
    vec2 uv = coord / cell;
    vec2 w = max(coordWidth / cell, vec2(1e-8));
    vec2 pixelsToLine = abs(fract(uv - 0.5) - 0.5) / w;
    vec2 coverage = clamp(min(pixelsToLine + 0.5, vec2(halfWidthPixels)) -
                          max(pixelsToLine - 0.5, vec2(-halfWidthPixels)),
                          0.0, 1.0);
    // 格距从 16 像素缩到 6 像素时逐渐淡出；两个方向分别处理，
    // 倾斜视角下仅隐藏过密的那组线，保留仍然清晰的另一组线。
    coverage *= vec2(1.0) - smoothstep(vec2(1.0 / 16.0), vec2(1.0 / 6.0), w);
    return 1.0 - (1.0 - coverage.x) * (1.0 - coverage.y);
}

void main() {
    vec2 ndc = in_UV * 2.0 - 1.0;

    // 反向 Z：ndc.z = 1 是近平面、0 是远平面，取这两点即可还原模型空间射线
    vec4 nearClip = uInvModelProjView * vec4(ndc, 1.0, 1.0);
    vec4 farClip = uInvModelProjView * vec4(ndc, 0.0, 1.0);
    vec3 rayBegin = nearClip.xyz / nearClip.w;
    vec3 rayEnd = farClip.xyz / farClip.w;

    vec3 ray = rayEnd - rayBegin;
    // 先用安全分母计算交点与导数，再丢弃无效射线，保证导数处于一致控制流中。
    float rayY = (ray.y < 0.0 ? -1.0 : 1.0) * max(abs(ray.y), 1e-12);
    float t = (uPlaneY - rayBegin.y) / rayY;

    vec3 hit = rayBegin + ray * t;
    vec2 coord = hit.xz;
    vec2 coordWidth = fwidth(coord);
    if (abs(ray.y) < 1e-12 || t < 0.0 || t > 1.0) { discard; }

    // 距离渐隐：uFadeRadius 正好是"沿视线方向到平面中心的距离 + 可见半径"，
    // 也就是远裁剪面在该方向的深度，所以网格在远平面切到它之前就已经淡到 0，
    // 不会出现硬边。
    float distanceToCamera = length(hit - uCameraPosModel);
    float distanceFade =
            1.0 - smoothstep(0.6 * uFadeRadius, uFadeRadius, distanceToCamera);
    if (distanceFade <= 0.0) { discard; }

    // 各十进位层：粗层是细层的子集，合并时取最大覆盖率，避免重复叠加。
    // 最细层只使用基准格距，不再额外细分成 1/10 格。
    float line = GridLine(coord, coordWidth, uBaseCell, 0.75);

    float major = max(GridLine(coord, coordWidth, uBaseCell * 10.0, 1.0),
                      GridLine(coord, coordWidth, uBaseCell * 100.0, 1.1));
    major = max(major, GridLine(coord, coordWidth, uBaseCell * 1000.0, 1.2));
    // 强调线覆盖的部分使用强调色，其余使用细线色；避免 major > line
    // 的硬切换在两层覆盖率接近时造成颜色跳变。alpha 仍取最大覆盖率。
    float coverage = max(line, major);
    vec3 color = mix(uLineColor, uMajorColor, major / max(coverage, 1e-8));

    // 不再画"过底板中心的强化线"：无限网格里它的位置是包围盒中心按格距取整，
    // 一般不与"每 10 格"的中线对齐，看上去就像凭空多出来一条粗线，
    // 而且这个位置在世界坐标里没有参考意义（要参考原点请用中心坐标轴）。

    float alpha = coverage * distanceFade;
    if (alpha <= 0.0) { discard; }

    vec4 clip = uModelProjView * vec4(hit, 1.0);
    gl_FragDepth = clip.z / clip.w;

    out_ScreenColor = vec4(color, alpha);
}
