#ifndef LC_CHUNK
#define LC_CHUNK
#pragma once

#include <cglm/cglm.h>
#include <stdint.h>

#include "lc/lc_common.h"

typedef struct
{
	uint8_t type;
} LC_Block;

typedef struct
{
	LC_Block blocks[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][LC_CHUNK_LENGTH]; //Three dimensional array that stores blocks

	ivec3 global_position; //Global Position of the first block

	int opaque_index;
	int transparent_index;
	int water_index;

	int draw_cmd_index;

	int chunk_data_index;

	int aabb_tree_index;

	int16_t alive_blocks; //How many blocks are actually active

	int16_t opaque_blocks; //Num opaque blocks

	int16_t transparent_blocks; //Num transparent blocks
	
	int16_t water_blocks; //Num water blocks

	int16_t light_blocks; //Num blocks that emit light
	
	bool is_deleted;

} LC_Chunk;


GeneratedChunkVerticesResult* LC_Chunk_GenerateVertices(LC_Chunk* const chunk);
LC_Chunk LC_Chunk_Create(int p_x, int p_y, int p_z);
void LC_Chunk_GenerateBlocks(LC_Chunk* const _chunk, int _seed);
void LC_Chunk_SetBlock(LC_Chunk* const p_chunk, int x, int y, int z, uint8_t block_type);
uint8_t LC_Chunk_getType(LC_Chunk* const p_chunk, int x, int y, int z);
LC_Block* LC_Chunk_GetBlock(LC_Chunk* const p_chunk, int x, int y, int z);

#endif // !LC_CHUNK