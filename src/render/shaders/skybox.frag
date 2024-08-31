#version 460 core

layout(location = 0) out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox_texture;

void main()
{
	FragColor = texture(skybox_texture, TexCoords);
}
