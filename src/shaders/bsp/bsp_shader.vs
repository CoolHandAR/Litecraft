#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TexCoords;
layout (location = 2) in vec2 a_LightCoords;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

out vec2 TexCoords;
out vec2 LightCoords;

void main()
{
    gl_Position = g_viewProjectionMatrix * vec4(a_Pos.x, a_Pos.z, a_Pos.y, 1.0);
    TexCoords = vec2(a_TexCoords.x, -a_TexCoords.y);
    LightCoords = vec2(a_LightCoords.x, a_LightCoords.y);
}