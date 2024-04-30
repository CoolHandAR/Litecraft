#version 460 core

out vec4 f_color;

uniform sampler2D ColorBuffer;
uniform int u_hdr;
uniform float u_exposure;

in vec2 v_texCoords;


void main(void)
{
	const float gamma = 1.5;
	vec4 texture_color = texture(ColorBuffer, v_texCoords);
	
	if(u_hdr == 1)
	{
		vec3 result = vec3(1.0) - exp(-texture_color.rgb * u_exposure);

		//gamma correct
		result = pow(result, vec3(1.0 / gamma));

		f_color = vec4(result, 1.0);
	}
	else
	{
		f_color = texture(ColorBuffer, v_texCoords);
	}
	
}
