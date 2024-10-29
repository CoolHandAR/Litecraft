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


uniform mat4 u_view;
uniform mat4 u_proj;

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




void main()
{
   vec3 world_pos = chunk_data.data[gl_DrawID].min_point.xyz;
   gl_Position = u_proj * u_view * vec4(world_pos + a_Pos - 0.5, 1.0);

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
}
