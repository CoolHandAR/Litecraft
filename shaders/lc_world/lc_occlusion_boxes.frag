#version 460 core

#include "lc_world_incl.incl"

out vec4 FragColor;

layout(early_fragment_tests) in;

flat in uint ChunkIndex;

void main()
{
    chunk_data.data[ChunkIndex].vis_flags |= CHUNK_FLAG_VISIBLE;

    //Only for debugging
    FragColor = vec4(1, 1, 1, 1);
}