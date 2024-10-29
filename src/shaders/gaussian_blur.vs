#version 460 core 

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_texCoords;

out vec2 v_texCoords;

void main()
{
 vec2 rcpFrame = vec2(1.0/ 1280.0, 1.0/ 720.0);
   v_texCoords =  a_texCoords * rcpFrame;
   gl_Position = vec4(a_Pos, 0.0, 1.0);
}
