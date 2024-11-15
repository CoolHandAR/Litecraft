#pragma once

#include <cglm/cglm.h>
#include <stdint.h>
#include "render/r_renderer.h"

#define LC_CHUNK_WIDTH 16
#define LC_CHUNK_HEIGHT 16
#define LC_CHUNK_LENGTH 16
#define LC_CHUNK_TOTAL_SIZE LC_CHUNK_WIDTH * LC_CHUNK_HEIGHT * LC_CHUNK_LENGTH

#define LC_BLOCK_STARTING_HP 7

typedef enum
{
	LC_Biome_None,
	LC_Biome_SnowyMountains,
	LC_Biome_RockyMountains,
	LC_Biome_GrassyPlains,
	LC_Biome_SnowyPlains,
	LC_Biome_Desert,
	LC_Biome_Max
} LC_BiomeType;

typedef struct
{
	int8_t position[3];
	int8_t packed_norm_hp;
	uint8_t block_type;
} ChunkVertex;

typedef struct
{
	int8_t position[3];
} ChunkWaterVertex;

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

	size_t vertex_count;

	size_t transparent_vertex_count;

	size_t water_vertex_count;
	
	bool vertex_loaded;

	bool user_modified;

	bool is_deleted;

	bool is_generated;

} LC_Chunk;

typedef struct
{
	ChunkVertex* opaque_vertices;
	ChunkVertex* transparent_vertices;
	ChunkWaterVertex* water_vertices;
} GeneratedChunkVerticesResult;

uint8_t LC_getBlockInheritedType(uint8_t block_type);
bool LC_isBlockOpaque(uint8_t block_type);
bool LC_isBlockCollidable(uint8_t block_type);
bool LC_isblockEmittingLight(uint8_t block_type);
bool LC_isBlockTransparent(uint8_t block_type);
bool LC_isBlockWater(uint8_t blockType);
bool LC_isBlockProp(uint8_t blockType);
void LC_getBlockTypeAABB(uint8_t blockType, vec3 dest[2]);
const char* LC_getBlockName(uint8_t block_type);

LC_BiomeType LC_getBiomeType(float p_x, float p_z);
void LC_getBiomeNoiseVarianceData(LC_BiomeType p_biome, float* r_surfaceHeightMin, float* r_surfaceHeightMax);
LC_Chunk LC_Chunk_Create(int p_x, int p_y, int p_z);
void LC_Chunk_GenerateBlocks(LC_Chunk* const _chunk, int _seed);
unsigned LC_Chunk_getAliveBlockCount(LC_Chunk* const _chunk);
GeneratedChunkVerticesResult* LC_Chunk_GenerateVertices(LC_Chunk* const chunk);
GeneratedChunkVerticesResult* LC_Chunk_GenerateVerticesTest(LC_Chunk* const chunk);
unsigned LC_generateBlockInfoGLBuffer();
void LC_Chunk_CalculateWaterBounds(LC_Chunk* const _chunk, ivec3 dest[2]);
void LC_Chunk_SetBlock(LC_Chunk* const p_chunk, int x, int y, int z, uint8_t block_type);
uint8_t LC_Chunk_getType(LC_Chunk* const p_chunk, int x, int y, int z);