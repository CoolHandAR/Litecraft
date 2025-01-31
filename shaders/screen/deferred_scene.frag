#version 460 core

#include "../lights_data.incl"
#include "../shader_commons.incl"
#include "../scene_incl.incl"

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

const float PI = 3.14159265359;
//temp hack for now, ibl stuff needs more work
const float ambient_intensity = 0.3;
/*
~~~~~~~~~~~~~~~
TEXTURES
~~~~~~~~~~~~~~~
*/
//GBUFFER
uniform sampler2D gNormalMetal;
uniform sampler2D gColorRough;
uniform sampler2D gEmissive;

uniform sampler2D depth_texture;
uniform sampler2D SSAO_texture;
uniform sampler2DArrayShadow shadowMapsDepth;

//IBL
uniform sampler2D brdfLUT;
uniform samplerCube irradianceMap;
uniform samplerCube preFilterMap;

uniform vec2 u_shadowSampleKernels[32];
uniform int u_shadowSampleAmount;
uniform float u_shadowQualityRadius;


float quick_hash(vec2 pos) 
{
	const vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
	return fract(magic.z * fract(dot(pos, magic.xy)));
}

float shadowSample(vec3 coords, float depth)
{   
    vec4 sampleCoords;
    sampleCoords.xyw = vec3(coords.xy, depth);
    sampleCoords.z = coords.z;

    float shadow = texture(shadowMapsDepth, sampleCoords);

    return shadow;
}

float sampleBlurShadowMap(vec3 coords, float depth, float blurFactor)
{
    if(u_shadowSampleAmount == 0)
    {
        return shadowSample(vec3(coords.xy, coords.z), depth);
    }

    float blurring = u_shadowQualityRadius * (blurFactor + (1.0 - blurFactor));
        
    vec2 texSize = blurring * (1.0 / textureSize(shadowMapsDepth, 0).xy);

    mat2 disk_rotation;
    {
        float r = quick_hash(gl_FragCoord.xy + 5.588238) * 2.0 * PI;
        float sr = sin(r);
        float cr = cos(r);
        disk_rotation = mat2(vec2(cr, -sr), vec2(sr, cr));
    }

    float avg = 0.0;
    for(int i = 0; i < u_shadowSampleAmount; i++)
    {
        avg += shadowSample(vec3(coords.xy + texSize * (disk_rotation * u_shadowSampleKernels[i]), coords.z), depth);  
    }
    return  avg * (1.0 / float(u_shadowSampleAmount));
}

float ShadowCalculation(vec3 fragPosViewSpace, vec3 fragPosWorldSpace, vec3 light_dir, vec3 p_normal)
{
    // select cascade layer
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < scene.shadow_split_count; ++i)
    {
        if (depthValue < scene.shadow_splits[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = scene.shadow_split_count - 1;
    }

    vec3 vertex = fragPosWorldSpace;

    vec3 flat_normal = normalize(round(p_normal));
    vec3 base_normal_bias = flat_normal * (1.0 - max(0.0, dot(light_dir, -flat_normal)));
    vertex.xyz += light_dir * scene.shadow_bias[layer];
    vec3 normal_bias = base_normal_bias * scene.shadow_normal_bias[layer];
    normal_bias -= light_dir * dot(light_dir, normal_bias);
    vertex.xyz += normal_bias;

    highp vec4 fragPosLightSpace = scene.shadow_matrixes[layer] * vec4(vertex, 1.0);

    // perform perspective divide
    highp vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xyz = projCoords.xyz * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 1.0;
    }
    
    float blurFactor = scene.shadow_blur_factors[layer];

    // PCF
    float shadow = sampleBlurShadowMap(vec3(projCoords.xy, layer), currentDepth, blurFactor);
    
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

    const float MAX_REFLECTION_LOD = 8.0;
    vec3 prefilteredColor = textureLod(preFilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(normal, view_dir), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (kS * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);

    return ambient;
}

float calc_light_attenuation(float distance, float inv_range, float decay) 
{
	float nd = distance * inv_range;
	nd *= nd;
	nd *= nd; // nd^4
	nd = max(1.0 - nd, 0.0);
	nd *= nd; // nd^2
	return nd * pow(max(distance, 0.0001), -decay);
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

void main()
{
    float depth = texture(depth_texture, TexCoords).r;

    //Discard if depth is 1.0
    //Fixes shading the sky and skip unneeded shading
    if(depth >= 1.0)
    {
        discard;
    }

    //GBUFFER DATA
    vec4 NormalMetal = texture(gNormalMetal, TexCoords);
    vec4 ColorRough = texture(gColorRough, TexCoords);
    float Emission = texture(gEmissive, TexCoords).r;

    //COMMONS 
    vec3 ViewPos = depthToViewPosDirect(depth, cam.invProj, TexCoords);
    vec3 WorldPos = (cam.invView * vec4(ViewPos, 1.0)).xyz;
    vec3 Normal = normalize(NormalMetal.rgb * 2.0 - 1.0); //convert to [-1, 1] range
    vec3 Albedo = ColorRough.rgb;
    float Metallic = NormalMetal.a;
    float Roughness = ColorRough.a;

    float AO = 1.0;

#ifdef USE_SSAO
    AO = texture(SSAO_texture, TexCoords).r;
#endif

    vec3 F0 = F0calc(Metallic, 0.5, Albedo);
    vec3 ViewDir = normalize(cam.position.xyz - WorldPos);

    float shadow = 1.0;

#ifdef USE_DIR_SHADOWS
    shadow = ShadowCalculation(ViewPos, WorldPos, normalize(scene.dirLightDirection.xyz), Normal);
#endif

    vec3 Lighting = vec3(0.0);

    //CALCULATE DIR LIGHT
    Lighting = shadow * calculate_light(scene.dirLightColor.rgb * scene.dirLightAmbientIntensity, normalize(scene.dirLightDirection.xyz), Normal, ViewDir, Albedo, 1.0, scene.dirLightSpecularIntensity, Roughness, Metallic, F0);
    
    //CALCULATE POINT LIGHTS
    for(int i = 0; i < scene.numPointLights; i++)
    {
#define pLight pLights.data[i]

        float inv_radius = 1.0 / pLight.radius;
        vec3 LW = pLight.position.xyz - WorldPos.xyz;
        float dist = length(LW);
        float normalized_dist = dist * inv_radius;

        float attenuation = calc_light_attenuation(dist, 1.0 / pLight.radius, pLight.attenuation);
            
        if(normalized_dist < 1.0)
        {       
            Lighting += calculate_light(pLight.color.rgb * pLight.ambient_intensity, normalize(LW), Normal, ViewDir, Albedo, attenuation, pLight.specular_intensity, Roughness,
                Metallic, F0);   
        }
    }
    //CALCULATE SPOT LIGHTS
    for(int i = 0; i < scene.numSpotLights; i++)
    {
#define sLight sLights.data[i]

        float inv_radius = 1.0 / sLight.range;
        vec3 LW = sLight.position.xyz - WorldPos.xyz;
        vec3 LDIR = normalize(LW);
        float dist = length(LW);
        float normalized_dist = dist * inv_radius;

        if(normalized_dist < 1.0)
        {
            float spot_cuffoff = cos(radians(sLight.angle));
            float scos = max(dot(-LDIR, sLight.direction.xyz), spot_cuffoff);
      
            float spot_attenuation = calc_light_attenuation(dist, inv_radius, sLight.attenuation);
        
            float spot_rim = max(0.0001, (1.0 - scos) / (1.0 - spot_cuffoff));
            spot_attenuation *= 1.0 - pow(spot_rim, sLight.angle_attenuation);

            Lighting += calculate_light(sLight.color.rgb * sLight.ambient_intensity, LDIR, Normal, ViewDir, Albedo, spot_attenuation, sLight.specular_intensity, Roughness,
             Metallic, F0);
        }               
    }

    //CALCULATE AMBIENT LIGHT
    vec3 ambient = IBL_Calc(Metallic, Roughness, Normal, ViewDir, Albedo, F0) * ambient_intensity;

    //CALCULATE FINAL COLOR
    Lighting = (ambient + Lighting + Emission) * AO;

    //FINAL COLOR
    FragColor = vec4(Lighting, 1.0);
}