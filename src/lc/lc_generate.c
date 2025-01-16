#include "lc/lc_common.h"

#include <stb_perlin/stb_perlin.h>

#include "lc/lc_chunk.h"
#include "utility/u_math.h"

static unsigned s_seed;

float LC_CalculateContinentalness(float p_x, float p_z)
{
	float noise = stb_perlin_noise3(p_x / 2048.0, 0.0, p_z / 2048.0, 0, 0, 0);

	noise = glm_clamp(noise, -1.0, 1.0);

	return noise;
}

float LC_CalculateSurfaceHeight(float p_x, float p_y, float p_z)
{
	float cont = LC_CalculateContinentalness(p_x, p_z);

	float surface_height_multiplier = fabsf(stb_perlin_noise3_seed(p_x / 1024.0, 0, p_z / 1024.0, 0, 0, 0, s_seed) * 450);
	float surface_height = fabsf(stb_perlin_noise3_seed(p_x / 256.0, p_y / 512.0, p_z / 256.0, 0, 0, 0, s_seed) * surface_height_multiplier);
	float flatness = stb_perlin_ridge_noise3(p_x / 256.0, 0, p_z / 256.0, 2.0, 0.6, 1.2, 6) * 12;

	surface_height += flatness;

	return surface_height;
}

static LC_BiomeType2 LC_DetermineBiome(float p_x, float p_y, float p_z)
{	
	float noise = fabsf(stb_perlin_noise3_seed(p_x / 512.0, p_y / 2048.0, p_z / 512.0, 0, 0, 0, s_seed) * 20);

	if (noise > 8.0)
	{
		return LC_Biome_RockyMountains;
	}
	else if (noise > 4.0)
	{
		return LC_Biome_SnowyMountains;
	}
	else if (noise > 2.8)
	{
		return LC_Biome_SnowyPlains;
	}
	else if (noise > 0.5)
	{
		return LC_Biome_GrassyPlains;
	}

	return LC_Biome_GrassyPlains;
}

static LC_BlockType LC_GenerateBlockBasedOnBiome(LC_BiomeType2 biome, float p_x, float p_y, float p_z)
{
	float noise_3d = fabsf(stb_perlin_turbulence_noise3(p_x / 256.0, p_y / 256.0, p_z / 256.0, 2.0, 0.6, 1)) * 35;
	float both = noise_3d;
	switch (biome)
	{
	case LC_Biome_SnowyMountains:
	{
		if (both > 50)
		{
			return LC_BT__SNOW;
		}
		else if (both * 2 > 60)
		{
			return LC_BT__GRASS_SNOW;
		}
		break;
	}
	case LC_Biome_SnowyPlains:
	{
		if (both > 25)
		{
			return  LC_BT__SNOW;
		}
		else
		{
			return LC_BT__GRASS_SNOW;
		}

		break;
	}
	case LC_Biome_RockyMountains:
	{
		if (both > 35)
		{
			return  LC_BT__STONE;
		}
		else
		{
			return  LC_BT__DIRT;
		}

	}
	case LC_Biome_Desert:
	{
		return LC_BT__SAND;
	}
	case LC_Biome_GrassyPlains:
	{
		if (p_y > 12)
		{
			return  LC_BT__GRASS;
		}
		else if (p_y < 33)
		{
			return LC_BT__STONE;
		}
		else
		{
			return LC_BT__DIRT;
		}
	}
	default:
	{
		return LC_BT__GRASS;
	}
	}

	return LC_BT__GRASS;
}
static bool LC_Chunk_CanTreeGrowInto(LC_BlockType p_bt)
{
	switch (p_bt)
	{
	case LC_BT__NONE:
	case LC_BT__TREELEAVES:
	case LC_BT__GRASS:
	case LC_BT__DIRT:
	case LC_BT__TRUNK:
	{
		return true;
	}
	default:
		break;
	}

	return false;
}

void LC_GenerateAdditionalBlocks(LC_Chunk* _chunk, int p_x, int p_y, int p_z, int p_gX, int p_gY, int p_gZ)
{
	uint8_t block_type = _chunk->blocks[p_x][p_y][p_z].type;

	uint8_t left_block = LC_Chunk_getType(_chunk, p_x - 1, p_y, p_z);
	uint8_t right_block = LC_Chunk_getType(_chunk, p_x + 1, p_y, p_z);
	uint8_t front_block = LC_Chunk_getType(_chunk, p_x, p_y, p_z + 1);
	uint8_t back_block = LC_Chunk_getType(_chunk, p_x, p_y, p_z - 1);
	uint8_t up_block = LC_Chunk_getType(_chunk, p_x, p_y + 1, p_z);
	uint8_t down_block = LC_Chunk_getType(_chunk, p_x, p_y - 1, p_z);


	if (block_type == LC_BT__SAND)
	{
		//Cactus
		if (p_gY > 0 && (Math_rand() % 512) == 0)
		{
			int cactus_height = Math_rand() % 5;

			for (int i = 0; i < cactus_height; i++)
			{
				LC_Chunk_SetBlock(_chunk, p_x, p_y + i, p_z, LC_BT__CACTUS);
			}
		}
		//Dead bush
		else if ((Math_rand() % 128) == 0)
		{
			LC_Chunk_SetBlock(_chunk, p_x, p_y + 1, p_z, LC_BT__DEAD_BUSH);
		}
	}
	else if (block_type == LC_BT__SNOW || block_type == LC_BT__GRASS_SNOW)
	{
		//Dead bush
		if ((Math_rand() % 16) == 0 && p_gY > 12 && (up_block == LC_BT__NONE || LC_IsBlockWater(up_block)) && (left_block == LC_BT__NONE || back_block == LC_BT__NONE))
		{
			LC_Chunk_SetBlock(_chunk, p_x, p_y + 1, p_z, LC_BT__DEAD_BUSH);
		}
		else if ((Math_rand() % 16) == 0 && p_gY > 12 && p_gY < 300 && p_y < LC_CHUNK_HEIGHT - 5 && p_x > 2 && p_z > 2 && p_x < LC_CHUNK_WIDTH - 2
			&& p_z < LC_CHUNK_LENGTH - 2 && up_block == LC_BT__NONE && right_block == LC_BT__NONE && front_block == LC_BT__NONE && back_block == LC_BT__NONE)
		{
			const int MIN_TREE_HEIGHT = 5;

			//Generate trunk
			int tree_height = max(Math_rand() % 5, MIN_TREE_HEIGHT);

			for (int i = 0; i < tree_height; i++)
			{
				LC_Chunk_SetBlock(_chunk, p_x, p_y + i, p_z, LC_BT__TRUNK);
			}

			//Generate tree leaves
			for (int iy = -2; iy <= 2; iy++)
			{
				int minH = (iy < -1 || iy > 1) ? 0 : -1;
				int maxH = (iy < -1 || iy > 1) ? 0 : 1;
				for (int ix = -1 + minH; ix <= 1 + maxH; ix++)
				{
					int x1 = abs(ix - p_x);
					for (int iz = -1 + minH; iz <= 1 + maxH; iz++)
					{
						int z1 = abs(iz - p_z);
						int total = ix * ix + iy * iy + iz * iz;

						uint8_t sample_block_type = LC_Chunk_getType(_chunk, p_x + ix, p_y + tree_height + iy, p_z + iz);

						if (total + 2 < Math_rand() % 24 && x1 != 2 - minH && x1 != 2 + maxH && z1 != 2 - minH && z1 != 2 + maxH &&
							(sample_block_type == LC_BT__NONE || sample_block_type == LC_BT__SNOWYLEAVES))
						{
							LC_Chunk_SetBlock(_chunk, p_x + ix, p_y + tree_height + iy, p_z + iz, LC_BT__SNOWYLEAVES);
						}
					}
				}
			}

		}

	}
	else if (block_type == LC_BT__GRASS || block_type == LC_BT__DIRT)
	{
		//generate a tree
		if ((Math_rand() % 2) == 0 && p_gY > 12 && p_gY < 300 && p_y < LC_CHUNK_HEIGHT - 5 && p_x > 2 && p_z > 2 && p_x < LC_CHUNK_WIDTH - 2
			&& p_z < LC_CHUNK_LENGTH - 2 && up_block == LC_BT__NONE && right_block == LC_BT__NONE && front_block == LC_BT__NONE && back_block == LC_BT__NONE)
		{
			const int MIN_TREE_HEIGHT = 5;

			//Generate trunk
			int tree_height = max(Math_rand() % 5, MIN_TREE_HEIGHT);

			for (int i = 0; i < tree_height; i++)
			{
				LC_Chunk_SetBlock(_chunk, p_x, p_y + i, p_z, LC_BT__TRUNK);
			}

			//Generate tree leaves
			for (int iy = -2; iy <= 2; iy++)
			{
				int minH = (iy < -1 || iy > 1) ? 0 : -1;
				int maxH = (iy < -1 || iy > 1) ? 0 : 1;
				for (int ix = -1 + minH; ix <= 1 + maxH; ix++)
				{
					int x1 = abs(ix - p_x);
					for (int iz = -1 + minH; iz <= 1 + maxH; iz++)
					{
						int z1 = abs(iz - p_z);
						int total = ix * ix + iy * iy + iz * iz;

						uint8_t sample_block_type = LC_Chunk_getType(_chunk, p_x + ix, p_y + tree_height + iy, p_z + iz);

						if (total + 2 < Math_rand() % 24 && x1 != 2 - minH && x1 != 2 + maxH && z1 != 2 - minH && z1 != 2 + maxH &&
							(sample_block_type == LC_BT__NONE || sample_block_type == LC_BT__TREELEAVES))
						{
							LC_Chunk_SetBlock(_chunk, p_x + ix, p_y + tree_height + iy, p_z + iz, LC_BT__TREELEAVES);
						}
					}
				}
			}

		}
		else if (p_gY > 5 && (up_block == LC_BT__NONE || LC_IsBlockWater(up_block)) && (left_block == LC_BT__NONE || back_block == LC_BT__NONE))
		{
			//grass prop
			if ((Math_rand() % 8) == 0)
			{
				LC_Chunk_SetBlock(_chunk, p_x, p_y + 1, p_z, LC_BT__GRASS_PROP);
			}
			//flower prop
			else if ((Math_rand() % 8) == 0)
			{
				LC_Chunk_SetBlock(_chunk, p_x, p_y + 1, p_z, LC_BT__FLOWER);
			}
		}
	}
}



LC_BlockType LC_Generate_Block(float p_x, float p_y, float p_z)
{	
	float surface_height = LC_CalculateSurfaceHeight(p_x, p_y, p_z);

	LC_BlockType block = LC_BT__NONE;	

	if (p_y < surface_height)
	{
		block = LC_BT__STONE;
	}
	else if (p_y < LC_WORLD_WATER_HEIGHT + 1)
	{
		block = LC_BT__WATER;
	}
	else
	{
		block = LC_BT__NONE;
	}

	if (block == LC_BT__STONE)
	{
		LC_BiomeType2 biome = LC_DetermineBiome(p_x, p_y, p_z);

		block = LC_GenerateBlockBasedOnBiome(biome, p_x, p_y, p_z);
	}

	return block;
}

void LC_Generate_SetSeed(unsigned seed)
{
	s_seed = seed;
}
