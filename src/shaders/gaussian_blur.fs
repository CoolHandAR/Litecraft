// Efficient box filter from Jimenez: http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
// Approximates a Gaussian in a single pass.

#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 v_texCoords;

uniform sampler2D source_texture;

uniform vec2 u_pixelSize;

void main()
{
    vec4 A = texture(source_texture, v_texCoords + u_pixelSize * vec2(-1.0, -1.0));
	vec4 B = texture(source_texture, v_texCoords + u_pixelSize * vec2(0.0, -1.0));
	vec4 C = texture(source_texture, v_texCoords + u_pixelSize * vec2(1.0, -1.0));
	vec4 D = texture(source_texture, v_texCoords + u_pixelSize * vec2(-0.5, -0.5));
	vec4 E = texture(source_texture, v_texCoords + u_pixelSize * vec2(0.5, -0.5));
	vec4 F = texture(source_texture, v_texCoords + u_pixelSize * vec2(-1.0, 0.0));
	vec4 G = texture(source_texture, v_texCoords);
	vec4 H = texture(source_texture, v_texCoords + u_pixelSize * vec2(1.0, 0.0));
	vec4 I = texture(source_texture, v_texCoords + u_pixelSize * vec2(-0.5, 0.5));
	vec4 J = texture(source_texture, v_texCoords + u_pixelSize * vec2(0.5, 0.5));
	vec4 K = texture(source_texture, v_texCoords + u_pixelSize * vec2(-1.0, 1.0));
	vec4 L = texture(source_texture, v_texCoords + u_pixelSize * vec2(0.0, 1.0));
	vec4 M = texture(source_texture, v_texCoords + u_pixelSize * vec2(1.0, 1.0));

    float weight = 0.5 / 4.0;
	float lesser_weight = 0.125 / 4.0;

    FragColor = (D + E + I + J) * weight;
	FragColor += (A + B + G + F) * lesser_weight;
	FragColor += (B + C + H + G) * lesser_weight;
	FragColor += (F + G + L + K) * lesser_weight;
	FragColor += (G + H + M + L) * lesser_weight;
}