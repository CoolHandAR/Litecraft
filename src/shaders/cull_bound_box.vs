#version 460 core

layout (location = 0) in ivec3 a_Pos;

struct ChunkData
{
    vec4 min_point;
};

layout (std430, binding = 9) readonly restrict buffer ChunkMinPointsBuffer
{
    ChunkData data[];
} chunk_data;

layout (std430, binding = 11) writeonly restrict buffer ChunkVisibles
{
    uint data[];
} VisibleChunk;


layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

flat out uint v_chunkID;

void main()
{   
    VisibleChunk.data[0] = 0;
    v_chunkID = 0;
    vec3 world_pos = chunk_data.data[0].min_point.xyz;
    gl_Position = g_viewProjectionMatrix * vec4(a_Pos - 0.5, 1.0);
}