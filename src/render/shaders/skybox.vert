#version 460 core

layout (location = 0) in vec3 a_Pos;

out vec3 TexCoords;

uniform mat4 u_view;
uniform mat4 u_proj;

void main()
{
    TexCoords = a_Pos;
    vec4 position = u_proj * u_view * vec4(a_Pos, 1.0);
    gl_Position = position.xyww;
}