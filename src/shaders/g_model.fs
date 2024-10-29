#version 460 core

layout(location = 0) out vec4 g_normal;
layout(location = 1) out vec4 g_colorSpec;

in VS_Out
{
    vec3 Normal;
    vec2 TexCoords;
} vs_in;



void main()
{

    g_normal.rgb = normalize(vs_in.Normal) * 0.5 + 0.5;
    g_normal.a = 1;
    g_colorSpec = vec4(1, 1, 0, 1);
}