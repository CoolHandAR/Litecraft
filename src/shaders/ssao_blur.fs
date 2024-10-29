#version 460 core


out vec4 f_color;

in vec2 v_texCoords;

uniform sampler2D ssao_texture;

void main()
{   
    vec2 texelSize = 1.0 / vec2(textureSize(ssao_texture, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssao_texture, v_texCoords + offset).r;
        }
    }
    float final_result = result / (4.0 * 4.0);
    f_color = vec4(final_result, 1, 1, 1);
}   