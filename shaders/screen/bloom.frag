#version 460 core

layout (location = 0) out vec3 FragColor;

uniform sampler2D source_texture;
uniform vec2 u_srcResolution;

uniform int u_mipLevel = 1;
uniform float u_threshold;
uniform float u_softThreshold;
uniform float u_gamma = 2.2;
uniform float u_filterRadius = 0.005f;


in vec2 TexCoords;

vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0 / u_gamma;
vec3 ToSRGB(vec3 v)   { return PowVec3(v, invGamma); }

float sRGBToLuma(vec3 col)
{
	return dot(col, vec3(0.299f, 0.587f, 0.114f));
}

float KarisAverage(vec3 col)
{
	// Formula is 1 / (1 + luma)
	float luma = sRGBToLuma(ToSRGB(col)) * 0.25f;
	return 1.0f / (1.0f + luma);
}

vec3 preFilter(vec3 col, float threshold, float soft_threshold)
{	
	float knee = threshold * soft_threshold;
	float y = threshold - knee;
	float z = 2.0 * knee;
	float w = 0.25f / (knee + 0.00001f);

	float brightness = max(col.r, max(col.g, col.b));
	float soft = brightness - y;
	soft = clamp(soft, 0, z);
	soft = soft * soft * w;
	float contribution = max(soft, brightness - threshold);
	contribution /= max(brightness, 0.00001);

	return col * contribution;
}

void main()
{
#ifdef DOWNSAMPLE_PASS
	vec2 srcTexelSize = 1.0 / u_srcResolution;
	float x = srcTexelSize.x;
	float y = srcTexelSize.y;

	vec3 a = texture(source_texture, vec2(TexCoords.x - 2*x, TexCoords.y + 2*y)).rgb;
	vec3 b = texture(source_texture, vec2(TexCoords.x,       TexCoords.y + 2*y)).rgb;
	vec3 c = texture(source_texture, vec2(TexCoords.x + 2*x, TexCoords.y + 2*y)).rgb;

	vec3 d = texture(source_texture, vec2(TexCoords.x - 2*x, TexCoords.y)).rgb;
	vec3 e = texture(source_texture, vec2(TexCoords.x,       TexCoords.y)).rgb;
	vec3 f = texture(source_texture, vec2(TexCoords.x + 2*x, TexCoords.y)).rgb;

	vec3 g = texture(source_texture, vec2(TexCoords.x - 2*x, TexCoords.y - 2*y)).rgb;
	vec3 h = texture(source_texture, vec2(TexCoords.x,       TexCoords.y - 2*y)).rgb;
	vec3 i = texture(source_texture, vec2(TexCoords.x + 2*x, TexCoords.y - 2*y)).rgb;

	vec3 j = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
	vec3 k = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;
	vec3 l = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
	vec3 m = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

	vec3 downsample = vec3(0.0);
	vec3 groups[5];
	switch (u_mipLevel)
	{
	case 0:
	  groups[0] = (a+b+d+e) * (0.125f/4.0f);
	  groups[1] = (b+c+e+f) * (0.125f/4.0f);
	  groups[2] = (d+e+g+h) * (0.125f/4.0f);
	  groups[3] = (e+f+h+i) * (0.125f/4.0f);
	  groups[4] = (j+k+l+m) * (0.5f/4.0f);
	  groups[0] *= KarisAverage(groups[0]);
	  groups[1] *= KarisAverage(groups[1]);
	  groups[2] *= KarisAverage(groups[2]);
	  groups[3] *= KarisAverage(groups[3]);
	  groups[4] *= KarisAverage(groups[4]);
	  downsample = groups[0]+groups[1]+groups[2]+groups[3]+groups[4];

	  downsample = preFilter(downsample, u_threshold, u_softThreshold);

	  downsample = max(downsample, 0.0001f);
	  break;
	default:
	  downsample = e*0.125;        
	  downsample += (a+c+g+i)*0.03125;    
	  downsample += (b+d+f+h)*0.0625;     
	  downsample += (j+k+l+m)*0.125;       
	  break;
	}

	FragColor = downsample;
#endif

#ifdef UPSAMPLE_PASS
	float x = u_filterRadius;
	float y = u_filterRadius;

	vec3 a = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
	vec3 b = texture(source_texture, vec2(TexCoords.x,     TexCoords.y + y)).rgb;
	vec3 c = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

	vec3 d = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y)).rgb;
	vec3 e = texture(source_texture, vec2(TexCoords.x,     TexCoords.y)).rgb;
	vec3 f = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y)).rgb;

	vec3 g = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
	vec3 h = texture(source_texture, vec2(TexCoords.x,     TexCoords.y - y)).rgb;
	vec3 i = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;
	
	vec3 upsample = vec3(0.0);

	upsample = e*4.0;
	upsample += (b+d+f+h)*2.0;
	upsample += (a+c+g+i);
	upsample *= 1.0 / 16.0;

	FragColor = upsample;
#endif
}
