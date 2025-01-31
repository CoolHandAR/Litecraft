#version 460 core

#include "../scene_incl.incl"
#include "../shader_commons.incl"

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depth_texture;
uniform sampler2D MainSceneTexture;
uniform sampler2D BloomSceneTexture;

uniform float u_Gamma;
uniform float u_Exposure = 0.6;
uniform float u_BloomStrength;
uniform float u_Brightness;
uniform float u_Contrast;
uniform float u_Saturation;

//Fog
uniform vec3 u_fogColor;

uniform float u_heightFogDensity;
uniform float u_heightFogMin;
uniform float u_heightFogMax;
uniform float u_heightFogCurve;

uniform float u_depthFogDensity;
uniform float u_depthFogBegin;
uniform float u_depthFogEnd;
uniform float u_depthFogCurve;

uniform bool u_depthFogEnabled;
uniform bool u_heightFogEnabled;

vec3 Uncharted2ToneMapping(vec3 color, float gamma, float exposure)
{
    float A = 0.22;//0.15;
    float B = 0.30;//0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.01;//0.02;
    float F = 0.30;//0.30;
    float W = 11.2;
   // float exposure = 2.;
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;
    color = pow(color, vec3(1.0 / gamma));
    return color;
}

vec3 ReinhardMapping(vec3 color, float gamma, float exposure)
{
    // exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-color * exposure);
    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));

    return mapped;
}

// Adapted from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// (MIT License).
vec3 tonemap_aces(vec3 color, float white) {
	const float exposure_bias = 1.8f;
	const float A = 0.0245786f;
	const float B = 0.000090537f;
	const float C = 0.983729f;
	const float D = 0.432951f;
	const float E = 0.238081f;

	// Exposure bias baked into transform to save shader instructions. Equivalent to `color *= exposure_bias`
	const mat3 rgb_to_rrt = mat3(
			vec3(0.59719f * exposure_bias, 0.35458f * exposure_bias, 0.04823f * exposure_bias),
			vec3(0.07600f * exposure_bias, 0.90834f * exposure_bias, 0.01566f * exposure_bias),
			vec3(0.02840f * exposure_bias, 0.13383f * exposure_bias, 0.83777f * exposure_bias));

	const mat3 odt_to_rgb = mat3(
			vec3(1.60475f, -0.53108f, -0.07367f),
			vec3(-0.10208f, 1.10813f, -0.00605f),
			vec3(-0.00327f, -0.07276f, 1.07602f));

	color *= rgb_to_rrt;
	vec3 color_tonemapped = (color * (color + A) - B) / (color * (C * color + D) + E);
	color_tonemapped *= odt_to_rgb;

	white *= exposure_bias;
	float white_tonemapped = (white * (white + A) - B) / (white * (C * white + D) + E);

	return color_tonemapped / white_tonemapped;
}


vec3 tonemapColor(vec3 color)
{   
#ifdef USE_REINHARD_TONEMAP
    return ReinhardMapping(color, u_Gamma, u_Exposure);
#endif

#ifdef USE_UNCHARTED2_TONEMAP
    return Uncharted2ToneMapping(color, u_Gamma, u_Exposure);
#endif

#ifdef USE_ACES_TONEMAP
    return tonemap_aces(color, u_Exposure);
#endif
    return color;
}

vec3 linear_to_srgb(vec3 color) { // convert linear rgb to srgb, assumes clamped input in range [0;1]
	const vec3 a = vec3(0.055f);
	return mix((vec3(1.0f) + a) * pow(color.rgb, vec3(1.0f / 2.4f)) - a, 12.92f * color.rgb, lessThan(color.rgb, vec3(0.0031308f)));
}
vec3 applyBloom(vec2 coords, vec3 color, float strength)
{
    vec3 bloom_color = texture(BloomSceneTexture, coords).rgb;

    vec3 out_color = mix(color, bloom_color, strength);
    
    return out_color;
}

vec3 applyBrightnessContrastSaturation(vec3 color, float brightness, float contrast, float saturation)
{
	color = mix(vec3(0.0), color, brightness);
	color = mix(vec3(0.5), color, contrast);
	color = mix(vec3(dot(vec3(1.0), color) * 0.33333), color, saturation);

	return color;
}

// From https://alex.vlachos.com/graphics/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf
// and https://www.shadertoy.com/view/MslGR8 (5th one starting from the bottom)
// NOTE: `frag_coord` is in pixels (i.e. not normalized UV).
vec3 screen_space_dither(vec2 frag_coord) 
{
	// Iestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified for VR.
	vec3 dither = vec3(dot(vec2(171.0, 231.0), frag_coord));
	dither.rgb = fract(dither.rgb / vec3(103.0, 71.0, 97.0));

	// Subtract 0.5 to avoid slightly brightening the whole viewport.
	return (dither.rgb - 0.5) / 255.0;
}

vec3 applyFXAA(vec3 color, float exposure)
{
//src https://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/3/
#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_REDUCE_MUL   (1.0/8.0)
#define FXAA_SPAN_MAX     8.0
    
    vec3 rgbNW = texture(MainSceneTexture, TexCoords).rgb * exposure;
    vec3 rgbNE = textureOffset(MainSceneTexture, TexCoords, ivec2(1, 0)).rgb * exposure;
    vec3 rgbSW = textureOffset(MainSceneTexture, TexCoords, ivec2(0, 1)).rgb * exposure;
    vec3 rgbSE = textureOffset(MainSceneTexture, TexCoords, ivec2(1, 1)).rgb * exposure;
    vec3 rgbM = texture(MainSceneTexture, TexCoords).rgb;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir; 
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    vec2 texSize = textureSize(MainSceneTexture, 0);
    vec2 rcpFrame = vec2(1.0/ texSize.x, 1.0/ texSize.y);

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX), 
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), 
          dir * rcpDirMin)) * rcpFrame.xy;

     vec3 rgbA = (1.0/2.0) * (
        texture(MainSceneTexture, TexCoords.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texture(MainSceneTexture, TexCoords.xy + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        texture(MainSceneTexture, TexCoords.xy + dir * (0.0/3.0 - 0.5)).xyz +
        texture(MainSceneTexture, TexCoords.xy + dir * (3.0/3.0 - 0.5)).xyz);
    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return rgbA;
    }

    return rgbB;
}

vec3 apply_fog(vec3 color, vec2 coords)
{
    vec3 ViewPos = depthToViewPos(depth_texture, cam.invProj, coords);
    vec3 WorldPos = (cam.invView * vec4(ViewPos, 1.0)).xyz;

    vec3 fog_color = (scene.dirLightColor.rgb * scene.dirLightAmbientIntensity);
    vec3 emission = vec3(0.0);
    float fog_amount = 0.0;

    //Depth fog
   if(u_depthFogEnabled && u_depthFogDensity > 0) 
   {
        float fog_depth_begin = u_depthFogBegin;
        float fog_depth_end = u_depthFogEnd;

        float fog_far = fog_depth_end > 0.0 ? fog_depth_end : cam.z_far;
        float fog_z = smoothstep(fog_depth_begin, fog_far, length(ViewPos.xyz));

        fog_amount = pow(fog_z, u_depthFogCurve);
        fog_amount *= u_depthFogDensity;

        {
           // vec3 total_light = emission + color;
            //float transmit = pow(fog_z, 12);
            //fog_color = mix(max(total_light, fog_color), fog_color, transmit);
        }

    }

    //Height fog
    if(u_heightFogEnabled)
    {
        fog_amount = max(fog_amount, pow(smoothstep(u_heightFogMin, u_heightFogMax, WorldPos.y), u_heightFogCurve));
        fog_amount *= u_heightFogDensity;
    }
  
    emission = emission * (1.0 - fog_amount) + fog_color * fog_amount;

    color *= 1.0 - fog_amount;
    color += emission;

    return color;
}

void main()
{
    vec3 scene_color = texture(MainSceneTexture, TexCoords).rgb;

    //EXPOSURE
    scene_color *= u_Exposure;

#ifdef USE_FXAA
    //FXAA
    scene_color = applyFXAA(scene_color, u_Exposure);
#endif

#ifdef USE_FOG
    //FOG
    scene_color = apply_fog(scene_color, TexCoords);
#endif

#ifdef USE_BLOOM
    //BLOOM
    scene_color = applyBloom(TexCoords, scene_color, u_BloomStrength);
#endif
    
    //TONEMAP
    scene_color = tonemapColor(scene_color);

    //BCS
    scene_color = applyBrightnessContrastSaturation(scene_color, u_Brightness, u_Contrast, u_Saturation);

    //DEBANDING
    scene_color.rgb += screen_space_dither(gl_FragCoord.xy);

    //FINAL COLOR
    FragColor = vec4(scene_color.rgb, 1.0);
}