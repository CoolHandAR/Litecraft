#version 460 core

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_TexCoords;
layout (location = 2) in vec4 a_Color;

out VS_Out
{
    vec2 tex_coords;
    vec4 color;
} vs_out;


uniform mat4 u_orthoProjection;
uniform vec2 u_windowScale;

void main()
{   
    gl_Position = u_orthoProjection * vec4(a_Pos.x * u_windowScale.x, a_Pos.y * u_windowScale.y, 0.0, 1.0);

    vs_out.tex_coords = a_TexCoords;
    vs_out.color = a_Color;
}