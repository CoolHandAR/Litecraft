#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_BiTangent;

out VS_Out
{
    vec2 tex_coords;
    vec3 world_pos;
    vec3 normal;
    vec3 frag_pos;
    vec3 tangent_view_pos;
    vec3 tangent_frag_pos;
    vec3 tangent_light_pos;
} VS_out;

layout (location = 0) out int v_materialIndex;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

layout (std430, binding = 25) readonly restrict buffer Transforms
{
    vec4 data[];
} transforms;


void main()
{
    uint read_index = gl_InstanceID * (3 + 1); //xform + custom

    //read matrix data from the buffer
    mat4 xform;
    xform[0] = transforms.data[read_index + 0];
    xform[1] = transforms.data[read_index + 1];
    xform[2] = transforms.data[read_index + 2];
    xform[3] = vec4(0.0, 0.0, 0.0, 1.0);
    xform = transpose(xform);    

    vec4 custom = transforms.data[read_index + 3];

    v_materialIndex = int(custom.x);
    
    vec4 world_pos = xform * vec4(a_Pos, 1.0);
    gl_Position = g_viewProjectionMatrix * world_pos;


    //OUT
    VS_out.tex_coords = a_TexCoords;
    VS_out.world_pos = world_pos.xyz;
    VS_out.normal = a_Normal;
    VS_out.frag_pos = a_Pos;

    //calc tangents
    mat3 normal_matrix = transpose(inverse(mat3(xform)));
    vec3 T = normalize(normal_matrix * a_Tangent);
    vec3 N = normalize(normal_matrix * a_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));

    //VS_out.tangent_light_pos = TBN *

}