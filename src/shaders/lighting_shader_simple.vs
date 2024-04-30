#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec2 a_TexOffset;
layout (location = 4) in float a_Hp;


struct VecOutput
{
   vec3 v_FragPos;
	vec3 v_Normal;
	vec2 v_texCoords;
   vec2 v_texOffset;
};

layout (location = 0) out VecOutput v_Output;

flat out float v_hp;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};


void main()
{
   gl_Position = g_viewProjectionMatrix * vec4(a_Pos, 1.0);

   v_Output.v_FragPos = a_Pos;
   v_Output.v_texCoords = a_TexCoords;
   v_Output.v_texOffset = a_TexOffset;
   v_Output.v_Normal = a_Normal;
   v_hp = a_Hp;
}
