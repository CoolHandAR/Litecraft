#ifndef RSHADER_H
#define RSHADER_H
#pragma once

#include <stdint.h>
#include "utility/Custom_Hashmap.h"
#include <cglm/cglm.h>

typedef struct
{
	uint64_t key;
	int* uniforms_locations;

	unsigned program_id;
} ShaderVariant;

typedef struct
{
	unsigned char* vertex_code;
	unsigned char* fragment_code;
	unsigned char* geo_code;
	unsigned char* compute_code;
	
	int vertex_length;
	int fragment_length;
	int geo_length;
	int compute_length;

	uint64_t current_variant_key;
	uint64_t new_variant_key;
	
	int max_uniforms;
	int max_defines;
	int max_tex_units;

	CHMap variant_map;

	bool is_compute;
	bool is_loaded;

	const char** define_names;
	const char** uniform_names;
	const char** tex_unit_names;

	ShaderVariant* active_variant;

} RShader;

RShader Shader_PixelCreate(const char* vert_src, const char* frag_src, int max_defines, int max_uniforms, int max_texunits, const char** defines, const char** uniforms, const char** tex_units, bool* r_result);
RShader Shader_ComputeCreate(const char* comp_src, int max_defines, int max_uniforms, int max_texunits, const char** defines, const char** uniforms, const char** tex_units, bool* r_result);


void Shader_Destruct(RShader* const shader);

void Shader_Use(RShader* const shader);

int Shader_GetUniformLocation(RShader* const shader, int uniform);

void Shader_SetInt(RShader* const shader, int uniform, int value);
//void Shader_SetUnsigned(RShader* const shader, int uniform, unsigned value);
void Shader_SetFloaty(RShader* const shader, int uniform, float value);

void Shader_SetFloat2(RShader* const shader, int uniform, float x, float y);
void Shader_SetFloat3(RShader* const shader, int uniform, float x, float y, float z);
void Shader_SetFloat4(RShader* const shader, int uniform, float x, float y, float z, float w);

void Shader_SetVec2(RShader* const shader, int uniform, vec2 value);
void Shader_SetVec3(RShader* const shader, int uniform, vec3 value);
void Shader_SetVec4(RShader* const shader, int uniform, vec4 value);

void Shader_SetMat4(RShader* const shader, int uniform, mat4 value);


inline void Shader_SetDefine(RShader* const shader, int define, bool state)
{
	if (state)
	{
		shader->new_variant_key |= (1 << define);
	}
	else
	{
		shader->new_variant_key &= ~(1 << define);
	}
}

inline void Shader_ResetDefines(RShader* const shader)
{
	shader->new_variant_key = 0;
}

#endif