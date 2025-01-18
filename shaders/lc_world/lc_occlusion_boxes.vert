#version 460 core

#include "../scene_incl.incl"

#include "lc_world_incl.incl"

layout (std430, binding = 16) readonly restrict buffer VisiblesBuffer
{
    int data[];
} visibles;

layout (location = 0) in vec4 a_Pos;

out uint ChunkIndex;

const float HALF_WIDTH = CHUNK_WIDTH / 2.0;
const float HALF_HEIGHT = CHUNK_HEIGHT / 2.0;
const float HALF_LENGTH = CHUNK_LENGTH / 2.0;

void main()
{
	uint c_index = uint(visibles.data[gl_InstanceID]);

	vec3 worldPos = chunk_data.data[c_index].min_point.xyz - 0.5;

	mat4 xform = mat4(1.0);
	xform[0].xyz *= HALF_WIDTH;
	xform[1].xyz *= HALF_HEIGHT;
	xform[2].xyz *= HALF_LENGTH;
	xform[3].xyz = worldPos + vec3(HALF_WIDTH, HALF_LENGTH, HALF_HEIGHT);

	vec3 local_pos = cam.position.xyz - xform[3].xyz;
	
	vec3 box_extents = vec3(HALF_WIDTH, HALF_LENGTH, HALF_HEIGHT);

	chunk_data.data[c_index].vis_flags = 0;

	//Fix so that it wont fail if we are inside or very close to the box
	if (all(lessThan(abs(local_pos), box_extents)))
    {
        chunk_data.data[c_index].vis_flags |= CHUNK_FLAG_VISIBLE;
    }

	ChunkIndex = c_index;

	gl_Position = cam.viewProjection * xform * vec4(a_Pos.xyz, 1.0);
}