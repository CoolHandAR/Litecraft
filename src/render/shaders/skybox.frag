#version 460 core

#include "scene_incl.incl"

layout(location = 0) out vec4 FragColor;

in vec3 TexCoords;
in vec3 CubeNormal;

uniform samplerCube skybox_texture;

void main()
{

	float sunangle = acos(dot(scene.dirLightDirection.xyz, CubeNormal));
	vec4 sky = texture(skybox_texture, TexCoords);
	if(sunangle < 25.1)
	{
		//sky.rgb  = scene.dirLightColor.rgb * scene.dirLightAmbientIntensity;
	}
	else
	{
	float c2 =  (sunangle - 25) / (30 - 25);


	

	//sky.rgb  = mix(scene.dirLightColor.rgb * scene.dirLightAmbientIntensity, sky.rgb, clamp(1.0 - pow(1.0 - c2, 1.0 / 0.15), 0.0, 1.0));	
	}
	

	FragColor = sky;
}
