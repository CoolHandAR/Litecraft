#version 460 core

out vec4 f_color;

struct VecOutput
{
   vec3 v_FragPos;
	vec3 v_Normal;
   vec4 v_worldSpace;
   vec3 v_viewDir;
   vec3 v_lightDir;
};

layout (location = 0) in VecOutput v_Output;
flat in float v_hp;

uniform sampler2D reflection_texture;
uniform sampler2D refraction_texture;
uniform sampler2D dudv_map;
uniform sampler2D normal_map;
uniform sampler2D refraction_depth_map;
uniform float u_moveFactor;

const float damper_value = 100.0;
const float reflectivity = 0.02;

vec2 FragPosToTexCoords()
{
   vec2 coords = vec2(0.0);
   if(v_Output.v_Normal.z != 0)
   {
      coords = vec2(fract(v_Output.v_FragPos.x - 0.5), fract(-v_Output.v_FragPos.y - 0.5));
   }
   else if(v_Output.v_Normal.x != 0)
   {
      coords = vec2(fract(v_Output.v_FragPos.x + v_Output.v_FragPos.z), fract(-v_Output.v_FragPos.y - 0.5));
   }

   else if(v_Output.v_Normal.y != 0)
   {
      coords = vec2(fract(v_Output.v_FragPos.x + v_Output.v_FragPos.y), fract(-v_Output.v_FragPos.z - 0.5));
   }
   return coords;
}

void main()
{
   vec2 ndc = (vec2(v_Output.v_worldSpace.x, -v_Output.v_worldSpace.y) / v_Output.v_worldSpace.w) / 2.0 + 0.5;
   vec2 ndc_refract = (vec2(v_Output.v_worldSpace.x, v_Output.v_worldSpace.y) / v_Output.v_worldSpace.w) / 2.0 + 0.5;
   vec2 reflect_tex_coords = vec2(ndc.x, ndc.y);
   vec2 refract_tex_corods = vec2(ndc_refract.x, ndc_refract.y);
   vec2 tex_coords = FragPosToTexCoords();

   float near = 0.1; //must be same as camera
   float far = 500.0; //must be same as camera
   float depth = texture(refraction_depth_map, refract_tex_corods).r;
   float floor_distance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));
   
   depth = gl_FragCoord.z;
   float water_distance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));
   float water_depth = floor_distance - water_distance;

   vec2 distortion_coords = texture(dudv_map, vec2(tex_coords.x + u_moveFactor, tex_coords.y)).rg * 0.1;
   distortion_coords = tex_coords + vec2(distortion_coords.x, distortion_coords.y + u_moveFactor);
   vec2 total_distortion = (texture(dudv_map, distortion_coords).rg * 2.0 - 1.0) * 0.08;
  

   reflect_tex_coords += total_distortion;
   refract_tex_corods += total_distortion;

   reflect_tex_coords = clamp(reflect_tex_coords, 0.001, 0.999);
   refract_tex_corods = clamp(refract_tex_corods, 0.001, 0.999);

   vec4 reflect_color = texture(reflection_texture, reflect_tex_coords);
   vec4 refract_color = texture(refraction_texture, refract_tex_corods);

   vec3 view_Dir = normalize(v_Output.v_viewDir);
   
   vec4 normal_color = texture(normal_map, distortion_coords);
   vec3 normal = vec3(normal_color.r * 2.0 - 1.0, normal_color.b, normal_color.g * 2.0 - 1.0);
   normal = normalize(normal);
   
   vec3 reflected_light = reflect(normalize(v_Output.v_lightDir), normal);
   float specular = max(dot(reflected_light, view_Dir), 0.0);
   specular = pow(specular, damper_value);
   vec3 specular_highlights = vec3(1.0, 1.0, 1.0) * specular * reflected_light;

   float refractive_factor = dot(view_Dir, normal);
   refractive_factor = clamp(refractive_factor, 0.0, 1.0);

   f_color = mix(reflect_color, refract_color, refractive_factor);
   f_color = mix(f_color, vec4(0.0, 0.3, 0.5, 1.0), 0.4) + vec4(specular_highlights, 0.0);
}

