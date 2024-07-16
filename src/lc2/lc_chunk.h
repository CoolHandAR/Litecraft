#pragma once

#include <cglm/cglm.h>
#include <stdint.h>
#include "render/r_renderer.h"

#define LC_CHUNK_WIDTH 16
#define LC_CHUNK_HEIGHT 16
#define LC_CHUNK_LENGTH 16
#define LC_CHUNK_TOTAL_SIZE LC_CHUNK_WIDTH * LC_CHUNK_HEIGHT * LC_CHUNK_LENGTH

#define LC_BLOCK_STARTING_HP 7


typedef struct
{
	vec3 position;
	int8_t hp;
	int8_t normal;
	uint16_t texture_offset;
} ChunkVertex;

typedef struct
{
	vec3 position;
} ChunkWaterVertex;

typedef struct
{
	int8_t hp;
	uint8_t type;
} LC_Block;

typedef struct
{
	LC_Block blocks[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][LC_CHUNK_LENGTH]; //Three dimensional array that stores blocks

	ivec3 global_position; //Global Position of the first block

	int opaque_index;
	int transparent_index;
	int water_index;

	int opaque_drawcmd_index;
	int transparent_drawcmd_index;
	int water_drawcmd_index;

	int chunk_data_index;

	int16_t alive_blocks; //How many blocks are actually active

	int16_t opaque_blocks; //Num opaque blocks

	int16_t transparent_blocks; //Num transparent blocks
	
	int16_t water_blocks; //Num water blocks

	size_t vertex_count;

	size_t transparent_vertex_count;

	size_t water_vertex_count;

} LC_Chunk;

typedef struct
{
	ChunkVertex* opaque_vertices;
	ChunkVertex* transparent_vertices;
	ChunkVertex* water_vertices;
} GeneratedChunkVerticesResult;

bool LC_isBlockOpaque(uint8_t block_type);
bool LC_isBlockCollidable(uint8_t block_type);
bool LC_isblockEmittingLight(uint8_t block_type);
bool LC_isBlockTransparent(uint8_t block_type);
bool LC_isBlockWater(uint8_t blockType);

LC_Chunk LC_Chunk_Create(int p_x, int p_y, int p_z);
void LC_Chunk_GenerateBlocks(LC_Chunk* const _chunk, struct osn_context* osn_ctx);
unsigned LC_Chunk_getAliveBlockCount(LC_Chunk* const _chunk);
GeneratedChunkVerticesResult* LC_Chunk_GenerateVertices(LC_Chunk* const chunk);

