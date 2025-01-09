#include "lc_common.h"

#include <stb_perlin/stb_perlin.h>

float LC_CalculateContinentalness(float p_x, float p_z)
{
	float noise = stb_perlin_noise3(p_x / 2048.0, 0.0, p_z / 2048.0, 0, 0, 0);

	noise = glm_clamp(noise, -1.0, 1.0);

	return noise;
}

float LC_CalculateSurfaceHeight(float p_x, float p_y, float p_z)
{
	float cont = LC_CalculateContinentalness(p_x, p_z);

	float surface_height_multiplier = fabsf(stb_perlin_noise3(p_x / 1024.0, 0, p_z / 1024.0, 0, 0, 0) * 450);
	float surface_height = fabsf(stb_perlin_noise3(p_x / 256.0, p_y / 512.0, p_z / 256.0, 0, 0, 0) * surface_height_multiplier);
	float flatness = stb_perlin_ridge_noise3(p_x / 256.0, 0, p_z / 256.0, 2.0, 0.6, 1.2, 6) * 12;

	surface_height += flatness;

	return surface_height;
}

static LC_BiomeType2 LC_DetermineBiome(float p_x, float p_y, float p_z)
{	
	float noise = fabsf(stb_perlin_noise3(p_x / 512.0, p_y / 2048.0, p_z / 512.0, 0, 0, 0) * 20);

	//printf("%f \n", noise);

	if (noise > 8.0)
	{
		return LC_Biome_RockyMountains2;
	}
	else if (noise > 4.0)
	{
		return LC_Biome_SnowyMountains2;
	}
	else if (noise > 2.8)
	{
		return LC_Biome_SnowyPlains2;
	}
	else if (noise > 0.5)
	{
		return LC_Biome_GrassyPlains2;
	}

	return LC_Biome_GrassyPlains2;
}

static LC_BlockType LC_GenerateBlockBasedOnBiome(LC_BiomeType2 biome, float p_x, float p_y, float p_z)
{
	float noise_3d = fabsf(stb_perlin_turbulence_noise3(p_x / 256.0, p_y / 256.0, p_z / 256.0, 2.0, 0.6, 1)) * 35;
	float both = noise_3d;
	switch (biome)
	{
	case LC_Biome_SnowyMountains2:
	{
		return LC_BT__SNOW;
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
	case LC_Biome_SnowyPlains2:
	{
		return  LC_BT__SNOW;
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
	case LC_Biome_RockyMountains2:
	{
		return  LC_BT__STONE;
		if (both > 35)
		{
			return  LC_BT__STONE;
		}
		else
		{
			return  LC_BT__DIRT;
		}

	}
	case LC_Biome_Desert2:
	{
		return LC_BT__SAND;
	}
	case LC_Biome_GrassyPlains2:
	{
		return  LC_BT__GRASS;
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
}

LC_BlockType LC_Generate_Block(float p_x, float p_y, float p_z)
{	
	float surface_height = LC_CalculateSurfaceHeight(p_x, p_y, p_z);

	LC_BlockType block;

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