#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec3 a_Tangent;
layout (location = 3) in vec3 a_bitTangent; 
layout (location = 4) in vec2 a_TexCoords;


out VS_OUT
{
    vec2 tex_coords;
    vec3 world_pos;
    vec3 normal;
    vec3 frag_pos;
    vec3 tangent_view_pos;
    vec3 tangent_frag_pos;
    vec3 tangent_light_pos;
    uint material_index;
} vs_out;

layout (std140, binding = 0) uniform CameraData
{
	mat4 viewProjectionMatrix;
    mat4 view;
    vec4 frustrum_planes[6];
    vec3 position;
    ivec2 screen_size;

    float z_near;
    float z_far;
} camera;

void main()
{
    vs_out.tex_coords = a_TexCoords;
}
