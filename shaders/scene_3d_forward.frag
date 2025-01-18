#version 460 core

layout(location = 0) out vec4 FragColor;

in VS_OUT
{
    vec4 Color;

    vec2 TexCoords;
    
    vec3 Normal;

    flat int TexIndex;

} vs_in;

    uniform sampler2D texture_arr[32];
    uniform sampler2D texture_sample_albedo;

void main()
{   
    vec4 texColor = vec4(1);

#ifdef SEMI_TRANSPARENT_PASS
    float alpha = 1.0;
#ifdef USE_TEXTURE_ARR
    alpha = texture(texture_arr[vs_in.TexIndex], vs_in.TexCoords).a;
#endif 

    if(alpha < 0.5)
    {
        discard;
    }
#endif

#ifdef USE_TEXTURE_ARR
    texColor = texture(texture_arr[vs_in.TexIndex], vs_in.TexCoords);
#endif //USE_TEXTURE_ARR
    
#ifndef RENDER_DEPTH
    vec4 finalColor = vs_in.Color * texColor.rgba;

    FragColor = finalColor;
#endif
}