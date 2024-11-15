#version 460 core 

#include "../scene_incl.incl"
#include "lc_world_incl.incl"

layout (location = 0) in ivec3 a_Pos;

out VS_OUT
{
    vec2 TexCoords;
    vec4 ClipPos;
    vec3 WorldPos;
} vs_out;

uniform float u_tilingFactor;

void main()
{
    vec4 worldPos = vec4(chunk_data.data[gl_BaseInstance].min_point.xyz + a_Pos.xyz, 1.0);
    worldPos.xz -= 0.5;

    vec4 clipPos = cam.viewProjection * worldPos;
    
    vec2 vertexPos = vec2(a_Pos.x, a_Pos.z);

    vs_out.WorldPos = worldPos.xyz;
    vs_out.ClipPos = clipPos; 
    vs_out.TexCoords = vec2(vertexPos.x / 2.0 + 0.5, vertexPos.y / 2.0 + 0.5) * 0.08;

    gl_Position = clipPos;
}