#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_texCoords;
layout (location = 2) in uint a_texIndex;
layout (location = 3) in uvec4 a_Color;

out VS_OUT
{
    vec2 TexCoords;
    vec4 Color;
    flat uint TextureIndex;
} vs_out;

uniform mat4 u_projection;

void main()
{
    vec4 fColor = vec4(a_Color);
    vs_out.Color = vec4(fColor.r / 255.0, fColor.g / 255.0, fColor.b / 255.0, fColor.a / 255.0);

    vs_out.TexCoords = vec2(a_texCoords.x, a_texCoords.y);
    vs_out.TextureIndex = a_texIndex;
    gl_Position = u_projection * vec4(a_Pos.xy, 0.0, 1.0);
}