#version 460 core 

layout (location = 0) in ivec3 a_Pos;
layout (location = 1) in int a_PackedNormHp;
layout (location = 2) in uint a_BlockType;

struct VecOutput
{
   vec3 v_FragPos;
	vec3 v_Normal;
   vec2 v_texOffset;
};

layout (location = 0) out VecOutput v_Output;

flat out int v_hp;
flat out ivec2 v_posIndex;

struct ChunkData
{
    vec4 min_point;

};

struct BlockMaterialData
{
	int texture_offsets[6];
	
	float shininess;
	float specular;
	float reflects;
};

layout (std430, binding = 9) readonly restrict buffer ChunkMinPointsBuffer
{
    ChunkData data[];
} chunk_data;

layout (shared, binding = 2) uniform BlockMaterialDataBuffer
{
	BlockMaterialData data[14];
}blockMat;


layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

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
	ivec2(2, 1),

   // Front face
	ivec2(2, 1),

	// Left face
	ivec2(2, 1),

	// Right face
	ivec2(2, 1),

	 // Bottom face
	ivec2(1, 2),

	 // Top face
	ivec2(1, 2)
};

invariant gl_Position;
void main()
{  
   vec3 w_pos = chunk_data.data[gl_DrawID].min_point.xyz;
   vec3 offset = (gl_VertexID % 2 == 0) ? vec3(-0.00001) : vec3(0.00001); //add some offset to avoid z-fighting issues

   vec4 world_pos = g_viewProjectionMatrix * vec4(offset + w_pos + a_Pos - 0.5, 1.0);

   gl_Position = world_pos;

   int unpacked_hp = (a_PackedNormHp & ~(7 << 3));
   int unpacked_norm = a_PackedNormHp >> 3;
   int texture_offset = blockMat.data[a_BlockType].texture_offsets[unpacked_norm];

   v_Output.v_FragPos = w_pos + a_Pos - 0.5;
   v_Output.v_texOffset = vec2(texture_offset % 25, texture_offset / 25);
   v_Output.v_Normal = CUBE_NORMALS_TABLE[unpacked_norm];
   v_hp = unpacked_hp;
   v_posIndex = POS_INDEX_TABLE[unpacked_norm];
}