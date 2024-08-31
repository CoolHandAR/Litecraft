#version 460 core

#include "scene_incl.incl"

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;


void main()
{          
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = scene.shadow_matrixes[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}  