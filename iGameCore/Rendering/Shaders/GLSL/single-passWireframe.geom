#version 330 core

#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 in_MCPosition[];
layout(location = 1) in vec3 in_VCPosition[];
layout(location = 2) in vec3 in_Color[];
layout(location = 3) in vec3 in_Normal[];
layout(location = 4) in vec2 in_UV[];

/*
 * The layout(location) syntax cannot be used in geometry shaders.
 * Make sure that variable names in both the geometry shader and fragment shader are consistent.
 */
out vec3 gs_OutMCPosition;
out vec3 gs_OutVCPosition;
out vec3 gs_OutColor;
out vec3 gs_OutNormal;
out vec2 gs_OutUV;

/*
  Each element in the edgeEqn array is a vec4, representing the line equation of one edge of the triangle along with additional data:
  - xyz components (a, b, c): coefficients of the line equation in screen viewport coordinates: ax + by + c = 0
  - w component: added to the distance calculated in the fragment shader. For edges that do not need to be rendered, the w component is set to lineWidth, ensuring the calculated opacity is always 1.0.
*/
flat out vec4 edgeEqn[3];
uniform float lineWidth;

uniform vec4 vpDims;
uniform samplerBuffer edgeMasks;

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define DBL_MAX 1.7976931348623158e+308
#define DBL_MIN 2.2250738585072014e-308
// 7f7f ffff = 0 11111110 11111111111111111111111 = 2139095039
float fMaxFloat = intBitsToFloat(2139095039);

void main() {
    vec2 pos[4];

    for (int i = 0; i < 3; i++) {
        pos[i] = gl_in[i].gl_Position.xy / gl_in[i].gl_Position.w;
        pos[i] = pos[i] * vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f);
        pos[i] = pos[i] * vpDims.zw + vpDims.xy;
    }
    pos[3] = pos[0];

    for (int i = 0; i < 3; i++) {
        vec2 dir = normalize(pos[i + 1] - pos[i]);
        edgeEqn[i].xy = vec2(-dir.y, dir.x);
        edgeEqn[i].z = -dot(edgeEqn[i].xy, pos[i]);
        edgeEqn[i].w = 0.0f;
    }

    // The shader normalizes the value and multiplies it by 255 to reconstruct the original value.
    float edgeValues = 255.0 * texelFetch(edgeMasks, gl_PrimitiveIDIn).r;
    if (mod(edgeValues, 2.0) < 1.0) edgeEqn[0].w = lineWidth;
    if (mod(edgeValues, 4.0) < 2.0) edgeEqn[1].w = lineWidth;
    if (edgeValues < 4.0) edgeEqn[2].w = lineWidth;

    for (int i = 0; i < 3; i++) {
        // When a geometry shader is active, gl_PrimitiveID must be assigned explicitly to ensure
        // the fragment shader receives the correct primitive ID.
        // Reference: https://registry.khronos.org/OpenGL-Refpages/gl4/html/gl_PrimitiveID.xhtml
        gl_PrimitiveID = gl_PrimitiveIDIn;

        gs_OutMCPosition = in_MCPosition[i];
        gs_OutVCPosition = in_VCPosition[i];
        gs_OutColor = in_Color[i];
        gs_OutNormal = in_Normal[i];
        gs_OutUV = in_UV[i];

        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }

    EndPrimitive();
}
