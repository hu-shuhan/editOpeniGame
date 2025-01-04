#version 460

layout(location = 0) out vec4 out_ScreenColor;

//in PerVertexData {
//    vec4 color;
//} fragIn;

void main()
{
    //    out_ScreenColor = fragIn.color;
    out_ScreenColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}