#version 450 core

layout(location = 0) out vec3 g_position;
layout(location = 1) out vec3 g_normal;
layout(location = 2) out vec4 g_colorSpec;

struct VecOutput
{
	vec3 v_FragPos;
	vec3 v_Normal;
	vec2 v_texCoords;
    vec2 v_texOffset;
};

layout(location = 0) in VecOutput v_Output;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

void main()
{
	//vec2 tex_coords_offsetted = vec2((fract(v_Output.v_texCoords.x) + v_Output.v_texOffset.x) / 64, -(fract(v_Output.v_texCoords.y) + v_Output.v_texOffset.y) / 32);

	g_position = v_Output.v_FragPos;

    g_normal = normalize(v_Output.v_Normal);

    g_colorSpec.rgb = texture(texture_diffuse1, v_Output.v_texCoords).rgb;

    g_colorSpec.a = texture(texture_specular1, v_Output.v_texCoords).r;
}
