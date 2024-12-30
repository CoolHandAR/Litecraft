#pragma once

#include <cglm/cglm.h>

/*
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Hardcoded block data. Used for rendering, collisions.
	Isn't very practical, but it is good enough for this demo project
	The arrays are index by the order of the LC_BlockType Enums
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#define LC_BLOCK_ATLAS_WIDTH 1024
#define LC_BLOCK_ATLAS_HEIGHT 512
#define LC_BLOCK_ATLAS_TEX_SIZE 8
#define LC_BLOCK_ATLAS_WIDTH_DIVIDED 64
#define LC_BLOCK_ATLAS_HEIGHT_DIVIDED 32

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
	
} LC_Block_LightingData;

static const LC_Block_LightingData LC_BLOCK_LIGHTING_DATA[] =
{
	//GLOWSTONE
	LC_BT__GLOWSTONE,
	0.98, 0.85, 0.45,
	24.8,
	0.2,
	0.42,
	0.20,

	//MAGMA
	LC_BT__MAGMA,
	0.95, 0.06, 0.12,
	32.0,
	12.4,
	0.72,
	0.20,

	//OBSIDIAN 
	LC_BT__OBSIDIAN,
	0.51, 0.03, 0.89,
	12.0,
	0.4,
	0.22,
	0.20,

	//TORCH
	LC_BT__TORCH,
	0.95, 0.56, 0.01,
	4.0,
	0.4,
	0.22,
	0.20,
};

