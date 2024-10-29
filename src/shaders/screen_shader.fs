#version 460 core

out vec4 f_color;

in vec2 v_texCoords;
flat in float v_texIndex;

uniform sampler2D u_textures[32];


void main()
{
	int tex_index = int(v_texIndex);

	vec4 tex_color = texture(u_textures[tex_index], v_texCoords);

	if(tex_color.a < 0.1)
		discard;

	f_color = tex_color;


}
