#version 460 core

out vec4 f_color;

in vec2 v_texCoords;

uniform sampler2D texture_sampler;

void main()
{

    f_color = texture(texture_sampler, v_texCoords);
}