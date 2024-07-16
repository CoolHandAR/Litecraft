#ifndef R_SHADER_H
#define R_SHADER_H

#include <cglm/cglm.h>

typedef unsigned int R_Shader;


R_Shader Shader_CompileFromMemory(const char* p_vertexShader, const char* p_fragmentShader, const char* p_geometryShader);
R_Shader Shader_CompileFromFile(const char* p_vertexShaderPath, const char* p_fragmentShaderPath, const char* p_geometryShaderPath);
R_Shader ComputeShader_CompileFromMemory(const char* p_computeShader);
R_Shader ComputeShader_CompileFromFile(const char* p_computeShaderPath);
   
void Shader_SetFloat(R_Shader shader, const char* name, float value);
void Shader_SetInteger(R_Shader shader, const char* name, int value);
void Shader_SetUnsigned(R_Shader shader, const char* name, unsigned value);
void Shader_SetVector2f(R_Shader shader, const char* name, float x, float y);
void Shader_SetVector2f_2(R_Shader shader, const char* name, vec2* const value);
void Shader_SetVector3f(R_Shader shader, const char* name, float x, float y, float z);
void Shader_SetVector3f_2(R_Shader shader, const char* name, vec3 const value);
void Shader_SetVector4f(R_Shader shader, const char* name, float x, float y, float z, float w);
void Shader_SetVector4f_2(R_Shader shader, const char* name, vec4* const value);
void Shader_SetMatrix4(R_Shader shader, const char* name, mat4 const matrix);


#endif