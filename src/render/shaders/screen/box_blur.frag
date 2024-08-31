#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D source_texture;
uniform int u_size;

void main()
{
    int half_size = u_size / 2;
    vec2 texelSize = 1.0 / vec2(textureSize(source_texture, 0));
    vec4 result = vec4(0.0);
    for (int x = -half_size; x < half_size; ++x) 
    {
        for (int y = -half_size; y < half_size; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(source_texture, TexCoords + offset);
        }
    }
    vec4 final_result = result / (u_size * u_size);
    FragColor = vec4(final_result.rgb, 1.0);
}
