#version 460 core

layout (location = 0) out vec3 upsample;

// This shader performs upsampling on a texture,
// as taken from Call Of Duty method, presented at ACM Siggraph 2014.

// Remember to add bilinear minification filter for this texture!
// Remember to use a floating-point texture format (for HDR)!
// Remember to use edge clamping for this texture!
uniform sampler2D source_texture;
uniform float u_filterRadius;

in vec2 TexCoords;

void main()
{
	// The filter kernel is applied with a radius, specified in texture
	// coordinates, so that the radius will vary across mip resolutions.
	float x = u_filterRadius;
	float y = u_filterRadius;

	// Take 9 samples around current texel:
	// a - b - c
	// d - e - f
	// g - h - i
	// === ('e' is the current texel) ===
	vec3 a = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
	vec3 b = texture(source_texture, vec2(TexCoords.x,     TexCoords.y + y)).rgb;
	vec3 c = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

	vec3 d = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y)).rgb;
	vec3 e = texture(source_texture, vec2(TexCoords.x,     TexCoords.y)).rgb;
	vec3 f = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y)).rgb;

	vec3 g = texture(source_texture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
	vec3 h = texture(source_texture, vec2(TexCoords.x,     TexCoords.y - y)).rgb;
	vec3 i = texture(source_texture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

	// Apply weighted distribution, by using a 3x3 tent filter:
	//  1   | 1 2 1 |
	// -- * | 2 4 2 |
	// 16   | 1 2 1 |
	upsample = e*4.0;
	upsample += (b+d+f+h)*2.0;
	upsample += (a+c+g+i);
	upsample *= 1.0 / 16.0;
}