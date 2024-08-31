#version 460

#include "../scene_incl.incl"
#include "lc_world_incl.incl"

layout (location = 0) in ivec3 a_Pos;


void main()
{
	vec3 worldPos = chunk_data.data[gl_DrawID].min_point.xyz + a_Pos.xyz;

	gl_Position = cam.viewProjection * vec4(worldPos.xyz, 1.0);
}