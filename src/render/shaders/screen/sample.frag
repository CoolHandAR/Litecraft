#version 460 core

#include "../scene_incl.incl"
#include "../shader_commons.incl"

layout(location = 0) out vec3 NormalColor;

in vec2 TexCoords;

uniform sampler2D depth_texture;
uniform vec2 u_viewportSize;

const ivec2 SAMPLE_OFFSETS[4] = {ivec2(0, 0), ivec2(0, 1), ivec2(1, 0), ivec2(1, 1)};

vec3 depthToViewSpacePosition(vec2 coords, ivec2 offset, vec2 pixel_size)
{
	float depth = textureOffset(depth_texture, coords, offset).r;
	vec2 uv = coords + (offset * pixel_size);

	vec4 ndc = vec4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_space = cam.invProj * ndc;

    view_space.xyz = view_space.xyz / view_space.w;

    return view_space.xyz;
}

vec3 viewNormalAtPixelPosition(vec2 vpos, vec2 offset)
{		
		//src https://gist.github.com/bgolus/a07ed65602c009d5e2f753826e8078a0

        vec2 pixel_size = 1.0 / textureSize(depth_texture, 0);
		
		vpos += offset * pixel_size;

        // get current pixel's view space position
        vec3 viewSpacePos_c = depthToViewSpacePosition(vpos, ivec2(0), pixel_size);

        // get view space position at 1 pixel offsets in each major direction
        vec3 viewSpacePos_l = depthToViewSpacePosition(vpos, ivec2( -1, 0), pixel_size);
        vec3 viewSpacePos_r = depthToViewSpacePosition(vpos, ivec2( 1, 0), pixel_size);
        vec3 viewSpacePos_d = depthToViewSpacePosition(vpos, ivec2( 0, -1), pixel_size);
        vec3 viewSpacePos_u = depthToViewSpacePosition(vpos, ivec2( 0, 1), pixel_size);

        vec3 l = viewSpacePos_c - viewSpacePos_l;
        vec3 r = viewSpacePos_r - viewSpacePos_c;
        vec3 d = viewSpacePos_c - viewSpacePos_d;
        vec3 u = viewSpacePos_u - viewSpacePos_c;

        // get the difference between the current and each offset position
        vec3 hDeriv = abs(l.z) < abs(r.z) ? l : r;
        vec3 vDeriv = abs(d.z) < abs(u.z) ? d : u;

        // get view space normal from the cross product of the diffs
        vec3 viewNormal = cross(hDeriv, vDeriv);

		mat3 normalMatrix = transpose(inverse(mat3(cam.invView)));
        viewNormal = normalMatrix * viewNormal;
        viewNormal = normalize(viewNormal);

        return viewNormal;
}

void main()
{
	vec2 coords = TexCoords;

	//coords
	
	//BEST REPRESENTIVE SAMPLING SRC: https://eleni.mutantstargoat.com/hikiko/depth-aware-upsampling-6/

	vec2 pixel_size = 1.0 / textureSize(depth_texture, 0);
		
	float depth_samples[4];
	depth_samples[0] = textureOffset(depth_texture, coords, SAMPLE_OFFSETS[0]).r;
	depth_samples[1] = textureOffset(depth_texture, coords, SAMPLE_OFFSETS[1]).r;
	depth_samples[2] = textureOffset(depth_texture, coords, SAMPLE_OFFSETS[2]).r;
	depth_samples[3] = textureOffset(depth_texture, coords, SAMPLE_OFFSETS[3]).r;

	float center = (depth_samples[0] + depth_samples[1] + depth_samples[2] + depth_samples[3]) / 4.0;
	
	float dist[4];
	dist[0] = abs(center - depth_samples[0]);
	dist[1] = abs(center - depth_samples[1]);
	dist[2] = abs(center - depth_samples[2]);
	dist[3] = abs(center - depth_samples[3]);

	float max_dist = max(max(dist[0], dist[1]), max(dist[2], dist[3]));

	float rem_samples[3];
	int rejected_idx[3];
 
	int j = 0; int i; int k = 0;
	for (i = 0; i < 4; i++) {
		if (dist[i] < max_dist) 
		{
			rem_samples[j] = depth_samples[i];
			j++;
		} 
		else 
		{
			/* for the edge case where 2 or more samples
			have max_dist distance from the centroid */
			rejected_idx[k] = i;
			k++;
		}
	}
	if (j < 2) {
		for (i = 3; i > j; i--) 
		{
			rem_samples[i] = depth_samples[rejected_idx[k]];
			k--;
		}
	}

	center = (rem_samples[0] + rem_samples[1] + rem_samples[2]) / 3.0;

	dist[0] = abs(rem_samples[0] - center);
	dist[1] = abs(rem_samples[1] - center);
	dist[2] = abs(rem_samples[2] - center);

	float min_dist = min(dist[0], min(dist[1], dist[2]));

	float best_depth = depth_samples[0];
	for (i = 0; i < 3; i++) 
	{
		if (dist[i] == min_dist)
		{
			best_depth = rem_samples[i];
			break;
		}
	}
	
	vec3 normal = vec3(0.0);
	bool normal_set = false;
	for(i = 0; i < 4; i++)
	{
		if(best_depth == depth_samples[i])
		{
			normal = viewNormalAtPixelPosition(coords, vec2(SAMPLE_OFFSETS[i]));
			normal_set = true;
			break;
		}
	}

	if(!normal_set)
	{
		normal = viewNormalAtPixelPosition(coords, vec2(0.0, 0.0));
		best_depth = depth_samples[i];
	}

	//normal = viewNormalAtPixelPosition(coords, vec2(0.0, 0.0));

	normal = normal * 0.5 + 0.5; //convert to [0, 1] so that we can store in a unsigned format

	NormalColor = normal;

//	NormalColor = 
	float ds = texture(depth_texture, coords).r;

	gl_FragDepth = best_depth;
}