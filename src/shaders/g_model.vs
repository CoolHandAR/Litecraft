#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_BiTangent;


out VS_Out
{
    //mat3 TBN;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform mat4 u_model;
uniform mat4 u_view;

void main()
{
    vec4 WorldPos = u_model * vec4(a_Pos, 1.0);
    
    //OUT
    vs_out.TexCoords = a_TexCoords;

    //calc tangents
    mat3 normal_matrix = transpose(inverse(mat3(u_view * u_model)));
    vec3 Normal = normal_matrix * a_Normal;
    vec3 T = normalize(normal_matrix * a_Tangent);
    vec3 N = normalize(normal_matrix * a_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    //vs_out.TBN = transpose(mat3(T, B, N));
    vs_out.Normal = Normal;

    gl_Position = g_viewProjectionMatrix * WorldPos;
}