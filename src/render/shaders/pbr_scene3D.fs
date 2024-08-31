#version 460 core
#extension GL_ARB_bindless_texture : require

#define SHADOW_CASCADE_COUNT 4
#define LIGHT_SPACE_MATRIXES_COUNT 5

out vec4 f_color;

in VS_Out
{
    vec2 tex_coords;
    vec3 world_pos;
    vec3 normal;
    vec3 frag_pos;
    vec3 tangent_view_pos;
    vec3 tangent_frag_pos;
    vec3 tangent_light_pos;
    uint material_index;
} fs_in;

struct Material
{
    vec3 color;

    uint base_color_texture;
    uint roughness_texture;
    uint metallness_texture;
    uint normal_texture;
    uint ao_texture;

    float vector_color_ratio;
    float roughness_ratio;
    float metallness_ratio;
    float ao_ratio;
};
struct DirLight 
{
    vec3 direction;
	
    vec3 color;
    float ambient_intensity;
    float specular_intensity;
};
struct PointLight
{
    vec3 position;

    vec3 color;
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

layout (std140, binding = 0) uniform CameraData
{
	mat4 viewProjectionMatrix;
    mat4 view;
    vec4 frustrum_planes[6];
    vec3 position;
    ivec2 screen_size;

    float z_near;
    float z_far;
} camera;
layout (std140, binding = 1) uniform SceneData
{
	DirLight dir_light;
    PointLight pointLights[12];
    SpotLight spotLights[12];

    sampler2DArray shadow_maps;
    float shadowCascadePlaneDistances[SHADOW_CASCADE_COUNT];
    mat4 lightSpaceMatrices[LIGHT_SPACE_MATRIXES_COUNT];
} scene;
layout (std140, binding = 2) uniform Textures
{
	sampler2D data[12];
} textures;
layout (std140, binding = 3) uniform Materials
{
	Material data[11];
} materials;

float CascadingShadowCalculation(vec3 fragPosWorldSpace, vec3 light_dir)
{
    // select cascade layer
    vec4 fragPosViewSpace = camera.view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < SHADOW_CASCADE_COUNT; ++i)
    {
        if (depthValue < scene.shadowCascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = SHADOW_CASCADE_COUNT;
    }

    vec4 fragPosLightSpace = scene.lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);

    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fs_in.normal);
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
    const float biasModifier = 0.5f;
    if (layer == SHADOW_CASCADE_COUNT)
    {
        bias *= 1 / (camera.z_far * biasModifier);
    }
    else
    {
        bias *= 1 / (scene.shadowCascadePlaneDistances[layer] * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(scene.shadow_maps, 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(scene.shadow_maps, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
        
    return shadow;
}

const float PI = 3.14159265359;
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
vec3 F0calc(float metallic, float specular, vec3 albedo)
{
    float dielectric = 0.16 * specular * specular;
    return mix(vec3(dielectric), albedo, vec3(metallic));
}

vec3 calculate_light(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, vec3 albedo, float attenuation, float specular_intensity, float roughness, float metallic, float shadow, vec3 F0)
{   
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float NdotV = max(dot(normal, view_dir), 0.0);
    float NdotL = max(dot(normal, light_dir), 0.0);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfway_dir, roughness);   
    float G   = GeometrySmith(NdotV, NdotL, roughness);      
    vec3 F    = fresnelSchlick(max(dot(halfway_dir, view_dir), 0.0), F0);

    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / max (denominator, 0.0001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    specular *= specular_intensity;

    vec3 value = (kD * albedo / PI + specular) * light_color * NdotL;

    value *= (1.0 - shadow);

    return value;
}

const float specular = 0.5;
void main()
{   
    //COMMON
    float shadow_value = CascadingShadowCalculation(fs_in.world_pos, scene.dir_light.direction);
    Material material = materials.data[fs_in.material_index];
    vec3 view_dir = normalize(camera.position - fs_in.world_pos);
    vec3 albedo = vec3(1.0);
    float metallic = texture(textures.data[material.metallness_texture], fs_in.tex_coords).r * material.metallness_ratio;
    float roughness = texture(textures.data[material.roughness_texture], fs_in.tex_coords).r * material.roughness_ratio;
    float ao = texture(textures.data[material.ao_texture], fs_in.tex_coords).r * material.ao_ratio;
    vec3 normal = vec3(1.0);
    vec3 F0 = F0calc(metallic, specular, albedo); 

    vec3 lighting = vec3(0.0);
    //Calculate directional light
#define DIR_LIGHT scene.dir_light
    lighting += calculate_light(DIR_LIGHT.color * DIR_LIGHT.ambient_intensity, DIR_LIGHT.direction, normal, view_dir, albedo, 1, DIR_LIGHT.specular_intensity ,roughness, metallic, shadow_value, F0);
    //Calculate point lights
    for(int i = 0; i < 12; ++i)
    {
        PointLight pLight = scene.pointLights[i];

        float dist = (length(pLight.position - fs_in.frag_pos) / max(pLight.radius, 0.00001));
        float attenuation = 1.0 / (pLight.constant + pLight.linear * dist + pLight.quadratic * (dist * dist)); 
        lighting += calculate_light(pLight.color * pLight.ambient_intensity,  normalize(pLight.position - fs_in.frag_pos), normal, view_dir, albedo, attenuation, pLight.specular_intensity, roughness, metallic, shadow_value, F0);
    }
     //Calculate spot lights
    for(int i = 0; i < 12; ++i)
    {
        SpotLight sLight = scene.spotLights[i];

        vec3 light_dir = normalize(sLight.position - fs_in.frag_pos);
        float dist = length(sLight.position - fs_in.frag_pos);
        float attenuation = 1.0 / (sLight.constant + sLight.linear * dist + sLight.quadratic * (dist * dist)); 
        //SPOTLIGHT INTENSITY
        float theta = dot(light_dir, normalize(-sLight.direction));
        float epsilon = sLight.cutOff - sLight.outerCutOff;
        float intensity = clamp((theta - sLight.outerCutOff) / epsilon, 0.0, 1.0);
        //APPLY ATTENUATION AND SPOTLIGHT INTENSITY
        float att_ins = attenuation * intensity;
        lighting += calculate_light(sLight.color * sLight.ambient_intensity,  light_dir, normal, view_dir, albedo, att_ins, sLight.specular_intensity, roughness, metallic, shadow_value, F0);
    }


    
}