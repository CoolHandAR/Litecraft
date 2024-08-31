#version 460 core

out vec4 f_color;

in VS_OUT
{
    vec4 color;
} fs_in;


void main()
{
   f_color = fs_in.color;
}
