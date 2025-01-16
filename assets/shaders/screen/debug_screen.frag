#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D NormalMetalTexture;
uniform sampler2D AlbedoRoughTexture;
uniform sampler2D DepthTexture;
uniform sampler2D AOTexture;
uniform sampler2D BloomTexture;

uniform int u_selection;

void main()
{
    //-1 disabled, 0 = Normal, 1 = Albedo, 2 = Depth, 3 = Metal, 4 = Rough, 5 = AO, 6 = BLOOM
    
    vec3 finalColor = vec3(0.0);

    switch(u_selection)
    {
        //do nothing
        case -1:
        {
            finalColor = vec3(1, 1, 1);
            break;
        }
        //Normal
        case 0:
        {
            finalColor = texture(NormalMetalTexture, TexCoords).rgb;
            break;
        }
        //Albedo
        case 1:
        {
            finalColor = texture(AlbedoRoughTexture, TexCoords).rgb;
            break;
        }
        //Depth
        case 2:
        {
            float depth = texture(DepthTexture, TexCoords).r;
            float z = depth * 2.0 - 1.0;
            float linear_depth = (2.0 * 0.1 * 1500.0) / (1500.0 + 0.1 - z * (1500.0 - 0.1)) / 1500.0;
            finalColor = vec3(linear_depth);
            break;
        }
        //Metal
        case 3:
        {
            float metal = texture(NormalMetalTexture, TexCoords).a;
            finalColor = vec3(metal);
            break;
        }
        //Rough
        case 4:
        {
            float rough = texture(AlbedoRoughTexture, TexCoords).a;
            finalColor = vec3(rough);
            break;
        }
        //AO
        case 5:
        {
            float AO = texture(AOTexture, TexCoords).r;
            finalColor = vec3(AO);
            break;
        }
        //Bloom
        case 6:
        {
            finalColor = texture(BloomTexture, TexCoords).rgb;
            break;
        }
        default:
        {
            discard;
            break;
        }
    }

    FragColor = vec4(finalColor.rgb, 1.0);
}