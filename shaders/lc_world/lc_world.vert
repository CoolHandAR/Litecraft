#version 460 core 

#include "../scene_incl.incl"
#include "lc_world_incl.incl"

layout (location = 0) in ivec3 a_Pos;
layout (location = 1) in int a_NormalUnit;
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

const vec3 CUBE_TANGENT_NORMALS_TABLE[6] =
{
	//back face
	vec3(-1.0, 0.0, 0.0),

   // Front face
	vec3(1.0, 0.0,  0.0),

	// Left face
	vec3(0.0,  0.0,  -1.0),

	// Right face
	vec3(0.0,  0.0,  1.0),

	 // Bottom face
	vec3(-1.0, 0.0, 0.0),

	 // Top face
	vec3(1.0,  0.0, 0.0)
};

const vec3 CUBE_BITANGENT_NORMALS_TABLE[6] =
{
	//back face
	vec3(0.0, -1.0, 0.0),

   // Front face
	vec3(0.0, 1.0,  0.0),

	// Left face
	vec3(0.0,  -1.0,  0.0),

	// Right face
	vec3(0.0,  1.0,  0.0),

	 // Bottom face
	vec3(0.0, 0.0, -1.0),

	 // Top face
	vec3(0.0,  0.0, 1.0)
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

vec2 calcWave2D(vec3 pos, float fm, float mm, float ma, float f0, float f1, float f2, float f3, float f4, float f5) 
{
	const float PI = 3.1415927;
	const float PI48 = 150.796447372;
	float animationSpeed = 1;
	float pi2wt = (PI48* scene.time) * animationSpeed;

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

#ifdef USE_TBN_MATRIX
out mat3 out_TBN;
#endif

#ifdef USE_TEXCOORDS
out vec2 out_TexCoords;
out vec2 out_TexOffset;
#endif

#ifdef FORWARD_PASS
out vec3 out_Normal;
out vec3 out_WorldPos;
#endif

uniform int u_chunkOffset;
uniform float u_clipDistance;
uniform mat4 u_matrix;

#ifdef CHUNK_INDEX_USE_OFFSET
layout (std430, binding = 16) readonly restrict buffer VisiblesBuffer
{
    int data[];
} visibles;
#endif


void main()
{
	uint chunk_index = 0;

#ifdef CHUNK_INDEX_USE_OFFSET
	chunk_index = visibles.data[u_chunkOffset + gl_DrawID];
#else
	chunk_index = gl_BaseInstance;
#endif

	vec3 WorldPos = (chunk_data.data[chunk_index].min_point.xyz + a_Pos.xyz);
	vec3 normal = CUBE_NORMALS_TABLE[a_NormalUnit];

#ifdef USE_TEXCOORDS
	int texture_offset = block_info.data[a_BlockType].texture_offsets[a_NormalUnit];
	ivec2 posIndex = POS_INDEX_TABLE[a_NormalUnit];
    ivec2 absPosIndex = abs(posIndex);
    int xPosSign = sign(posIndex.x);

    out_TexCoords = vec2((WorldPos.x * xPosSign) + (WorldPos[absPosIndex.x]) * xPosSign, -WorldPos[absPosIndex.y]);
	out_TexOffset = vec2(texture_offset % 25, texture_offset / 25);
#endif

#ifdef USE_TBN_MATRIX
	vec3 tangent = CUBE_TANGENT_NORMALS_TABLE[a_NormalUnit];
	vec3 biTangent = CUBE_BITANGENT_NORMALS_TABLE[a_NormalUnit];
    mat3 TBN = mat3(tangent, biTangent, normal);

    out_TBN = TBN;
#endif

#ifdef SEMI_TRANSPARENT
	float posOffset = block_info.data[a_BlockType].position_offset;
	WorldPos.xz -= (posOffset * normal.xz);
	
	vec2 windOffset = vec2(0.0);
	

	switch(a_BlockType)
	{
		//LEAVES
		case 6:
		case 23:
		{
			windOffset = calcMove2D(WorldPos.xyz,
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
			windOffset = calcMove2D(WorldPos.xyz,
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
	
	WorldPos.xz += windOffset.xy;
#endif

	WorldPos -= 0.5;

#ifdef USE_CLIP_DISTANCE
	const vec4 plane = vec4(0, 1, 0, u_clipDistance);
	gl_ClipDistance[0] = dot(vec4(WorldPos, 1.0), plane);
#endif
	
#ifdef USE_UNIFORM_MATRIX
	gl_Position = u_matrix * vec4(WorldPos.xyz, 1.0);
#else
	gl_Position = cam.viewProjection * vec4(WorldPos.xyz, 1.0);
#endif

#ifdef FORWARD_PASS
	out_Normal = normal;
	out_WorldPos = WorldPos;
#endif
}