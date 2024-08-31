#version 460 core

#include "../shader_commons.incl"
#include "../scene_incl.incl"

layout(location = 0) out float FragColor;

in vec2 TexCoords;

#define NUM_KERNLES_SAMPLES 32
const vec3 kernelSamples[NUM_KERNLES_SAMPLES] = // precalculated hemisphere kernel (low discrepancy noiser)
{
    vec3(-0.668154, -0.084296, 0.219458),
    vec3(-0.092521,  0.141327, 0.505343),
    vec3(-0.041960,  0.700333, 0.365754),
    vec3( 0.722389, -0.015338, 0.084357),
    vec3(-0.815016,  0.253065, 0.465702),
    vec3( 0.018993, -0.397084, 0.136878),
    vec3( 0.617953, -0.234334, 0.513754),
    vec3(-0.281008, -0.697906, 0.240010),
    vec3( 0.303332, -0.443484, 0.588136),
    vec3(-0.477513,  0.559972, 0.310942),
    vec3( 0.307240,  0.076276, 0.324207),
    vec3(-0.404343, -0.615461, 0.098425),
    vec3( 0.152483, -0.326314, 0.399277),
    vec3( 0.435708,  0.630501, 0.169620),
    vec3( 0.878907,  0.179609, 0.266964),
    vec3(-0.049752, -0.232228, 0.264012),
    vec3( 0.537254, -0.047783, 0.693834),
    vec3( 0.001000,  0.177300, 0.096643),
    vec3( 0.626400,  0.524401, 0.492467),
    vec3(-0.708714, -0.223893, 0.182458),
    vec3(-0.106760,  0.020965, 0.451976),
    vec3(-0.285181, -0.388014, 0.241756),
    vec3( 0.241154, -0.174978, 0.574671),
    vec3(-0.405747,  0.080275, 0.055816),
    vec3( 0.079375,  0.289697, 0.348373),
    vec3( 0.298047, -0.309351, 0.114787),
    vec3(-0.616434, -0.117369, 0.475924),
    vec3(-0.035249,  0.134591, 0.840251),
    vec3( 0.175849,  0.971033, 0.211778),
    vec3( 0.024805,  0.348056, 0.240006),
    vec3(-0.267123,  0.204885, 0.688595),
    vec3(-0.077639, -0.753205, 0.070938)
};

uniform sampler2D depth_texture;
uniform sampler2D noise_texture;
uniform sampler2D gNormalMetal;

uniform float u_radius = 0.6;
uniform float u_bias = 0.025;
uniform float u_strength = 5;
uniform vec2 u_viewportSize;

void main()
{
    vec3 ViewPos = depthToViewPos(depth_texture, cam.invProj, TexCoords);
    vec3 WorldPos = (cam.invView * vec4(ViewPos, 1.0)).xyz;
    vec3 Normal = texture(gNormalMetal, TexCoords).rgb * 2.0 - 1.0;
    vec2 NoiseScale = vec2(u_viewportSize.x / 4.0, u_viewportSize.y / 4.0);
    vec3 RandomVec = normalize(texture(noise_texture, TexCoords * NoiseScale).xyz);

    vec3 tangent = normalize(RandomVec - Normal * dot(RandomVec, Normal));
    vec3 biTangent = cross(Normal, tangent);
    mat3 TBN = mat3(tangent, biTangent, Normal);
    
    float occlusion = 0.0;
    for(int i = 0; i < NUM_KERNLES_SAMPLES; ++i)
    {
        vec3 SamplePos = TBN * kernelSamples[i];
        SamplePos = WorldPos + SamplePos * u_radius;
        SamplePos = (cam.view * vec4(SamplePos, 1.0)).xyz;

        vec4 offset = vec4(SamplePos, 1.0);
        offset = cam.proj * offset;
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = depthToViewSpace(depth_texture, cam.invProj, offset.xy);

        float rangeCheck = smoothstep(0.0, 1.0, u_radius / abs(ViewPos.z - sampleDepth));
        occlusion += (sampleDepth >= SamplePos.z + u_bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / NUM_KERNLES_SAMPLES);

    FragColor = pow(occlusion, u_strength);
}