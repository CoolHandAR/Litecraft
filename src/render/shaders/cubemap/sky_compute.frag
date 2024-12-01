#version 460 core

#include "../scene_incl.incl"

layout(location = 0) out vec4 FragColor;

uniform sampler2D noise_texture;
uniform samplerCube night_texture;

uniform float u_time; 
uniform vec3 u_skyColor;
uniform vec3 u_skyHorizonColor;
uniform vec3 u_groundHorizonColor;
uniform vec3 u_groundColor;

in vec3 WorldPos;

const float PI = 3.14159265359;


vec3 calcSunDiscColor(vec3 dir, vec3 sunDir, vec3 sunColor, float sunAngleMax, float sunAngleMin, float sunCurve, vec3 sky)
{
	float sun_angle = clamp(dot(sunDir, dir), -1.0, 1.0);
	sun_angle = degrees(acos(sun_angle));

	sunCurve = max(sunCurve, 0.00001);

	if(sun_angle < sunAngleMin)
	{
		sky.rgb = sunColor;
	}
	else if(sun_angle < sunAngleMax)
	{
		float c2 = (sun_angle - sunAngleMin) / max((sunAngleMax - sunAngleMin), 0.0001);
		c2 = clamp(1.0 - pow(1.0 - c2, 1.0 / sunCurve), 0.0, 1.0);
		sky.rgb = mix(sunColor, sky.rgb, c2);
	}

	return sky;
}

vec3 addCloudsToSkyColor(vec3 sky, vec3 sky_horizon, vec3 cloud_color, vec3 dir, float cloud_height, vec2 cloud_dir, float cloud_tiling, float min_noise, float max_noise)
{
    vec2 noise_scale = 1.0 / textureSize(noise_texture, 0);
	float len = cloud_height / max(dot(vec3(0, 1.0, 0.0), dir), 0.0001);
	vec2 span = normalize(cloud_dir) * (u_time * 0.01);
	vec2 coords = dir.xz * (noise_scale * len);
	coords.xy *= cloud_tiling;
	coords.xy += span;
	
	float noise = texture(noise_texture, coords).r;
	noise = mix(min_noise, max_noise, noise);

	vec2 v = vec2(0, cloud_height) - (dir.xy * len);
	float ds = dot(v, v);
	ds = step(10000000.0, ds);
	ds = clamp(ds, 0.0, 1.0);
	ds = ds * ds;

	vec3 lerp_cloud_color = mix(cloud_color, sky_horizon, ds);

	return mix(sky.rgb, lerp_cloud_color.rgb, noise);
}

void main()
{		
	vec3 final_color = vec3(0.0);

	vec3 dir = normalize(WorldPos);

	if(scene.dirLightAmbientIntensity > 0)
	{
		float sky_curve = 0.15;
		float ground_curve = 0.02;

		//mix sky top color and the sky horizon color
		float v_angle = acos(clamp(dir.y, -1.0, 1.0));
		float c = (1.0 - v_angle / (PI * 0.5));
		vec3 sky_color = mix(u_skyHorizonColor.rgb, u_skyColor.rgb, clamp(1.0 - pow(1.0 - c, 1.0 / max(sky_curve, 0.001)), 0.0, 1.0));
	
		//mix sun disk color to sky color
		sky_color = calcSunDiscColor(dir, normalize(scene.dirLightDirection.xyz), scene.dirLightColor.rgb * 64, 90, 1, 0.05, sky_color.rgb);

		//add clouds to sky color
		sky_color = addCloudsToSkyColor(sky_color, u_skyHorizonColor, vec3(1.5, 1.5, 1.5), dir, 100, vec2(1, 0), 2, 0.0, 0.2);
	
		//mix ground bottom color with ground horizon color
		c = (v_angle - (PI * 0.5)) / (PI * 0.5);
		vec3 ground_color = mix(u_groundHorizonColor.rgb, u_groundColor.rgb, clamp(1.0 - pow(1.0 - c, 1.0 / ground_curve), 0.0, 1.0));

		//mix ground color and sky color
		final_color = mix(ground_color.rgb, sky_color.rgb, step(0.0, dir.y));
	}
	else
	{
		final_color.rgb = texture(night_texture, WorldPos).rgb;
		final_color.rgb = addCloudsToSkyColor(final_color, u_skyHorizonColor, vec3(1.5, 1.5, 1.5), dir, 100, vec2(1, 0), 1, 0.0, 0.6);
	}
	

	FragColor = vec4(final_color.rgb, 1.0);
}