#pragma once

#include <stdbool.h>
#include "utility/dynamic_array.h"

/*
~~~~~~~~~~~~~
FILE UTILITES
~~~~~~~~~~~~~
*/
char* File_ParseString(const char* p_filePath);
bool File_PrintString(const char* p_string, const char* p_filePath);

/*
~~~~~~~~~~~~~
STRING UTILITES
~~~~~~~~~~~~~
*/
bool String_StartsWith(const char* p_str, const char* p_starts, bool p_caseSensitive);
char* String_safeCopy(const char* p_source);
bool String_usingEmptyString(const char* p_str);

/*
~~~~~~~~~~~~~
HASHING UTILITES
~~~~~~~~~~~~~
*/
size_t Hash_stringSeeded(char* p_str, size_t p_seed, unsigned p_max);
size_t Hash_string(char* p_str, unsigned p_max);

/*
~~~~~~~~~~~~~
GL UTILITES
~~~~~~~~~~~~~
*/
//Render storage buffer is used to manage a glBuffer in which the data is always the same
//This should be used for UBO'S or GLO's. Putting multiple data in the same buffer gives a big perfomance boost in a gl setting.
//Compute shaders really benefit
typedef struct
{	
	size_t item_size; //byte size of a singular item
	size_t used_size; //the current amount of items stored
	size_t reserve_size; //max amount of items in the buffer 
	unsigned buffer; //gl buffer handle
	unsigned buffer_flags; //flags used when initializing the buffer
	bool resizable; //can the buffer be resized
	bool _mapped; //Internal. Used to mark that the buffer is currently mapped
	size_t _map_offset; //Internal
	size_t _map_size; //Internal
	dynamic_array* free_list;
} RenderStorageBuffer;

RenderStorageBuffer RSB_Create(size_t p_initReserveSize, size_t p_itemSize, bool p_readable, bool p_writable, bool p_persistent, bool p_resizable);
void RSB_Destruct(RenderStorageBuffer* const rsb);
unsigned RSB_Request(RenderStorageBuffer* const rsb);
void RSB_FreeItem(RenderStorageBuffer* const rsb, unsigned p_index, bool p_zeroMem);
void RSB_Resize(RenderStorageBuffer* const rsb, unsigned p_newSize);
void* RSB_Map(RenderStorageBuffer* const rsb, unsigned p_mapFlags);
void* RSB_MapRange(RenderStorageBuffer* const rsb, size_t p_offset, size_t p_size, unsigned p_mapFlags);
void* RSB_MapIndex(RenderStorageBuffer* const rsb, unsigned p_index, unsigned p_mapFlags);
void RSB_Unmap(RenderStorageBuffer* const rsb);

