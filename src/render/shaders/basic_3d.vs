#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec4 a_Color;

out VS_OUT
{
    vec4 color;
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

    vs_out.color = a_Color;
}
