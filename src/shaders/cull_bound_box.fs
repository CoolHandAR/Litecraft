#version 460 core

layout(early_fragment_tests) in;

flat in uint v_chunkID;


layout (std430, binding = 11) writeonly restrict buffer ChunkVisibles
{
    uint data[];
} VisibleChunk;



void main()
{   
    VisibleChunk.data[v_chunkID] = 1; 
}
