#version 450 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec4 a_Color;

out vec4 v_Color;

layout (std140, binding = 0) uniform Camera
{
    mat4 g_viewProjectionMatrix;
};


void main()
{
   gl_Position = g_viewProjectionMatrix * vec4(a_Pos, 1.0);
   v_Color = a_Color;
}
