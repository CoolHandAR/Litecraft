#version 460 core

out vec4 f_color;

uniform sampler2D ColorBuffer;
uniform sampler2D BloomBuffer;
uniform int u_hdr;
uniform float u_exposure;
const float exposure = 1.0;
const float bloomStrength = 0.05;

in vec2 v_texCoords;

float getRandom(vec2 coords)
{
	return fract(sin(dot(coords.xy, vec2(12.9898,78.233))) * 43758.5453);
}

vec3 ditherSmoothing(vec2 coords, vec3 color)
{
	const highp float NOISE_GRANULARITY = 0.5/255.0;

	color += mix(-NOISE_GRANULARITY, NOISE_GRANULARITY, getRandom(coords));

	return color;
}

vec3 bloom_new()
{
    vec3 hdrColor = texture(ColorBuffer, v_texCoords).rgb;
    vec3 bloomColor = texture(BloomBuffer, v_texCoords).rgb;
	bloomColor =  bloomColor / ( bloomColor + vec3(1.0));
    // gamma correct
   bloomColor = pow(bloomColor, vec3(1.0/2.2)); 

    return mix(hdrColor, bloomColor, bloomStrength); // linear interpolation
}

void main(void)
{
	const float gamma = 1.5;
	vec4 texture_color = texture(ColorBuffer, v_texCoords);
	
	texture_color.rgb = ditherSmoothing(v_texCoords, texture_color.rgb);


	if(u_hdr == 1)
	{
		vec3 result = vec3(1.0) - exp(-texture_color.rgb * u_exposure);

		//gamma correct
		result = pow(result, vec3(1.0 / gamma));

		f_color = vec4(result, 1.0);
	}
	else
	{
		//f_color = texture_color;
	}
	vec3 color = bloom_new();
	//color =  color / ( color + vec3(1.0));
    // gamma correct
   //color = pow(color, vec3(1.0/2.2)); 

	f_color = vec4(color, 1.0);
}
