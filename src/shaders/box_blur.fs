#version 460 core

out vec4 f_color;

in vec2 v_texCoords;

uniform sampler2D input_texture;
uniform int u_size;

void main()
{   int half_size = u_size / 2;
    vec2 texelSize = 1.0 / vec2(textureSize(input_texture, 0));
    vec4 result = vec4(0.0);
    for (int x = -half_size; x < half_size; ++x) 
    {
        for (int y = -half_size; y < half_size; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(input_texture, v_texCoords + offset);
        }
    }
    vec4 final_result = result / (u_size * u_size);
    f_color = vec4(final_result.rgb, 1.0);
}   