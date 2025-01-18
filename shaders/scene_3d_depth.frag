#version 460 core

#ifdef SEMI_TRANSPARENT_PASS
in VS_OUT
{
#ifdef TEXCOORD_ATTRIB
    vec2 TexCoords;
#endif //TEXCOORD_ATTRIB
#if defined(TEXTUREINDEX_ATTRIB) || defined(INSTANCE_CUSTOM) 
    flat int TexIndex;
#endif //TEXTUREINDEX_ATTRIB
} vs_in;

#ifdef USE_TEXTURE_ARR
    uniform sampler2D texture_arr[32];
#endif //USE_TEXTURE_ARR
#endif


void main()
{   
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

}