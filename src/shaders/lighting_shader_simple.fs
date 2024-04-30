#version 460 core

#define MAX_LIGHTS 32
#define LIGHT_SPACE_MATRIXES_COUNT 5
#define SHADOW_CASCADE_COUNT 4

out vec4 f_color;

struct VecOutput
{
    vec3 v_FragPos;
    vec3 v_Normal;
    vec2 v_texCoords;
    vec2 v_texOffset;
};

layout (location = 0) in VecOutput v_Output;
flat in float v_hp;

struct Fog
{
    vec4 color;
    float density;
};

struct Material 
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 

struct DirLight 
{
    vec3 direction;
	
    vec3 color;
    float ambient_intensity;
    float diffuse_intensity;
};

struct PointLight
{
    vec3 position;

    vec3 color;
    float ambient_intensity;
    float diffuse_intensity;

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
    float diffuse_intensity;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
};

layout (std140, binding = 1) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[LIGHT_SPACE_MATRIXES_COUNT];
};

uniform float u_cascadePlaneDistances[SHADOW_CASCADE_COUNT];
uniform float u_camerafarPlane;
uniform mat4 u_view;
uniform vec3 u_viewPos;

uniform DirLight u_dirLight;
uniform PointLight u_pointLight;
uniform SpotLight u_spotLight;

uniform PointLight u_pointLights[MAX_LIGHTS];
uniform int u_pointLightCount;

uniform sampler2D atlas_texture;
uniform sampler2DArray shadowMap;
uniform samplerCube skybox_texture;

uniform Material u_material;

uniform Fog u_Fog;


vec3 dirLightCalc(DirLight light, vec3 normal, vec3 view_dir, float shadow_value)
{
    vec3 light_dir = normalize(light.direction);

    //AMBIENT
    vec3 ambient = light.color * light.ambient_intensity;
    //DIFFUSE
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light.color * light.diffuse_intensity * diffuse_factor;
    //SPECULAR
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 64);
    vec3 specular = light.color * specular_factor;

    //APPLY MATERIAL
    //ambient *= u_material.ambient;
    //diffuse *= u_material.diffuse;
    //specular *= u_material.specular;

    return (ambient + (1.0 - shadow_value) * (diffuse + specular));
}

vec3 pointLightCalc(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, float shadow_value)
{
    float dist = length(light.position - frag_pos);
    
        

        vec3 light_dir = normalize(light.position - frag_pos);
        //AMBIENT
        vec3 ambient = light.color * light.ambient_intensity;
        //DIFFUSE
        float diffuse_factor = max(dot(light_dir, normal), 0.0);
        vec3 diffuse = light.color * light.diffuse_intensity * diffuse_factor;
        //SPECULAR
        vec3 halfway_dir = normalize(light_dir + view_dir);
        float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 128.0);
        vec3 specular = light.color * specular_factor;
        //ATTENUATION
        float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist)); 

        //APPLY ATTENUATION
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;

        return (ambient + diffuse + specular);
    
    
    return vec3(0.0);
}

vec3 spotLightCalc(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, float shadow_value)
{
    vec3 light_dir = normalize(light.position - frag_pos);

    //AMBIENT
    vec3 ambient = light.color * light.ambient_intensity;
    //DIFFUSE
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light.color * light.diffuse_intensity * diffuse_factor;
    //SPECULAR
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 64.0);
    vec3 specular = light.color * specular_factor;
    //ATTENUATION
    float dist = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist)); 
    //SPOTLIGHT INTENSITY
    float theta = dot(light_dir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    //APPLY ATTENUATION AND SPOTLIGHT INTENSITY
    float att_ins = attenuation * intensity;
    ambient *= att_ins;
    diffuse *= att_ins;
    specular *= att_ins;
    
    return (ambient + (1.0 - shadow_value) * (diffuse + specular));
}


float ShadowCalculation(vec3 fragPosWorldSpace, vec3 light_dir)
{
    // select cascade layer
    vec4 fragPosViewSpace = u_view * vec4(fragPosWorldSpace, 1.0);
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

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);

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
    vec3 normal = normalize(v_Output.v_Normal);
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
    const float biasModifier = 0.5f;
    if (layer == SHADOW_CASCADE_COUNT)
    {
        bias *= 1 / (u_camerafarPlane * biasModifier);
    }
    else
    {
        bias *= 1 / (u_cascadePlaneDistances[layer] * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
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


void main()
{
    vec2 tex_coords_offsetted = vec2((fract(v_Output.v_texCoords.x) + v_Output.v_texOffset.x) / 64, -(fract(v_Output.v_texCoords.y) + v_Output.v_texOffset.y) / 32);
    vec4 tex_color = texture(atlas_texture,  tex_coords_offsetted);

    int hp = int(v_hp);

    //apply the shatter texture based on the hp of the block
    if(hp < 7)
    {
        vec2 shatter_tex_coords = vec2((fract(v_Output.v_texCoords.x) + 21) / 64, -(fract(v_Output.v_texCoords.y) + (7 - hp)) / 32); 
        vec4 shatter_color = texture(atlas_texture,  shatter_tex_coords);

        if(shatter_color.a < 0.5)
        {
            tex_color = tex_color;
        }
        else
        {
            tex_color = tex_color + shatter_color;
        }
    }

   
    //check for texture transperancy
    if(tex_color.a < 0.4)
    {
        discard;
    }

    vec3 normal = normalize(v_Output.v_Normal);
    vec3 view_dir = normalize(u_viewPos - v_Output.v_FragPos);

    vec3 reflect_vector = reflect(view_dir, normal);
    vec3 refract_vector = refract(view_dir, normal, 1.0/1.33);

    vec4 reflected_color = texture(skybox_texture, reflect_vector);
    vec4 refracted_color = texture(skybox_texture, refract_vector);

    vec4 enviroment_color = mix(reflected_color, refracted_color, 0.5);

    //DIR SHADOW CALC
    float shadow = ShadowCalculation(v_Output.v_FragPos, u_dirLight.direction);                      

    //DIR LIGHT CALC
    vec3 lighting = dirLightCalc(u_dirLight, normal, view_dir, shadow);

    //POINT LIGHT CALC
    for(int i = 0; i < u_pointLightCount; i++)
    {
        lighting += pointLightCalc(u_pointLights[i], normal, v_Output.v_FragPos, view_dir, 0);
    }

    //SPOT LIGHT CALC
    //lighting += spotLightCalc(u_spotLight, normal, v_Output.v_FragPos, view_dir, 0);
   
    //TEXTURE COLOR
    lighting *= tex_color.rgb;

    f_color = vec4(lighting, 1.0);

    //f_color = mix(f_color, enviroment_color, 0.5);
    
    //FOG
	float z = gl_FragCoord.z / gl_FragCoord.w;
  	float fog = clamp(exp(-u_Fog.density * z * z), 0.0, 1.0);
	f_color = mix(u_Fog.color, f_color, fog);
 
}

