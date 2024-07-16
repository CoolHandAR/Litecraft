#pragma once

#include <cglm/cglm.h>

/*
	Hardcoded block data. Used for rendering, collisions. 
	Isn't very practical, but it is good enough for this demo project
	The arrays are index by the order of the LC_BlockType Enums
	LiteCraft only supports blocks that are singular cuboids
	So no custom types like chairs or beds
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
	LC_BT__OBSIDIAN

} LC_BlockType;

typedef struct LC_Block_Texture_Offset_Data
{
	LC_BlockType block_type;
	vec2 back_face;
	vec2 front_face;
	vec2 left_face;
	vec2 right_face;
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
	0, 0,
	0, 0,
	0, 0,

	//GRASS
	LC_BT__GRASS,
	25, 8,
	25, 8,
	25, 8,
	25, 8,
	11, 14,
	10, 16,

	//SAND
	LC_BT__SAND,
	5, 30,
	5, 30,
	5, 30,
	5, 30,
	5, 30,
	5, 30,

	//STONE
	LC_BT__STONE,
	10, 6,
	10, 6,
	10, 6,
	10, 6,
	10, 6,
	10, 6,

	//DIRT
	LC_BT__DIRT,
	13, 9,
	13, 9,
	13, 9,
	13, 9,
	13, 9,
	13, 9,

	//TRUNK
	LC_BT__TRUNK,
	1, 26,
	1, 26,
	1, 26,
	1, 26,
	2, 26,
	2, 26,

	//TREE LEAVES
	LC_BT__TREELEAVES,
	4, 7,
	4, 7,
	4, 7,
	4, 7,
	4, 7,
	4, 7,

	//WATER
	LC_BT__WATER,
	2, 29,
	2, 29,
	2, 29,
	2, 29,
	2, 29,
	2, 29,

	//GLASS
	LC_BT__GLASS,
	24, 15,
	24, 15,
	24, 15,
	24, 15,
	24, 15,
	24, 15,

	//FLOWER
	LC_BT__FLOWER,
	4, 28,
	0, 0,
	0, 0,
	0, 0,
	0, 0,
	0, 0,

	//GLOWSTONE
	LC_BT__GLOWSTONE,
	25, 3,
	25, 3,
	25, 3,
	25, 3,
	25, 3,
	25, 3,

	//MAGMA
	LC_BT__MAGMA,
	27, 16,
	27, 16,
	27, 16,
	27, 16,
	27, 16,
	27, 16,

	//OBSIDIAN
	LC_BT__OBSIDIAN,
	17, 4,
	17, 4,
	17, 4,
	17, 4,
	17, 4,
	17, 4,

};

typedef struct
{
	LC_BlockType block_type;
	int material_type; //0 == opaque, 1 == transparent, 2 == water
	int emits_light;
	int collidable;
} LC_Block_MiscData;

static const LC_Block_MiscData LC_BLOCK_MISC_DATA[] =
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
	1,
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
};

typedef struct
{
	LC_BlockType block_type;

	vec3 color;
	float ambient_intensity;
	float specular_intensity;

	float linear;
	float quadratic;
	float constant;
} LC_Block_LightingData;

static const LC_Block_LightingData LC_BLOCK_LIGHTING_DATA[] =
{
	//GLOWSTONE
	LC_BT__GLOWSTONE,
	0.98, 0.85, 0.45,
	0.8,
	0.2,
	0.42,
	0.20,
	1.0,

	//MAGMA
	LC_BT__MAGMA,
	0.85, 0.35, 0.04,
	1.0,
	0.4,
	0.22,
	0.20,
	1.0,


	//OBSIDIAN 
	LC_BT__OBSIDIAN,
	0.51, 0.03, 0.89,
	1.0,
	0.4,
	0.22,
	0.20,
	1.0,

};