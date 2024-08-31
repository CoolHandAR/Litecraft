#version 460 core

#include "scene_incl.incl"

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in uvec4 a_Color;

out vec4 Color;

void main()
{
    gl_Position = cam.viewProjection * vec4(a_Pos, 1.0);

    Color = vec4(a_Color.r / 255, a_Color.g / 255, a_Color.b / 255, a_Color.a / 255);
}
