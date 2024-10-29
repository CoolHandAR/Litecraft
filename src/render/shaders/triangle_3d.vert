#version 460 core

#include "scene_incl.incl"

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in uvec4 a_Color;
layout (location = 2) in vec2 a_TexOffset;
layout (location = 3) in int a_TexIndex;

out VS_OUT
{
    vec4 Color;
    vec2 TexCoords;
    flat int TexIndex;
} vs_out;
out vec4 Color;

void main()
{
    gl_Position = cam.viewProjection * vec4(a_Pos, 1.0);

    vec4 fColor = vec4(a_Color);
    vs_out.Color = vec4(fColor.r / 255.0, fColor.g / 255.0, fColor.b / 255.0, fColor.a / 255.0);

    vs_out.TexCoords = vec2(a_TexOffset.x, -a_TexOffset.y);
    vs_out.TexIndex = 0;
}
