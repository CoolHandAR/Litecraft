#version 460 core 

#include "../scene_incl.incl"
#include "../shader_commons.incl"

layout (location = 0) out vec4 FragColor;

in VS_OUT
{
    vec4 ClipPos;
    vec3 WorldPos;
    vec2 TexCoords;
} vs_in;


const float fresnelReflective = 0.5;
const float edgeSoftness = 1;
const float minBlueness = 0.5;
const float maxBlueness = 0.8;
const float murkyDepth = 4;
const float specularReflectivity = 0.4;
/*
~~~~~~~~~~~~~~~
TEXTURES
~~~~~~~~~~~~~~~
*/
uniform samplerCube skybox_texture;
uniform sampler2D dudv_map;
uniform sampler2D normal_map;
uniform sampler2D reflection_texture;
uniform sampler2D refraction_texture;
uniform sampler2D refraction_depth;
uniform sampler2D gradient_map;

uniform float u_moveFactor;


vec3 sample_gradient_map(float depth)
{   
    float inv_depth = clamp((1.0 - depth), 0.1, 0.99);
    vec3 gradient_color = texture(gradient_map, vec2(inv_depth, 0)).rgb;
 
    return gradient_color;
}

void main()
{
    //Fixes a bug when w falls out of range
    if(vs_in.ClipPos.w < cam.z_near)
    {
       discard;
    }

    vec2 ndcCoords = (vec2(-vs_in.ClipPos.x, vs_in.ClipPos.y) / vs_in.ClipPos.w) * 0.5 + 0.5;
    vec2 ndcCoords2 = (vec2(vs_in.ClipPos.x, vs_in.ClipPos.y) / vs_in.ClipPos.w) * 0.5 + 0.5;

    ndcCoords = clamp(ndcCoords, 0.002, 0.998);
    ndcCoords2 = clamp(ndcCoords2, 0.002, 0.998);

    vec2 reflectionCoords = vec2(ndcCoords.x, ndcCoords.y);
    vec2 refractionCoords = vec2(ndcCoords2.x, ndcCoords2.y);

    float refraction_depth = texture(refraction_depth, refractionCoords).r;
    float floor_distance = linearizeDepth(refraction_depth, cam.z_near, cam.z_far);
    float water_distance = linearizeDepth(gl_FragCoord.z, cam.z_near, cam.z_far);

    if(water_distance > floor_distance)
    {
        discard;
    }

    float water_depth = floor_distance - water_distance;

    //get distortion coords
    vec2 distortionCoords = texture(dudv_map, vec2(vs_in.TexCoords.x + u_moveFactor * 0.01, vs_in.TexCoords.y)).rg;
    distortionCoords = distortionCoords + vec2(distortionCoords.x, distortionCoords.y + u_moveFactor * 0.1);
    vec2 totalDistortion = (texture(dudv_map, distortionCoords).rg * 2.0 - 1.0) * 0.01;

    reflectionCoords += totalDistortion;
    reflectionCoords = clamp(reflectionCoords, 0.002, 0.998);

    refractionCoords += totalDistortion;
    refractionCoords = clamp(refractionCoords, 0.0002, 0.998);
    
    //sample from scene
    vec4 reflection_color = texture(reflection_texture, reflectionCoords);
    vec4 refraction_color = texture(refraction_texture, refractionCoords);

    vec3 normalColor = texture(normal_map, distortionCoords).rgb;
    vec3 normal = normalize(vec3(normalColor.r * 2.0 - 1.0, normalColor.b * 3.0, normalColor.g * 2.0 - 1.0));

    vec3 viewDir = normalize(cam.position.xyz - vs_in.WorldPos.xyz);
    float refractiveFactor = dot(viewDir, normal);
   // refractiveFactor = pow(refractiveFactor, 0.5);
    refractiveFactor = clamp(refractiveFactor, 0.0, 1.0);

    float clamp_depth = clamp(1.0 / water_depth, 0.0, 1.0);
    float step_alpha = smoothstep(1.0, 0.0, clamp_depth);
    float alpha = clamp(step_alpha, 0.0, 1.0);

    vec3 water_color = sample_gradient_map(alpha);
    float murkFactor = clamp(water_depth / murkyDepth, 0.0, 1.0);
    refraction_color.rgb = mix(refraction_color.rgb, water_color, murkFactor);

    vec3 r = reflect(viewDir, normal);
    vec3 sky_color = texture(skybox_texture, -r).rgb;

    reflection_color.rgb = mix(reflection_color.rgb, sky_color.rgb, refractiveFactor);

    vec3 world_color = mix(reflection_color.rgb, refraction_color.rgb, refractiveFactor);

    vec3 finalColor = world_color;
    

    FragColor = vec4(world_color.rgb, 1);
}


