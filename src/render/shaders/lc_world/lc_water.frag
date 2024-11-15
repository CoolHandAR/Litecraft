#version 460 core 

#include "../scene_incl.incl"
#include "../shader_commons.incl"

layout (location = 0) out vec4 FragColor;

in VS_OUT
{
    vec2 TexCoords;
    vec4 ClipPos;
    vec3 WorldPos;
} vs_in;

/*
~~~~~~~~~~~~~~~
TEXTURES
~~~~~~~~~~~~~~~
*/
uniform samplerCube skybox_texture;
uniform sampler2D dudv_map;
uniform sampler2D normal_map;

uniform float u_moveFactor;

void main()
{
    //Fixes a bug when w falls out of range
    if(vs_in.ClipPos.w < cam.z_near)
    {
        discard;
    }

    //Get distorted coords from the dudv map
    vec2 distortionCoords = texture(dudv_map, vec2(vs_in.TexCoords.x + u_moveFactor, vs_in.TexCoords.y)).rg * 0.2;
    distortionCoords = vs_in.TexCoords + vec2(distortionCoords.x, distortionCoords.y + u_moveFactor);
    vec2 totalDistortionCoords = (texture(dudv_map, distortionCoords).rg * 2.0 - 1.0) * 0.4;
    
    //sample normal
    vec3 normal = texture(normal_map, distortionCoords).rgb;
    normal = normalize(vec3(normal.r * 2.0 - 1.0, normal.b * 3.0, normal.g * 2.0 - 1.0));
    
    vec3 viewDir = normalize(cam.position.xyz - vs_in.WorldPos.xyz);

    //Calculate light
    vec3 reflectedLight = reflect(normalize(-scene.dirLightDirection.xyz), normal);
    float specular = max(dot(reflectedLight, viewDir), 0.0);
    specular = pow(specular, 0.4);
    vec3 specularHighlights = (scene.dirLightColor.rgb * scene.dirLightAmbientIntensity * 0.5) * specular * 0.1;

    vec3 r = reflect(viewDir, normal);
    vec3 reflectedColor = texture(skybox_texture, -r).rgb;

    vec3 blue_color = vec3(0, 0.2, 1.0);

    reflectedColor = mix(blue_color, reflectedColor, 0.4) + specularHighlights;

   
    FragColor = vec4(reflectedColor, 0.85);
}


