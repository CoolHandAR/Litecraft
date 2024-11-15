#version 460 core

layout(location = 0) out vec4 FragColor;

in vec3 WorldPos;

uniform sampler2D skyboxTextureMap;

vec2 SampleSkybox(vec3 direction)
{
    float l = max(max(abs(direction.x), abs(direction.y)), abs(direction.z));
	vec3 dir = direction / l;
	vec3 absDir = abs(dir);
	
	vec2 skyboxUV = vec2(0);
	vec4 backgroundColor;
	if (absDir.x >= absDir.y && absDir.x > absDir.z) {
		if (dir.x > 0) {
			skyboxUV = vec2(0, 0.5) + (dir.zy * vec2(1, -1) + 1) / 2 / vec2(3, 2);
		} else {
			skyboxUV = vec2(2.0 / 3, 0.5) + (dir.zy + 1) / 2 / vec2(3, 2);
		}
	} else if (absDir.y >= absDir.z) {
		if (dir.y > 0) {
			//skyboxUV = vec2(1.0 / 3, 0) + (dir.xz * vec2(-1, 1) + 1) / 2 / vec2(3, 2);
			skyboxUV = vec2(1.0 / 3, 0.5) + (dir.xz + 1) / 2 / vec2(3, 2);
		} else {
			skyboxUV = vec2(2.0 / 3, 0.5) + (dir.xz + 1) / 2 / vec2(3, 2);
		}
	} else {
		if (dir.z > 0) {
			//skyboxUV = vec2(1.0 / 3, 0.5) + (-dir.xy + 1) / 2 / vec2(3, 2);
		} else {
			//skyboxUV = vec2(2.0 / 3, 0) + (dir.xy * vec2(1, -1) + 1) / 2 / vec2(3, 2);
		}
	}
    return skyboxUV;
}

void main()
{		
    vec2 uv = SampleSkybox(normalize(WorldPos));
    vec3 color = texture(skyboxTextureMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}