#version 460 core 

#include "../scene_incl.incl"
#include "lc_world_incl.incl"

layout (location = 0) in ivec2 a_Pos;

const float PI = 3.1415926535897932384626433832795;

const float waveAmplitude = 3.8;
const float waterHeight = 15;

out VS_OUT
{
    vec4 ClipPos;
    vec3 WorldPos;
    vec2 TexCoords;
} vs_out;


uniform float u_tilingFactor;
uniform float u_moveFactor;


vec3 calcDistortion(vec3 vertex)
{
    float fy = fract(vertex.y + 0.001);
	float wave = 0.05 * sin(2 * PI * (u_moveFactor *0.8 + vertex.x /  2.5 +vertex.z / 5.0))
			+ 0.05 * sin(2 * PI * (u_moveFactor *0.6 + vertex.x / 6.0 + vertex.z /  12.0));

    vertex.y += clamp(wave, -fy, 1.0-fy)* waveAmplitude;

    return vertex;
}


void main()
{   
    vec4 worldPos = vec4(chunk_data.data[gl_BaseInstance].min_point.xyz, 1.0);
    worldPos.xz -= 0.5;

    vec3 v0 = vec3(a_Pos.x, waterHeight, a_Pos.y) + worldPos.xyz;
  
    //apply distortion
    v0 = calcDistortion(v0);

    vec4 ClipPosDistorted = cam.viewProjection * vec4(v0.xyz, 1.0);

    vs_out.ClipPos = ClipPosDistorted;
    vs_out.WorldPos = v0.xyz;
    vs_out.TexCoords = vec2(v0.x / 2.0 + 0.5, v0.z / 2.0 + 0.5) * (1.0 / 8.0);

    gl_Position = ClipPosDistorted;
}