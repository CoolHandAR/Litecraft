#version 450 core

layout(location = 0) out vec4 g_normal;
layout(location = 1) out vec4 g_colorSpec;
layout(location = 2) out float g_emissive;

struct VecOutput
{
   vec2 v_texCoords;
   vec2 v_texOffset;
   float v_ao;
};

layout(location = 0) in VecOutput v_Output;

flat in int v_hp;
in vec3 v_Normal;
in mat3 v_TBN;
in vec3 v_worldPos;

uniform sampler2D texture_atlas;
uniform sampler2D texture_atlas_normal;
uniform sampler2D texture_atlas_mer;

vec3 getNormalFromMap(vec3 WorldPos, vec2 TexCoords, vec3 Normal)
{
    vec3 tangentNormal = texture(texture_atlas_normal, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
vec3 getNormalFromMap2(vec3 WorldPos, vec2 TexCoords, vec3 Normal)
{
    vec3 tangentNormal = texture(texture_atlas_normal, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return T;
}


void main()
{
   vec4 tex_color = vec4(0.0);
	vec2 coords = vec2(0.0);
   vec2 tex_coords_offsetted = vec2(0.0);
   
   int reflective = (v_hp > 68) ? 1 : 0;

   coords = v_Output.v_texCoords;

   coords = fract(coords);

   tex_coords_offsetted = vec2((coords.x + v_Output.v_texOffset.x) / 25, -(coords.y + v_Output.v_texOffset.y) / 25);
   
   tex_color = texture(texture_atlas, tex_coords_offsetted);

   //apply the shatter texture based on the hp of the block
   if(v_hp < 7)
   {
      vec2 shatter_tex_coords = vec2((coords.x + 24) / 25, -(coords.y + (7 - v_hp)) / 25);
      vec4 shatter_color = texture(texture_atlas,  shatter_tex_coords);

      tex_color = tex_color + (shatter_color * (max(sign(shatter_color.a - 0.5), 0.0))); //add shatter color if alpha is less than 0.5
   }

   vec3 normal_color = texture(texture_atlas_normal, tex_coords_offsetted).rgb * 2.0 - 1.0;

   vec2 tex_coords_offsetted2 = vec2((coords.x + v_Output.v_texOffset.x) / 25, -(coords.y + v_Output.v_texOffset.y) / 25);
   vec3 mer_color = texture(texture_atlas_mer, tex_coords_offsetted).rgb;

   vec3 norm = normalize(v_TBN * normal_color);

	g_normal.rgb = normalize(norm) * 0.5 + 0.5;
   g_normal.a = mer_color.r;
   g_colorSpec.rgb = tex_color.rgb;
	g_colorSpec.a = mer_color.b;
   g_emissive = mer_color.g;
}
