#version 460 core

#include "../shader_commons.incl"
#include "../scene_incl.incl"

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D depth_texture;
uniform sampler2D gNormalMetal;
uniform sampler2D scene_texture;
uniform vec2 u_viewportSize;

vec2 view_to_screen_space(vec3 view_pos, out float w)
{   
    vec4 clip_space = cam.proj * vec4(view_pos, 1.0);
    clip_space.xyz /= clip_space.w;
    clip_space.xy = clip_space.xy * 0.5 + 0.5;

    w = clip_space.w;

    return clip_space.xy;
}

void main()
{

    vec2 pixel_size = 1.0 / (u_viewportSize * 0.5);
    vec2 coords = TexCoords;

    vec4 normal_metal = texture(gNormalMetal, coords);
    float metal = normal_metal.a;

    vec3 normal = normalize(normal_metal.rgb * 2.0 - 1.0); //convert to [-1, 1] range
   // normal = round(normal); 
   // normal = (cam.view * vec4(normal.rgb, 0)).xyz;
   
    mat3 normal_matrix = mat3(transpose(inverse(cam.view)));

    normal = normal_matrix * normal;

    vec3 viewPos = depthToViewPos(depth_texture, cam.invProj, coords);
    vec3 viewDir = normalize(viewPos);
    vec3 rayDir = normalize(reflect(viewDir, normal));

    if(dot(rayDir, normal) < 0.001)
    {
        FragColor = vec4(0.0);
        return;
    }

    float rayLength = (viewPos.z + rayDir.z * cam.z_far) > -cam.z_near ? (-cam.z_near - viewPos.z) / rayDir.z : cam.z_far;
    vec3 rayEnd = viewPos + rayDir * rayLength;

    float w_begin;
    vec2 vp_line_begin = view_to_screen_space(viewPos, w_begin);
    float w_end;
    vec2 vp_line_end = view_to_screen_space(rayEnd, w_end);
    vec2 vp_line_dir = vp_line_end - vp_line_begin;

    w_begin = 1.0 / w_begin;
    w_end = 1.0 / w_end;

    float z_begin = viewPos.z * w_begin;
    float z_end = rayEnd.z * w_end;

    vec2 line_begin = vp_line_begin / pixel_size;
    vec2 line_dir = vp_line_dir / pixel_size;
    float z_dir = z_end - z_begin;
    float w_dir = w_end - w_begin;

    float scale_max_x = min(1.0, 0.99 * (1.0 - vp_line_begin.x) / max(1e-5, vp_line_dir.x));
	float scale_max_y = min(1.0, 0.99 * (1.0 - vp_line_begin.y) / max(1e-5, vp_line_dir.y));
	float scale_min_x = min(1.0, 0.99 * vp_line_begin.x / max(1e-5, -vp_line_dir.x));
	float scale_min_y = min(1.0, 0.99 * vp_line_begin.y / max(1e-5, -vp_line_dir.y));
	float line_clip = min(scale_max_x, scale_max_y) * min(scale_min_x, scale_min_y);
    line_dir *= line_clip;
	z_dir *= line_clip;
	w_dir *= line_clip;

    vec2 line_advance = normalize(line_dir);
	float step_size = length(line_advance) / length(line_dir);
	float z_advance = z_dir * step_size;
	float w_advance = w_dir * step_size;

    float advance_angle_adj = 1.0 / max(abs(line_advance.x), abs(line_advance.y));
	line_advance *= advance_angle_adj; // adapt z advance to line advance
	z_advance *= advance_angle_adj;
	w_advance *= advance_angle_adj;

    vec2 pos = line_begin;
	float z = z_begin;
	float w = w_begin;
	float z_from = z / w;
	float z_to = z_from;
	float depth;
	vec2 prev_pos = pos;

    bool found = false;

    float steps_taken = 0.0;

    const int num_steps = 64;
    const float depth_tolerance = 32.2;
    const float distance_fade = 2;
    const float curve_fade_in = 0.150;

    for (int i = 0; i < num_steps; i++)
    {
		pos += line_advance;
		z += z_advance;
		w += w_advance;

        depth = texture(depth_texture, pos * pixel_size).r * 2.0 - 1.0;
        depth = 2.0 * cam.z_near * cam.z_far / (cam.z_far + cam.z_near - depth * (cam.z_far - cam.z_near));

        depth = -depth;

        z_from = z_to;
        z_to = z / w;

        if(depth > z_to)
        {

            if ((depth <= max(z_to, z_from) + depth_tolerance) && (-depth < cam.z_far)) 
            {
				found = true;
			}
            break;
        }

        steps_taken += 1.0;
		prev_pos = pos;

    }

    if(found)
    {
        float margin_blend = 1.0;

		vec2 margin = vec2((u_viewportSize.x + u_viewportSize.y) * 0.5 * 0.05); // make a uniform margin
		if (any(bvec4(lessThan(pos, vec2(0.0, 0.0)), greaterThan(pos,  u_viewportSize * 0.5)))) 
        {
			// clip at the screen edges
			 FragColor = vec4(0.0);
			return;
		}

        {
			//blend fading out towards inner margin
			// 0.25 = midpoint of half-resolution reflection
			vec2 margin_grad = mix(u_viewportSize - pos, pos, lessThan(pos, u_viewportSize * 0.25));
			margin_blend = smoothstep(0.0, margin.x * margin.y, margin_grad.x * margin_grad.y);
			//margin_blend = 1.0;
		}

        vec2 final_pos;
		float grad = (steps_taken + 1.0) / float(num_steps);
		float initial_fade = curve_fade_in == 0.0 ? 1.0 : pow(clamp(grad, 0.0, 1.0), curve_fade_in);
		float fade = pow(clamp(1.0 - grad, 0.0, 1.0), distance_fade) * initial_fade;
		final_pos = pos;

        vec4 final_color = vec4(textureLod(scene_texture, final_pos * pixel_size, 0.0).rgb, fade * margin_blend);

       
        //FragColor = vec4(1);
        
        FragColor = final_color;
    }
    else
    {
          FragColor = vec4(0.0);
    }
}