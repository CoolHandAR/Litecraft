#include "resource_manager.h"


#include "utility/u_object_pool.h"
#include "utility/u_utility.h"
#include "render/r_texture.h"
#include "render/r_model.h"
#include <assert.h>

typedef struct
{
	char* path;
	ResourceType type;
	int ref_counter;

	void* data;

	struct Resource* next;
	struct Resource* hash_next;
} Resource;

#define MAX_RESOURCES 512
#define MAX_RESOURCE_CHAR_SIZE 256
#define RESOURCES_HASH_SIZE 256

typedef struct
{
	Resource resources[MAX_RESOURCES];
	int index_count;

	Resource* hash_table[RESOURCES_HASH_SIZE];
	Resource* next;

} ResourceManagerCore;

static ResourceManagerCore resource_core;

static  Resource* _findResource(const char* p_resourcePath)
{
	Resource* resource;
	uint32_t hash = 0;

	hash = Hash_string(p_resourcePath);

	hash &= RESOURCES_HASH_SIZE - 1;

	for (resource = resource_core.hash_table[hash]; resource; resource = resource->hash_next)
	{
		if (!_strcmpi(p_resourcePath, resource->path))
		{
			return resource;
		}
	}

	return NULL;
}

static Resource* _createResource(const char* p_string)
{
	Resource* res = NULL;

	res = &resource_core.resources[resource_core.index_count];
	resource_core.index_count++;

	res->path = String_safeCopy(p_string);
	res->next = resource_core.next;
	resource_core.next = res;

	uint32_t hash = Hash_string(p_string);
	hash &= RESOURCES_HASH_SIZE - 1;

	res->hash_next = resource_core.hash_table[hash];
	resource_core.hash_table[hash] = res;

	return res;
}

void* Resource_get(const char* p_path, ResourceType p_resType)
{
	//if the resource exists return it
	Resource* find_resource = _findResource(p_path);
	if (find_resource && find_resource->type == p_resType)
	{
		return find_resource->data;
	}
	
	//otherwise we create it
	void* data = NULL;

	switch (p_resType)
	{
	case RESOURCE__SOUND:
	{
		break;
	}
	case RESOURCE__TEXTURE:
	{
		R_Texture texture = Texture_Load(p_path, NULL);

		//failed to load
		if (texture.id == 0)
		{
			return NULL;
		}

		data = malloc(sizeof(R_Texture));

		if (data)
		{
			memcpy(data, &texture, sizeof(R_Texture));
		}
		
		break;
	}
	case RESOURCE__FONT:
	{
		break;
	}
	case RESOURCE__MODEL:
	{
		R_Model model = Model_Load(p_path, NULL);

		data = malloc(sizeof(R_Model));

		if (data)
		{
			memcpy(data, &model, sizeof(R_Model));
		}
		break;
	}
	default:
		break;
	}

	//failed to load data?
	if (!data)
	{
		return NULL;
	}

	Resource* res = _createResource(p_path);
	res->data = data;
	res->type = p_resType;
	
	return res->data;
}

void* Resource_getFromMemory(const char* p_name, void* p_data, size_t p_bufLen, ResourceType p_resType)
{
	//if the resource exists return it
	Resource* find_resource = _findResource(p_name);
	if (find_resource && find_resource->type == p_resType)
	{
		return find_resource->data;
	}

	//no data found?
	if (!p_data)
	{
		return NULL;
	}

	//otherwise we create it
	void* data = NULL;

	switch (p_resType)
	{
	case RESOURCE__SOUND:
	{
		break;
	}
	case RESOURCE__TEXTURE:
	{
		R_Texture texture = Texture_LoadFromData(p_data, p_bufLen, NULL);

		//failed to load
		if (texture.id == 0)
		{
			return NULL;
		}

		data = malloc(sizeof(R_Texture));

		if (data)
		{
			memcpy(data, &texture, sizeof(R_Texture));
		}

		break;
	}
	case RESOURCE__FONT:
	{
		break;
	}
	case RESOURCE__MODEL:
	{
		
		break;
	}
	default:
		break;
	}

	Resource* res = _createResource(p_name);
	res->data = data;
	res->type = p_resType;

	return res->data;
}

void* Resource_getCustom(const char* p_name, Resource_fun p_function, void* p_args, ResourceType p_resType)
{
	//if the resource exists return it
	Resource* find_resource = _findResource(p_name);
	if (find_resource && find_resource->type == p_resType)
	{
		return find_resource->data;
	}

	void* data = (*p_function)(p_args);
	
	Resource* res = _createResource(p_name);
	res->data = data;
	res->type = p_resType;

	
	return res->data;
}

static void _DestructResource(Resource* res)
{
	if (!res)
	{
		return;
	}

	if (res->path && !String_usingEmptyString(res->path))
	{
		free(res->path);
	}

	if (!res->data)
	{
		return;
	}

	//delete the resource by it's type
	switch (res->type)
	{
	case RESOURCE__SOUND:
	{
		break;
	}
	case RESOURCE__TEXTURE:
	{
		R_Texture* texture_data = res->data;
		Texture_Destruct(texture_data);

		break;
	}
	case RESOURCE__FONT:
	{
		break;
	}
	case RESOURCE__MODEL:
	{
		R_Model* model_data = res->data;
		Model_Destruct(model_data);
		break;
	}
	default:
		break;
	}

	free(res->data);

	//remove from hash table and flat array
	resource_core.next = res->next;

	uint32_t hash = Hash_string2(res->path);
	hash &= RESOURCES_HASH_SIZE;

	resource_core.hash_table[hash] = NULL;

	for (int i = 0; i < resource_core.index_count; i++)
	{
		Resource* array_resource = &resource_core.resources[i];

		if (array_resource == res)
		{
			memset(array_resource, 0, sizeof(Resource));
			break;
		}
	}
}

void Resource_destruct(const char* p_path)
{
	Resource* res = _findResource(p_path);

	_DestructResource(res);
}


void ResourceManager_Init()
{
	memset(&resource_core, 0, sizeof(resource_core));
}

void ResourceManager_Cleanup()
{
	for (int i = 0; i < resource_core.index_count; i++)
	{
		Resource* res = &resource_core.resources[i];

		_DestructResource(res);
	}
}