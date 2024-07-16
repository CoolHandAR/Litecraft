#pragma once

#include "utility/dynamic_array.h"

typedef struct Object_Pool
{
	dynamic_array* pool;
	dynamic_array* free_list;

	unsigned used_pool_size;
} Object_Pool;

#define Object_Pool_INIT(T, SIZE) _objPoolInit(sizeof(T), SIZE);

Object_Pool* _objPoolInit(size_t alloc_size, unsigned init_size);
unsigned Object_Pool_Request(Object_Pool* const p_pool);
void Object_Pool_Free(Object_Pool* const p_pool, unsigned p_index, bool p_zeroMem);
void Object_Pool_ClearAll(Object_Pool* const p_pool);
void Object_Pool_Destruct(Object_Pool* p_pool);