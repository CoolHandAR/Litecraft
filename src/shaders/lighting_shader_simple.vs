#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in int a_Hp;
layout (location = 2) in int a_Normal;
layout (location = 3) in uint a_TextureOffset;

struct VecOutput
{
   vec3 v_FragPos;
	vec3 v_Normal;
   vec2 v_texOffset;
};

layout (location = 0) out VecOutput v_Output;

flat out float v_hp;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform vec4 u_planeNormal;

vec3 intToVectorNormal(int n)
{
   vec3 normal = vec3(0, 0, 0);
   normal[clamp(abs(n) - 1, 0, 2)] = sign(n);
   return normal;
}

void main()
{
   vec4 world_pos = g_viewProjectionMatrix * vec4(a_Pos, 1.0);

   gl_Position = world_pos;

   gl_ClipDistance[0] = dot(vec4(a_Pos, 1.0), u_planeNormal);

   v_Output.v_FragPos = a_Pos;
   v_Output.v_texOffset = vec2(a_TextureOffset % 64, a_TextureOffset / 32);;
   v_Output.v_Normal = intToVectorNormal(a_Normal);
   v_hp = a_Hp;
}
