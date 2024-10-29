#version 460 core

layout(location = 0) out vec4 FragColor;

in VS_OUT
{
    vec4 Color;
    vec2 TexCoords;
    flat int TexIndex;
} vs_out;

uniform sampler2D texture_arr[32];

void main()
{
    vec2 texture_size = textureSize(texture_arr[vs_out.TexIndex], 0);
    vec2 coords = vs_out.TexCoords;

    vec4 texColor = texture(texture_arr[vs_out.TexIndex], coords);

    vec3 finalColor = vs_out.Color.rgb * texColor.rgb;
    
    float alpha = texColor.a * vs_out.Color.a;

    FragColor = vec4(finalColor.rgb, alpha);
}