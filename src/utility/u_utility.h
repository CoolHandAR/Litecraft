#ifndef UTILITY_H
#define UTILITY_H
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "utility/dynamic_array.h"
#include <cglm/cglm.h>

/*
~~~~~~~~~~~~~
FILE UTILITES
~~~~~~~~~~~~~
*/
unsigned char* File_Parse(const char* p_filePath, int* r_length);
bool File_PrintString(const char* p_string, const char* p_filePath);
int File_GetLength(FILE* p_file);

/*
~~~~~~~~~~~~~
STRING UTILITES
~~~~~~~~~~~~~
*/
bool String_StartsWith(const char* p_str, const char* p_starts, bool p_caseSensitive);
bool String_Contains(const char* p_str, const char* p_contains);
char* String_safeCopy(const char* p_source);
bool String_usingEmptyString(const char* p_str);
int String_findLastOfIndex(const char* p_str, int p_char);
int String_findFirstOfIndex(const char* p_str, int p_char);

/*
~~~~~~~~~~~~~
HASHING UTILITES
~~~~~~~~~~~~~
*/
size_t Hash_stringSeeded(char* p_str, size_t p_seed, unsigned p_max);
size_t Hash_string(char* p_str);
uint32_t Hash_string2(const char* p_str);
uint32_t Hash_uint32(uint32_t p_in, uint32_t p_seed);
uint32_t Hash_float(float p_in, uint32_t p_seed);
uint32_t Hash_vec3(vec3 v);
uint32_t Hash_ivec3(ivec3 v);
uint32_t Hash_id(uint32_t x);
uint32_t Hash_uint64(uint64_t x);
/*
~~~~~~~~~~~~~
GL UTILITES
~~~~~~~~~~~~~
*/
//Render storage buffer is used to manage a glBuffer in which the data is always the same
//This should be used for UBO'S or SSBO's. Putting multiple data in the same buffer gives a big perfomance boost in a gl setting.
//Compute shaders really benefit from it

typedef enum
{
	RSB_FLAG__NONE = 0,
	RSB_FLAG__RESIZABLE = 1 << 0,
	RSB_FLAG__USE_CPU_BACK_BUFFER = 1 << 1,
	RSB_FLAG__WRITABLE = 1 << 2,
	RSB_FLAG__READABLE = 1 << 3,
	RSB_FLAG__PERSISTENT = 1 << 4,
	RSB_FLAG__ALWAYS_MAP_TO_MAX_RESERVE = 1 << 5,
	RSB_FLAG__POOLABLE = 1 << 6
} RSB_Flags;

typedef struct
{	
	size_t item_size; //byte size of a singular item
	size_t used_size; //the current amount of items stored
	size_t reserve_size; //max amount of items in the buffer 
	unsigned buffer; //gl buffer handle
	unsigned buffer_flags; //flags used when initializing the buffer
	unsigned rsb_flags;

	void* _back_buffer; //Internal
	void* _data_map;  //Internal
	unsigned _map_flags;  //Internal
	size_t _map_offset; //Internal
	size_t _map_size; //Internal
	dynamic_array* free_list;
} RenderStorageBuffer;

RenderStorageBuffer RSB_Create(size_t p_initReserveSize, size_t p_itemSize, unsigned p_rsbFlags);
void RSB_Destruct(RenderStorageBuffer* const rsb);
unsigned RSB_Request(RenderStorageBuffer* const rsb);
void RSB_FreeItem(RenderStorageBuffer* const rsb, unsigned p_index, bool p_zeroMem);
void RSB_Resize(RenderStorageBuffer* const rsb, unsigned p_newSize);
void RSB_IncreaseSize(RenderStorageBuffer* const rsb, unsigned p_toAdd);
void* RSB_Map(RenderStorageBuffer* const rsb, unsigned p_mapFlags);
void* RSB_MapRange(RenderStorageBuffer* const rsb, size_t p_offset, size_t p_size, unsigned p_mapFlags);
void* RSB_MapIndex(RenderStorageBuffer* const rsb, unsigned p_index, unsigned p_mapFlags);
void RSB_Unmap(RenderStorageBuffer* const rsb);

typedef enum
{
	DRB_FLAG__NONE = 0,
	DRB_FLAG__RESIZABLE = 1 << 0,
	DRB_FLAG__USE_CPU_BACK_BUFFER = 1 << 1,
	DRB_FLAG__WRITABLE = 1 << 2,
	DRB_FLAG__READABLE = 1 << 3,
	DRB_FLAG__PERSISTENT = 1 << 4,
	DRB_FLAG__ALWAYS_MAP_TO_MAX_RESERVE = 1 << 5,
	DRB_FLAG__POOLABLE = 1 << 6,
} DRB_Flags;

typedef struct
{
	size_t offset;
	size_t count;
} DRB_Item;

typedef struct
{
	size_t used_bytes;
	size_t reserve_size;
	unsigned buffer;
	unsigned buffer_flags;
	dynamic_array* item_list;

	//internals
	unsigned drb_flags;
	void* _data_map;
	unsigned _map_flags;
	size_t _map_offset;
	size_t _map_size;
	void* _back_buffer;
	size_t _modified_offset;
	size_t _modified_size;
	size_t _resize_chunk_size;
	dynamic_array* _free_list;
} DynamicRenderBuffer;

DynamicRenderBuffer DRB_Create(size_t p_initReserveSize, size_t p_initItemCount, unsigned p_drbFlags);
void DRB_Destruct(DynamicRenderBuffer* const drb);
unsigned DRB_EmplaceItem(DynamicRenderBuffer* const drb, size_t p_len, const void* p_data);
void DRB_ChangeData(DynamicRenderBuffer* const drb, size_t p_len, const void* p_data, unsigned p_drbItemIndex);
void DRB_RemoveItem(DynamicRenderBuffer* const drb, unsigned p_drbItemIndex);
void* DRB_Map(DynamicRenderBuffer* const drb, unsigned p_mapFlags);
void* DRB_MapReserve(DynamicRenderBuffer* const drb, unsigned p_mapFlags);
void* DRB_MapRange(DynamicRenderBuffer* const drb, size_t p_offset, size_t p_size, unsigned p_mapFlags);
void* DRB_MapItem(DynamicRenderBuffer* const drb, unsigned p_drbItemIndex, unsigned p_mapFlags);
DRB_Item DRB_GetItem(DynamicRenderBuffer* const drb, unsigned p_drbItemIndex);
void DRB_Unmap(DynamicRenderBuffer* const drb);
void DRB_WriteDataToGpu(DynamicRenderBuffer* const drb);
void DRB_setResizeChunkSize(DynamicRenderBuffer* const drb, size_t p_chunkSize);

#endif