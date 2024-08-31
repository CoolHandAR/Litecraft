#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depth_texture;
uniform sampler2D MainSceneTexture;
uniform sampler2D BloomSceneTexture;
uniform sampler2D BlurredSceneTexture;

uniform float u_Gamma;
uniform float u_Exposure = 0.6;
uniform float u_BloomStrength;
uniform float u_Brightness;
uniform float u_Contrast;
uniform float u_Saturation;

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

vec3 tonemapColor(vec3 color)
{   
    return Uncharted2ToneMapping(color, 1.2, u_Gamma);
}

vec3 applyBloom(vec2 coords, vec3 color, float strength)
{
    vec3 bloom_color = texture(BloomSceneTexture, coords).rgb;
    return mix(color, bloom_color, strength);
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

vec3 applyFXXA(vec3 color, float exposure)
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

void main()
{
    vec3 scene_color = texture(MainSceneTexture, TexCoords).rgb;

    //EXPOSURE
    scene_color *= u_Exposure;

    //FXXA
    scene_color = applyFXXA(scene_color, u_Exposure);

    //BLOOM
    scene_color = applyBloom(TexCoords, scene_color, 0.07);

    //TONEMAP
    scene_color = tonemapColor(scene_color);

    //BCS
    scene_color = applyBrightnessContrastSaturation(scene_color, u_Brightness, u_Contrast, u_Saturation);

    //DEBANDING
    scene_color.rgb += screen_space_dither(gl_FragCoord.xy);

    //FINAL COLOR
    FragColor = vec4(scene_color.rgb, 1.0);
}