#version 460 core

#include "../scene_incl.incl"

layout(location = 0) out vec4 FragColor;

in vec3 WorldPos;

uniform sampler2D equirectangularMap;
uniform samplerCube environmentMap;

uniform sampler2D noise_texture;
uniform samplerCube night_texture;

uniform vec3 u_skyColor;
uniform vec3 u_skyHorizonColor;
uniform vec3 u_groundHorizonColor;
uniform vec3 u_groundColor;

uniform float u_IrradiancesampleDelta = 0.025;

uniform float u_prefilterRoughness;
uniform int u_prefilterSampleCount = 1024;

const float PI = 3.14159265359;
const vec2 invAtan = vec2(0.1591, 0.3183);

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

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
	vec2 span = normalize(cloud_dir) * (scene.time * 0.02);
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
 
#ifdef CONVERT_TO_CUBEMAP_PASS
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
#endif

#ifdef IRRADIANCE_PASS
   vec3 N = normalize(WorldPos);

    vec3 irradiance = vec3(0.0);   
    
    // tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = u_IrradiancesampleDelta;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    FragColor = vec4(irradiance, 1.0);
#endif

#ifdef PREFILTER_PASS
    vec3 N = normalize(WorldPos);
    
    // make the simplifying assumption that V equals R equals the normal 
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = u_prefilterSampleCount;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, u_prefilterRoughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            // sample from the environment's mip level based on roughness/pdf
            float D   = DistributionGGX(N, H, u_prefilterRoughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

            float resolution = 512.0; // resolution of source cubemap (per face)
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = u_prefilterRoughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    FragColor = vec4(prefilteredColor, 1.0);
#endif

#ifdef COMPUTE_SKY_PASS
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
		//final_color.rgb = addCloudsToSkyColor(final_color, u_skyHorizonColor, vec3(1.5, 1.5, 1.5), dir, 100, vec2(1, 0), 1, 0.0, 0.6);
	}
	
	FragColor = vec4(final_color.rgb, 1.0);
#endif

#ifdef RENDER_SKYBOX_PASS
    vec4 sky = texture(environmentMap, WorldPos);
	
	FragColor = vec4(sky.rgb, 1.0);
#endif
}