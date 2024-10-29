#version 450 core

out vec4 f_color;

in vec4 v_Color;


void main()
{
	f_color = v_Color;
}
