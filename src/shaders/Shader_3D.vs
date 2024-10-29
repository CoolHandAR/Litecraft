#version 460 core 

layout (location = 0) in vec3 a_Pos;


layout (std140, binding = 0) uniform Camera
{
    mat4 g_viewProjectionMatrix;
};


void main()
{
   gl_Position = g_viewProjectionMatrix * vec4(a_Pos, 1.0);
   
}
