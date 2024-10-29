#version 460 core

out vec4 FragColor;

in vec2 v_texCoords;

struct VecOutput
{
   vec3 v_FragPos;
	vec3 v_Normal;
   vec2 v_texOffset;
};

layout(location = 0) in VecOutput v_Output;

flat in int v_hp;
flat in ivec2 v_posIndex;

uniform sampler2D texture_atlas;
uniform samplerCube skybox_texture;

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
uniform DirLight u_dirLight;
ivec4 tileSizes = ivec4(16, 9, 24, ceil(1280.0 / 16.0));


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

    return (ambient + diffuse + specular);
}
uniform vec3 u_viewPos;
void main()
{
    float scale = tileSizes[2] / log2(500 / 0.1);
    float bias = -(tileSizes[2] * log2(0.1) / log2(500 / 0.1)) ;

     uint zTile     = uint(max(log2(gl_FragCoord.z) * scale + bias, 0.0));
    uvec3 tiles    = uvec3( uvec2( gl_FragCoord.xy / tileSizes[3] ), zTile);
    uint tileIndex = tiles.x +
                     tileSizes.x * tiles.y +
                     (tileSizes.x * tileSizes.y) * tiles.z;  

     uint lightCount       = lightGrid[tileIndex].count;
    uint lightIndexOffset = lightGrid[tileIndex].offset;

    vec4 tex_color = vec4(0.0);
    vec2 coords = vec2(0.0);
   vec2 tex_coords_offsetted = vec2(0.0);

 
   coords = vec2(fract(v_Output.v_FragPos.x + v_Output.v_FragPos[v_posIndex.x]), fract(-v_Output.v_FragPos[v_posIndex.y] - 0.5));

   tex_coords_offsetted = vec2((coords.x + v_Output.v_texOffset.x) / 25, -(coords.y + v_Output.v_texOffset.y) / 25);
  
   tex_color = texture(texture_atlas, tex_coords_offsetted);

    //apply the shatter texture based on the hp of the block
   if(v_hp < 7)
   {
      vec2 shatter_tex_coords = vec2((coords.x + 24) / 25, -(coords.y + (7 - v_hp)) / 25);
      vec4 shatter_color = texture(texture_atlas,  shatter_tex_coords);

      tex_color = tex_color + (shatter_color * (max(sign(shatter_color.a - 0.5), 0.0))); //add shatter color if alpha is less than 0.5
   }

    if(tex_color.a < 0.2)
    {
        //discard;
    }

    vec3 view_dir = normalize(u_viewPos - v_Output.v_FragPos);
    vec3 lighting = vec3(0.0);
    lighting = calculate_light(u_dirLight.color * u_dirLight.ambient_intensity, normalize(u_dirLight.direction), normalize(v_Output.v_Normal), view_dir, u_dirLight.specular_intensity, 1.0, 1, 1);
    //POINT LIGHT CALC
    for(int i = 0; i < lightCount; i++)
    {
        uint lightVectorIndex = globalLightIndexList[lightIndexOffset + i];
#define pLight pLights.data[lightVectorIndex]
        float dist = length(pLight.position.xyz - v_Output.v_FragPos);
        if(dist < pLight.radius)
        {
            //smoothstep(-1, 1, sin((u_delta * 2)))
            float attenuation = 1.0 / (pLight.constant + pLight.linear * dist + pLight.quadratic * (dist * dist)); 
            lighting += calculate_light(pLight.color.rgb * pLight.ambient_intensity,  normalize(pLight.position.xyz - v_Output.v_FragPos), normalize(v_Output.v_Normal), view_dir, pLight.specular_intensity, 1.0, attenuation, 1);
        }
    }
    lighting *= tex_color.rgb;

    vec3 I = normalize(v_Output.v_FragPos - u_viewPos);
    vec3 R = reflect(I, normalize(v_Output.v_Normal));

    vec3 reflective_color = texture(skybox_texture, R).rgb;

    //lighting = mix(lighting, reflective_color, 0.5);

    FragColor = vec4(lighting, tex_color.a);
}