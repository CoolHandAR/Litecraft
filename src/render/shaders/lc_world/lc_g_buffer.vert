#version 460 core 

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
	vec3(0.0, -1.0,  0.0),

	// Left face
	vec3(0.0,  -1.0,  0.0),

	// Right face
	vec3(0.0,  -1.0,  0.0),

	 // Bottom face
	vec3(0.0, 0.0, -1.0),

	 // Top face
	vec3(0.0,  0.0, -1.0)
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


out VS_OUT
{
    mat3 TBN;
    vec2 TexCoords;
	vec2 TexOffset;
    int Block_hp;
} vs_out;

void main()
{
    vec3 worldPos = chunk_data.data[gl_DrawID].min_point.xyz + a_Pos.xyz;

    int unpacked_hp = (a_PackedNormHp & ~(7 << 3));
    int unpacked_norm = a_PackedNormHp >> 3;
	int texture_offset = block_info.data[a_BlockType].texture_offsets[unpacked_norm];

    vec3 normal = CUBE_NORMALS_TABLE[unpacked_norm];
    vec3 tangent = CUBE_TANGENT_NORMALS_TABLE[unpacked_norm];
    vec3 biTangent = normalize(cross(normal, tangent));
    mat3 TBN = mat3(tangent, biTangent, normal);

    ivec2 posIndex = POS_INDEX_TABLE[unpacked_norm];
    ivec2 absPosIndex = abs(posIndex);
    int xPosSign = sign(posIndex.x);


    vs_out.TBN = TBN;
    vs_out.TexCoords = vec2((worldPos.x * xPosSign) + (worldPos[absPosIndex.x]) * xPosSign, -worldPos[absPosIndex.y]);
	vs_out.TexOffset = vec2(texture_offset % 25, texture_offset / 25);
    vs_out.Block_hp = unpacked_hp;

	gl_Position = cam.viewProjection * vec4(worldPos - 0.5, 1.0);
}