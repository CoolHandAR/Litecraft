#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D source_texture;

//src https://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/3/

#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_REDUCE_MUL   (1.0/8.0)
#define FXAA_SPAN_MAX     8.0

vec3 FXXA(vec2 rcpFrame)
{
    vec3 rgbNW = texture(source_texture, TexCoords).rgb;
    vec3 rgbNE = textureOffset(source_texture, TexCoords, ivec2(1, 0)).rgb;
    vec3 rgbSW = textureOffset(source_texture, TexCoords, ivec2(0, 1)).rgb;
    vec3 rgbSE = textureOffset(source_texture, TexCoords, ivec2(1, 1)).rgb;
    vec3 rgbM = texture(source_texture, TexCoords).rgb;

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
        texture(source_texture, TexCoords.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texture(source_texture, TexCoords.xy + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        texture(source_texture, TexCoords.xy + dir * (0.0/3.0 - 0.5)).xyz +
        texture(source_texture, TexCoords.xy + dir * (3.0/3.0 - 0.5)).xyz);
    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return rgbA;
    }

    return rgbB;
}

void main()
{   
    vec2 textureSize = textureSize(source_texture, 0);
    vec2 rcpFrame = vec2(1.0/ textureSize.x, 1.0/ textureSize.y);
    vec3 FXAA_COLOR = FXXA(rcpFrame);

    FragColor = vec4(FXAA_COLOR, 1.0);
}