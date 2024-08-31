#version 460 core

#include "../lights_data.incl"
#include "../shader_commons.incl"
#include "../scene_incl.incl"

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

const float PI = 3.14159265359;

/*
~~~~~~~~~~~~~~~
TEXTURES
~~~~~~~~~~~~~~~
*/
//GBUFFER
uniform sampler2D gNormalMetal;
uniform sampler2D gColorRough;

uniform sampler2D depth_texture;
uniform sampler2D SSAO_texture;
uniform sampler2DArray shadowMaps;

//IBL
uniform sampler2D brdfLUT;
uniform samplerCube irradianceMap;
uniform samplerCube preFilterMap;


float when_lessThan(float x, float y) 
{
  return max(sign(y - x), 0.0);
}

float when_greaterThan(float x, float y) 
{
  return 1.0 - when_lessThan(x, y);
}

float ShadowCalculation(vec3 fragPosViewSpace, vec3 fragPosWorldSpace, vec3 light_dir, vec3 p_normal)
{
    // select cascade layer
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < SHADOW_CASCADE_COUNT; ++i)
    {
        if (depthValue < scene.shadow_splits[i])
        {
            layer = i;
            break;
        }
    }
    layer = 0;
    if (layer == -1)
    {
        layer = SHADOW_CASCADE_COUNT;
    }

    vec4 fragPosLightSpace = scene.shadow_matrixes[layer] * vec4(fragPosWorldSpace, 1.0);

    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xyz = projCoords.xyz * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    

    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(p_normal);
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
    const float biasModifier = 0.5f;
    if (layer == SHADOW_CASCADE_COUNT)
    {
        bias *= 1 / (cam.z_far * biasModifier);
    }
    else
    {
        bias *= 1 / (scene.shadow_splits[layer] * biasModifier);
    }

    float t = mod(1, 1);
    int i1 = int(mod(gl_FragCoord.x, 4.0));
    int i2 = int(mod(gl_FragCoord.y, 4.0));
    //float dither_value = ditherPattern[i1][i2];
    //bias * dither_value;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps, 0));
    //texelSize *= (0.05 * (layer + 2)); //fixes noise issues
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMaps, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
        
    return shadow;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 fresnelSchlickRougness(float cosTheta, vec3 F0, float roughness)
{
     return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 F0calc(float metallic, float specular, vec3 albedo)
{
    float dielectric = 0.16 * specular * specular;
    return mix(vec3(dielectric), albedo, vec3(metallic));
}

vec3 IBL_Calc(float metallic, float roughness, vec3 normal, vec3 view_dir, vec3 albedo, vec3 F0)
{
    vec3 kS = fresnelSchlickRougness(max(dot(normal, view_dir), 0.0), F0, roughness);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 R = reflect(-view_dir, normal); 

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(preFilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(normal, view_dir), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (kS * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);

    return ambient;
}
vec3 calculate_light(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, vec3 albedo, float attenuation, float specular_intensity, float roughness, float metallic, vec3 F0)
{   
    vec3 halfway_dir = normalize(view_dir + light_dir);
    float NdotV = max(dot(normal, view_dir), 0.000);
    float NdotL = max(dot(normal, light_dir), 0.000);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfway_dir, roughness);   
    float G   = GeometrySmith(NdotV, NdotL, roughness);      
    vec3 F    = fresnelSchlick(max(dot(halfway_dir, view_dir), 0.00), F0);

    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / max (denominator, 0.0001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    vec3 radiance = light_color * attenuation;
    kD *= 1.0 - metallic;	  

    vec3 value = (kD * albedo / PI + specular) * radiance * NdotL;

    return value;
}
const float specular = 0.5;
void main()
{
    //GBUFFER DATA
    vec4 NormalMetal = texture(gNormalMetal, TexCoords);
    vec4 ColorRough = texture(gColorRough, TexCoords);

    //COMMONS
    vec3 ViewPos = depthToViewPos(depth_texture, cam.invProj, TexCoords);
    vec3 WorldPos = (cam.invView * vec4(ViewPos, 1.0)).xyz;
    vec3 Normal = NormalMetal.rgb * 2.0 - 1.0; //convert to [-1, 1] range
    vec3 Albedo = ColorRough.rgb;
    float Metallic = NormalMetal.a;
    float Roughness = ColorRough.a;
    float AO = texture(SSAO_texture, TexCoords).r;

    vec3 F0 = F0calc(Metallic, specular, Albedo);
    vec3 ViewDir = normalize(cam.position.xyz - WorldPos);

    
    float shadow = ShadowCalculation(ViewPos, WorldPos, normalize(scene.dirLightDirection.xyz), Normal);

    vec3 Lighting = vec3(0.0);
    //CALCULATE DIR LIGHT
    Lighting = calculate_light(scene.dirLightColor.rgb * 1, normalize(scene.dirLightDirection.xyz), Normal, ViewDir, Albedo, 1.0, scene.dirLightSpecularIntensity, Roughness, Metallic, F0);


    //CALCULATE POINT LIGHTS
    for(int i = 0; i < scene.numPointLights; i++)
    {
#define pLight pLights.data[i]

        vec3 LW = pLight.position.xyz - WorldPos.xyz;
        float dist = length(LW);

        if(dist < pLight.radius)
        {
            float attenuation = 1.0 / (pLight.constant + pLight.linear * dist + pLight.quadratic * (dist * dist));
            Lighting += calculate_light(pLight.color.rgb * 12, normalize(LW), Normal, ViewDir, Albedo, attenuation, pLight.specular_intensity, Roughness,
            Metallic, F0);
        }
    }

    //CALCULATE AMBIENT LIGHT
    vec3 Ambient = IBL_Calc(Metallic, Roughness, Normal, ViewDir, Albedo, F0);

    //CALCULATE FINAL COLOR
    Lighting = (Ambient + Lighting) * AO;
    
    //FINAL COLOR
    FragColor = vec4(Lighting, 1.0);
}