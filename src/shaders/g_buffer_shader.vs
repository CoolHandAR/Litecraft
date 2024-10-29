#version 460 core 

layout (location = 0) in ivec3 a_Pos;
layout (location = 1) in int a_PackedNormHp;
layout (location = 2) in uint a_BlockType;

struct VecOutput
{
   vec2 v_texCoords;
   vec2 v_texOffset;
   float v_ao;
};

layout (location = 0) out VecOutput v_Output;

flat out int v_hp;
out mat3 v_TBN;
out vec3 v_Normal;
out vec3 v_worldPos;

struct ChunkData
{
   vec4 min_point;
    uint vis_flags;
};

struct BlockMaterialData
{
	int texture_offsets[6];
};


layout (std430, binding = 9) readonly restrict buffer ChunkMinPointsBuffer
{
    ChunkData data[];
} chunk_data;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

layout (shared, binding = 2) uniform BlockMaterialDataBuffer
{
	BlockMaterialData data[14];
}blockMat;


uniform mat4 u_view;

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

mat3 tangentTest(vec3 normal)
{
	vec3 tangent = vec3(0.0);

	vec3 c1 = cross(normal, vec3(0.0, 0.0, 1.0));
	vec3 c2 = cross(normal, vec3(0.0, 1.0, 0.0));

	if (length(c1)>length(c2))
	{
    	tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	tangent = normalize(tangent);

	vec3 biTangent = cross(normal, tangent);
	biTangent = normalize(biTangent);
	mat3 TBN = mat3(tangent, biTangent, normal);

	return TBN;
}

void main()
{
   vec3 world_pos = chunk_data.data[gl_DrawID].min_point.xyz;
   gl_Position = g_viewProjectionMatrix * vec4(world_pos + a_Pos - 0.5, 1.0);

   int unpacked_hp = (a_PackedNormHp & ~(7 << 3));
   int unpacked_norm = a_PackedNormHp >> 3;
   int texture_offset = blockMat.data[a_BlockType].texture_offsets[unpacked_norm];

   vec3 FragPos = world_pos + a_Pos;
   v_Output.v_texOffset = vec2(texture_offset % 25, texture_offset / 25);
   v_hp = unpacked_hp;
   ivec2 posIndex = POS_INDEX_TABLE[unpacked_norm];
   ivec2 absPosIndex = abs(posIndex);
   int xPosSign = sign(posIndex.x);

   v_Output.v_texCoords = vec2((FragPos.x * xPosSign) + (FragPos[absPosIndex.x]) * xPosSign, -FragPos[absPosIndex.y]);
	
   vec3 normal = CUBE_NORMALS_TABLE[unpacked_norm];
   vec3 tangent = CUBE_TANGENT_NORMALS_TABLE[unpacked_norm];
   vec3 reTangent = normalize(tangent - dot(tangent, normal) * normal);
   vec3 biTangent = normalize(cross(normal, tangent));
   mat3 TBN = mat3(tangent, biTangent, normal);

	mat4 model = mat4(1.0);

	model[3].xyz += FragPos - 0.5;
	mat3 normalMatrix = transpose(inverse(mat3(u_view)));
    vec3 tNormal = normalMatrix * tangent;

    v_Normal = normal;
	v_worldPos = FragPos;

   v_TBN = TBN;
}
