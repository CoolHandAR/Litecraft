#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

in vec3 v_instancePos[1];

void main() {

    vec4 in_pos = gl_in[0].gl_Position;

	gl_Position =  vec4(in_pos.x - 8, in_pos.y - 8, in_pos.z - 8, 1.0);
	EmitVertex();

	gl_Position =  vec4(in_pos.x + 8, in_pos.y - 8, in_pos.z - 8, 1.0);
	EmitVertex();

	gl_Position =  vec4(in_pos.x + 8, in_pos.y + 8, in_pos.z - 8, 1.0);
	EmitVertex();

	gl_Position =  vec4(in_pos.x + 8, in_pos.y + 8, in_pos.z + 8, 1.0);
	EmitVertex();

    gl_Position =  vec4(in_pos.x + 8, in_pos.y + 8, in_pos.z - 8, 1.0);

	EndPrimitive(); 
}
