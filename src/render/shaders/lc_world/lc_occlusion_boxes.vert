#version 460 core

#include "../scene_incl.incl"

layout (location = 0) in vec4 a_Pos;


out uint ChunkIndex;

void main()
{
	ChunkIndex = uint(a_Pos.w);

	gl_Position = cam.viewProjection * vec4(a_Pos.xyz, 1.0);
}