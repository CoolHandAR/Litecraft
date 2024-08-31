#version 460 core 

layout (location = 0) out vec4 FragColor;

in VS_OUT
{
    vec2 TexCoords;
    vec4 clipPos;
} vs_in;

/*
~~~~~~~~~~~~~~~
TEXTURES
~~~~~~~~~~~~~~~
*/
uniform sampler2D dudv_map;
uniform sampler2D normal_map;

void main()
{

}


