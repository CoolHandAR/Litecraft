#version 460 core

#define TEXTURE_BLOCK_UNIT_SIZE 16

layout(location = 0) out vec4 g_normalMetal;
layout(location = 1) out vec4 g_colorRough;
layout(location = 2) out float g_emissive;

in VS_OUT
{
    mat3 TBN;
    vec2 TexCoords;
	vec2 TexOffset;
    flat int Block_hp;
} vs_in;

uniform sampler2D texture_atlas;
uniform sampler2D texture_atlas_normal;
uniform sampler2D texture_atlas_mer;

void main()
{
    vec2 texCoords = fract(vs_in.TexCoords);

    //Offset the coords
    texCoords = vec2((texCoords.x + vs_in.TexOffset.x) / 25, -(texCoords.y + vs_in.TexOffset.y) / 25);

    vec4 AlbedoColor = texture(texture_atlas, texCoords);

#ifdef SEMI_TRANSPARENT
    if(AlbedoColor.a < 0.5)
    {
        discard;
    }

#endif

    vec3 MerColor = texture(texture_atlas_mer, texCoords).rgb;
    vec3 NormalColor = texture(texture_atlas_normal, texCoords).rgb * 2.0 - 1.0;

   g_normalMetal.rgb = normalize(vs_in.TBN * NormalColor) * 0.5 + 0.5; //Normal. We convert the normal value to [0, 1] range so that we can store in a unsigned texture format
   g_normalMetal.a = MerColor.r; //Metal

   g_colorRough.rgb = AlbedoColor.rgb; //Color
   g_colorRough.a = MerColor.b; //Rough

   g_emissive = MerColor.g; //Emissive
}