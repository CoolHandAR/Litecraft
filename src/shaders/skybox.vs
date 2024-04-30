#version 460 core

layout (location = 0) in vec3 a_Pos;

out vec3 v_texCoords;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    v_texCoords = a_Pos;

    vec4 position = u_projection * u_view * vec4(a_Pos, 1.0);
    gl_Position = position.xyww;
}