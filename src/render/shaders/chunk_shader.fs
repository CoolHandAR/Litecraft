#version 460 core
#extension GL_ARB_bindless_texture : require

#define ATLAS_TEXTURE_WIDTH 64
#define ATLAS_TEXTURE_HEIGHT 32
#define SHADOW_CASCADE_COUNT 4
#define LIGHT_SPACE_MATRIXES_COUNT 5

out vec4 f_color;

in VS_OUT
{
    vec3 world_pos;
    vec3 normal;
    vec2 tex_offset;
    vec2 tex_coords;
    float hp;
} fs_in;

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


uniform sampler2D atlas_texture;

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

vec3 calculate_light(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, float specular_intensity, float shadow_value, float attenuation)
{
    //AMBIENT
    vec3 ambient = light_color;
    //DIFFUSE
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light_color * diffuse_factor;
    //SPECULAR
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 64);
    vec3 specular = light_color * specular_intensity;

    //APPLY ATTENUATION
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + (1.0 - shadow_value) * (diffuse + specular));
}

void main()
{
    vec2 tex_coords_offsetted = vec2((fract(fs_in.tex_coords.x) + fs_in.tex_offset.x) / ATLAS_TEXTURE_WIDTH, -(fract(fs_in.tex_coords.y) + fs_in.tex_offset.y) / ATLAS_TEXTURE_HEIGHT);
    vec4 tex_color = texture(atlas_texture,  tex_coords_offsetted);

    int hp = int(fs_in.hp);

    //apply the shatter texture based on the hp of the block
    if(hp < 7)
    {
        vec2 shatter_tex_coords = vec2((fract(fs_in.tex_coords.x) + 21) / ATLAS_TEXTURE_WIDTH, -(fract(fs_in.tex_coords.y) + (7 - hp)) / ATLAS_TEXTURE_HEIGHT); 
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

    //discard transparent pixels
    if(tex_color.a < 0.5)
    {
        discard;
    }

    //Common
    vec3 view_dir = normalize(camera.position - fs_in.world_pos);
    vec3 lighting = vec3(0.0);
    float shadow_value = CascadingShadowCalculation(fs_in.world_pos, scene.dir_light.direction);

    //calculate dir light
#define DIR_LIGHT scene.dir_light
    lighting += calculate_light(DIR_LIGHT.color * DIR_LIGHT.ambient_intensity, DIR_LIGHT.direction, fs_in.normal, view_dir, DIR_LIGHT.specular_intensity, shadow_value, 1);

    lighting *= tex_color.rgb;

    f_color = vec4(lighting, 1.0);
}
