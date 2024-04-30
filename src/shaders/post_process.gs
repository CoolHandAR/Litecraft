#version 450 core

uniform sampler2D ColorBuffer;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 v_texCoords;
flat out vec2 TexSize;

void main() {
	gl_Position = vec4( 1.0, 1.0, 0.5, 1.0 );
	v_texCoords = vec2( 1.0, 1.0 );
	TexSize = textureSize( ColorBuffer, 0 );  
	EmitVertex();

	gl_Position = vec4(-1.0, 1.0, 0.5, 1.0 );
	v_texCoords= vec2( 0.0, 1.0 ); 
	TexSize = textureSize( ColorBuffer, 0 );  
	EmitVertex();

	gl_Position = vec4( 1.0,-1.0, 0.5, 1.0 );
	v_texCoords= vec2( 1.0, 0.0 ); 
	TexSize = textureSize( ColorBuffer, 0 );  
	EmitVertex();

	gl_Position = vec4(-1.0,-1.0, 0.5, 1.0 );
	v_texCoords = vec2( 0.0, 0.0 ); 
	TexSize = textureSize( ColorBuffer, 0 );  
	EmitVertex();

	EndPrimitive(); 
}
