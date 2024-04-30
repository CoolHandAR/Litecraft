#version 460 core

out vec4 f_color;
layout(early_fragment_tests) in;

layout(std430, binding = 3) writeonly restrict buffer SSBO
{
    int data_SSBO[];
};
//flat in int v_chunkIndex;

void main()
{
   // int index = int(v_chunkIndex);


    f_color = vec4(1, 1, 1, 1);
}
