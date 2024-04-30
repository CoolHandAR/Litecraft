#version 450 core 

layout (location = 0) in vec3 a_Pos;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform mat4 u_model;

void main()
{
   gl_Position = g_viewProjectionMatrix * u_model * vec4(a_Pos, 1.0);
}
