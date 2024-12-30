#ifndef CHM_H
#define CHM_H
#pragma once


/*
Usage
#define DYNAMIC_ARRAY_IMPLEMENTATION
#define CHM_IMPLEMENTATION
#include "Custom_Hashmap.h"
*/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "dynamic_array.h"

typedef uint32_t(*CHMap_HashFun)(const void* _key);
typedef int(*CHMap_CompareFun)(const void* _key, const void* _other);

typedef struct
{
	CHMap_HashFun hash_function;
	CHMap_CompareFun compare_function;
	dynamic_array* item_data;
	dynamic_array* hash_table;
	size_t key_bit_size;
	size_t num_items;
	
	dynamic_array* _item_free_list;

	bool _is_key_string;
	bool _is_item_pooled;
	void* _next;
} CHMap;

#ifdef __cplusplus
extern "C" {
#endif
/**
* Init the custom hashmap
\param HASH_FUN The function that will be used to hash the key. If NULL, the default buffer hash function will be used
\param T The type of key that will be used. 
\param U The type of item that will be stored
\param RESERVE_SIZE The amount of item that can be stored initially
\return SHMap
*/
#define CHMAP_INIT(HASH_FUN, CMP_FUN, T, U, RESERVE_SIZE) _CHMap_Init(HASH_FUN, CMP_FUN, sizeof(T), sizeof(U), RESERVE_SIZE, false)
#define CHMAP_INIT_STRING(U, RESERVE_SIZE) _CHMap_Init(NULL, NULL, 0, sizeof(U), RESERVE_SIZE, false)
#define CHMAP_INIT_POOLED(HASH_FUN, CMP_FUN, T, U, RESERVE_SIZE) _CHMap_Init(HASH_FUN, CMP_FUN, sizeof(T), sizeof(U), RESERVE_SIZE, true)
	extern void* CHMap_Find(CHMap* const chmap, const void* p_key);
	extern int CHMap_getItemIndex(CHMap* const chmap, const void* p_key);
	extern bool CHMap_Has(CHMap* const chmap, const void* p_key);
	extern void* CHMap_Insert(CHMap* const chmap, const void* p_key, const void* p_data);
	extern void CHMap_Erase(CHMap* const chmap, const void* p_key);
	extern void CHMap_Reserve(CHMap* const chmap, size_t p_amount);
	extern void* CHMap_AtIndex(CHMap* const chmap, size_t p_index);
	extern size_t CHMap_Size(CHMap* const chmap);
	extern void CHMap_Clear(CHMap* const chmap);
	extern void CHMap_Destruct(CHMap* chmap);

#ifdef __cplusplus
}
#endif

typedef struct
{
	unsigned long long data_array_index;
	void* key;

	struct _CHMap_item* hash_next;
	struct _CHMap_item* next;
	struct _CHMap_item* prev;
} _CHMap_item;

#define CHM_HASHTABLE_BLOCK_ALLOCATION_SIZE 256
static CHMap _CHMap_Init(CHMap_HashFun p_hashFun, CHMap_CompareFun p_cmpFun,size_t p_keyBitSize, size_t p_allocSize, size_t p_initReserveSize, bool p_useItemPooling)
{
	assert(p_allocSize > 0);

	CHMap map;
	memset(&map, 0, sizeof(CHMap));
	map._next = NULL;

	map.item_data = dA_INIT2(p_allocSize, p_initReserveSize);

	size_t hash_table_size = max(p_initReserveSize, CHM_HASHTABLE_BLOCK_ALLOCATION_SIZE);
	map.hash_table = dA_INIT(sizeof(_CHMap_item*), hash_table_size);

	map.hash_function = p_hashFun;
	map.compare_function = p_cmpFun;
	map.key_bit_size = p_keyBitSize;

	if (map.key_bit_size == 0)
	{
		map._is_key_string = true;
	}

	if (p_useItemPooling)
	{
		map._item_free_list = dA_INIT(unsigned, 0);
		map._is_item_pooled = true;
	}

	return map;
}

/*
This is for preventing greying out of the implementation section.
*/
#if defined(Q_CREATOR_RUN) || defined(__INTELLISENSE__) || defined(__CDT_PARSER__)
#define CHM_IMPLEMENTATION
#endif

#if defined(CHM_IMPLEMENTATION)
#ifndef CHM_C
#define CHM_C

uint32_t _CHMap_Hash_string(const char* p_str)
{
	//src: https://theartincode.stanis.me/008-djb2/

	const unsigned char* chr = (const unsigned char*)p_str;

	uint32_t hash = 5381;
	uint32_t c = *chr++;

	while (c)
	{
		hash = ((hash << 5) + hash) ^ c; /* hash * 33 ^ c */
		c = *chr++;
	}

	return hash;
}

//src https://github.com/godotengine/godot/blob/master/core/templates/hashfuncs.h
uint32_t _CHMap_hash_rotl32(uint32_t x, int8_t r)
{
	return (x << r) | (x >> (32 - r));
}
uint32_t _CHMap_hash_fmix32(uint32_t h) {
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

uint32_t _CHMap_hash_buffer(const void* key, int length)
{
	uint32_t seed = 0x7F07C65;

	const uint8_t* data = (const uint8_t*)key;
	const int nblocks = length / 4;

	uint32_t h1 = seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

	for (int i = -nblocks; i; i++) {
		uint32_t k1 = blocks[i];

		k1 *= c1;
		k1 = _CHMap_hash_rotl32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = _CHMap_hash_rotl32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

	uint32_t k1 = 0;

	switch (length & 3) {
	case 3:
		k1 ^= tail[2] << 16;
		//FALLTHROUGH
	case 2:
		k1 ^= tail[1] << 8;
		//FALLTHROUGH
	case 1:
		k1 ^= tail[0];
		k1 *= c1;
		k1 = _CHMap_hash_rotl32(k1, 15);
		k1 *= c2;
		h1 ^= k1;
	};

	// Finalize with additional bit mixing.
	h1 ^= length;

	return _CHMap_hash_fmix32(h1);
}

uint32_t _CHMap_getNewItemIndex(CHMap* const chmap)
{
	uint32_t r_index = 0;
	if (chmap->_is_item_pooled && chmap->_item_free_list->elements_size > 0)
	{
		unsigned new_free_list_size = chmap->_item_free_list->elements_size - 1;
		unsigned* index = dA_at(chmap->_item_free_list, new_free_list_size);

		dA_resize(chmap->_item_free_list, new_free_list_size);

		r_index = *index;
	}
	else
	{
		dA_emplaceBack(chmap->item_data);
		r_index = chmap->item_data->elements_size - 1;
	}

	return r_index;
}

void _CHMap_eraseItem(CHMap* const chmap, uint32_t p_index)
{
	if (chmap->_is_item_pooled)
	{
		//add the index id to the free list
		unsigned* emplaced = dA_emplaceBack(chmap->_item_free_list);
		*emplaced = p_index;
	}
	else
	{
		dA_erase(chmap->item_data, p_index, 1);
	}
}

uint32_t CHMap_Hash(CHMap* const chmap, const void* p_key)
{
	uint32_t hash = 0;

	//if a hash function is provided, call it
	if (chmap->hash_function)
	{
		hash = (*chmap->hash_function)((const int*)p_key);
	}
	//string
	else if (chmap->_is_key_string)
	{
		hash = _CHMap_Hash_string(p_key);
	}
	//otherwise we will do hash function on the buffer
	else
	{
		hash = _CHMap_hash_buffer(p_key, chmap->key_bit_size);
	}

	return hash;
}

bool _CHMap_Compare(CHMap* const chmap, const void* p_key, const void* p_otherKey)
{
	//If a compare function is provided, call it
	if (chmap->compare_function)
	{
		return (*chmap->compare_function)(p_key, p_otherKey);
	}
	else if (chmap->_is_key_string)
	{
		return strcmp(p_key, p_otherKey) == 0;
	}

	return memcmp(p_key, p_otherKey, chmap->key_bit_size) == 0;
}

void* _CHMap_getItemData(CHMap* const chmap, _CHMap_item* item)
{
	return dA_at(chmap->item_data, item->data_array_index);
}

_CHMap_item* _CHMap_findItem(CHMap* const chmap, const void* p_key)
{
	uint32_t hash = CHMap_Hash(chmap, p_key);
	hash &= chmap->hash_table->capacity - 1;

	_CHMap_item** hash_array = chmap->hash_table->data;

	_CHMap_item* item;
	for (item = hash_array[hash]; item; item = item->hash_next)
	{
		//found
		if (*(char*)item->key == *(char*)p_key && _CHMap_Compare(chmap, item->key, p_key))
		{
			return item;
		}
	}

	return NULL;
}

void _CHMap_hashTableReserve(CHMap* const chmap, size_t p_toReserve)
{
	size_t old_hash_table_capacity = chmap->hash_table->capacity;
	size_t new_hash_table_capacity = chmap->hash_table->capacity + p_toReserve;
	
	dA_reserve(chmap->hash_table, p_toReserve);
	memset(chmap->hash_table->data, 0, chmap->hash_table->alloc_size * chmap->hash_table->capacity);

	_CHMap_item** hash_array = chmap->hash_table->data;

	//Rehash all items
	_CHMap_item* item = NULL;
	for (item = chmap->_next; item; item = item->next)
	{
		uint32_t item_hash = CHMap_Hash(chmap, item->key);

		uint32_t index = item_hash & (chmap->hash_table->capacity - 1);

		item->hash_next = hash_array[index];
		hash_array[index] = item;
	}
}

void* CHMap_Find(CHMap* const chmap, const void* p_key)
{
	_CHMap_item* find_item = _CHMap_findItem(chmap, p_key);

	if (!find_item)
	{
		return NULL;
	}

	return _CHMap_getItemData(chmap, find_item);
}


int CHMap_getItemIndex(CHMap* const chmap, const void* p_key)
{
	_CHMap_item* find_item = _CHMap_findItem(chmap, p_key);

	if (!find_item)
	{
		return -1;
	}

	return find_item->data_array_index;
}

void* CHMap_getItemAtIndex(CHMap* const chmap, size_t p_index)
{
	if (chmap->_is_item_pooled)
	{
		if (chmap->_item_free_list->elements_size > 0)
		{
			for (int i = 0; i < chmap->_item_free_list->elements_size; i++)
			{
				unsigned* index = dA_at(chmap->_item_free_list, i);

				if (*index == p_index)
				{
					return NULL;
				}
			}
		}
	}

	return (char*)dA_at(chmap->item_data, p_index);
}

bool CHMap_Has(CHMap* const chmap, const void* p_key)
{
	_CHMap_item* find_item = _CHMap_findItem(chmap, p_key);

	return find_item != NULL;
}
void* CHMap_Insert(CHMap* const chmap, const void* p_key, const void* p_data)
{
	_CHMap_item* find_item = _CHMap_findItem(chmap, p_key);

	//if it exists already, return the data
	if (find_item)
	{
		return _CHMap_getItemData(chmap, find_item);
	}
	_CHMap_item* item = NULL;

	item = malloc(sizeof(_CHMap_item));

	if (!item)
	{
		return NULL;
	}
	memset(item, 0, sizeof(_CHMap_item));

	if (chmap->_is_key_string)
	{
		item->key = malloc(strlen(p_key) + 1);
	}
	else
	{
		item->key = malloc(chmap->key_bit_size);
	}

	if (!item->key)
	{
		free(item);
		return NULL;
	}
	//copy the key data
	if (chmap->_is_key_string)
	{
		strcpy(item->key, p_key);
	}
	else
	{
		memcpy(item->key, p_key, chmap->key_bit_size);
	}
	

	//emplace the item in the data array
	uint32_t item_index = _CHMap_getNewItemIndex(chmap);

	void* data_ptr = dA_at(chmap->item_data, item_index);
	memcpy(data_ptr, p_data, chmap->item_data->alloc_size);
	item->data_array_index = item_index;

	if (chmap->hash_table->capacity + 1 < chmap->item_data->elements_size)
	{
		_CHMap_hashTableReserve(chmap, chmap->hash_table->capacity + CHM_HASHTABLE_BLOCK_ALLOCATION_SIZE);
	}

	uint32_t hash = CHMap_Hash(chmap, p_key);
	hash &= chmap->hash_table->capacity - 1;

	_CHMap_item** hash_array = chmap->hash_table->data;

	item->hash_next = hash_array[hash];
	hash_array[hash] = item;
	
	item->next = chmap->_next;
	if (item->next)
	{
		_CHMap_item* next = item->next;
		next->prev = item;
	}
	chmap->_next = item;

	chmap->num_items++;

	return data_ptr;
}

void CHMap_Erase(CHMap* const chmap, const void* p_key)
{
	//need to remove this
	if (!CHMap_Find(chmap, p_key))
	{
		return;
	}

	_CHMap_item** hash_array = chmap->hash_table->data;

	uint32_t hash = CHMap_Hash(chmap, p_key);
	hash &= chmap->hash_table->capacity - 1;

	bool item_found = false;
	unsigned long long data_array_index = 0;
	//remove from the hash table
	_CHMap_item* hash_prev = NULL;
	_CHMap_item* item = NULL;
	for (item = hash_array[hash]; item; item = item->hash_next)
	{
		//Found the item
		if (*(char*)item->key == *(char*)p_key && _CHMap_Compare(chmap, item->key, p_key))
		{
			data_array_index = item->data_array_index;

			//if we have a previous hash, give the hash next to it
			if (hash_prev)
			{
				hash_prev->hash_next = item->hash_next;
			}
			//otherwise we are the top most hash
			//so set the hash next to the top
			else
			{
				hash_array[hash] = item->hash_next;
			}

			//if we have a previous item, assign the next to it
			if (item->prev)
			{
				_CHMap_item* prev = item->prev;
				prev->next = item->next;
				
				if (item->next)
				{
					_CHMap_item* next = item->next;
					next->prev = item->prev;
				}
			}
			//otherwise it means we are the core next item
			else
			{
				chmap->_next = item->next;

				if (item->next)
				{
					_CHMap_item* next = item->next;
					next->prev = NULL;
				}
			}

			//erase the item data
			_CHMap_eraseItem(chmap, item->data_array_index);

			if (item->key)
			{
				free(item->key);
			}

			free(item);

			item_found = true;
			break;
		}

		hash_prev = item;
	}
	item = NULL;

	if (!item_found)
	{
		return;
	}

	chmap->num_items--;

	if (!chmap->_is_item_pooled)
	{
		//slow but, it checkes for every item in hash table,
		//looking if the data array index is bigger than the just deleted one
		//and decreases the index number by one
		for (item = chmap->_next; item; item = item->next)
		{
			if (item->data_array_index > data_array_index)
			{
				item->data_array_index--;
			}
		}
	}
}

void CHMap_Reserve(CHMap* const chmap, size_t p_amount)
{
	dA_reserve(chmap->item_data, p_amount);
}

void* CHMap_AtIndex(CHMap* const chmap, size_t p_index)
{
	return dA_at(chmap->item_data, p_index);
}

size_t CHMap_Size(CHMap* const chmap)
{
	return chmap->item_data->elements_size;
}

void CHMap_Clear(CHMap* const chmap)
{
	dA_clear(chmap->item_data);

	memset(chmap->hash_table, 0, sizeof(_CHMap_item*) * chmap->hash_table->capacity);
}

void CHMap_Destruct(CHMap* chmap)
{
	_CHMap_item* item = chmap->_next;

	while (item)
	{
		_CHMap_item* next = item->next;

		if (item->key)
		{
			free(item->key);
		}

		free(item);

		item = next;
	}

	dA_Destruct(chmap->hash_table);
	dA_Destruct(chmap->item_data);

	if (chmap->_is_item_pooled)
	{
		dA_Destruct(chmap->_item_free_list);
	}
}

#endif
#endif
#endif