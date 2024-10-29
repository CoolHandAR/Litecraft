#version 460 core 

#define Z_VALUE_DIVISOR 100

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_texCoords;
layout (location = 2) in float a_texIndex;

out vec2 v_texCoords;
out float v_texIndex;

uniform mat4 u_projection;
uniform vec2 u_windowScale;

void main()
{
   v_texCoords = a_texCoords;
   v_texIndex = a_texIndex;
   gl_Position = u_projection * vec4(a_Pos.x * u_windowScale.x, a_Pos.y * u_windowScale.y, a_Pos.z / Z_VALUE_DIVISOR, 1.0);
}
