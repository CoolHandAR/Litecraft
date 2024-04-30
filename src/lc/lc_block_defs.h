#pragma once

#include "lc_chunk.h"
#include <cglm/cglm.h>

/*
	Hardcoded block data. Used for rendering, collisions. 
	Isn't very practical, but pretty scalable and error free :)
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
	LC_BT__GLOWSTONE

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
	0, 9,
	0, 9,
	0, 9,
	0, 9,
	0, 9,
	0, 9,

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

};

typedef struct LC_Block_Collision_Data
{
	LC_BlockType block_type;
	bool collidable;
} LC_Block_Collision_Data;

static const LC_Block_Collision_Data LC_BLOCK_COLLISION_DATA[] =
{
	//NONE
	LC_BT__NONE,
	false,

	//GRASS
	LC_BT__GRASS,
	true,

	//SAND
	LC_BT__SAND,
	true,

	//STONE
	LC_BT__STONE,
	true,

	//DIRT
	LC_BT__DIRT,
	true,

	//TRUNK
	LC_BT__TRUNK,
	true,

	//TREE LEAVES
	LC_BT__TREELEAVES,
	true,

	//WATER
	LC_BT__WATER,
	true,

	//GLASS
	LC_BT__GLASS,
	true,

	//FLOWER
	LC_BT__FLOWER,
	false,

	//GLOWSTONE
	LC_BT__GLOWSTONE,
	true,
};

typedef struct LC_Block_Material_Data
{
	LC_BlockType block_type;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;

	float shadow_intensity;

} LC_Block_Material_Data;

static const LC_Block_Material_Data LC_BLOCK_MATERIAL_DATA[] =
{
	//NONE
	LC_BT__NONE,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,
	//GRASS
	LC_BT__GRASS,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//SAND
	LC_BT__SAND,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//STONE
	LC_BT__STONE,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//DIRT
	LC_BT__DIRT,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//TRUNK
	LC_BT__TRUNK,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//TREE LEAVES
	LC_BT__TREELEAVES,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//WATER
	LC_BT__WATER,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//GLASS
	LC_BT__GLASS,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,

	//FLOWER
	LC_BT__FLOWER,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0,
	0,
};


typedef struct LC_Block_Shader_Data
{
	LC_Block_Texture_Offset_Data texture_data;
	LC_Block_Material_Data material_data;
	
} LC_Block_Shader_Data;