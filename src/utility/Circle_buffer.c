#include "utility/Circle_buffer.h"

#include <stdint.h>

static int _advance(int *_var, int _size, int _mask)
{
	int ret = *_var;
	*_var += _size;
	*_var = *_var & _mask;
	return ret;
}


Circle_Buffer CircleBuf_Init(size_t p_elementSize, int p_power)
{
	Circle_Buffer cb;
	memset(&cb, 0, sizeof(Circle_Buffer));

	cb.data = dA_INIT2(p_elementSize, 0);

	CircleBuf_Resize(&cb, p_power);

	return cb;
}

void CircleBuf_Destruct(Circle_Buffer* const cb)
{
	dA_Destruct(cb->data);
}

int CircleBuf_SpaceLeft(Circle_Buffer* const cb)
{
	int left = cb->read_pos - cb->write_pos;
	if (left < 0)
	{
		return cb->data->elements_size + left - 1;
	}
	if (left == 0)
	{
		return cb->data->elements_size - 1;
	}

	return left - 1;
}


int CircleBuf_DataLeft(Circle_Buffer* const cb)
{
	return cb->data->elements_size - CircleBuf_SpaceLeft(cb) - 1;
}


int CircleBuf_Read(Circle_Buffer* const cb, const void* p_dest, int p_size, bool p_advance)
{
	int left = CircleBuf_DataLeft(cb);
	int size = min(left, p_size);
	int pos = cb->read_pos;
	int to_read = size;
	void* dest = p_dest;
	void* read = cb->data->data;
	while (to_read)
	{
		int end = pos + to_read;
		end = min(end, cb->data->elements_size);
		int total = end - pos;
		
		for (int i = 0; i < total; i++)
		{
			memcpy(dest, read, cb->data->alloc_size);

			//advance pointers
			(char*)dest += cb->data->alloc_size;
			(char*)read += (pos + i) * cb->data->alloc_size;
		}
		to_read -= total;
		pos = 0;
	}
	if (p_advance)
	{
		_advance(&cb->read_pos, size, cb->size_mask);
	}

	return size;
}

int CircleBuf_Get(Circle_Buffer* const cb, void* p_dest, int p_size, bool p_advance)
{
	int left = CircleBuf_DataLeft(cb);
	int size = min(left, p_size);
	int pos = cb->read_pos;
	int to_read = size;
	void* dest = p_dest;
	void* read = cb->data->data;
	while (to_read)
	{
		int end = pos + to_read;
		end = min(end, cb->data->elements_size);
		int total = end - pos;

		for (int i = 0; i < total; i++)
		{
			p_dest = read;

			//advance pointers
			(char*)dest += cb->data->alloc_size;
			(char*)read += (pos + i) * cb->data->alloc_size;
		}
		to_read -= total;
		pos = 0;
	}
	if (p_advance)
	{
		_advance(&cb->read_pos, size, cb->size_mask);
	}

	return size;
}

int CircleBuf_Write(Circle_Buffer* const cb, const void* p_src, int p_size)
{
	int left = CircleBuf_SpaceLeft(cb);
	int size = min(left, p_size);
	int pos = cb->write_pos;
	int to_write = size;
	void* src = p_src;
	void* write = cb->data->data;
	while (to_write)
	{
		int end = pos + to_write;
		end = min(end, cb->data->elements_size);
		int total = end - pos;

		for (int i = 0; i < total; i++)
		{
			memcpy(write, src, cb->data->alloc_size);

			//advance pointers
			(char*)src += cb->data->alloc_size;
			(char*)write += (pos + i) * cb->data->alloc_size;
		}
		to_write -= total;
		pos = 0;
	}

	_advance(&cb->write_pos, size, cb->size_mask);

	return size;
}

void CircleBuf_Resize(Circle_Buffer* const cb, int p_power)
{
	int old_size = cb->data->elements_size;
	int new_size = 1 << p_power;
	int mask = new_size - 1;
	dA_resize(cb->data, (int64_t)1 << (int64_t)p_power);

	if (old_size < new_size && cb->read_pos > cb->write_pos) 
	{
		for (int i = 0; i < cb->write_pos; i++)
		{
			void* write = dA_at(cb->data, (old_size + 1) & mask);
			void* src = dA_at(cb->data, i);
			memcpy(write, src, cb->data->elements_size);
			
		}
		cb->write_pos = (old_size + cb->write_pos) & mask;
	}
	else 
	{
		cb->read_pos = cb->read_pos & mask;
		cb->write_pos = cb->write_pos & mask;
	}

	cb->size_mask = mask;
}
