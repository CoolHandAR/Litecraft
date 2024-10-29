#version 460 core

flat out uint v_chunkIndex;

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

void main(void)
{
    VisibleChunk.data[gl_VertexID] = 0;
    v_chunkIndex = gl_VertexID;
    
    gl_Position = g_viewProjectionMatrix * vec4(chunk_data.data[gl_VertexID].min_point.xyz - 0.5, 1.0);
}
