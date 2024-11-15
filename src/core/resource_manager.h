#pragma once

typedef enum
{
	RESOURCE__SOUND,
	RESOURCE__TEXTURE,
	RESOURCE__TEXTURE_HDR,
	RESOURCE__FONT,
	RESOURCE__MODEL,
	RESOURCE__MAX
} ResourceType;

typedef void*(*Resource_fun)(void*);

void* Resource_get(const char* p_path, ResourceType p_resType);
void* Resource_getFromMemory(const char* p_name, void* p_data, size_t p_bufLen, ResourceType p_resType);
void* Resource_getCustom(const char* p_name, Resource_fun p_function, void* p_args, ResourceType p_resType);
void Resource_destruct(const char* p_path);