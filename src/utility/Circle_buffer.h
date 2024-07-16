#pragma once

#include "utility/dynamic_array.h"

typedef struct
{
	dynamic_array* data;
	int read_pos;
	int write_pos;
	int size_mask;
} Circle_Buffer;


Circle_Buffer CircleBuf_Init(size_t p_elementSize, int p_power);
void CircleBuf_Destruct(Circle_Buffer* const cb);
int CircleBuf_SpaceLeft(Circle_Buffer* const cb);
int CircleBuf_DataLeft(Circle_Buffer* const cb);


int CircleBuf_Read(Circle_Buffer* const cb, const void* p_dest, int p_size, bool p_advance);
int CircleBuf_Get(Circle_Buffer* const cb, void* p_dest, int p_size, bool p_advance);
int CircleBuf_Write(Circle_Buffer* const cb, const void* p_src, int p_size);
void CircleBuf_Resize(Circle_Buffer* const cb, int p_power);
