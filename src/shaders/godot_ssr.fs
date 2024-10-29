#version 460 core

out vec4 FragColor;

in vec2 v_texCoords;

uniform sampler2D depth_texture;
uniform sampler2D gNormalMetal;
uniform sampler2D gColorRough;

uniform mat4 u_InverseProj;
uniform mat4 u_proj;

uniform float u_zFar;
uniform float u_zNear;

uniform vec2 u_screenSize;

vec2 view_to_screen(vec3 view_pos, out float w) {
	vec4 projected = u_proj * vec4(view_pos, 1.0);
	projected.xyz /= projected.w;
	projected.xy = projected.xy * 0.5 + 0.5;
	w = projected.w;
	return projected.xy;
}

uniform mat3 u_view;

void main()
{
    vec2 pixelSize = vec2(1.0 / (u_screenSize.x * 0.5), 1.0 / (u_screenSize.y * 0.5));
    vec2 endPixelSize = vec2(1.0 / (u_screenSize.x * 0.5), 1.0 / (u_screenSize.y * 0.5));

    vec4 ColorRough = texture(gColorRough, v_texCoords);
    vec4 NormalMetal = texture(gNormalMetal, v_texCoords);

    vec3 Normal = NormalMetal.rgb * 2.0 - 1.0;

    float roughness = ColorRough.a;

    float depth_sample = texture(depth_texture, v_texCoords).r;

    vec4 worldPos = u_InverseProj * vec4(v_texCoords * 2.0 - 1.0, depth_sample * 2.0 - 1.0, 1.0);
    vec3 vertex = worldPos.xyz / worldPos.w;

    vec3 viewDir = normalize(vertex);
    vec3 rayDir = normalize(reflect(viewDir, Normal));

    if(dot(rayDir, Normal) < 0.001)
    {
        FragColor = vec4(0.0);
        return;
    }

    float rayLength = (vertex.z + rayDir.z * u_zFar) > -u_zNear ? (-u_zNear - vertex.z) / rayDir.z : u_zFar;
    vec3 rayEnd = vertex + rayDir * rayLength;

    float wBegin = 0;
    float wEnd = 0;
    vec2 vpLineBegin = view_to_screen(vertex, wBegin);
    vec2 vpLineEnd = view_to_screen(rayEnd, wEnd);
    vec2 vpLineDir = vpLineEnd - vpLineBegin;

    wBegin = 1.0 / wBegin;
    wEnd = 1.0 / wEnd;

    float zBegin = vertex.z * wBegin;
    float zEnd = rayEnd.z * wEnd;

    vec2 lineBegin = vpLineBegin / pixelSize;
    vec2 lineDir = vpLineDir / pixelSize;

    float zDir = zEnd - zBegin;
    float wDir = wEnd - wBegin;

    float scale_max_x = min(1.0, 0.99 * (1.0 - vpLineBegin.x) / max(1e-5, vpLineDir.x));
	float scale_max_y = min(1.0, 0.99 * (1.0 - vpLineBegin.y) / max(1e-5, vpLineDir.y));
	float scale_min_x = min(1.0, 0.99 * vpLineBegin.x / max(1e-5, -vpLineDir.x));
	float scale_min_y = min(1.0, 0.99 * vpLineBegin.y / max(1e-5, -vpLineDir.y));
	float line_clip = min(scale_max_x, scale_max_y) * min(scale_min_x, scale_min_y);
	lineDir *= line_clip;
	zDir *= line_clip;
	wDir *= line_clip; 

    // clip z and w advance to line advance
	vec2 line_advance = normalize(lineDir); // down to pixel
	float step_size = length(line_advance) / length(lineDir);
	float z_advance = zDir * step_size; // adapt z advance to line advance
	float w_advance = wDir * step_size; // adapt w advance to line advance

	// make line advance faster if direction is closer to pixel edges (this avoids sampling the same pixel twice)
	float advance_angle_adj = 1.0 / max(abs(line_advance.x), abs(line_advance.y));
	line_advance *= advance_angle_adj; // adapt z advance to line advance
	z_advance *= advance_angle_adj;
	w_advance *= advance_angle_adj;

    vec2 pos = lineBegin;
	float z = zBegin;
	float w = wBegin;
	float z_from = z / w;
	float z_to = z_from;
	float depth;
	vec2 prev_pos = pos;

	bool found = false;

	float steps_taken = 0.0;

    //paramets
    int num_steps = 64;
    float depth_tolerance = 0.2;
    float curve_fade_in = 0.150;
    float distance_fade = 2.0;

    for (int i = 0; i < num_steps; i++) 
    {
		pos += line_advance;
		z += z_advance;
		w += w_advance;

		// convert to linear depth

		depth = texture(depth_texture, pos * pixelSize).r * 2.0 - 1.0;

        depth = 2.0 * u_zNear * u_zFar / (u_zFar + u_zNear - depth * (u_zFar - u_zNear));

        depth = -depth;

        z_from = z_to;
		z_to = z / w;

        if (depth > z_to) 
        {
			// if depth was surpassed
			if ((depth <= max(z_to, z_from) + depth_tolerance) && (-depth < u_zFar)) 
            {
				// check the depth tolerance and far clip
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
        vec2 margin = vec2((u_screenSize.x + u_screenSize.y) * 0.5 * 0.05); // make a uniform margin

        if (any(bvec4(lessThan(pos, vec2(0.0, 0.0)), greaterThan(pos, u_screenSize * 0.5))))
        {
            //FragColor = vec4(0.0);
			//return;
        }

        {
			//blend fading out towards inner margin
			// 0.25 = midpoint of half-resolution reflection
			vec2 margin_grad = mix(u_screenSize * 0.5 - pos, pos, lessThan(pos, u_screenSize * 0.25));
			margin_blend = smoothstep(0.0, margin.x * margin.y, margin_grad.x * margin_grad.y);
			//margin_blend = 1.0;
		}

        vec2 final_pos;
		float grad = (steps_taken + 1.0) / float(num_steps);
		float initial_fade = curve_fade_in == 0.0 ? 1.0 : pow(clamp(grad, 0.0, 1.0), curve_fade_in);
		float fade = pow(clamp(1.0 - grad, 0.0, 1.0), distance_fade) * initial_fade;
		final_pos = pos;

        FragColor = vec4(texture(gColorRough, final_pos * pixelSize).rgb, fade * margin_blend);
        //FragColor = vec4(final_pos * pixelSize, 1.0, 1.0);
    }
    else
    {
        FragColor = vec4(0.0);
    }
    
}   