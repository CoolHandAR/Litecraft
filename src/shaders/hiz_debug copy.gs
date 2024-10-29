#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;


layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

flat out uint v_chunkIndexOut;
flat in uint v_chunkIndex[];

void main() 
{
#define CHUNK chunk_data.data[v_chunkIndex[0]]
  
    v_chunkIndexOut = v_chunkIndex[0];

    vec4 c = gl_in[0].gl_Position;
    vec4 dx = g_viewProjectionMatrix[0] * (16);
    vec4 dy = g_viewProjectionMatrix[1] * (16);
    vec4 dz = g_viewProjectionMatrix[2] * (16);


    vec4 v0=c, v1=c+dx, v2=c+dy, v3=v1+dy, v4=v0+dz, v5=v1+dz, v6=v2+dz, v7=v3+dz;


#define emit(p) gl_Position=p;EmitVertex();
    emit(v6);emit(v7);emit(v4);emit(v5);emit(v1);emit(v7);emit(v3);
    emit(v6);emit(v2);emit(v4);emit(v0);emit(v1);emit(v2);emit(v3);
#undef emit
	
	EndPrimitive(); 
}
