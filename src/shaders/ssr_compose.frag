#version 460 core

#include "../shader_commons.incl"
#include "../scene_incl.incl"

layout(location = 0) out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D reflected_texture;
uniform sampler2D specular_texture;

void main()
{
	vec4 ssr = texture(reflected_texture, TexCoords);
	vec4 specular_texture = texture(specular_texture, TexCoords);

	specular_texture.rgb = mix(specular_texture.rgb, ssr.rgb * specular_texture.a, ssr.a);

	FragColor = vec4(specular_texture.rgb, 1.0); 
}