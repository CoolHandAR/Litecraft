#version 450 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec2 a_TexOffset;

struct VecOutput
{
    vec3 v_FragPos;
	vec3 v_Normal;
	vec2 v_texCoords;
    vec2 v_texOffset;
};

layout (location = 0) out VecOutput v_Output;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform mat4 u_model;

void main()
{
    vec4 world_pos = u_model * vec4(a_Pos, 1.0);
   gl_Position = g_viewProjectionMatrix * world_pos;

   v_Output.v_FragPos = world_pos.xyz;
   v_Output.v_texCoords = a_TexCoords;
   v_Output.v_texOffset = a_TexOffset;

    mat3 normal_matrix = transpose(inverse(mat3(u_model)));
    v_Output.v_Normal = normal_matrix * a_Normal;
}
