#version 460 core 

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_texCoords;

out vec2 v_texCoords;

uniform mat4 u_proj;

void main()
{
   v_texCoords = a_texCoords;
   gl_Position = vec4(a_Pos, 1.0, 1.0);
}
