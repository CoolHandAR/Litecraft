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

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

void main()
{   
    vec3 world_pos = chunk_data.data[gl_DrawID].min_point.xyz;
    gl_Position = g_viewProjectionMatrix * vec4(world_pos + a_Pos - 0.5, 1.0);
}