#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec2 a_TexOffset;
layout (location = 4) in float a_Hp;

out VS_OUT
{
    vec3 world_pos;
    vec3 normal;
    vec2 tex_offset;
    vec2 tex_coords;
    float hp;
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
    gl_Position = camera.viewProjectionMatrix * vec4(a_Pos, 1.0);

    vs_out.world_pos = a_Pos;
    vs_out.normal = a_Normal;
    vs_out.tex_coords = a_TexCoords;
    vs_out.tex_offset = a_TexOffset;
    vs_out.hp = a_Hp;
}
