#version 450 core

out vec4 f_color;

in vec4 v_Color;


const vec4 fog_color = vec4(0.6, 0.8, 1.0, 0.1);
const float fog_density = .00009;

void main()
{
//FOG
	float z = gl_FragCoord.z / gl_FragCoord.w;
  	float fog = clamp(exp(-fog_density * z * z), 0.2f, 1.0f);
	f_color = mix(fog_color, f_color, fog);

	f_color = v_Color;
}
