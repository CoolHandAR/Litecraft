#version 460 core

layout (location = 0) in vec4 a_Pos;

flat out uint v_chunkIndex;
out vec3 v_LocalViewPos;
flat out uint v_directionIndex;
out vec3 v_chunkPos;
flat out uint v_forceDraw;

struct ChunkData
{
    vec4 min_point;
};

layout (std430, binding = 9) readonly restrict buffer ChunkMinPointsBuffer
{
    ChunkData data[];
} chunk_data;



layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform vec3 u_viewPos;


void main(void)
{
    
    vec3 aabb_ctr = (chunk_data.data[gl_VertexID].min_point.xyz - 0.5 + vec3(8.0));
    vec3 aabb_dim = vec3(8);

    //mat4 model = mat4(1.0);
    //model[3].xyz += chunk_data.data[gl_VertexID].min_point.xyz - 0.5;
   // mat4 model_inv = inverse(model);

    vec3 local_view_pos = u_viewPos;
    local_view_pos -= aabb_ctr;
    v_LocalViewPos.x = (local_view_pos.x > 0) ? 1 : -1;
    v_LocalViewPos.y = (local_view_pos.y > 0) ? 1 : -1;
    v_LocalViewPos.z = (local_view_pos.z > 0) ? 1 : -1;

    v_forceDraw = 0;
    //Draw if our view pos is inside the box
    if (all(lessThan(abs(local_view_pos), aabb_dim)))
    {
        v_forceDraw = 1;
    }
   
    v_chunkIndex = uint(a_Pos.w);
    
    v_chunkPos = (chunk_data.data[gl_VertexID].min_point.xyz - 0.5) + vec3(8);

    gl_Position = g_viewProjectionMatrix * vec4(a_Pos.xyz, 1.0);
}
