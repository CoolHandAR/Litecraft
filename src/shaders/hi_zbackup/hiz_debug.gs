#version 460 core

#define CHUNK_FLAG_VISIBLE uint(1)
#define CHUNK_FLAG_IN_FRUSTRUM uint(2)
#define CHUNK_FLAG_PREV_VISIBLE uint(4)


layout(points, invocations = 3) in;
layout(triangle_strip, max_vertices = 4) out;


layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

layout (std430, binding = 11) restrict buffer ChunkVisibles
{
    uint data[];
} VisibleChunk;

flat out uint v_chunkIndexOut;
flat in uint v_chunkIndex[];
in vec3 v_LocalViewPos[];
flat in uint v_directionIndex[];
in vec3 v_chunkPos[];
flat in uint v_forceDraw[];

void main() 
{
#define CHUNK chunk_data.data[v_chunkIndex[0]]
  
    v_chunkIndexOut = v_chunkIndex[0];

    uint chunk_index = v_chunkIndex[0];
    
    if(bool(VisibleChunk.data[v_chunkIndex[0]] & CHUNK_FLAG_IN_FRUSTRUM) == false)
    {
        return;
    }

    if(v_forceDraw[0] == 1)
    {
        VisibleChunk.data[v_chunkIndex[0]] |= CHUNK_FLAG_VISIBLE;
        return;
    }

  vec3 faceNormal = vec3(0);
  vec3 edgeBasis0 = vec3(0);
  vec3 edgeBasis1 = vec3(0);
  
  int id = gl_InvocationID;

    switch(id)
    {
        case 0:
        {
            faceNormal.x = 8;
            edgeBasis0.y = 8;
            edgeBasis1.z = 8;
            break;
        }
        case 1:
        {
            faceNormal.y = 8;
            edgeBasis1.x = 8;
            edgeBasis0.z = 8;
            break;
        }
        case 2:
        {
            faceNormal.z = 8;
            edgeBasis0.x = 8;
            edgeBasis1.y = 8;
            break;
        }
    }


    
    vec3 worldCtr = v_chunkPos[0];

    float proj = v_LocalViewPos[0][id];  

   
    faceNormal = faceNormal * proj;
    edgeBasis1 = edgeBasis1 * proj;    

    gl_Position = g_viewProjectionMatrix * vec4(worldCtr + (faceNormal - edgeBasis0 - edgeBasis1),1);
    EmitVertex();
  

    gl_Position = g_viewProjectionMatrix * vec4(worldCtr + (faceNormal + edgeBasis0 - edgeBasis1),1);
    EmitVertex();
  

    gl_Position = g_viewProjectionMatrix * vec4(worldCtr + (faceNormal - edgeBasis0 + edgeBasis1),1);
    EmitVertex();
  

    gl_Position = g_viewProjectionMatrix * vec4(worldCtr + (faceNormal + edgeBasis0 + edgeBasis1),1);
    EmitVertex();
	
	EndPrimitive(); 
}
