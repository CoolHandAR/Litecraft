#version 460 core 

layout (location = 0) in vec3 a_Pos;



layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform mat4 u_model;

void main()
{
    vec4 WorldPos = u_model * vec4(a_Pos, 1.0);

    gl_Position = g_viewProjectionMatrix * WorldPos;
}