#version 460 core

layout(location = 0) out float FragColor;

in VS_OUT
{
    vec2 TexCoords;
    flat int TexIndex;
} vs_in;

#ifdef USE_BINDLESS_TEXTURES

#else
    uniform sampler2D textures_arr[32];
#endif

void main()
{
    float alpha = texture(textures_arr[vs_in.TexIndex], vs_in.TexCoords).a;

    if(alpha < 0.1)
    {
        discard;
    }

    float depth = gl_FragCoord.z;

    float dx = dFdx(depth);
    float dy = dFdy(depth);
    float moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);

    FragColor = moment2;
}
