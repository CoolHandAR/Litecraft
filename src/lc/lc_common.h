#ifndef LC_COMMON_H 
#define LC_COMMON_H
#pragma once

#include <cglm/cglm.h>

/*
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Hardcoded block data. Used for rendering, collisions.
	Isn't very practical, but it is good enough for this demo project
	The arrays are index by the order of the LC_BlockType Enums
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define LC_BASE_RESOLUTION_WIDTH 1280
#define LC_BASE_RESOLUTION_HEIGHT 720
#define LC_BLOCK_ATLAS_WIDTH 400
#define LC_BLOCK_ATLAS_HEIGHT 400
#define LC_BLOCK_ATLAS_TEX_SIZE 16
#define LC_BLOCK_ATLAS_WIDTH_DIVIDED LC_BLOCK_ATLAS_WIDTH / LC_BLOCK_ATLAS_TEX_SIZE
#define LC_BLOCK_ATLAS_HEIGHT_DIVIDED LC_BLOCK_ATLAS_HEIGHT / LC_BLOCK_ATLAS_TEX_SIZE
#define LC_CHUNK_WIDTH 16
#define LC_CHUNK_HEIGHT 16
#define LC_CHUNK_LENGTH 16
#define LC_CHUNK_TOTAL_SIZE LC_CHUNK_WIDTH * LC_CHUNK_HEIGHT * LC_CHUNK_LENGTH
#define LC_WORLD_MAX_CHUNK_LIMIT 5000
#define LC_WORLD_WATER_HEIGHT 15
#define LC_BLOCK_STARTING_HP 7

typedef enum LC_BlockType
{
	LC_BT__NONE,
	LC_BT__GRASS,
	LC_BT__SAND,
	LC_BT__STONE,
	LC_BT__DIRT,
	LC_BT__TRUNK,
	LC_BT__TREELEAVES,
	LC_BT__WATER,
	LC_BT__GLASS,
	LC_BT__FLOWER,
	LC_BT__GLOWSTONE,
	LC_BT__MAGMA,
	LC_BT__OBSIDIAN,
	LC_BT__DIAMOND,
	LC_BT__IRON,
	LC_BT__SPECULAR,
	LC_BT__CACTUS,
	LC_BT__SNOW,
	LC_BT__GRASS_SNOW,
	LC_BT__SPRUCE_PLANKS,
	LC_BT__GRASS_PROP,
	LC_BT__TORCH,
	LC_BT__DEAD_BUSH,
	LC_BT__SNOWYLEAVES,
	LC_BT__AMETHYST,
	LC_BT__MAX
} LC_BlockType;

typedef struct
{
	LC_BlockType block_type;
	vec2 side_face;
	vec2 bottom_face;
	vec2 top_face;
} LC_Block_Texture_Offset_Data;

static const LC_Block_Texture_Offset_Data LC_BLOCK_TEX_OFFSET_DATA[] =
{
	//NONE
	LC_BT__NONE,
	0, 0,
	0, 0,
	0, 0,

	//GRASS
	LC_BT__GRASS,
	1, 0,
	3, 0,
	2, 0,

	//SAND
	LC_BT__SAND,
	4, 0,
	4, 0,
	4, 0,

	//STONE
	LC_BT__STONE,
	5, 0,
	5, 0,
	5, 0,

	//DIRT
	LC_BT__DIRT,
	6, 0,
	6, 0,
	6, 0,

	//TRUNK
	LC_BT__TRUNK,
	7, 0,
	8, 0,
	8, 0,

	//TREE LEAVES
	LC_BT__TREELEAVES,
	9, 0,
	9, 0,
	9, 0,

	//WATER
	LC_BT__WATER,
	2, 29,
	2, 29,
	2, 29,

	//GLASS
	LC_BT__GLASS,
	10, 0,
	10, 0,
	10, 0,

	//FLOWER
	LC_BT__FLOWER,
	23, 0,
	23, 0,
	23, 0,

	//GLOWSTONE
	LC_BT__GLOWSTONE,
	11, 0,
	11, 0,
	11, 0,

	//MAGMA
	LC_BT__MAGMA,
	12, 0,
	12, 0,
	12, 0,

	//OBSIDIAN
	LC_BT__OBSIDIAN,
	13, 0,
	13, 0,
	13, 0,

	//DIAMOND
	LC_BT__DIAMOND,
	14, 0,
	14, 0,
	14, 0,

	//IRON
	LC_BT__IRON,
	15, 0,
	15, 0,
	15, 0,

	//SPECULAR
	LC_BT__SPECULAR,
	0, 24,
	0, 24,
	0, 24,

	//CACTUS
	LC_BT__CACTUS,
	17, 0,
	18, 0,
	16, 0,

	//SNOW
	LC_BT__SNOW,
	19, 0,
	19, 0,
	19, 0,

	//SNOW
	LC_BT__GRASS_SNOW,
	20, 0,
	3, 0,
	21, 0,

	//SPRUCE PLANKS
	LC_BT__SPRUCE_PLANKS,
	22, 0,
	22, 0,
	22, 0,

	//GRASS PROP
	LC_BT__GRASS_PROP,
	1, 1,
	1, 1,
	1, 1,

	//TORCH
	LC_BT__TORCH,
	2, 1,
	2, 1,
	2, 1,

	//DEAD BUSH
	LC_BT__DEAD_BUSH,
	3, 1,
	3, 1,
	3, 1,

	//SNOWY LEAVES
	LC_BT__SNOWYLEAVES,
	4, 1,
	9, 0,
	21, 0,

	//SNOWY LEAVES
	LC_BT__AMETHYST,
	5, 1,
	5, 1,
	5, 1,

	//MAX
	LC_BT__MAX,
	0, 0,
	0, 0,
	0, 0,

};

typedef struct
{
	LC_BlockType block_type;
	int material_type; //0 == opaque, 1 == semi transparent, 2 == water, 3 == prop
	int emits_light;
	int collidable;
} LC_Block_MaterialData;

static const LC_Block_MaterialData LC_BLOCK_MATERIAL_DATA[] =
{
	//NONE
	LC_BT__NONE,
	1,
	0,
	0,

	//GRASS
	LC_BT__GRASS,
	0,
	0,
	1,

	//SAND
	LC_BT__SAND,
	0,
	0,
	1,

	//STONE
	LC_BT__STONE,
	0,
	0,
	1,

	//DIRT
	LC_BT__DIRT,
	0,
	0,
	1,

	//TRUNK
	LC_BT__TRUNK,
	0,
	0,
	1,
	//TREE LEAVES
	LC_BT__TREELEAVES,
	1,
	0,
	1,

	//WATER
	LC_BT__WATER,
	2,
	0,
	1,

	//GLASS
	LC_BT__GLASS,
	1,
	0,
	1,

	//FLOWER
	LC_BT__FLOWER,
	3,
	0,
	0,

	//GLOWSTONE
	LC_BT__GLOWSTONE,
	0,
	1,
	1,

	//MAGMA
	LC_BT__MAGMA,
	0,
	1,
	1,

	//OBSIDIAN
	LC_BT__OBSIDIAN,
	0,
	1,
	1,

	//DIAMOND
	LC_BT__DIAMOND,
	0,
	0,
	1,

	//IRON
	LC_BT__IRON,
	0,
	0,
	1,

	//SPECULAR
	LC_BT__SPECULAR,
	0,
	0,
	1,

	//CACTUS
	LC_BT__CACTUS,
	0,
	0,
	1,

	//SNOW
	LC_BT__SNOW,
	0,
	0,
	1,

	//GRASS SNOW
	LC_BT__GRASS_SNOW,
	0,
	0,
	1,

	//SPRUCE PLANKS
	LC_BT__SPRUCE_PLANKS,
	0,
	0,
	1,

	//GRASS PROP
	LC_BT__GRASS_PROP,
	3,
	0,
	0,

	//TORCH
	LC_BT__TORCH,
	3,
	1,
	0,

	//DEAD BUSH
	LC_BT__DEAD_BUSH,
	3,
	0,
	0,

	//SNOWY LEAVES
	LC_BT__SNOWYLEAVES,
	1,
	0,
	1,

	//AMETHYST
	LC_BT__AMETHYST,
	0,
	0,
	1,

	//MAX
	LC_BT__MAX,
	0,
	0,
	0,
};

typedef struct
{
	LC_BlockType block_type;

	vec3 color;
	float ambient_intensity;
	float specular_intensity;

	float radius;
	float attenuation;
	
} LC_Block_LightData;

static const LC_Block_LightData LC_BLOCK_LIGHTING_DATA[] =
{
	//GLOWSTONE
	LC_BT__GLOWSTONE,
	0.98, 0.85, 0.45,
	24.8,
	0.2,
	6.42,
	0.20,

	//MAGMA
	LC_BT__MAGMA,
	0.95, 0.06, 0.12,
	32.0,
	12.4,
	8.72,
	0.20,

	//OBSIDIAN 
	LC_BT__OBSIDIAN,
	0.51, 0.03, 0.89,
	12.0,
	0.4,
	9.22,
	0.20,

	//TORCH
	LC_BT__TORCH,
	0.95, 0.56, 0.01,
	4.0,
	0.4,
	3.22,
	1.20,
};

static const char* LC_BLOCK_CHAR_NAME[] =
{
	//NONE
	"None",

	//GRASS
	"Grass",

	//SAND
	"Sand",

	//STONE
	"Stone",

	//DIRT
	"Dirt",

	//TRUNK
	"Trunk",

	//TREE LEAVES
	"Tree leaves",

	//WATER
	"Water",

	//GLASS
	"Glass",

	//FLOWER
	"Flower",

	//GLOWSTONE
	"Glowstone",

	//MAGMA
	"Magma",

	//OBSIDIAN
	"Obsidian",

	//DIAMOND
	"Diamond",

	//IRON
	"Iron",

	//SPECULAR
	"Specular",

	//CACTUS
	"Cactus",

	//SNOW
	"Snow",

	//GRASS SNOW
	"Grass snow",

	//SPRUCE PLANKS
	"Spruce planks",

	//GRASS PROP
	"Grass prop",

	//TORCH
	"Torch",

	//Dead bush
	"Dead bush",

	//Snowy leaves
	"Snowy leaves",

	//Amethyst,
	"Amethyst",

	//MAX
	"Max"
};

typedef enum
{
	LC_Biome_None,
	LC_Biome_SnowyMountains,
	LC_Biome_RockyMountains,
	LC_Biome_GrassyPlains,
	LC_Biome_SnowyPlains,
	LC_Biome_Desert,
	LC_Biome_Max
} LC_BiomeType2;


bool LC_IsBlockWater(uint8_t block_type);
bool LC_isBlockOpaque(uint8_t block_type);
bool LC_isBlockSemiTransparent(uint8_t block_type);
bool LC_isBlockCollidable(uint8_t block_type);
bool LC_isblockEmittingLight(uint8_t block_type);
bool LC_isBlockProp(uint8_t block_type);
LC_Block_LightData LC_getBlockLightingData(uint8_t block_type);
const char* LC_getBlockName(uint8_t block_type);
void LC_getBlockTypeAABB(uint8_t blockType, vec3 dest[2]);

void LC_getNormalizedChunkPosition(float p_x, float p_y, float p_z, ivec3 dest);
unsigned LC_generateBlockInfoGLBuffer();

float LC_CalculateContinentalness(float p_x, float p_z);
float LC_CalculateSurfaceHeight(float p_x, float p_y, float p_z);
LC_BlockType LC_Generate_Block(float p_x, float p_y, float p_z);
void LC_Generate_SetSeed(unsigned seed);


typedef struct
{
	int8_t position[3];
	int8_t normal;
	uint8_t block_type;
} ChunkVertex;

typedef struct
{
	int8_t position[2];
} ChunkWaterVertex;

typedef struct
{
	ChunkVertex* opaque_vertices;
	ChunkVertex* transparent_vertices;
	ChunkWaterVertex* water_vertices;

	size_t opaque_vertex_count;
	size_t transparent_vertex_count;
	size_t water_vertex_count;
} GeneratedChunkVerticesResult;

#endif