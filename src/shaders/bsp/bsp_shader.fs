#version 460 core 

out vec4 FragColor;
in vec2 TexCoords;
in vec2 LightCoords;

uniform sampler2D tex_image;
uniform sampler2D light_map;

void main()
{
    vec4 texColor = texture(tex_image, TexCoords);
    vec4 lightColor = texture(light_map, LightCoords);

    FragColor = vec4(texColor.rgb + lightColor.rgb, 1);
}