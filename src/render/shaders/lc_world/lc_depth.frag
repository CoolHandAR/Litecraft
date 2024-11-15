#version 460 core

#ifdef SEMI_TRANSPARENT
in vec2 TexCoords;
in vec2 TexOffset;

uniform sampler2D texture_atlas;
#endif

void main()
{   
#ifdef SEMI_TRANSPARENT
     vec2 texCoords = fract(TexCoords);

    //Offset the coords
    texCoords = vec2((texCoords.x + TexOffset.x) / 25, -(texCoords.y + TexOffset.y) / 25);

    float alpha = texture(texture_atlas, texCoords).a;

    if(alpha < 0.5)
    {
        discard;
    }
#endif
}