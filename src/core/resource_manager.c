#include "core/resource_manager.h"

#include "utility/u_object_pool.h"
#include "utility/u_utility.h"
#include "utility/Custom_Hashmap.h"
#include "render/r_texture.h"
#include "core/sound.h"

typedef struct
{
	ResourceType type;
	int ref_counter;

	void* data;
} Resource;

typedef struct
{
	CHMap resource_map;

} ResourceManagerCore;

static ResourceManagerCore resource_core;

void* Resource_get(const char* p_path, ResourceType p_resType)
{
	//if the resource exists return it
	Resource* find_resource = CHMap_Find(&resource_core.resource_map, p_path);
	if (find_resource)
	{
		assert(find_resource->type == p_resType);

		return find_resource->data;
	}
	
	//otherwise we create it
	void* data = NULL;

	switch (p_resType)
	{
	case RESOURCE__SOUND:
	{
		
		data = malloc(sizeof(ma_sound));

		if (data)
		{
			if (!Sound_load(p_path, 0, data))
			{
				return NULL;
			}
		}


		break;
	}
	case RESOURCE__TEXTURE_HDR:
	case RESOURCE__TEXTURE:
	{
		R_Texture texture;
		texture.id = 0;
		
		if (p_resType == RESOURCE__TEXTURE_HDR)
		{
			texture = HDRTexture_Load(p_path, NULL);
		}
		else
		{
			texture = Texture_Load(p_path, NULL);
		}

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
		//R_Model model = Model_Load(p_path, NULL);

		//data = malloc(sizeof(R_Model));

		if (data)
		{
			//memcpy(data, &model, sizeof(R_Model));
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
	Resource res;
	memset(&res, 0, sizeof(res));
	res.data = data;
	res.type = p_resType;
	CHMap_Insert(&resource_core.resource_map, p_path, &res);
	
	return res.data;
}

void* Resource_getFromMemory(const char* p_name, void* p_data, size_t p_bufLen, ResourceType p_resType)
{
	//if the resource exists return it
	Resource* find_resource = NULL;
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

	//Resource* res = _createResource(p_name);
	//res->data = data;
	//res->type = p_resType;

	//return res->data;
}

void* Resource_getCustom(const char* p_name, Resource_fun p_function, void* p_args, ResourceType p_resType)
{
	//if the resource exists return it
	//Resource* find_resource = _findResource(p_name);
	//if (find_resource && find_resource->type == p_resType)
	//{
	//	return find_resource->data;
	//}

	//void* data = (*p_function)(p_args);
	
	//Resource* res = _createResource(p_name);
	//res->data = data;
	//res->type = p_resType;

	
	//return res->data;

	return NULL;
}

static void Resource_destroy(Resource* res)
{
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
		//R_Model* model_data = res->data;
		//Model_Destruct(model_data);
		break;
	}
	default:
		break;
	}
}

void Resource_destruct(const char* p_path)
{
	Resource* res = CHMap_Find(&resource_core.resource_map, p_path);

	if (!res)
	{
		return;
	}

	Resource_destroy(res);

	CHMap_Erase(&resource_core.resource_map, p_path);
}

static bool stringCompare(const char* p_first, const char* p_second)
{
	return strcmp(p_first, p_second) == 0;
}

void ResourceManager_Init()
{
	memset(&resource_core, 0, sizeof(resource_core));

	resource_core.resource_map = CHMAP_INIT_STRING(Resource, 1);
}

void ResourceManager_Cleanup()
{
	for (int i = 0; i < resource_core.resource_map.num_items; i++)
	{
		Resource* res = resource_core.resource_map.item_data->data;
		Resource_destroy(res);
	}

	CHMap_Destruct(&resource_core.resource_map);
}