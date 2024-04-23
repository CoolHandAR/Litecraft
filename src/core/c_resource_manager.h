#pragma once

#include "render/r_texture.h"

typedef struct C_Resource
{
	char path[256];
	void* handle;

} C_Resource;



R_Texture* RM_getTexture(const char* p_path);


