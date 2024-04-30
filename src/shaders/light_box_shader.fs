#version 450 core

layout (location = 0) out vec4 f_color;

uniform vec3 u_lightColor;

void main()
{
    f_color = vec4(1, 1, 1, 1.0);
}