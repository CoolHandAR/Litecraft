#version 460 core

#include "scene_incl.incl"

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in uvec4 a_Color;

out vec4 Color;

void main()
{
    gl_Position = cam.viewProjection * vec4(a_Pos, 1.0);

    vec4 fColor = vec4(a_Color);

    Color = vec4(fColor.r / 255.0, fColor.g / 255.0, fColor.b / 255.0, fColor.a / 255.0);
}
