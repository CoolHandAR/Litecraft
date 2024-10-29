#version 450 core

layout(location = 0) out vec4 FragColor;

uniform vec3 u_color;

void main()
{
    FragColor = vec4(u_color, 1.0);
}