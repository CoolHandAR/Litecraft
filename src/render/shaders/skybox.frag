#version 460 core

#include "scene_incl.incl"

layout(location = 0) out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox_texture;
const float PI = 3.14159265359;

void main()
{
	vec4 sky = texture(skybox_texture, TexCoords);
	
	FragColor = vec4(sky.rgb, 1.0);
}
