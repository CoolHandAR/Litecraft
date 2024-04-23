#include "c_resource_manager.h"
#pragma once

#include "utility/u_object_pool.h"
#include <assert.h>

typedef struct TextureResource
{
	char path[256];
	R_Texture texture;
} TextureResource;

typedef struct RM_ResourceManager
{
	dynamic_array* textures;
	int loaded_textures;
	bool init;
} RM_ResourceManager;

static RM_ResourceManager s_RM;

void RM_Init()
{
	memset(&s_RM, 0, sizeof(RM_ResourceManager));

	s_RM.textures = dA_INIT(TextureResource, 0);


	s_RM.init = true;
}

void RM_Exit()
{
	assert(s_RM.init == true);

	for (int i = 0; i < s_RM.textures->elements_size; i++)
	{
		TextureResource* array = s_RM.textures->data;
		Texture_Destruct(&array[i].texture);
	}

	dA_Destruct(s_RM.textures);

}

R_Texture* RM_getTexture(const char* p_path)
{	
	assert(s_RM.init == true);
	

	for (int i = 0; i < s_RM.textures->elements_size; i++)
	{
		TextureResource* array = s_RM.textures->data;
		if (strcmp(array[i].path, p_path) == 0)
		{
			return &array[i].texture;
		}
	}
	//We need to load the texture if not found
	TextureResource* res = dA_emplaceBack(s_RM.textures);
	
	assert(strlen(p_path) < 256 && "Path too long");

	strncpy(res->path, p_path, strlen(p_path));
	res->texture = Texture_Load(p_path, NULL);

	assert(res->texture.id != 0);

	return &res->texture;
}
