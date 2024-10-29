#version 460 core
#extension GL_ARB_bindless_texture : require

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
} FS_in;

struct Material
{
    uint flags;
    int albedo_map;
    int metalic_map;
    int roughness_map;
    int normal_map;
    int ao_map;
    float metallic_ratio;
    float roughness_ratio;

    vec4 color;
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

layout (location = 0) flat in int v_materialIndex;

uniform vec3 u_viewPos;
uniform DirLight u_dirLight;
uniform PointLight u_pointLight;

layout (std430, binding = 26) readonly restrict buffer Textures
{
    sampler2D data[];
} textures;

layout (std430, binding = 27) readonly restrict buffer Materials
{
    Material data[];
} Mats;

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

vec3 getNormalFromMap(uint normal_index)
{
    vec3 tangentNormal = texture(textures.data[normal_index], FS_in.tex_coords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FS_in.world_pos);
    vec3 Q2  = dFdy(FS_in.world_pos);
    vec2 st1 = dFdx(FS_in.tex_coords);
    vec2 st2 = dFdy(FS_in.tex_coords);

    vec3 N   = normalize(FS_in.normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec3 dirLightCalc(DirLight light, vec3 normal, vec3 view_dir, vec3 albedo, float roughness, float metallic, float shadow, vec3 F0)
{
    vec3 light_dir = normalize(light.direction);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    vec3 radiance = light.color * light.ambient_intensity;
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

    specular *= light.specular_intensity;

    vec3 value = (kD * albedo / PI + specular) * radiance * NdotL;

   // return_value *= (1.0 - shadow);

    return value;
}

vec3 pointLightCalc(PointLight light, vec3 normal, vec3 view_dir, vec3 albedo, float roughness, float metallic, float shadow, vec3 F0)
{
    float dist = (length(light.position - FS_in.frag_pos) / max(light.radius, 0.00001));
    vec3 light_dir = normalize(light.position - FS_in.frag_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    vec3 radiance = light.color * light.ambient_intensity;
    float NdotV = max(dot(normal, view_dir), 0.0);
    float NdotL = max(dot(normal, light_dir), 0.0);
    //ATTENUATION
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist)); 
    //APPLY ATTENUATION
    radiance *= attenuation;

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

    specular *= light.specular_intensity;

    vec3 value = (kD * albedo / PI + specular) * radiance * NdotL;

    return value;  
}
vec3 spotLightCalc(SpotLight light, vec3 normal, vec3 view_dir, vec3 albedo, float roughness, float metallic, float shadow, vec3 F0)
{
    vec3 light_dir = normalize(light.position - FS_in.frag_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float NdotV = max(dot(normal, view_dir), 0.0);
    float NdotL = max(dot(normal, light_dir), 0.0);
    //ATTENUATION
    float dist = length(light.position - FS_in.frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist)); 
    //SPOTLIGHT INTENSITY
    float theta = dot(light_dir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    //APPLY ATTENUATION AND SPOTLIGHT INTENSITY
    float att_ins = attenuation * intensity;
    vec3 radiance = light.color * light.ambient_intensity;
    radiance *= att_ins;

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

    specular *= light.specular_intensity;

    vec3 value = (kD * albedo / PI + specular) * radiance * NdotL;

    return value;
}


vec3 F0calc(float metallic, float specular, vec3 albedo)
{
    float dielectric = 0.16 * specular * specular;
    return mix(vec3(dielectric), albedo, vec3(metallic));
}

void main()
{
    int mat_index = int(v_materialIndex);
    Material material = Mats.data[mat_index];

    vec3 albedo = pow(texture(textures.data[material.albedo_map], FS_in.tex_coords).rgb, vec3(2.2));
    float metallic = texture(textures.data[material.metalic_map], FS_in.tex_coords).r;
    float roughness = texture(textures.data[material.roughness_map], FS_in.tex_coords).r;
    float ao = texture(textures.data[material.ao_map], FS_in.tex_coords).r;
    vec3 normal = getNormalFromMap(material.normal_map);

    vec3 F0 = F0calc(metallic, 0.5, albedo); 

    vec3 Lo = vec3(0.0);
    vec3 view_dir = normalize(u_viewPos - FS_in.world_pos);

    vec3 dir_lighting = dirLightCalc(u_dirLight, normal, view_dir, albedo, roughness, metallic, 1.0, F0);
    
    Lo += dir_lighting;
    Lo += pointLightCalc(u_pointLight,  normal, view_dir, albedo, roughness, metallic, 1.0, F0);
    
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    f_color = vec4(color, 1.0);
}