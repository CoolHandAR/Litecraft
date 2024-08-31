#version 460 core 

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_texCoords;

out VS_OUT
{
    vec2 TexCoords;
    vec4 clipPos;
} vs_out;


uniform float u_WaterHeight;
uniform float u_tilingFactor;


void main()
{
    vec4 worldPos = vec4(a_Pos.x, u_WaterHeight, a_Pos.y, 1.0);

    vs_out.TexCoords = vec2(a_Pos.x / 2.0 + 0.5, a_Pos.y / 2.0 + 0.5) * u_tilingFactor;
    //vs_out.clipPos = world_pos;
}