#version 460 core

//src https://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/3/

#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_REDUCE_MUL   (1.0/8.0)
#define FXAA_SPAN_MAX     8.0

out vec4 FragColor;

in vec2 v_texCoords;

uniform sampler2D screen_texture;

vec3 FXXA(vec2 rcpFrame)
{
    vec3 rgbNW = texture(screen_texture, v_texCoords).rgb;
    vec3 rgbNE = textureOffset(screen_texture, v_texCoords, ivec2(1, 0)).rgb;
    vec3 rgbSW = textureOffset(screen_texture, v_texCoords, ivec2(0, 1)).rgb;
    vec3 rgbSE = textureOffset(screen_texture, v_texCoords, ivec2(1, 1)).rgb;
    vec3 rgbM = texture(screen_texture, v_texCoords).rgb;

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

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX), 
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), 
          dir * rcpDirMin)) * rcpFrame.xy;

     vec3 rgbA = (1.0/2.0) * (
        texture(screen_texture, v_texCoords.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texture(screen_texture, v_texCoords.xy + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        texture(screen_texture, v_texCoords.xy + dir * (0.0/3.0 - 0.5)).xyz +
        texture(screen_texture, v_texCoords.xy + dir * (3.0/3.0 - 0.5)).xyz);
    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return rgbA;
    }

    return rgbB;
}

void main()
{
    vec2 rcpFrame = vec2(1.0/ 1280.0, 1.0/ 720.0);
    vec3 FXAA_COLOR = FXXA(rcpFrame);

    FragColor = vec4(FXAA_COLOR, 1.0);
}