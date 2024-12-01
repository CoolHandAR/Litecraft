#version 460 core

#include "scene_incl.incl"

//ATTRIBUTES
layout (location = 0) in vec3 a_Pos;

#ifdef TEXCOORD_ATTRIB
layout (location = 1) in vec2 a_TexCoords;
#endif //TEXCOORD_ATTRIB

#ifdef COLOR_ATTRIB
#ifdef COLOR_8BIT
layout (location = 2) in uvec4 a_Color;
#else
layout (location = 2) in vec4 a_Color;
#endif //COLOR_8BIT
#endif //COLOR_ATTRIB

#ifdef NORMAL_ATTRIB
layout (location = 3) in vec3 a_Normal;
#endif //NORMAL_ATTRIB

#ifdef TANGENT_ATTRIB
layout (location = 4) in vec3 a_Tangent;
#endif //TANGENT_ATTRIB

#ifdef BITANGENT_ATTRIB
layout (location = 5) in vec3 a_BiTangent;
#endif //BITANGENT_ATTRIB

#ifdef TEXTUREINDEX_ATTRIB
layout (location = 6) in int a_TexIndex;
#endif //TEXTUREINDEX_ATTRIB

#ifdef INSTANCE_MAT3
layout (location = 7) in vec4 a_InstanceMatrix1;
layout (location = 8) in vec4 a_InstanceMatrix2;
layout (location = 9) in vec4 a_InstanceMatrix3;
#endif //INSTANCE_MAT3

#ifdef INSTANCE_UV
layout (location = 10) in vec4 a_InstanceUV;
#endif //INSTANCE_UV

#ifdef INSTANCE_COLOR
layout (location = 11) in vec4 a_InstanceColor;
#endif //INSTANCE_UV

#ifdef INSTANCE_CUSTOM
layout (location = 12) in vec4 a_InstanceCustom;
#endif //INSTANCE_UV

//UNIFORMS
#ifdef USE_UNIFORM_CAMERA_MATRIX
uniform mat4 u_cameraMatrix;
#endif

//VERTEX OUTPUT
out VS_OUT
{
#ifndef RENDER_DEPTH
    vec4 Color;
#endif

#ifdef TEXCOORD_ATTRIB
    vec2 TexCoords;
#endif //TEXCOORD_ATTRIB
    
#ifdef NORMAL_ATTRIB
    vec3 Normal;
#endif //Normal

#if defined(TEXTUREINDEX_ATTRIB) || defined(INSTANCE_CUSTOM) 
    flat int TexIndex;
#endif //TEXTUREINDEX_ATTRIB

} vs_out;

void main()
{
    mat4 matrix = mat4(1.0);
    mat4 projection_matrix = mat4(1.0);

#ifdef COLOR_ATTRIB
#ifdef COLOR_8BIT
    vec4 fColor = vec4(a_Color);
    vs_out.Color = vec4(fColor.r / 255.0, fColor.g / 255.0, fColor.b / 255.0, fColor.a / 255.0);
#else
    vs_out.Color = a_Color;
#endif //COLOR_8BIT
#else
#ifndef RENDER_DEPTH
    vs_out.Color = vec4(1, 1, 1, 1);
#endif
#endif //COLOR_ATTRIB

#ifdef TEXCOORD_ATTRIB
    vs_out.TexCoords = vec2(a_TexCoords.x, a_TexCoords.y);
#endif //TEXCOORD_ATTRIB

#ifdef TEXTUREINDEX_ATTRIB
    vs_out.TexIndex = a_TexIndex;
#endif //TEXTUREINDEX_ATTRIB

#ifdef INSTANCE_MAT3
    matrix = mat4(a_InstanceMatrix1, a_InstanceMatrix2, a_InstanceMatrix3, vec4(0.0, 0.0, 0.0, 1.0));
    matrix = transpose(matrix);
#endif //INSTANCE_MAT3

#ifdef INSTANCE_UV
    vec2 coords = vec2(a_TexCoords.x + a_InstanceUV.x, a_TexCoords.y + a_InstanceUV.y) / vec2(a_InstanceUV.z, a_InstanceUV.w);
    vs_out.TexCoords = coords;
#endif //INSTANCE UV

#ifdef INSTANCE_COLOR
    vs_out.Color = a_InstanceColor;
#endif //INSTANCE COLOR

#ifdef INSTANCE_CUSTOM
    vs_out.TexIndex = int(a_InstanceCustom.x);
#endif //INSTANCE_CUSTOM

#ifdef BILLBOARD_SHADOWS
      //Extract scale and position from model matrix
    vec3 scale = vec3(length(matrix[0]), length(matrix[1]), length(matrix[2]));
    vec3 position = matrix[3].xyz;

     //Rebuild the matrix to ignore rotations
     matrix = mat4(1.0);
     matrix[0].xyz *= scale.x;
     matrix[1].xyz *= scale.y;
     matrix[2].xyz *= scale.z;
     matrix[3].xyz = position;
   
     vec3 up = vec3(0.0, 1.0, 0.0);
     vec3 dirLightDirection = normalize(-scene.dirLightDirection.xyz);
     mat3 local = mat3(normalize(cross(up, dirLightDirection)), up, dirLightDirection);
     //Rotate the matrix so that it will always face the dir light direction
     //this allows our shadows to always look flat and ignore stuff like billboarding
     local = local * mat3(matrix);
     matrix[0].xyz = local[0];
     matrix[1].xyz = local[1];
     matrix[2].xyz = local[2];
#endif

#if defined(NORMAL_ATTRIB)
     mat3 normalMatrix = transpose(inverse(mat3(matrix)));
     vs_out.Normal = normalMatrix * a_Normal;
#endif

#ifdef USE_UNIFORM_CAMERA_MATRIX
    projection_matrix = u_cameraMatrix;
#else
    projection_matrix = cam.viewProjection;
#endif

    gl_Position = projection_matrix * matrix * vec4(a_Pos, 1.0);
}
