#version 460 core

out vec4 f_color;

in vec2 v_texCoords;


uniform samplerCube texture_sample;


void main()
{
    vec4 texColor = texture(texture_sample, vec3(v_texCoords, 1));

    f_color = vec4(texColor.rgb, 1.0);
}