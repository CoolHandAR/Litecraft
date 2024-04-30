#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_BiTangent;

struct VecOutput
{
   vec3 v_FragPos;
   vec3 v_Normal;
   vec2 v_texCoords;
   vec3 v_tangentLightPos;
   vec3 v_tangentViewPos;
   vec3 v_tangentFragPos;
};

layout (location = 0) out VecOutput v_Output;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

uniform vec3 u_viewPos;
uniform vec3 u_lightPos;
uniform mat4 u_model;

void main()
{
    vec4 world_pos = u_model * vec4(a_Pos, 1.0);
    gl_Position = g_viewProjectionMatrix * world_pos;

    //mat3 model_m3 = mat3(u_model);
    mat3 normal_matrix = transpose(inverse(mat3(u_model)));
    //CALCULATE TBN MATRIX
    vec3 T = normalize(normal_matrix  * a_Tangent);
    vec3 N = normalize(normal_matrix  * a_Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    //vec3 T = normalize(model_m3 * a_Tangent);
    //vec3 N = normalize(model_m3 * a_Normal);
   // vec3 B = normalize(model_m3 * a_BiTangent);

    mat3 TBN = transpose(mat3(T, B, N));

    v_Output.v_FragPos = world_pos.xyz;
    v_Output.v_Normal = a_Normal;
    v_Output.v_texCoords = a_TexCoords;

    v_Output.v_tangentLightPos = TBN * u_lightPos;
    v_Output.v_tangentViewPos = TBN * u_viewPos;
    v_Output.v_tangentFragPos = TBN * v_Output.v_FragPos;
}