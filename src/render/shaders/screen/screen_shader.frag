#version 460 core

layout(location = 0) out vec4 FragColor;

in VS_OUT
{
    vec2 TexCoords;
    vec4 Color;
    flat uint TextureIndex;
} vs_in;

uniform sampler2D u_textures[32];

void main()
{
    vec4 textureColor = texture(u_textures[vs_in.TextureIndex], vs_in.TexCoords);
    vec3 finalColor = vs_in.Color.rgb * textureColor.rgb;
    
    float alpha = textureColor.a * vs_in.Color.a;

    FragColor = vec4(finalColor.rgb, alpha);
}