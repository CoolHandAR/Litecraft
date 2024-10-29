#version 460 core

layout(location = 0) out vec4 FragColor;

in VS_OUT
{
    vec2 TexCoords;
    vec3 LightColor;
    vec4 Color;
    flat int TexIndex;
} vs_in;

#ifdef USE_BINDLESS_TEXTURES

#else
    uniform sampler2D textures_arr[32];
#endif


void main()
{
    vec4 texColor = texture(textures_arr[vs_in.TexIndex], vs_in.TexCoords);
    
    if(texColor.a < 0.1)
    {
        discard;
    }

    vec4 FinalColor = vec4(vs_in.LightColor.rgb, 1.0);
    FinalColor.rgb = vs_in.LightColor + texColor.rgb;
    FinalColor.a = texColor.a;

	FragColor = vec4(FinalColor);
}
