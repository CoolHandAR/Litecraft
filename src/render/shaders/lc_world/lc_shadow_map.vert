#version 460

#include "../scene_incl.incl"
#include "lc_world_incl.incl"

layout (location = 0) in ivec3 a_Pos;
layout (location = 1) in int a_PackedNormHp;
layout (location = 2) in uint a_BlockType;

const vec3 CUBE_NORMALS_TABLE[6] =
{
	//back face
	vec3(0.0, 0.0, -1.0),

   // Front face
	vec3(0.0, 0.0,  1.0),

	// Left face
	vec3(-1.0,  0.0,  0.0),

	// Right face
	vec3(1.0,  0.0,  0.0),

	 // Bottom face
	vec3(0.0, -1.0, 0.0),

	 // Top face
	vec3(0.0,  1.0, 0.0)
};

const ivec2 POS_INDEX_TABLE[6] =
{
	//back face
	ivec2(-2, 1),

   // Front face
	ivec2(2, 1),

	// Left face
	ivec2(2, 1),

	// Right face
	ivec2(-2, 1),

	 // Bottom face
	ivec2(1, 2),

	 // Top face
	ivec2(-1, 2)
};


layout (std430, binding = 35) readonly restrict buffer VisiblesBuffer
{
    int data[];
} visibles;

uniform mat4 u_matrix;
uniform float u_bias;
uniform float u_slopeScale;
uniform int u_dataOffset;

#ifdef TRANSPARENT_SHADOWS
out vec2 TexCoords;
out vec2 TexOffset;
#endif

void main()
{
	int unpacked_norm = a_PackedNormHp >> 3;
	vec3 normal = CUBE_NORMALS_TABLE[unpacked_norm];

	int chunk_index = visibles.data[u_dataOffset + gl_DrawID];

	vec3 worldPos = chunk_data.data[chunk_index].min_point.xyz + a_Pos.xyz;

#ifdef TRANSPARENT_SHADOWS
	ivec2 posIndex = POS_INDEX_TABLE[unpacked_norm];
    ivec2 absPosIndex = abs(posIndex);
    int xPosSign = sign(posIndex.x);
	int texture_offset = block_info.data[a_BlockType].texture_offsets[unpacked_norm];
	 TexCoords = vec2((worldPos.x * xPosSign) + (worldPos[absPosIndex.x]) * xPosSign, -worldPos[absPosIndex.y]);
	 TexOffset = vec2(texture_offset % 25, texture_offset / 25);

	float posOffset = block_info.data[a_BlockType].position_offset;
	worldPos.xz -= (posOffset * normal.xz);
#endif

	gl_Position = u_matrix * vec4(worldPos.xyz - 0.5, 1.0);
}