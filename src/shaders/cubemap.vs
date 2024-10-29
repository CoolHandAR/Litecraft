#version 460 core
layout (location = 0) in vec3 a_Pos;

out vec3 WorldPos;

uniform mat4 u_proj;
uniform mat4 u_view;

void main()
{
    WorldPos = a_Pos;  
    gl_Position =  u_proj * u_view * vec4(WorldPos, 1.0);
}