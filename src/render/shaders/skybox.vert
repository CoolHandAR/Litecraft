#version 460 core

layout (location = 0) in vec3 a_Pos;

out vec3 TexCoords;
out vec3 CubeNormal;

uniform mat4 u_view;
uniform mat4 u_proj;

void main()
{
    TexCoords = a_Pos;
    CubeNormal.z = -1.0;
	CubeNormal.x = CubeNormal.z;
	CubeNormal.y = -CubeNormal.z;

   CubeNormal =  normalize(CubeNormal);

    vec4 position = u_proj * u_view * vec4(a_Pos, 1.0);
    gl_Position = position.xyww;
}