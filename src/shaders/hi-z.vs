#version 450 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TexCoords;

out vec2 TexCoord;

void main(void)
{
    gl_Position = vec4(a_Pos.xy, 0.5, 1.0);
    TexCoord = a_TexCoords;
}
