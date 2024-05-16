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

static void RSB_Assert(RenderStorageBuffer* const rsb)
{
	assert(rsb->item_size > 0 && "Item size not set");
	assert(rsb->reserve_size > 0 && "Reserve size invalid");
	assert(rsb->buffer > 0 && "GL buffer not set");
}
static void RSB_AssertMapFlags(RenderStorageBuffer* const rsb, unsigned p_mapFlags)
{
	if (p_mapFlags & GL_MAP_WRITE_BIT)
	{
		assert((rsb->buffer_flags & GL_DYNAMIC_STORAGE_BIT) && (rsb->buffer_flags & GL_MAP_WRITE_BIT) && "Invalid buffer flags");
	}
	if (p_mapFlags & GL_MAP_READ_BIT)
	{
		assert(rsb->buffer_flags & GL_MAP_READ_BIT && "Invalid buffer flags");
	}
	if (p_mapFlags & GL_MAP_COHERENT_BIT)
	{
		assert(rsb->buffer_flags & GL_MAP_COHERENT_BIT && "Invalid Buffer Flags");
	}
	if (p_mapFlags & GL_MAP_PERSISTENT_BIT)
	{
		assert(rsb->buffer_flags & GL_MAP_PERSISTENT_BIT && "Invalid Buffer flags");
	}
}

static bool RSB_Realloc(RenderStorageBuffer* const rsb, size_t p_size)
{
	assert(!rsb->_mapped && "Buffer is mapped");

	size_t old_size = rsb->item_size * rsb->used_size;

	void* temp_buffer = malloc(old_size);

	if (!temp_buffer)
	{
		return false;
	}

	void* data = glMapNamedBufferRange(rsb->buffer, 0, old_size, GL_MAP_READ_BIT);

	//copy the data to the temp buffer
	memcpy(temp_buffer, data, old_size);

	glUnmapNamedBuffer(rsb->buffer);

	//delete the old buffer
	glDeleteBuffers(1, &rsb->buffer);
	
	//recreate the buffer
	glCreateBuffers(1, &rsb->buffer);
	glNamedBufferStorage(rsb->buffer, p_size * rsb->item_size, NULL, rsb->buffer_flags);

	size_t move_size = old_size;

	//if old size is bigger than the new one, trim the move size
	if (old_size > p_size)
	{
		move_size = old_size - (old_size - p_size);
	}

	glNamedBufferSubData(rsb->buffer, 0, move_size, temp_buffer); //we upload the old data now since buffer storage can read wrong memory

	rsb->reserve_size = p_size;

	//clean temp buffer
	free(temp_buffer);

	return true;
}


RenderStorageBuffer RSB_Create(size_t p_initReserveSize, size_t p_itemSize, bool p_readable, bool p_writable, bool p_persistent, bool p_resizable)
{
	assert(p_initReserveSize > 0 && "Reserve size must be bigger than 0");
	assert(p_itemSize > 0 && "Invalid item size");

	RenderStorageBuffer rsb;
	memset(&rsb, 0, sizeof(RenderStorageBuffer));

	rsb.buffer_flags = 0;

	if (p_readable) rsb.buffer_flags |= GL_MAP_READ_BIT;
	if (p_writable)
	{
		rsb.buffer_flags |= GL_DYNAMIC_STORAGE_BIT;
		rsb.buffer_flags |= GL_MAP_WRITE_BIT;
	}
	if (p_persistent)
	{
		rsb.buffer_flags |= GL_MAP_PERSISTENT_BIT;
		rsb.buffer_flags |= GL_MAP_COHERENT_BIT;
	}
	
	assert(rsb.buffer_flags > 0 && "No flags set");

	glCreateBuffers(1, &rsb.buffer);
	glNamedBufferStorage(rsb.buffer, p_itemSize * p_initReserveSize, NULL, rsb.buffer_flags);

	rsb.reserve_size = p_initReserveSize;
	rsb.item_size = p_itemSize;
	rsb.resizable = true;

	rsb.free_list = dA_INIT(unsigned, 0);

	return rsb;
}

void RSB_Destruct(RenderStorageBuffer* const rsb)
{
	RSB_Assert(rsb);

	//unmap if currently mapped
	if (rsb->_mapped)
	{
		RSB_Unmap(rsb);
	}

	glDeleteBuffers(1, &rsb->buffer);
	
	dA_Destruct(rsb->free_list);
}

unsigned RSB_Request(RenderStorageBuffer* const rsb)
{
	RSB_Assert(rsb);

	unsigned r_index = 0;

	if (rsb->free_list->elements_size > 0)
	{
		unsigned new_free_list_size = rsb->free_list->elements_size - 1;
		unsigned* free_list_array = rsb->free_list->data;
		r_index = free_list_array[new_free_list_size];

		dA_resize(rsb->free_list, new_free_list_size);

		return r_index;
	}
	
	r_index = rsb->used_size;

	if (!rsb->resizable)
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

	//memset zero the value?
	if (p_zeroMem)
	{
		GLintptr offset = p_index * rsb->item_size;

		void* data = glMapNamedBufferRange(rsb->buffer, offset, rsb->item_size, GL_MAP_WRITE_BIT);
		memset(data, 0, rsb->item_size);

		glUnmapNamedBuffer(rsb->buffer);
	}
	//add the index id to the free list
	unsigned* emplaced = dA_emplaceBack(rsb->free_list);
	*emplaced = p_index;

	rsb->used_size--;
}

void RSB_Resize(RenderStorageBuffer* const rsb, unsigned p_newSize)
{	
	//do nothing if the sizes are the same
	if (rsb->used_size == p_newSize)
		return;

	//is our new size bigget than our reserve size?
	if (p_newSize > rsb->reserve_size)
	{
		RSB_Realloc(rsb, p_newSize);
	}
}

void* RSB_Map(RenderStorageBuffer* const rsb, unsigned p_mapFlags)
{
	assert(!rsb->_mapped && "Buffer is already mapped");
	RSB_Assert(rsb);
	RSB_AssertMapFlags(rsb, p_mapFlags);

	void* data = glMapNamedBufferRange(rsb->buffer, 0, rsb->used_size * rsb->item_size, p_mapFlags);
	
	rsb->_mapped = true;
	rsb->_map_offset = 0;
	rsb->_map_size = rsb->used_size * rsb->item_size;

	return data;
}

void* RSB_MapRange(RenderStorageBuffer* const rsb, size_t p_offset, size_t p_size, unsigned p_mapFlags)
{
	RSB_Assert(rsb);
	RSB_AssertMapFlags(rsb, p_mapFlags);
	assert(!rsb->_mapped && "Buffer is already mapped");

	void* data = glMapNamedBufferRange(rsb->buffer, p_offset * rsb->item_size, p_size * rsb->item_size, p_mapFlags);

	rsb->_mapped = true;
	rsb->_map_offset = p_offset;
	rsb->_map_size = p_size * rsb->item_size;

	return data;
}

void* RSB_MapIndex(RenderStorageBuffer* const rsb, unsigned p_index, unsigned p_mapFlags)
{
	return RSB_MapRange(rsb, p_index, 1, p_mapFlags);
}

void RSB_Unmap(RenderStorageBuffer* const rsb)
{
	RSB_Assert(rsb);
	assert(rsb->_mapped && "Buffer is not mapped");

	glUnmapNamedBuffer(rsb->buffer);

	rsb->_mapped = false;
	rsb->_map_offset = 0;
	rsb->_map_size = 0;
}



