#version 450 core

uniform vec2 LodDistance;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 InstPos[1];
flat in int Visible[1];
in vec2 v_bounding_rect_min[1];
in vec2 v_bounding_rect_max[1];

in vec2 v_corner1[1];
in vec2 v_corner2[1];
in vec2 v_corner3[1];
in vec2 v_corner4[1];

layout(stream=0) out vec3 InstPosLOD0;
layout(stream=1) out vec3 InstPosLOD1;
layout(stream=2) out vec3 InstPosLOD2;

void main() {
	if ( Visible[0] == 1 ) 
    {
		//top left
		gl_Position = vec4(v_corner1[0].x, v_corner1[0].y, 0, 1);
	    EmitVertex();

		//top right
		gl_Position = vec4(v_corner2[0].x, v_corner2[0].y, 0, 1);
	    EmitVertex();

		//bottom left
		gl_Position = vec4(v_corner3[0].x, v_corner3[0].y, 0, 1);
	    EmitVertex();

		//bottom right
		gl_Position = vec4(v_corner4[0].x, v_corner4[0].y, 0, 1);
	    EmitVertex();

		EndPrimitive();
	} 
	
}