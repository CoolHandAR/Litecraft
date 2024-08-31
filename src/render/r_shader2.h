#pragma once

#include "utility/Custom_Hashmap.h"

typedef struct
{
	unsigned vert_id;
	unsigned fragment_id;
	unsigned geo_id;
	unsigned program_id;
} ShaderVersion;

typedef struct
{
	unsigned gl_id;
	CHMap uniforms_map;

	int uniform_count;

} Shader2;