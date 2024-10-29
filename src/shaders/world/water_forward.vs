#version 460 core 

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_texCoords;

struct VecOutput
{
    //vec3 v_FragPos;
   vec3 v_worldPos;
   vec4 v_worldSpace;
   vec3 v_viewDir;
   vec3 v_lightDir;
};

layout (location = 0) out VecOutput v_Output;

out vec2 v_texCoords;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform vec3 u_viewPos;

void main()
{  
	mat4 model = mat4(1.0);

	model[0].xyz *= 64; //(CHUNK WIDTH + LENGTH) * 2
	model[2].xyz *= 64;
	model[3].xz += 64;

   vec4 world_pos = g_viewProjectionMatrix * model * vec4(a_Pos.x, 12, a_Pos.y, 1.0);

   gl_Position = world_pos;

	vec3 fragPos = (model * vec4(a_Pos.x, 12, a_Pos.y, 1.0)).xyz;
	v_Output.v_worldPos = fragPos;
   v_texCoords = vec2(a_Pos.x / 2.0 + 0.5, a_Pos.y / 2.0 + 0.5) * 8;
   v_Output.v_viewDir = normalize(u_viewPos - fragPos);
   v_Output.v_worldSpace = world_pos;
}