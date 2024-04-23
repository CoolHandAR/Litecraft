#pragma once

#include <cglm/cglm.h>
#include "utility/dynamic_array.h"
#include <open_simplex_noise_in_c/open-simplex-noise.h>
#include "physics/p_physics_defs.h"

#define LC_CHUNK_WIDTH 16
#define LC_CHUNK_HEIGHT 16
#define LC_CHUNK_LENGTH 16
#define LC_CHUNK_TOTAL_SIZE LC_CHUNK_WIDTH * LC_CHUNK_HEIGHT * LC_CHUNK_LENGTH

#define LC_BLOCK_STARTING_HP 7

#define LC_CF__DIRTY_BIT 0x2
#define LC_CF__LOADED_VERTEX_DATA 0x4


typedef struct LC_Block
{
	int8_t hp;
	uint8_t type;	
} LC_Block;

typedef struct LC_Chunk
{
	LC_Block blocks[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][LC_CHUNK_LENGTH]; //Three dimensional array that stores blocks

	ivec3 global_position; //Global Position of the first block

	size_t vertex_count; //Total Vertex count

	size_t vertex_start; //Start of the first vertex belonging to this chunk

	vec3 box[2]; //AABB of the entire chunk. Used for frustrum culling operations

	int flags;

	uint16_t alive_blocks; //How many blocks are marked alive

} LC_Chunk;

void LC_Chunk_CreateEmpty(int p_gX, int p_gY, int p_gZ, LC_Chunk* chunk);
void LC_Chunk_Generate(int p_gX, int p_gY, int p_gZ, struct osn_context* osn_ctx, LC_Chunk* chunk, int chunk_index, unsigned vbo, unsigned* vertex_count);
void LC_Chunk_Update(LC_Chunk* const p_chunk, int* last_index);
void LC_Chunk_LoadVertexData(LC_Chunk* p_chunk);
void LC_Chunk_UnloadVertexData(LC_Chunk* p_chunk);
void LC_Chunk_Destroy(LC_Chunk* chunk);

