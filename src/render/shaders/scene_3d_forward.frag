#version 460 core

layout(location = 0) out vec4 FragColor;

in VS_OUT
{
    vec4 Color;

#ifdef TEXCOORD_ATTRIB
    vec2 TexCoords;
#endif //TEXCOORD_ATTRIB
    
#ifdef NORMAL_ATTRIB
    vec3 Normal;
#endif //Normal

#if defined(TEXTUREINDEX_ATTRIB) || defined(INSTANCE_CUSTOM) 
    flat int TexIndex;
#endif //TEXTUREINDEX_ATTRIB

} vs_in;

#ifdef USE_TEXTURE_ARR
    uniform sampler2D texture_arr[32];
#endif //USE_TEXTURE_ARR

#ifdef SAMPLE_ALBEDO
    uniform sampler2D texture_sample_albedo;
#endif //USE_TEXTURE_ARR

void main()
{   
    vec4 texColor = vec4(1);

#ifdef USE_TEXTURE_ARR
    texColor = texture(texture_arr[vs_in.TexIndex], vs_in.TexCoords);
#endif //USE_TEXTURE_ARR
    
    vec4 finalColor = vs_in.Color * texColor.rgba;

    FragColor = finalColor;
}