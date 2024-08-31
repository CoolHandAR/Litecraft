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
    //COMMON
    Material material = materials.data[fs_in.material_index];
    vec3 view_dir = normalize(camera.position - fs_in.world_pos);
    vec3 lighting = vec3(0.0);

    //calculate dir light
#define DIR_LIGHT scene.dir_light
    lighting += calculate_light(DIR_LIGHT.color * DIR_LIGHT.ambient_intensity, DIR_LIGHT.direction, fs_in.normal, view_dir, DIR_LIGHT.specular_intensity, 0.2, 1);

}