#version 460 core 

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_texCoords;

out vec2 v_texCoords;
out vec2 v_viewRay;

void main()
{
   v_texCoords = a_texCoords;
   v_viewRay.x = a_Pos.x * (1024 / 720) * tan(radians(85.0 / 2.0));
   v_viewRay.y = a_Pos.y * tan(radians(85.0 / 2.0));
   gl_Position = vec4(a_Pos, 0.0, 1.0);
}
