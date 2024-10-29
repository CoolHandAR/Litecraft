#include <glad/glad.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "utility/u_utility.h"

static const char* GL_sourceChar(GLenum source)
{
	switch (source)
	{
	case GL_DEBUG_SOURCE_API: return "API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
	case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
	case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
	case GL_DEBUG_SOURCE_OTHER: return "OTHER";
	}

	return "NULL";
}

static const char* GL_typeChar(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR: return "ERROR";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
	case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
	case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
	case GL_DEBUG_TYPE_MARKER: return "MARKER";
	case GL_DEBUG_TYPE_OTHER: return "OTHER";
	}

	return "NULL";
}

static const char* GL_severityChar(GLenum severity)
{
	switch (severity) 
	{
	case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
	case GL_DEBUG_SEVERITY_LOW: return "LOW";
	case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
	case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
	}

	return "NULL";
}

void GLmessage_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
{
	char src_buf[64];
	char type_buf[64];
	char sever_buf[64];

	memset(&src_buf, 0, 64);
	memset(&type_buf, 0, 64);
	memset(&sever_buf, 0, 64);

	strcpy(&src_buf, GL_sourceChar(source));
	strcpy(&type_buf, GL_typeChar(type));
	strcpy(&sever_buf, GL_severityChar(severity));

	printf("%s, %s, %s, %i, %s \n", src_buf, type_buf, sever_buf, id, message);
	
}

static void AssertGlMapFlags(unsigned p_bufferFlags, unsigned p_mapFlags)
{
	if (p_mapFlags & GL_MAP_WRITE_BIT)
	{
		assert((p_bufferFlags & GL_DYNAMIC_STORAGE_BIT) && (p_bufferFlags & GL_MAP_WRITE_BIT) && "Invalid buffer flags");
	}
	if (p_mapFlags & GL_MAP_READ_BIT)
	{
		assert(p_bufferFlags & GL_MAP_READ_BIT && "Invalid buffer flags");
	}
	if (p_mapFlags & GL_MAP_COHERENT_BIT)
	{
		assert(p_bufferFlags & GL_MAP_COHERENT_BIT && "Invalid Buffer Flags");
	}
	if (p_mapFlags & GL_MAP_PERSISTENT_BIT)
	{
		assert(p_bufferFlags & GL_MAP_PERSISTENT_BIT && "Invalid Buffer flags");
	}
}

static unsigned ReallocGlBuffer(size_t p_oldSize, size_t p_newSize, unsigned p_glBuffer, unsigned p_glBufferFlags)
{
	assert(p_glBuffer > 0 && "Invalid buffer");

	void* temp_buffer = NULL;
	if (p_oldSize > 0)
	{
		temp_buffer = malloc(p_oldSize);

		if (!temp_buffer)
		{
			return;
		}

		//map the data
		void* data = glMapNamedBufferRange(p_glBuffer, 0, p_oldSize, GL_MAP_READ_BIT);

		assert(data && "Failed to map buffer when reallocating");

		//copy the data to the temp buffer
		memcpy(temp_buffer, data, p_oldSize);

		glUnmapNamedBuffer(p_glBuffer);
	}

	//delete the old buffer
	glDeleteBuffers(1, &p_glBuffer);

	unsigned new_gl_buffer = 0;

	//recreate the buffer
	glCreateBuffers(1, &new_gl_buffer);
	glNamedBufferStorage(new_gl_buffer, p_newSize, NULL, p_glBufferFlags);

	size_t move_size = p_oldSize;

	//if old size is bigger than the new one, trim the move size
	if (p_oldSize > p_newSize)
	{
		move_size = p_oldSize - (p_oldSize - p_newSize);
	}

	if (p_oldSize > 0)
	{
		glNamedBufferSubData(new_gl_buffer, 0, move_size, temp_buffer); //we upload the old data now since buffer storage can read wrong memory

		//clean temp buffer
		free(temp_buffer);
	}

	return new_gl_buffer;
}

static void RSB_Assert(RenderStorageBuffer* const rsb)
{
	assert(rsb->item_size > 0 && "Item size not set");
	assert(rsb->reserve_size > 0 && "Reserve size invalid");
	assert(rsb->buffer > 0 && "GL buffer not set");
}

RenderStorageBuffer RSB_Create(size_t p_initReserveSize, size_t p_itemSize, unsigned p_rsbFlags)
{
	assert(p_initReserveSize > 0 && "Reserve size must be bigger than 0");
	assert(p_itemSize > 0 && "Invalid item size");

	RenderStorageBuffer rsb;
	memset(&rsb, 0, sizeof(RenderStorageBuffer));

	rsb.buffer_flags = 0;
	if (p_rsbFlags & RSB_FLAG__READABLE)
	{
		rsb.buffer_flags |= GL_MAP_READ_BIT;
		//rsb.buffer_flags |= GL_CLIENT_STORAGE_BIT;
	}
	if (p_rsbFlags & RSB_FLAG__WRITABLE)
	{
		rsb.buffer_flags |= GL_DYNAMIC_STORAGE_BIT;
		rsb.buffer_flags |= GL_MAP_WRITE_BIT;
	}
	if (p_rsbFlags & RSB_FLAG__PERSISTENT)
	{
		rsb.buffer_flags |= GL_MAP_PERSISTENT_BIT;
		rsb.buffer_flags |= GL_MAP_COHERENT_BIT;
	}
	if (p_rsbFlags & RSB_FLAG__RESIZABLE)
	{
		rsb.buffer_flags |= GL_MAP_READ_BIT; //need this bit when reallocating the buffer
	}
	rsb.buffer_flags |= GL_DYNAMIC_STORAGE_BIT;

	glCreateBuffers(1, &rsb.buffer);
	glNamedBufferStorage(rsb.buffer, p_itemSize * p_initReserveSize, NULL, rsb.buffer_flags);


	rsb.reserve_size = p_initReserveSize;
	rsb.item_size = p_itemSize;
	rsb.rsb_flags = p_rsbFlags;

	//init only if the storage is poolable
	if(rsb.rsb_flags & RSB_FLAG__POOLABLE)
		rsb.free_list = dA_INIT(unsigned, 0);

	if (rsb.rsb_flags & RSB_FLAG__USE_CPU_BACK_BUFFER)
	{
		rsb._back_buffer = malloc(p_initReserveSize * p_itemSize);
		if (rsb._back_buffer)
		{
			memset(rsb._back_buffer, 0, p_initReserveSize * p_itemSize);
		}
	}

	return rsb;
}

void RSB_Destruct(RenderStorageBuffer* const rsb)
{
	RSB_Assert(rsb);

	//unmap if currently mapped
	if (rsb->_data_map)
	{
		RSB_Unmap(rsb);
	}
	if (rsb->rsb_flags & RSB_FLAG__USE_CPU_BACK_BUFFER)
	{
		if (rsb->_back_buffer)
		{
			free(rsb->_back_buffer);
		}
	}
	glDeleteBuffers(1, &rsb->buffer);
	
	if (rsb->free_list)
	{
		dA_Destruct(rsb->free_list);
	}
}

unsigned RSB_Request(RenderStorageBuffer* const rsb)
{
	RSB_Assert(rsb);

	unsigned r_index = 0;

	if (rsb->rsb_flags & RSB_FLAG__POOLABLE && rsb->free_list->elements_size > 0)
	{
		unsigned new_free_list_size = rsb->free_list->elements_size - 1;
		unsigned* free_list_array = rsb->free_list->data;
		r_index = free_list_array[new_free_list_size];

		dA_resize(rsb->free_list, new_free_list_size);

		return r_index;
	}
	
	r_index = rsb->used_size;

	if (!(rsb->rsb_flags & RSB_FLAG__RESIZABLE))
	{
		assert(r_index + 1 <= rsb->reserve_size && "Max limit reached");
	}

	//we resize, so we can give the promised space in the array
	RSB_Resize(rsb, r_index + 1);
	
	rsb->used_size++;

	return r_index;
}


void RSB_FreeItem(RenderStorageBuffer* const rsb, unsigned p_index, bool p_zeroMem)
{
	RSB_Assert(rsb);

	//bounds check
	assert((p_index >= 0 && p_index < rsb->used_size) && "Can't free object with an invalid index\n");

	GLintptr offset = p_index * rsb->item_size;
	if (rsb->rsb_flags & RSB_FLAG__POOLABLE)
	{
		//memset zero the value?
		if (p_zeroMem)
		{
			void* data = glMapNamedBufferRange(rsb->buffer, offset, rsb->item_size, GL_MAP_WRITE_BIT);
			memset(data, 0, rsb->item_size);

			glUnmapNamedBuffer(rsb->buffer);
		}
		//add the index id to the free list
		unsigned* emplaced = dA_emplaceBack(rsb->free_list);
		*emplaced = p_index;
	}
	else
	{
		if (rsb->_data_map)
		{
			glUnmapNamedBuffer(rsb->buffer);
			rsb->_data_map = NULL;
		}

		void* map = glMapNamedBufferRange(rsb->buffer, offset, rsb->item_size * rsb->used_size, GL_MAP_WRITE_BIT);
		void* data_start_to_move_back = (char*)map + rsb->item_size;
		memcpy(map, data_start_to_move_back, rsb->item_size * rsb->used_size);
		glUnmapNamedBuffer(rsb->buffer);
		rsb->used_size--;
	}
}

void RSB_Resize(RenderStorageBuffer* const rsb, unsigned p_newSize)
{	
	//do nothing if the sizes are the same
	if (rsb->used_size == p_newSize)
		return;

	//is our new size bigget than our reserve size?
	if (p_newSize > rsb->reserve_size)
	{
		assert(!rsb->_data_map);
		rsb->buffer = ReallocGlBuffer(rsb->item_size * rsb->used_size, rsb->item_size * p_newSize, rsb->buffer, rsb->buffer_flags);
		rsb->reserve_size = p_newSize;
	}
}

void RSB_IncreaseSize(RenderStorageBuffer* const rsb, unsigned p_toAdd)
{
	if (p_toAdd == 0)
		return;

	assert(!rsb->_data_map);
	rsb->buffer = ReallocGlBuffer(rsb->item_size * rsb->used_size, (rsb->reserve_size + p_toAdd) * rsb->item_size, rsb->buffer, rsb->buffer_flags);
	rsb->reserve_size = rsb->reserve_size + p_toAdd;
}

void* RSB_Map(RenderStorageBuffer* const rsb, unsigned p_mapFlags)
{
	//if we are already mapped and the map settings match, return the map ptr
	if (rsb->_data_map && rsb->_map_offset == 0 && rsb->_map_size >= rsb->used_size && rsb->_map_flags == p_mapFlags)
	{
		return rsb->_data_map;
	}
	//otherwise we unmap if needed
	else
	{
		RSB_Unmap(rsb);
	}
	RSB_Assert(rsb);
	AssertGlMapFlags(rsb->buffer_flags, p_mapFlags);

	void* data = glMapNamedBufferRange(rsb->buffer, 0, rsb->reserve_size * rsb->item_size, p_mapFlags);
	
	assert(data && "This should not happen");

	rsb->_data_map = data;
	rsb->_map_offset = 0;
	rsb->_map_size = rsb->used_size * rsb->item_size;
	rsb->_map_flags = p_mapFlags;

	return data;
}

void* RSB_MapRange(RenderStorageBuffer* const rsb, size_t p_offset, size_t p_size, unsigned p_mapFlags)
{
	//if we are already mapped and the map settings match, return the map ptr
	if (rsb->_data_map && rsb->_map_offset == p_offset && rsb->_map_size == p_size && rsb->_map_flags == p_mapFlags)
	{
		return rsb->_data_map;
	}
	//otherwise we unmap if needed
	else
	{
		RSB_Unmap(rsb);
	}

	RSB_Assert(rsb);
	AssertGlMapFlags(rsb->buffer_flags, p_mapFlags);

	void* data = glMapNamedBufferRange(rsb->buffer, p_offset * rsb->item_size, p_size * rsb->item_size, p_mapFlags);

	rsb->_data_map = data;
	rsb->_map_offset = p_offset;
	rsb->_map_size = p_size * rsb->item_size;
	rsb->_map_flags = p_mapFlags;

	return data;
}

void* RSB_MapIndex(RenderStorageBuffer* const rsb, unsigned p_index, unsigned p_mapFlags)
{
	//bounds check
	assert((p_index >= 0 && p_index < rsb->used_size) && "Invalid index");
	AssertGlMapFlags(rsb->buffer_flags, p_mapFlags);

	return RSB_MapRange(rsb, p_index, 1, p_mapFlags);
}

void RSB_Unmap(RenderStorageBuffer* const rsb)
{
	RSB_Assert(rsb);
	
	//not mapped? do nothing
	if (!rsb->_data_map)
	{
		return;
	}
	glUnmapNamedBuffer(rsb->buffer);

	rsb->_data_map = NULL;
	rsb->_map_flags = 0;
	rsb->_map_offset = 0;
	rsb->_map_size = 0;
}

static void DRB_Assert(DynamicRenderBuffer* const drb)
{
	assert(drb->reserve_size > 0 && "Reserve size invalid");
	assert(drb->buffer > 0 && "GL buffer not set");
}

DynamicRenderBuffer DRB_Create(size_t p_initReserveSize, size_t p_initItemCount, unsigned p_drbFlags)
{	
	assert(p_initReserveSize > 0 && "Reserve size must be bigger than 0");	

	DynamicRenderBuffer drb;
	memset(&drb, 0, sizeof(DynamicRenderBuffer));
	
	drb.buffer_flags = 0;

	if (p_drbFlags & DRB_FLAG__READABLE)
	{
		drb.buffer_flags |= GL_MAP_READ_BIT;
	}
	if (p_drbFlags & DRB_FLAG__WRITABLE)
	{
		drb.buffer_flags |= GL_DYNAMIC_STORAGE_BIT;
		drb.buffer_flags |= GL_MAP_WRITE_BIT;
	}
	if (p_drbFlags & DRB_FLAG__PERSISTENT)
	{
		drb.buffer_flags |= GL_MAP_PERSISTENT_BIT;
		drb.buffer_flags |= GL_MAP_COHERENT_BIT;
	}
	if (p_drbFlags & DRB_FLAG__RESIZABLE)
	{
		drb.buffer_flags |= GL_MAP_READ_BIT; //need this bit when reallocating the buffer
	}
	drb.buffer_flags |= GL_DYNAMIC_STORAGE_BIT;
	drb.buffer_flags |= GL_MAP_WRITE_BIT;

	glCreateBuffers(1, &drb.buffer);
	glNamedBufferStorage(drb.buffer, p_initReserveSize, NULL, drb.buffer_flags);
	if (p_drbFlags & DRB_FLAG__USE_CPU_BACK_BUFFER)
	{
		drb._back_buffer = malloc(p_initReserveSize);
		if (drb._back_buffer)
		{
			memset(drb._back_buffer, 0, p_initReserveSize);
		}
	}
	if (p_drbFlags & DRB_FLAG__POOLABLE)
	{
		drb._free_list = dA_INIT(unsigned, 0);
	}

	drb._modified_offset = SIZE_MAX;

	drb.reserve_size = p_initReserveSize;
	drb.drb_flags = p_drbFlags;

	drb.item_list = dA_INIT(DRB_Item, p_initItemCount);

	return drb;
}

void DRB_Destruct(DynamicRenderBuffer* const drb)
{
	DRB_Assert(drb);

	//unmap if currently mapped
	DRB_Unmap(drb);

	glDeleteBuffers(1, &drb->buffer);
	
	if (drb->drb_flags & DRB_FLAG__USE_CPU_BACK_BUFFER && drb->_back_buffer)
	{
		free(drb->_back_buffer);
	}
	if (drb->item_list)
	{
		dA_Destruct(drb->item_list);
	}
}

unsigned DRB_EmplaceItem(DynamicRenderBuffer* const drb, size_t p_len, const void* p_data)
{
	DRB_Assert(drb);
	
	unsigned drb_item_index = 0;
	DRB_Item* drb_item = NULL;
	if (drb->drb_flags & DRB_FLAG__POOLABLE && drb->_free_list->elements_size > 0)
	{
		unsigned new_free_list_size = drb->_free_list->elements_size - 1;
		unsigned* free_list_array = drb->_free_list->data;
		unsigned r_index = free_list_array[new_free_list_size];

		dA_resize(drb->_free_list, new_free_list_size);
		drb_item_index = r_index;

		drb_item = dA_at(drb->item_list, drb_item_index);
	}
	else
	{
		drb_item = dA_emplaceBack(drb->item_list);
		drb_item_index = drb->item_list->elements_size - 1;

		drb_item->offset = drb->used_bytes;
	}

	assert(drb_item->count == 0);

	drb_item->offset = drb->used_bytes;
	
	drb->used_bytes += p_len;

	if(p_len > 0)
		DRB_ChangeData(drb, p_len, p_data, drb_item_index);

	return drb_item_index;
}

void DRB_ChangeData(DynamicRenderBuffer* const drb, size_t p_len, const void* p_data, unsigned p_drbItemIndex)
{
	DRB_Assert(drb);
	assert(p_drbItemIndex < drb->item_list->elements_size && "Invalid item index");

	if (drb->drb_flags & DRB_FLAG__POOLABLE)
	{
		for (int i = 0; i < drb->_free_list->elements_size; i++)
		{
			unsigned* free_list_index = dA_at(drb->_free_list, i);

			assert(*free_list_index != p_drbItemIndex && "Index in free list, Invalid!");
		}
	}
	
	
	DRB_Item* array = drb->item_list->data;

	DRB_Item* drb_item = &array[p_drbItemIndex];

	bool previously_mapped = false;
	//do we need to realloc?
	if (p_len > drb_item->count)
	{
		if ((drb->used_bytes - drb_item->count) + p_len >= drb->reserve_size)
		{
			//resize if we can
			if (drb->drb_flags & DRB_FLAG__RESIZABLE)
			{
				//unmap if we are mapped
				if (drb->_data_map)
				{
					glUnmapNamedBuffer(drb->buffer);
					drb->_data_map = NULL;
					previously_mapped = true;
				}

				size_t new_size = drb->reserve_size + drb->_resize_chunk_size + p_len;

				if (drb->drb_flags & DRB_FLAG__USE_CPU_BACK_BUFFER)
				{
					//the old size is 0, since our data is stored in the back buffer
					drb->buffer = ReallocGlBuffer(0, new_size, drb->buffer, drb->buffer_flags);

					void* reallocated_ptr = realloc(drb->_back_buffer, new_size);

					if (!reallocated_ptr)
					{
						//print error
					}
					else
					{
						drb->_back_buffer = reallocated_ptr;
						//reuploud data to gpu
						glNamedBufferSubData(drb->buffer, 0, drb->used_bytes, drb->_back_buffer);
					}
				}
				else
				{
					drb->buffer = ReallocGlBuffer(drb->used_bytes, new_size, drb->buffer, drb->buffer_flags);
				}

				drb->reserve_size = new_size;
				
				//remap
				if (previously_mapped)
				{
					if (drb->drb_flags & DRB_FLAG__ALWAYS_MAP_TO_MAX_RESERVE)
					{
						DRB_MapReserve(drb, drb->_map_flags);
					}
					else
					{
						DRB_MapRange(drb, drb->_map_offset, drb->_map_size, drb->_map_flags);
					}
				}
			}
			else
			{
				assert(false && "Too much data to add!");//this should not assert but rather print error
			}
		}
	}
	//Do we want to memset?
	//we need to do before moving anyhting
	//Redundant code???
	if (!p_data)
	{
		if (drb_item->count > 0)
		{
			void* null_data = malloc(drb_item->count);
			if (null_data)
			{
				//stupid way to memset data, since you can't pass NULL as data
				//glclearnamedbuffersubdata would be better, but it is pretty confusing to use
				memset(null_data, 0, drb_item->count);

				if (drb->drb_flags & DRB_FLAG__USE_CPU_BACK_BUFFER)
				{
					memcpy((char*)drb->_back_buffer + drb_item->offset, null_data, drb_item->count);

					if (drb_item->offset < drb->_modified_offset)
					{
						drb->_modified_offset = drb_item->offset;
					}
					drb->_modified_size += drb_item->count;
				}
				else
				{
					glNamedBufferSubData(drb->buffer, drb_item->offset, drb_item->count, null_data);
				}

				free(null_data);
			}
		}
	}

	if (drb_item->count != p_len)
	{
		assert(drb->used_bytes >= (drb_item->offset + drb_item->count) && "Underflow possible");

		int64_t size_to_move = drb->used_bytes - (drb_item->offset + drb_item->count);

		assert(size_to_move >= 0);
		int64_t to_offset = max(drb_item->count, p_len) - min(drb_item->count, p_len);
		
		if (drb_item->count > p_len)
		{
			to_offset = -to_offset;
		}

		//move data forward or backwards if the old size does not match the new size
		if (size_to_move > 0)
		{
			if (drb->drb_flags & DRB_FLAG__USE_CPU_BACK_BUFFER)
			{
				void* read_offset = (char*)drb->_back_buffer + (drb_item->offset + drb_item->count);
				void* write_offset = (char*)drb->_back_buffer + (drb_item->offset + p_len);
				memmove(write_offset, read_offset, size_to_move);

				drb->_modified_size += size_to_move;

				if (drb_item->offset < drb->_modified_offset)
				{
					drb->_modified_offset = drb_item->offset;
				}
			}
			else
			{
				//check if we are already mapped and if the map matches
				if (drb->_data_map && drb->_map_offset <= drb_item->offset && drb->_map_size >= size_to_move + (drb_item->offset - drb->_map_offset)
					&& drb->_map_flags & GL_MAP_WRITE_BIT)
				{
					void* read_offset = (char*)drb->_data_map + (drb_item->offset - drb->_map_offset) + drb_item->count;
					void* write_offset = (char*)drb->_data_map + (drb_item->offset - drb->_map_offset) + p_len;
					memmove(write_offset, read_offset, size_to_move);

				}
				//otherwise we map
				else
				{
					bool prev_mapped = false;
					if (drb->_data_map)
					{
						prev_mapped = true;
						glUnmapNamedBuffer(drb->buffer);
						drb->_data_map = NULL;
					}
					
					void* map = glMapNamedBufferRange(drb->buffer, drb_item->offset, size_to_move, GL_MAP_WRITE_BIT);		
					assert(map && "Failed to map");

					void* read_offset = (char*)map + drb_item->count;
					void* write_offset = (char*)map + p_len;
					memmove(write_offset, read_offset, size_to_move);
	
					glUnmapNamedBuffer(drb->buffer);

					if (prev_mapped)
					{
						DRB_MapRange(drb, drb->_map_offset, drb->_map_size, drb->_map_flags);
					}
				}
			}

			//update offset of all items
			DRB_Item* array = drb->item_list->data;

			for (int i = 0; i < drb->item_list->elements_size; i++)
			{
				DRB_Item* item = &array[i];

				if (item->offset > drb_item->offset)
				{
					item->offset += to_offset;
				}
			}
		}
		drb->used_bytes += to_offset;
		
	}
	//copy the data
	if (p_data)
	{
		if (drb->drb_flags & DRB_FLAG__USE_CPU_BACK_BUFFER)
		{
			memcpy((char*)drb->_back_buffer + drb_item->offset, p_data, p_len);

			if (drb_item->offset < drb->_modified_offset)
			{
				drb->_modified_offset = drb_item->offset;
			}
			drb->_modified_size += p_len;
		}
		//if we are mapped and the map matches, upload the data
		else if (drb->_data_map && drb->_map_offset <= drb_item->offset && drb->_map_size >= p_len + (drb_item->offset - drb->_map_offset)
			&& drb->_map_flags & GL_MAP_WRITE_BIT)
		{
			memcpy((char*)drb->_data_map + (drb_item->offset - drb->_map_offset), p_data, p_len);
		}
		//otherwise we do a simple buffer subdata call
		else
		{
			glNamedBufferSubData(drb->buffer, drb_item->offset, p_len, p_data);
		}
	}

	drb_item->count = p_len;
}

void DRB_RemoveItem(DynamicRenderBuffer* const drb, unsigned p_drbItemIndex)
{
	DRB_Item item = DRB_GetItem(drb, p_drbItemIndex);
	size_t old_size = item.count;

	DRB_ChangeData(drb, 0, NULL, p_drbItemIndex);

	if (drb->drb_flags & DRB_FLAG__POOLABLE)
	{
		//add the index id to the free list
		unsigned* emplaced = dA_emplaceBack(drb->_free_list);
		*emplaced = p_drbItemIndex;
	}
	else
	{
		dA_erase(drb->item_list, p_drbItemIndex, 1);
	}
}

void* DRB_Map(DynamicRenderBuffer* const drb, unsigned p_mapFlags)
{
	DRB_Assert(drb);
	//if we are already mapped and the entire buffer is mapped, return the map ptr
	if (drb->_data_map && drb->_map_offset == 0 && drb->_map_size == drb->used_bytes && drb->_map_flags == p_mapFlags)
	{
		return drb->_data_map;
	}
	//otherwise we unmap if needed
	else
	{
		DRB_Unmap(drb);
	}
	
	AssertGlMapFlags(drb->buffer_flags, p_mapFlags);
	
	drb->_map_offset = 0;
	drb->_map_size = drb->used_bytes;
	drb->_map_flags = p_mapFlags;

	drb->_data_map = glMapNamedBufferRange(drb->buffer, 0, drb->_map_size, p_mapFlags);

	return drb->_data_map;
}

void* DRB_MapReserve(DynamicRenderBuffer* const drb, unsigned p_mapFlags)
{
	return DRB_MapRange(drb, 0, drb->reserve_size, p_mapFlags);
}

void* DRB_MapRange(DynamicRenderBuffer* const drb, size_t p_offset, size_t p_size, unsigned p_mapFlags)
{
	DRB_Assert(drb);
	//if we are already mapped and the map settings match, return the map ptr
	if (drb->_data_map && drb->_map_offset == p_offset && drb->_map_size == p_size && drb->_map_flags == p_mapFlags)
	{
		return drb->_data_map;
	}
	//otherwise we unmap if needed
	else
	{
		DRB_Unmap(drb);
	}

	AssertGlMapFlags(drb->buffer_flags, p_mapFlags);

	drb->_map_offset = p_offset;
	drb->_map_size = p_size;
	drb->_map_flags = p_mapFlags;

	drb->_data_map = glMapNamedBufferRange(drb->buffer, p_offset, p_size, p_mapFlags);

	return drb->_data_map;
}

void* DRB_MapItem(DynamicRenderBuffer* const drb, unsigned p_drbItemIndex, unsigned p_mapFlags)
{
	DRB_Assert(drb);
	DRB_Item drb_item = DRB_GetItem(drb, p_drbItemIndex);
	
	//if we are already mapped and the map settings match, return the map ptr
	if (drb->_data_map && drb->_map_offset == drb_item.offset && drb->_map_size == drb_item.count && drb->_map_flags == p_mapFlags)
	{
		return drb->_data_map;
	}
	//otherwise we unmap if needed
	else
	{
		DRB_Unmap(drb);
	}

	AssertGlMapFlags(drb->buffer_flags, p_mapFlags);

	drb->_map_offset = drb_item.offset;
	drb->_map_size = drb_item.count;
	drb->_map_flags = p_mapFlags;
	drb->_data_map = glMapNamedBufferRange(drb->buffer, drb_item.offset, drb_item.count, p_mapFlags);
	
	return drb->_data_map;
}

DRB_Item DRB_GetItem(DynamicRenderBuffer* const drb, unsigned p_drbItemIndex)
{
	DRB_Assert(drb);

	assert(p_drbItemIndex < drb->item_list->elements_size && "Invalid item index");

	DRB_Item* array = drb->item_list->data;

	DRB_Item* drb_item = &array[p_drbItemIndex];

	assert(drb_item->offset + drb_item->count <= drb->used_bytes && "Invalid item offset and count");

	return *drb_item;
}

void DRB_Unmap(DynamicRenderBuffer* const drb)
{
	DRB_Assert(drb);
	//not mapped? do nothing
	if (!drb->_data_map)
	{
		return;
	}

	glUnmapNamedBuffer(drb->buffer);
	
	drb->_map_offset = 0;
	drb->_map_size = 0;
	drb->_data_map = NULL;
	drb->_map_flags = 0;
}

void DRB_WriteDataToGpu(DynamicRenderBuffer* const drb)
{
	DRB_Assert(drb);
	//not modified? do nothing
	if (drb->_modified_size == 0)
	{
		return;
	}

	glNamedBufferSubData(drb->buffer, drb->_modified_offset, drb->_modified_size, (char*)drb->_back_buffer + drb->_modified_offset);

	drb->_modified_size = 0;
	drb->_modified_offset = SIZE_MAX;
}

void DRB_setResizeChunkSize(DynamicRenderBuffer* const drb, size_t p_chunkSize)
{
	drb->_resize_chunk_size = p_chunkSize;
}



