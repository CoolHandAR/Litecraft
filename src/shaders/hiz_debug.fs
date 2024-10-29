#version 460 core

out vec4 FragColor;

layout(early_fragment_tests) in;
flat in uint v_chunkIndex;
layout (std430, binding = 11) restrict buffer ChunkVisibles
{
    uint data[];
} VisibleChunk;

#define CHUNK_FLAG_VISIBLE uint(1)
#define CHUNK_FLAG_IN_FRUSTRUM uint(2)
#define CHUNK_FLAG_PREV_VISIBLE uint(4)

void main()
{
    VisibleChunk.data[v_chunkIndex] |= CHUNK_FLAG_VISIBLE;
    FragColor = vec4(1, 1, 1, 0.5);
}