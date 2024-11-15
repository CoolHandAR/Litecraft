#version 460 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BloomThresholdColor;

in vec2 v_texCoords;
#define SHADOW_CASCADE_COUNT 5
#define LIGHT_SPACE_MATRIXES_COUNT 5
struct DirLight 
{
    vec3 direction;
	
    vec3 color;
    float ambient_intensity;
    float specular_intensity;
};
struct PointLight
{
    vec4 position;

    vec4 color;
    float ambient_intensity;
    float specular_intensity;

    float linear;
    float quadratic;
    float radius;
    float constant;
};
struct SpotLight
{
    vec3 position;
    vec3 direction;

    vec3 color;
    float ambient_intensity;
    float specular_intensity;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
};

struct LightGrid
{
    uint offset;
    uint count;
};

layout(std430, binding = 3) readonly restrict buffer PointLights
{
    PointLight data[];
} pLights;

layout (std430, binding = 4) readonly restrict buffer lightIndexSSBO
{
    uint globalLightIndexList[];
};

layout (std430, binding = 5) readonly restrict buffer lightGridSSBO
{
    LightGrid lightGrid[];
};

layout (std140, binding = 4) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[LIGHT_SPACE_MATRIXES_COUNT];
};

#define GAMMA 2.2

uniform sampler2D depth_texture;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;
uniform sampler2DArray shadowMap;
uniform samplerCube irradianceMap;
uniform samplerCube preFilterMap;
uniform sampler2D brdfLUT;

uniform mat4 u_view;
uniform mat4 u_InverseProj;
uniform mat4 u_InverseView;
uniform DirLight u_dirLight;
uniform PointLight u_pointLights[32];
uniform int u_pointLightCount;
uniform vec3 u_viewPos;
uniform float u_delta;
uniform float u_cascadePlaneDistances[SHADOW_CASCADE_COUNT];

const float PI = 3.14159265359;

vec3 Uncharted2ToneMapping(vec3 color)
{
  float A = 0.22;//0.15;
  float B = 0.30;//0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.01;//0.02;
  float F = 0.30;//0.30;
  float W = 11.2;
  float exposure = 2.;
  color *= exposure;
  color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
  float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
  color /= white;
  color = pow(color, vec3(1.0 / GAMMA));
  return color;
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
vec3 IBL_LightingCalc(float metallic, float roughness, vec3 normal, vec3 view_dir, vec3 albedo, vec3 F0)
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


float ShadowCalculation(vec3 fragPosViewSpace, vec3 fragPosWorldSpace, vec3 light_dir, vec3 p_normal)
{
    // select cascade layer
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < SHADOW_CASCADE_COUNT; ++i)
    {
        if (depthValue < u_cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = SHADOW_CASCADE_COUNT;
    }
    //layer = 3;

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);

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
        bias *= 1 / (1500 * biasModifier);
    }
    else
    {
        bias *= 1 / (u_cascadePlaneDistances[layer] * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    texelSize *= (0.05 * (layer + 2)); //fixes noise issues
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
        
    return shadow;
}

vec3 calcViewPos(vec2 coords)
{
    float depth = textureLod(depth_texture, coords, 0).r;

    vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_space = u_InverseProj * ndc;

    view_space.xyz = view_space.xyz / view_space.w;

    return view_space.xyz;
}

vec3 normalFromViewPos(vec3 viewPos)
{
    return normalize(cross(dFdx(viewPos), dFdy(viewPos)));
}
vec3 calculate_lightPBR(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, vec3 albedo, float attenuation, float specular_intensity, float roughness, float metallic, vec3 F0)
{   
    vec3 halfway_dir = normalize(view_dir + light_dir);
    float NdotV = max(dot(normal, view_dir), 0.001);
    float NdotL = max(dot(normal, light_dir), 0.001);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfway_dir, roughness);   
    float G   = GeometrySmith(NdotV, NdotL, roughness);      
    vec3 F    = fresnelSchlick(max(dot(halfway_dir, view_dir), 0.001), F0);

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

vec3 calculate_light(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, float specular_intensity, float shadow_value, float attenuation, float AO)
{
    //AMBIENT
    vec3 ambient = light_color * AO;
    //DIFFUSE
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light_color * diffuse_factor;
    //SPECULAR
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 64);
    vec3 specular = light_color * specular_factor * specular_intensity;

    //APPLY ATTENUATION
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + (1.0 - shadow_value) * (diffuse + specular));
}


ivec4 tileSizes = ivec4(16, 9, 24, ceil(1280.0 / 16.0));

float u_zNear = 0.1;
float u_zFar = 500;
uint getDepthSlice(float depth)
{
    uint num_slices = 16 * 9 * 24;

    return int(floor(log(depth) * (num_slices / log(u_zFar / u_zNear)) - (num_slices * log(u_zNear) / log(u_zFar / u_zNear))));
}

uint getClusterIndex(uint tileSizeInPx, vec3 pixelCoord)
{
    uint clusterZVal  = getDepthSlice(pixelCoord.z);

    uvec3 clusters    = uvec3( uvec2( pixelCoord.xy / tileSizeInPx), clusterZVal);
        uint clusterIndex = clusters.x +
                            tileSizes.x * clusters.y +
                            (tileSizes.x * tileSizes.y) * clusters.z;
        return clusterIndex;
}

const vec3 coeff = vec3(0.799,0.587,0.414);

const float specular = 0.5;
void main()
{             
    //retrieve data from gbuffer
    vec4 NormalMetal = texture(gNormal, v_texCoords);
    vec4 ColorRough = texture(gAlbedo, v_texCoords);

    vec3 ViewFragPos = calcViewPos(v_texCoords);
    vec3 FragPos = vec3(u_InverseView * vec4(ViewFragPos, 1.0));
    vec3 Normal = NormalMetal.rgb * 2.0 - 1.0;
    vec3 Diffuse = ColorRough.rgb;
    float AO = texture(ssao, v_texCoords).r;
    float Metallic = NormalMetal.a;
    float Roughness = ColorRough.a;

    vec3 F0 = F0calc(Metallic, specular, Diffuse);

    vec3 view_dir = normalize(u_viewPos - FragPos); // viewpos is (0.0.0)

    vec3 lighting = vec3(0.0);
    float shadow = ShadowCalculation(ViewFragPos, FragPos, normalize(u_dirLight.direction), Normal);


    //(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, vec3 albedo, float attenuation, float specular_intensity, float roughness, float metallic, vec3 F0)
    //DIRLIGHT CALC
    lighting = (1.0 - shadow) * calculate_lightPBR(u_dirLight.color * 8, normalize(u_dirLight.direction), Normal, view_dir, Diffuse, 1, u_dirLight.specular_intensity, Roughness
    ,Metallic, F0);
    
    //lighting = calculate_light(u_dirLight.color * u_dirLight.ambient_intensity, normalize(u_dirLight.direction), Normal, view_dir, u_dirLight.specular_intensity, shadow, 1, AO);

    uint clusterIndex = getClusterIndex(tileSizes[3], ViewFragPos );

    uint lightIndexOffset = lightGrid[clusterIndex].offset;
    uint lightCount = lightGrid[clusterIndex].count;


    
    //POINT LIGHT CALC
    for(int i = 0; i < lightCount; i++)
    {
        uint lightVectorIndex = globalLightIndexList[lightIndexOffset + i];
#define pLight pLights.data[lightVectorIndex]
        float dist = length(pLight.position.xyz - FragPos);
        if(dist < pLight.radius)
        {
            //smoothstep(-1, 1, sin((u_delta * 2)))
            float attenuation = 1.0 / (pLight.constant + pLight.linear * dist + pLight.quadratic * (dist * dist)); 
            lighting += calculate_lightPBR(pLight.color.rgb * 12,  normalize(pLight.position.xyz - FragPos), Normal, view_dir, Diffuse, attenuation, pLight.specular_intensity, Roughness, 
            Metallic, F0);
             //lighting += calculate_light(pLight.color.rgb * pLight.ambient_intensity,  normalize(pLight.position.xyz - FragPos), Normal, view_dir, pLight.specular_intensity, 1.0, attenuation, 1);
        }
    }

   // vec3 IBL_LightingCalc(float metallic, float roughness, vec3 normal, vec3 view_dir, vec3 albedo, vec3 F0)

    vec3 ambient = IBL_LightingCalc(Metallic, Roughness, Normal, view_dir, Diffuse, F0);

    //ambient *= AO;
    
    //apply color
    lighting = ambient + lighting * AO;
    //lighting *= Diffuse;

     float brightness = dot(lighting, vec3(0.2126, 0.7152, 0.0722));

     if(brightness > 1.0)
        BloomThresholdColor = vec4(lighting, 1.0);
    else
        BloomThresholdColor = vec4(0.0, 0.0, 0.0, 1.0);

    //lighting =  lighting / ( lighting + vec3(1.0));
    // gamma correct
   // lighting = pow(lighting, vec3(1.0/2.2)); 



    float exposure = 0.1;
    lighting = Uncharted2ToneMapping(exposure * lighting);  

    float lum = dot(lighting, coeff);
    vec3 mask = (lighting - vec3(lum));
    mask = clamp(mask, 0.0, 1.0);
    float lumMask = dot(coeff, mask);
    lumMask = 1.0 - lumMask;

    //lighting = mix(vec3(lum), lighting, 1.0 + 0.1 * lumMask);

   
        

    FragColor = vec4(lighting, 1.0);
}