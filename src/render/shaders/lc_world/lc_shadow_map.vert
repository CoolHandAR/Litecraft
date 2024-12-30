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

const float PI = 3.1415927;
const float PI48 = 150.796447372;
uniform float frameTimeCounter;
float animationSpeed = 2;
float pi2wt = (PI48*frameTimeCounter) * animationSpeed;


vec2 calcWave2D(vec3 pos, float fm, float mm, float ma, float f0, float f1, float f2, float f3, float f4, float f5) {
    vec2 ret;
    float magnitude,d0,d1,d2,d3;
    magnitude = sin(pi2wt*fm + pos.x*0.5 + pos.z*0.5 + pos.y*0.5) * mm + ma;
    d0 = sin(pi2wt*f0);
    d1 = sin(pi2wt*f1);
    d2 = sin(pi2wt*f2);
    ret.x = sin(pi2wt*f3 + d0 + d1 - pos.x + pos.z + pos.y) * magnitude;
	ret.y = sin(pi2wt*f5 + d2 + d0 + pos.z + pos.y - pos.y) * magnitude;
    return ret;
}

vec2 calcMove2D(vec3 pos, float f0, float f1, float f2, float f3, float f4, float f5, vec3 amp1, vec3 amp2) {
    vec2 move1 = calcWave2D(pos      , 0.0027, 0.0400, 0.0400, 0.0127, 0.0089, 0.0114, 0.0063, 0.0224, 0.0015) * amp1.xz;
	vec2 move2 = calcWave2D(pos+ vec3(move1, 0), 0.0348, 0.0400, 0.0400, f0, f1, f2, f3, f4, f5) * amp2.xz;
    return move1+move2;
}

uniform mat4 u_matrix;
uniform int u_dataOffset;
uniform bool u_enableWave;

#ifdef SEMI_TRANSPARENT
out vec2 TexCoords;
out vec2 TexOffset;
#endif

void main()
{
	int unpacked_norm = a_PackedNormHp >> 3;
	vec3 normal = CUBE_NORMALS_TABLE[unpacked_norm];

	int chunk_index = visibles.data[u_dataOffset + gl_DrawID];

	vec3 worldPos = chunk_data.data[chunk_index].min_point.xyz + a_Pos.xyz;

#ifdef SEMI_TRANSPARENT
	ivec2 posIndex = POS_INDEX_TABLE[unpacked_norm];
    ivec2 absPosIndex = abs(posIndex);
    int xPosSign = sign(posIndex.x);
	int texture_offset = block_info.data[a_BlockType].texture_offsets[unpacked_norm];
	 TexCoords = vec2((worldPos.x * xPosSign) + (worldPos[absPosIndex.x]) * xPosSign, -worldPos[absPosIndex.y]);
	 TexOffset = vec2(texture_offset % 25, texture_offset / 25);

	float posOffset = block_info.data[a_BlockType].position_offset;
	worldPos.xz -= (posOffset * normal.xz);

	vec2 windOffset = vec2(0.0);
	
	if(u_enableWave)
	{
	switch(a_BlockType)
	{
		//LEAVES
		case 6:
		case 23:
		{
			windOffset = calcMove2D(worldPos.xyz,
			0.0040,
			0.0064,
			0.0043,
			0.0035,
			0.0037,
			0.0041,
			vec3(1.0,0.2,1.0),
			vec3(0.5,0.1,0.5));
			break;
		}
		case 9:
		case 20:
		case 22:
		{
			windOffset = calcMove2D(worldPos.xyz,
			0.0041,
			0.0070,
			0.0044,
			0.0038,
			0.0240,
			0.0000,
			vec3(0.8,0.0,0.8),
			vec3(0.4,0.0,0.4));
			break;
		}
		default:
		{
			break;
		}
	}	
	}
	
	

	worldPos.xz += windOffset.xy;
#endif
	
	gl_Position = u_matrix * vec4(worldPos.xyz - 0.5, 1.0);
}