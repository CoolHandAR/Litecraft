#version 460 core

out vec4 FragColor;

in vec2 v_texCoords;

struct VecOutput
{
   //vec3 v_FragPos;
   vec3 v_worldPos;
   vec4 v_worldSpace;
   vec3 v_viewDir;
   vec3 v_lightDir;
};

layout(location = 0) in VecOutput v_Output;

struct DirLight 
{
   vec3 direction;

   vec3 color;
   float ambient_intensity;
   float specular_intensity;
};

uniform DirLight u_dirLight;
uniform sampler2D texture_atlas;
uniform samplerCube skybox_texture;
uniform sampler2D dudv_map;
uniform sampler2D normal_map;
uniform float u_moveFactor;
uniform vec3 u_viewPos;

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
const float damper_value = 0.2;
const float reflectivity = 0.02;
void main()
{
   
   vec3 ndc = (vec3(v_Output.v_worldSpace.x, -v_Output.v_worldSpace.y, v_Output.v_worldSpace.z) / v_Output.v_worldSpace.w) / 2.0 + 0.5;
   vec2 ndc_refract = (vec2(v_Output.v_worldSpace.x, v_Output.v_worldSpace.y) / v_Output.v_worldSpace.w) / 2.0 + 0.5;
   vec3 reflect_tex_coords = vec3(ndc.x, ndc.y, ndc.z);
   vec2 refract_tex_corods = vec2(ndc_refract.x, ndc_refract.y);
   
   vec2 distortion_coords = texture(dudv_map, vec2(v_texCoords.x + u_moveFactor, v_texCoords.y)).rg * 0.01;
   distortion_coords = v_texCoords + vec2(distortion_coords.x, distortion_coords.y + u_moveFactor);
   vec2 total_distortion = (texture(dudv_map, distortion_coords).rg * 2.0 - 1.0) * 0.2;
   

   reflect_tex_coords.xy += total_distortion;
   refract_tex_corods.xy += total_distortion;

   reflect_tex_coords = clamp(reflect_tex_coords, 0.001, 0.999);
   refract_tex_corods = clamp(refract_tex_corods, 0.001, 0.999);

   vec4 normal_color = texture(normal_map, distortion_coords);
   vec3 normal = normalize(vec3(normal_color.r * 2.0 - 1.0, normal_color.b, normal_color.g * 2.0 - 1.0));

   vec3 reflected_light = reflect(normalize(u_viewPos - v_Output.v_worldPos), normal);
   float specular = max(dot(reflected_light, normalize(v_Output.v_viewDir)), 0.0);
   specular = pow(specular, damper_value);
   vec3 specular_highlights = u_dirLight.color * specular;

   vec3 lighting = vec3(0.0);
   
   //DIRLIGHT CALC
   lighting = calculate_light(u_dirLight.color * u_dirLight.ambient_intensity, normalize(u_dirLight.direction), normal, v_Output.v_viewDir, u_dirLight.specular_intensity, 0, 1, 0);
 
   
   vec3 reflective_color = texture(skybox_texture, reflect_tex_coords).rgb;
   //reflective_color *= 0.1;

   vec3 blue_color = vec3(0, 0.8, 0.8);
   blue_color.xy + total_distortion;
   //reflective_color = mix(reflective_color, blue_color, 0.2);

   lighting *= reflective_color + specular_highlights;

   //lighting =  reflective_color + specular_highlights;

   FragColor = vec4(lighting, 0.5);
}