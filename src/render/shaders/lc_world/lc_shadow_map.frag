#version 460 core

layout(location = 0) out float FragColor;

#ifdef TRANSPARENT_SHADOWS
in vec2 TexCoords;
in vec2 TexOffset;

uniform sampler2D texture_atlas;
#endif

void main()
{   
#ifdef TRANSPARENT_SHADOWS
     vec2 texCoords = fract(TexCoords);

    //Offset the coords
    texCoords = vec2((texCoords.x + TexOffset.x) / 25, -(texCoords.y + TexOffset.y) / 25);

    float alpha = texture(texture_atlas, texCoords).a;

    if(alpha < 0.5)
    {
        discard;
    }
#endif

    float depth = gl_FragCoord.z;

    float dx = dFdx(depth);
    float dy = dFdy(depth);
    float moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);

    FragColor = moment2;
}