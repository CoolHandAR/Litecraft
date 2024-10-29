#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in int a_Hp;
layout (location = 2) in int a_Normal;
layout (location = 3) in uint a_TextureOffset;

struct VecOutput
{
   vec3 v_FragPos;
	vec3 v_Normal;
   vec4 v_worldSpace;
   vec3 v_viewDir;
   vec3 v_lightDir;
};

layout (location = 0) out VecOutput v_Output;

flat out float v_hp;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

vec3 intToVectorNormal(int n)
{
   vec3 normal = vec3(0, 0, 0);
   normal[clamp(abs(n) - 1, 0, 2)] = sign(n);
   return normal;
}

uniform vec3 u_viewPos;

void main()
{
   v_Output.v_worldSpace = g_viewProjectionMatrix * vec4(a_Pos.x, a_Pos.y, a_Pos.z, 1.0);

   gl_Position =  v_Output.v_worldSpace;
   
   v_Output.v_viewDir = u_viewPos - v_Output.v_worldSpace.xyz;
   v_Output.v_lightDir = v_Output.v_worldSpace.xyz - vec3(-5, -100, 0);
   v_Output.v_FragPos = a_Pos;
   v_Output.v_Normal = intToVectorNormal(a_Normal);
   v_hp = a_Hp;
}
