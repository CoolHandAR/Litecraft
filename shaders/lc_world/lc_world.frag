#version 460 core

#include "../scene_incl.incl"

#ifdef GBUFFER_PASS
layout(location = 0) out vec4 g_normalMetal;
layout(location = 1) out vec4 g_colorRough;
layout(location = 2) out float g_emissive;
#endif

#ifdef FORWARD_PASS
layout(location = 0) out vec4 FragColor;
#endif

#ifdef USE_TEXCOORDS
in vec2 out_TexCoords;
in vec2 out_TexOffset;
#endif

#ifdef USE_TBN_MATRIX
in mat3 out_TBN;
#endif

#ifdef FORWARD_PASS
in vec3 out_Normal;
in vec3 out_WorldPos;
#endif

const float PI = 3.14159265359;
//temp hack for now, ibl stuff needs more work
const float ambient_intensity = 0.1;

uniform sampler2D texture_atlas;
uniform sampler2D texture_atlas_normal;
uniform sampler2D texture_atlas_mer;
uniform samplerCube irradianceMap;
uniform samplerCube preFilterMap;
uniform sampler2D brdfLUT;

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

void main()
{
#ifdef USE_TEXCOORDS
    vec2 texCoords = fract(out_TexCoords);
    //Offset the coords
    texCoords = vec2((texCoords.x + out_TexOffset.x) / 25, -(texCoords.y + out_TexOffset.y) / 25);
#endif

#ifdef GBUFFER_PASS
   vec4 AlbedoColor = texture(texture_atlas, texCoords);
   vec3 MerColor = texture(texture_atlas_mer, texCoords).rgb;
   vec3 NormalColor = texture(texture_atlas_normal, texCoords).rgb * 2.0 - 1.0;

   g_normalMetal.rgb = normalize(out_TBN * NormalColor) * 0.5 + 0.5; //Normal. We convert the normal value to [0, 1] range so that we can store in a unsigned texture format
   g_normalMetal.a = MerColor.r; //Metal

   g_colorRough.rgb = AlbedoColor.rgb; //Color
   g_colorRough.a = MerColor.b; //Rough

   g_emissive = MerColor.g; //Emissive
#endif

#ifdef DEPTH_PASS
#ifdef SEMI_TRANSPARENT
    vec4 AlbedoColor = texture(texture_atlas, texCoords);
    if (AlbedoColor.a < 0.5)
    {
        discard;
    }
#endif
#endif
    
#ifdef FORWARD_PASS
    vec4 AlbedoColor = texture(texture_atlas, texCoords);
#ifdef SEMI_TRANSPARENT
    if (AlbedoColor.a < 0.5)
    {
        discard;
    }
#endif
     vec3 MerColor = texture(texture_atlas_mer, texCoords).rgb;

     vec3 F0 = F0calc(MerColor.r, 0.5, AlbedoColor.rgb);

     vec3 Normal = normalize(out_Normal);
     vec3 ViewDir = normalize(cam.position.xyz - out_WorldPos.xyz);

     vec3 Lighting = calculate_light(scene.dirLightColor.rgb * scene.dirLightAmbientIntensity, normalize(scene.dirLightDirection.xyz), Normal, ViewDir, AlbedoColor.rgb, 1.0, scene.dirLightSpecularIntensity, MerColor.b, MerColor.r, F0);

     vec3 Ambient = IBL_Calc(MerColor.r, MerColor.b, Normal, ViewDir, AlbedoColor.rgb, F0) * ambient_intensity;

     Lighting = Ambient + Lighting;

     FragColor = vec4(Lighting, 1.0);
#endif
}