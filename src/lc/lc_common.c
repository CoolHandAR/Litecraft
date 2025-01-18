#include "lc/lc_common.h"

#include <assert.h>
#include <glad/glad.h>
#include <string.h>

static void LC_AssertBoundType(uint8_t block_type)
{
	assert(block_type < LC_BT__MAX && "Block type invalid");
}

bool LC_IsBlockWater(uint8_t block_type)
{
	LC_AssertBoundType(block_type);
	
	return LC_BLOCK_MATERIAL_DATA[block_type].material_type == 2;
}

bool LC_isBlockOpaque(uint8_t block_type)
{
	LC_AssertBoundType(block_type);

	return LC_BLOCK_MATERIAL_DATA[block_type].material_type == 0;
}
bool LC_isBlockSemiTransparent(uint8_t block_type)
{
	LC_AssertBoundType(block_type);

	return LC_BLOCK_MATERIAL_DATA[block_type].material_type == 1 || LC_BLOCK_MATERIAL_DATA[block_type].material_type == 3;
}
bool LC_isBlockCollidable(uint8_t block_type)
{
	LC_AssertBoundType(block_type);

	return LC_BLOCK_MATERIAL_DATA[block_type].collidable == 1;
}
bool LC_isblockEmittingLight(uint8_t block_type)
{
	LC_AssertBoundType(block_type);

	return LC_BLOCK_MATERIAL_DATA[block_type].emits_light == 1;
}
bool LC_isBlockProp(uint8_t block_type)
{
	LC_AssertBoundType(block_type);

	return LC_BLOCK_MATERIAL_DATA[block_type].material_type == 3;
}

const char* LC_getBlockName(uint8_t block_type)
{
	LC_AssertBoundType(block_type);

	return LC_BLOCK_CHAR_NAME[block_type];
}

void LC_getBlockTypeAABB(uint8_t blockType, vec3 dest[2])
{
	float width = 1;
	float height = 1;
	float length = 1;

	if (LC_isBlockProp(blockType))
	{
		if (blockType == LC_BT__DEAD_BUSH || blockType == LC_BT__GRASS_PROP)
		{
			width = 0.5;
			height = 0.8;
			length = 0.5;
		}
		else
		{
			width = 0.5;
			height = 0.5;
			length = 0.5;
		}
	}
	else
	{

		width = 1.0;
		height = 1.0;
		length = 1.0;
	}
	dest[0][0] = -(0.5 * width);
	dest[0][1] = -(0.5 * height);
	dest[0][2] = -(0.5) * length;

	dest[1][0] = width - (width * 0.5);
	dest[1][1] = height - (height * 0.5);
	dest[1][2] = length - (length * 0.5);
}

LC_Block_LightData LC_getBlockLightingData(uint8_t block_type)
{	
	LC_AssertBoundType(block_type);

	assert(LC_isblockEmittingLight(block_type));

	const int array_size = sizeof(LC_BLOCK_LIGHTING_DATA) / sizeof(LC_Block_LightData);

	for (int i = 0; i < array_size; i++)
	{
		if (LC_BLOCK_LIGHTING_DATA[i].block_type == block_type)
		{
			return LC_BLOCK_LIGHTING_DATA[i];
		}
	}

	return LC_BLOCK_LIGHTING_DATA[0];
}

void LC_getNormalizedChunkPosition(float p_x, float p_y, float p_z, ivec3 dest)
{
	dest[0] = floorf((p_x / LC_CHUNK_WIDTH));
	dest[1] = floorf((p_y / LC_CHUNK_HEIGHT));
	dest[2] = floorf((p_z / LC_CHUNK_LENGTH));
}
unsigned LC_generateBlockInfoGLBuffer()
{
	typedef struct
	{
		int texture_offsets[6];
		float position_offset;
	} LC_BlockMaterialData;

	unsigned buffer = 0;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, buffer);

	LC_BlockMaterialData data[LC_BT__MAX];
	memset(&data, 0, sizeof(data));
	for (int i = 0; i < LC_BT__MAX; i++)
	{
		LC_Block_Texture_Offset_Data texture_data = LC_BLOCK_TEX_OFFSET_DATA[i];
		data[i].texture_offsets[0] = (LC_BLOCK_ATLAS_WIDTH_DIVIDED * texture_data.side_face[1]) + texture_data.side_face[0];
		data[i].texture_offsets[1] = (LC_BLOCK_ATLAS_WIDTH_DIVIDED * texture_data.side_face[1]) + texture_data.side_face[0];
		data[i].texture_offsets[2] = (LC_BLOCK_ATLAS_WIDTH_DIVIDED * texture_data.side_face[1]) + texture_data.side_face[0];
		data[i].texture_offsets[3] = (LC_BLOCK_ATLAS_WIDTH_DIVIDED * texture_data.side_face[1]) + texture_data.side_face[0];
		data[i].texture_offsets[4] = (LC_BLOCK_ATLAS_WIDTH_DIVIDED * texture_data.bottom_face[1]) + texture_data.bottom_face[0];
		data[i].texture_offsets[5] = (LC_BLOCK_ATLAS_WIDTH_DIVIDED * texture_data.top_face[1]) + texture_data.top_face[0];

		data[i].position_offset = (LC_isBlockProp(i)) ? 0.5 : 0.0;
	}

	glBufferData(GL_UNIFORM_BUFFER, sizeof(LC_BlockMaterialData) * (LC_BT__MAX), data, GL_STATIC_DRAW);

	return buffer;
}