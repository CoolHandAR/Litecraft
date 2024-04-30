#version 460 core

out vec4 f_color;

in vec3 v_texCoords;

uniform samplerCube skybox_texture;

void main()
{
	f_color = texture(skybox_texture, v_texCoords);
}
