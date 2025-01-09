
#include "lc/lc_chunk.h"
#include <string.h>
#include <assert.h>
#include "utility/u_math.h"

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin/stb_perlin.h>
#include <glad/glad.h>
#include "lc_common.h"


#define VERTICES_PER_CUBE 36
#define FACES_PER_CUBE 6

static const vec3 CUBE_POSITION_VERTICES[] =
{
	//back face
	-0.5f, -0.5f, -0.5f,  // Bottom-left
	 0.5f,  0.5f, -0.5f,   // top-right
	 0.5f, -0.5f, -0.5f,   // bottom-right         
	 0.5f,  0.5f, -0.5f,   // top-right
	-0.5f, -0.5f, -0.5f,   // bottom-left
	-0.5f,  0.5f, -0.5f,   // top-left
	// Front face
	-0.5f, -0.5f,  0.5f,   // bottom-left
	 0.5f, -0.5f,  0.5f,   // bottom-right
	 0.5f,  0.5f,  0.5f,   // top-right
	 0.5f,  0.5f,  0.5f,   // top-right
	-0.5f,  0.5f,  0.5f,   // top-left
	-0.5f, -0.5f,  0.5f,   // bottom-left
	// Left face
	-0.5f,  0.5f,  0.5f,  // top-right
	-0.5f,  0.5f, -0.5f,   // top-left
	-0.5f, -0.5f, -0.5f,   // bottom-left
	-0.5f, -0.5f, -0.5f,   // bottom-left
	-0.5f, -0.5f,  0.5f,   // bottom-right
	-0.5f,  0.5f,  0.5f,   // top-right
	// Right face
	 0.5f,  0.5f,  0.5f,   // top-left
	 0.5f, -0.5f, -0.5f,   // bottom-right
	 0.5f,  0.5f, -0.5f,  // top-right         
	 0.5f, -0.5f, -0.5f,   // bottom-right
	 0.5f,  0.5f,  0.5f,  // top-left
	 0.5f, -0.5f,  0.5f,  // bottom-left     
	 // Bottom face
	 -0.5f, -0.5f, -0.5f,   // top-right
	  0.5f, -0.5f, -0.5f,   // top-left
	  0.5f, -0.5f,  0.5f,   // bottom-left
	  0.5f, -0.5f,  0.5f,   // bottom-left
	 -0.5f, -0.5f,  0.5f,  // bottom-right
	 -0.5f, -0.5f, -0.5f,   // top-right
	 // Top face
	 -0.5f,  0.5f, -0.5f,   // top-left
	  0.5f,  0.5f,  0.5f,   // bottom-right
	  0.5f,  0.5f, -0.5f,   // top-right     
	  0.5f,  0.5f,  0.5f,   // bottom-right
	 -0.5f,  0.5f, -0.5f,   // top-left
	 -0.5f,  0.5f,  0.5f // bottom-left   
};


static const ivec3 CUBE_POSITION_VERTICES_INT[] =
{
	//back face
	-1, -1, -1,  // Bottom-left
	 1,  1, -1,   // top-right
	 1, -1, -1,   // bottom-right         
	 1,  1, -1,   // top-right
	-1, -1, -1,   // bottom-left
	-1,  1, -1,   // top-left
	// Front face
	-1, -1,  1,   // bottom-left
	 1, -1,  1,   // bottom-right
	 1,  1,  1,   // top-right
	 1,  1,  1,   // top-right
	-1,  1,  1,   // top-left
	-1, -1,  1,   // bottom-left
	// Left face
	-1,  1,  1,  // top-right
	-1,  1, -1,   // top-left
	-1, -1, -1,   // bottom-left
	-1, -1, -1,   // bottom-left
	-1, -1,  1,   // bottom-right
	-1,  1,  1,   // top-right
	// Right face
	 1,  1,  1,   // top-left
	 1, -1, -1,   // bottom-right
	 1,  1, -1,  // top-right         
	 1, -1, -1,   // bottom-right
	 1,  1,  1,  // top-left
	 1, -1,  1,  // bottom-left     
	 // Bottom face
	 -1, -1, -1,   // top-right
	  1, -1, -1,   // top-left
	  1, -1,  1,   // bottom-left
	  1, -1,  1,   // bottom-left
	 -1, -1,  1,  // bottom-right
	 -1, -1, -1,   // top-right
	 // Top face
	 -1,  1, -1,   // top-left
	  1,  1,  1,   // bottom-right
	  1,  1, -1,   // top-right     
	  1,  1,  1,   // bottom-right
	 -1,  1, -1,   // top-left
	 -1,  1,  1 // bottom-left   
};

static const vec3 CUBE_NORMALS[] =
{
	//back face
	0.0f, 0.0f, -1.0f,

	// Front face
	0.0f, 0.0f,  1.0f,

	// Left face
	-1.0f,  0.0f,  0.0f,

	// Right face
	 1.0f,  0.0f,  0.0f,

	 // Bottom face
	 0.0f, -1.0f, 0.0f,

	 // Top face
	 0.0f,  1.0f, 0.0f
};

static const vec2 CUBE_TEX_COORDS[] =
{
	//Back face
	1.0f, 1.0f, // top-right
	0.0f, 0.0f, // bottom-left
	0.0f, 1.0f, // top-left
	0.0f, 0.0f, // Bottom-left
	1.0f, 1.0f, // top-right
	1.0f, 0.0f, // bottom-right         
	// Front face
	1.0f, 1.0f, // top-right
	0.0f, 1.0f, // top-left
	0.0f, 0.0f, // bottom-left
	0.0f, 0.0f, // bottom-left
	1.0f, 0.0f, // bottom-right
	1.0f, 1.0f, // top-right
	// Left face
	0.0f, 0.0f, // top-right
	1.0f, 0.0f, // top-left
	1.0f, 1.0f, // bottom-left
	1.0f, 1.0f, // bottom-left
	0.0f, 1.0f, // bottom-right
	0.0f, 0.0f, // top-right
	// Right face
	0.0f, 0.0f, // Bottom-left
	1.0f, 1.0f, // top-right
	1.0f, 0.0f, // bottom-right     
	1.0f, 1.0f, // top-right
	0.0f, 0.0f, // bottom-left
	0.0f, 1.0f, // top-left
	// Bottom face
   1.0f, 0.0f, // bottom-left
   0.0f, 0.0f, // bottom-right
   0.0f, 1.0f, // top-right
   0.0f, 1.0f, // top-right
   1.0f, 1.0f, // top-left
   1.0f, 0.0f, // bottom-left
   // Top face
   1.0f, 0.0f, // bottom-right
   0.0f, 1.0f, // top-left
   0.0f, 0.0f,  // bottom-left    
   0.0f, 1.0f, // top-left
   1.0f, 0.0f, // bottom-right
   1.0f, 1.0f // top-right     
};

static bool compareBlocks(LC_Block const b1, LC_Block const b2)
{
	if (b1.type != b2.type)
	{
		return false;
	}

	return true;
}

uint8_t LC_getBlockInheritedType(uint8_t block_type)
{
	switch (block_type)
	{
	case LC_BT__CACTUS:
	case LC_BT__TREELEAVES:
	case LC_BT__GRASS:
	{
		return LC_BT__GRASS;
	}
	case LC_BT__DIRT:
	case LC_BT__SAND:
	{
		return LC_BT__SAND;
	}
	case LC_BT__SNOW:
	case LC_BT__OBSIDIAN:
	case LC_BT__IRON:
	case LC_BT__DIAMOND:
	case LC_BT__MAGMA:
	case LC_BT__GLOWSTONE:
	case LC_BT__STONE:
	{
		return LC_BT__STONE;
	}
	case LC_BT__TRUNK:
	{
		return LC_BT__TRUNK;
	}
	default:
		break;
	}
	return LC_BT__NONE;
}





//#define CULL_SKIP_TRANSPARENT_FACES
static inline bool skipCheck(LC_Block const b1, LC_Block const b2)
{
	//If the other block is none, dont skip
	if (b2.type == LC_BT__NONE)
	{
		return false;
	}
	//Always skip props
	if (LC_isBlockProp(b1.type) || LC_isBlockProp(b2.type))
	{
		return false;
	}

#ifdef CULL_SKIP_TRANSPARENT_FACES
	//If the first and the second block are transparent don't skip
	if (LC_isBlockTransparent(b1.type) && LC_isBlockTransparent(b2.type))
	{
		return false;
	}
	//If the first is transparent and the second block isn't, don't skip
	if (LC_isBlockTransparent(b1.type) && !LC_isBlockTransparent(b2.type))
	{
		return false;
	}
	//If the other block is transparent, don't skip
	if (LC_isBlockTransparent(b2.type))
	{
		return false;
	}
#endif
	//if the first one isn't water and the other is don't skip
	if (!LC_IsBlockWater(b1.type) && LC_IsBlockWater(b2.type))
	{
		return false;
	}

	//we skip
	return true;
}

static inline int8_t pack_norm_hp(int8_t hp, int8_t norm)
{
	int8_t packed = hp;
	packed = packed | (norm << 3);

	return packed;
}

#include <GLFW/glfw3.h>

static int numTrailingZeroes(uint32_t n, int offset)
{
	int mask = 1 << offset;
	for (int i = 0; i < 32; i++, mask <<= 1)
	{
		if ((n & mask) != 0)
		{
			return i;
		}
	}

	return 32;
}

static int numTrailingOnes(uint32_t n, int offset)
{
	int mask = 1 << offset;
	for (int i = 0; i < 32; i++, mask <<= 1)
	{
		if ((n & mask) != 1)
		{
			return i;
		}
	}

	return 32;
}

static int findNextZeroes(uint32_t n, int offset)
{
	int mask = 1 << offset;
	for (int i = offset; i < 32; i++, mask <<= 1)
	{
		if ((n & mask) == 0)
		{
			return i;
		}
	}

	return 0;
}

static unsigned newMaskTest(int numZeroes, int offset)
{
	unsigned mask = 0;

	for (int i = 0; i < numZeroes; i++)
	{
		mask |= (1 << offset);
		offset++;

		if (offset >= 32)
		{
			return mask;
		}
	}

	return mask;
}

static unsigned clearBits(uint32_t n, int num, int offset)
{
	for (int i = 0; i < num; i++)
	{
		n &= ~(1 << offset);
		offset++;

		if (offset >= 32)
		{
			return n;
		}
	}

	return n;
}


static void meshTest(LC_Chunk* const chunk, ChunkVertex* vertices, size_t* index)
{
	uint32_t masks[16][16];
	memset(masks, 0, sizeof(masks));
	
	for (int x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
		{
			for (int z = 0; z < LC_CHUNK_LENGTH; z++)
			{
				if (chunk->blocks[x][y][z].type != LC_BT__NONE)
				{
					masks[x][y] |= 1 << z;
				}
			}
		}
	}
	
	for (int x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		if (x > 0)
		{
			break;
		}
		vec3 quad[2];
		memset(quad, 0, sizeof(quad));
		for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
		{
			if (masks[x][y] == 0)
			{
				continue;
			}
			if (x == LC_CHUNK_WIDTH - 1 || y == -LC_CHUNK_HEIGHT - 1)
			{
				continue;
			}
			uint32_t inverted = masks[x][y] ^ UINT32_MAX;
			
			quad[0][0] = x;
			quad[0][1] = y;
			quad[0][2] = 0;

			int original_y = 0;
			int z = 0;
			while (z < LC_CHUNK_LENGTH)
			{
				uint32_t num_trailing_zeroes = numTrailingZeroes(inverted, z);

				if (num_trailing_zeroes == 0)
				{	
					z++;
					quad[0][2] = z;
					continue;
				}

				uint32_t new_mask = newMaskTest(num_trailing_zeroes, z);

				int nextY = -1;

				for (int iy = y + 1; iy < LC_CHUNK_HEIGHT; iy++)
				{
					unsigned andMask = new_mask & masks[x][iy];

					//it contains us
					if (andMask == new_mask)
					{
						quad[1][1] = iy;
						masks[x][iy] = clearBits(masks[x][iy], num_trailing_zeroes, z);

						if (masks[x][iy] == 0)
						{
							nextY = iy;
						}
					}
					else
					{
						break;
					}
				}

				//masks[x][y] = clearBits(masks[x][y], num_trailing_zeroes, z);
				z += num_trailing_zeroes;
				quad[1][2] = num_trailing_zeroes;

				if (nextY != -1)
				{
					y = nextY;
				}
			}
			if (masks[x][original_y] > 0)
			{
				vertices[*index].position[0] = x;
				vertices[*index].position[1] = quad[0][1];
				vertices[*index].position[2] = quad[0][2];
				vertices[*index].block_type = LC_BT__STONE;
				*index = *index + 1;

				vertices[*index].position[0] = x;
				vertices[*index].position[1] = quad[0][1];
				vertices[*index].position[2] = quad[1][2];
				vertices[*index].block_type = LC_BT__STONE;
				*index = *index + 1;

				vertices[*index].position[0] = x;
				vertices[*index].position[1] = quad[1][1];
				vertices[*index].position[2] = quad[1][2];
				vertices[*index].block_type = LC_BT__STONE;
				*index = *index + 1;

				vertices[*index].position[0] = x;
				vertices[*index].position[1] = quad[0][1];
				vertices[*index].position[2] = quad[0][2];
				vertices[*index].block_type = LC_BT__STONE;
				*index = *index + 1;

				vertices[*index].position[0] = x;
				vertices[*index].position[1] = quad[1][1];
				vertices[*index].position[2] = quad[0][2];
				vertices[*index].block_type = LC_BT__STONE;
				*index = *index + 1;

				vertices[*index].position[0] = x;
				vertices[*index].position[1] = quad[1][1];
				vertices[*index].position[2] = quad[1][2];
				vertices[*index].block_type = LC_BT__STONE;
				*index = *index + 1;
			}
		
			
		}
	}


}



GeneratedChunkVerticesResult* LC_Chunk_GenerateVerticesTest(LC_Chunk* const chunk)
{
	
	float start_time = 1;

	GeneratedChunkVerticesResult* result = malloc(sizeof(GeneratedChunkVerticesResult));

	if (!result)
	{
		return NULL;
	}

	ChunkVertex* vertices = NULL;
	ChunkVertex* transparent_vertices = NULL;
	ChunkWaterVertex* water_vertices = NULL;

	assert(chunk->alive_blocks > 0 && "Invalid chunk block count");

	if (chunk->opaque_blocks > 0)
	{
		vertices = calloc(chunk->opaque_blocks * VERTICES_PER_CUBE, sizeof(ChunkVertex));

		if (!vertices)
		{
			printf("Failed to malloc cube vertices\n");
			return NULL;
		}
	}

	if (chunk->transparent_blocks > 0)
	{
		transparent_vertices = calloc(chunk->transparent_blocks * VERTICES_PER_CUBE, sizeof(ChunkVertex));

		if (!transparent_vertices)
		{
			printf("Failed to malloc cube vertices\n");
			return NULL;
		}
	}



	float end_time = 1;

	//printf("%f \n", end_time - start_time);
	//[0] BACK, [1] FRONT, [2] LEFT, [3] RIGHT, [4] BOTTOM, [5] TOP
	bool drawn_faces[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][LC_CHUNK_LENGTH][6];
	memset(&drawn_faces, 0, sizeof(drawn_faces));

	ChunkVertex* buffer = NULL;
	size_t* index = NULL;

	size_t vert_index = 0;
	size_t transparent_index = 0;
	size_t water_index = 0;
	
	//meshTest(chunk, vertices, &vert_index);

	for (int x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		const int next_x = (x + 1) % LC_CHUNK_WIDTH;

		for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
		{
			const int next_y = (y + 1) % LC_CHUNK_HEIGHT;

			for (int z = 0; z < LC_CHUNK_LENGTH; z++)
			{
				const int next_z = (z + 1) % LC_CHUNK_LENGTH;

				//SKIP IF NOT ALIVE
				if (chunk->blocks[x][y][z].type == LC_BT__NONE)
					continue;

				//The water is just a flat quad
				if (LC_IsBlockWater(chunk->blocks[x][y][z].type))
				{
					continue;
				}
				//continue;
				LC_Block_Texture_Offset_Data tex_offset_data = LC_BLOCK_TEX_OFFSET_DATA[chunk->blocks[x][y][z].type];

				bool skip_back = (z > 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y][z - 1])) || drawn_faces[x][y][z][0] == true;
				bool skip_front = (next_z != 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y][next_z])) || drawn_faces[x][y][z][1] == true;
				bool skip_left = (x > 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x - 1][y][z])) || drawn_faces[x][y][z][2] == true;
				bool skip_right = (next_x != 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[next_x][y][z])) || drawn_faces[x][y][z][3] == true;
				bool skip_bottom = (y > 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y - 1][z])) || drawn_faces[x][y][z][4] == true;
				bool skip_top = (next_y != 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][next_y][z])) || drawn_faces[x][y][z][5] == true;
				
				/*
				if (neighbours)
				{
					if (z == 0 && neighbours[0])
					{
						if (skipCheck(chunk->blocks[x][y][z], neighbours[0]->blocks[x][y][LC_CHUNK_LENGTH - 1]))
						{
							skip_back = true;
						}
					}
					else if (z == LC_CHUNK_LENGTH - 1 && neighbours[1])
					{
						if (skipCheck(chunk->blocks[x][y][z], neighbours[1]->blocks[x][y][0]))
						{
							skip_front = true;
						}
					}
					if (x == 0 && neighbours[2])
					{
						if (skipCheck(chunk->blocks[x][y][z], neighbours[2]->blocks[LC_CHUNK_WIDTH - 1][y][z]))
						{
							skip_left = true;
						}
					}
					else if (x == LC_CHUNK_WIDTH - 1 && neighbours[3])
					{
						if (skipCheck(chunk->blocks[x][y][z], neighbours[3]->blocks[0][y][z]))
						{
							skip_right = true;
						}
					}
					if (y == 0 && neighbours[4])
					{
						if (skipCheck(chunk->blocks[x][y][z], neighbours[4]->blocks[x][LC_CHUNK_HEIGHT - 1][z]))
						{
							skip_bottom = true;
						}
					}
					else if (y == LC_CHUNK_HEIGHT - 1 && neighbours[5])
					{
						if (skipCheck(chunk->blocks[x][y][z], neighbours[5]->blocks[x][0][z]))
						{
							skip_top = true;
						}
					}
				}
				*/
				

				if (LC_isBlockProp(chunk->blocks[x][y][z].type))
				{
					skip_bottom = true;
					skip_top = true;
				}

				//choose buffer and index
				if (LC_isBlockSemiTransparent(chunk->blocks[x][y][z].type) || LC_isBlockProp(chunk->blocks[x][y][z].type))
				{
					buffer = transparent_vertices;
					index = &transparent_index;
				}
				else if (!LC_IsBlockWater(chunk->blocks[x][y][z].type))
				{
					buffer = vertices;
					index = &vert_index;
				}

				size_t max_x = x;
				size_t max_y = y;
				size_t max_z = z;

				/*
				*~~~~~~~~~~~~~~
				*MERGE FACES
				*~~~~~~~~~~~~~~
				*/
				//Check if we need to find max y
				if (!skip_back || !skip_front || !skip_left || !skip_right)
				{
					size_t y_search = y;

					while (y_search < LC_CHUNK_HEIGHT)
					{

						//Find the last compatible block on the y axis
						if (chunk->blocks[x][y_search][z].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z], chunk->blocks[x][y_search][z]))
						{
							max_y = y_search;

						}
						//Break immediately if it doesn't match
						else
						{
							break;
						}

						y_search++;
					}

				}
				/*
				*~~~~~~~~~~~~~~
				*HANDLE BACK AND FRONT
				*~~~~~~~~~~~~~~
				*/
				if (!skip_back || !skip_front)
				{
					size_t x_search = x;
					size_t max_y2 = 0;
					while (x_search < LC_CHUNK_WIDTH)
					{
						//Find the last block with the matching y block
						if (chunk->blocks[x_search][y][z].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search][y][z]))
						{
							size_t y_search = y;
							while (y_search <= max_y)
							{
								if (chunk->blocks[x_search][y_search][z].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search][y_search][z]))
								{
									max_y2 = y_search;
								}
								else
								{
									break;
								}

								y_search++;
							}
							//did our max y2 match the max y of the first block?
							if (max_y2 == max_y)
							{
								//set and continue to search
								max_x = x_search;
								x_search++;
							}
							//other wise we stop the search
							else
							{
								break;
							}
						}
						else
						{
							break;
						}
					}

					for (int64_t x_mark = x; x_mark <= max_x; x_mark++)
					{
						for (int64_t y_mark = y; y_mark <= max_y; y_mark++)
						{
							if (!skip_back) drawn_faces[x_mark][y_mark][z][0] = true;
							if (!skip_front) drawn_faces[x_mark][y_mark][z][1] = true;
						}
					}

					int start_itr = 0;
					int max_itr = 0;

					//draw both?
					if (!skip_back && !skip_front)
					{
						start_itr = 0;
						max_itr = 12;
					}
					//draw only back
					else if (!skip_back && skip_front)
					{
						start_itr = 0;
						max_itr = 6;
					}
					//draw only front
					else if (skip_back && !skip_front)
					{
						start_itr = 6;
						max_itr = 12;
					}

					//Do we need to merge faces?
					if (max_x > x || max_y > y)
					{
						int64_t to_add_x = (max_x > 0) ? (max_x + 1) - x : 1;
						int64_t to_add_y = (max_y > 0) ? (max_y + 1) - y : 1;

						for (int i = start_itr; i < max_itr; i++)
						{
							buffer[*index].position[0] = x + (0.5f * (to_add_x - 1)) + (0.5 + CUBE_POSITION_VERTICES[i][0] * to_add_x);
							buffer[*index].position[1] = y + (0.5f * (to_add_y - 1)) + (0.5 + CUBE_POSITION_VERTICES[i][1] * to_add_y);
							buffer[*index].position[2] = z + 0.5 + CUBE_POSITION_VERTICES[i][2];

							int8_t norm = 0;
							if (i < 6)
							{
								norm = 0;

							}
							else
							{
								norm = 1;
							}
							buffer[*index].block_type = chunk->blocks[x][y][z].type;
							buffer[*index].normal = pack_norm_hp(7, norm);

							*index = *index + 1;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{
							buffer[*index].position[0] = x + 0.5 + CUBE_POSITION_VERTICES[i][0];
							buffer[*index].position[1] = y + 0.5 + CUBE_POSITION_VERTICES[i][1];
							buffer[*index].position[2] = z + 0.5 + CUBE_POSITION_VERTICES[i][2];

							int8_t norm = 0;
							if (i < 6)
							{
								norm = 0;
							}
							else
							{
								norm = 1;
							}
							buffer[*index].block_type = chunk->blocks[x][y][z].type;
							buffer[*index].normal = pack_norm_hp(7, norm);

							*index = *index + 1;
						}
					}

				}
				/*
				*~~~~~~~~~~~~~~
				*HANDLE LEFT AND RIGHT
				*~~~~~~~~~~~~~~
				*/
				if (!skip_left || !skip_right)
				{
					size_t z_search = z;
					size_t max_y2 = 0;
					while (z_search < LC_CHUNK_LENGTH)
					{
						//Find the last block with the matching y block
						if (chunk->blocks[x][y][z_search].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z], chunk->blocks[x][y][z_search]))
						{
							size_t y_search = y;
							while (y_search <= max_y)
							{
								if (chunk->blocks[x][y_search][z_search].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z_search], chunk->blocks[x][y_search][z_search]))
								{
									max_y2 = y_search;
								}
								else
								{
									break;
								}

								y_search++;
							}
							//did our max y2 match the max y of the first block?
							if (max_y2 == max_y)
							{
								//set and continue to search
								max_z = z_search;
								z_search++;
							}
							//other wise we stop the search
							else
							{
								break;
							}
						}
						else
						{
							break;
						}
					}

					for (int64_t z_mark = z; z_mark <= max_z; z_mark++)
					{
						for (int64_t y_mark = y; y_mark <= max_y; y_mark++)
						{
							if (!skip_left) drawn_faces[x][y_mark][z_mark][2] = true;
							if (!skip_right) drawn_faces[x][y_mark][z_mark][3] = true;
						}
					}

					int start_itr = 0;
					int max_itr = 0;

					//draw both?
					if (!skip_left && !skip_right)
					{
						start_itr = 12;
						max_itr = 24;
					}
					//draw only left
					else if (!skip_left && skip_right)
					{
						start_itr = 12;
						max_itr = 18;
					}
					//draw only right
					else if (skip_left && !skip_right)
					{
						start_itr = 18;
						max_itr = 24;
					}

					//Do we need to merge faces?
					if (max_z > z || max_y > y)
					{
						int64_t to_add_z = (max_z > 0) ? (max_z + 1) - z : 1;
						int64_t to_add_y = (max_y > 0) ? (max_y + 1) - y : 1;

						for (int i = start_itr; i < max_itr; i++)
						{
							buffer[*index].position[0] = x + 0.5 + CUBE_POSITION_VERTICES[i][0];
							buffer[*index].position[1] = y + (0.5f * (to_add_y - 1)) + (0.5 + CUBE_POSITION_VERTICES[i][1] * to_add_y);
							buffer[*index].position[2] = z + (0.5f * (to_add_z - 1)) + (0.5 + CUBE_POSITION_VERTICES[i][2] * to_add_z);

							int8_t norm = 0;
							if (i < 18)
							{
								norm = 2;
							}
							else
							{
								norm = 3;
							}
							buffer[*index].block_type = chunk->blocks[x][y][z].type;
							buffer[*index].normal = pack_norm_hp(7, norm);

							*index = *index + 1;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{

							buffer[*index].position[0] = x + 0.5 + CUBE_POSITION_VERTICES[i][0];
							buffer[*index].position[1] = y + 0.5 + CUBE_POSITION_VERTICES[i][1];
							buffer[*index].position[2] = z + 0.5 + CUBE_POSITION_VERTICES[i][2];

							int8_t norm = 0;
							if (i < 18)
							{
								norm = 2;
							}
							else
							{
								norm = 3;
							}
							buffer[*index].block_type = chunk->blocks[x][y][z].type;
							buffer[*index].normal = pack_norm_hp(7, norm);


							*index = *index + 1;
						}
					}
				}

				/*
				*~~~~~~~~~~~~~~
				*HANDLE BOTTOM AND TOP
				*~~~~~~~~~~~~~~
				*/
				if (!skip_bottom || !skip_top)
				{
					size_t x_search = x;

					while (x_search < LC_CHUNK_WIDTH)
					{
						//Find the last compatible block on the x axis
						if (chunk->blocks[x_search][y][z].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search][y][z]))
						{
							max_x = x_search;

						}
						//Break immediately if it doesn't match
						else
						{
							break;
						}

						x_search++;
					}
					size_t z_search = z;
					size_t max_x2 = 0;

					while (z_search < LC_CHUNK_LENGTH)
					{
						//Find the last block with the matching z block
						if (chunk->blocks[x][y][z_search].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z], chunk->blocks[x][y][z_search]))
						{
							size_t x_search_2 = x;
							while (x_search_2 <= max_x)
							{
								if (chunk->blocks[x_search_2][y][z_search].type != LC_BT__NONE && compareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search_2][y][z_search]))
								{
									max_x2 = x_search_2;
								}
								else
								{
									break;
								}

								x_search_2++;
							}
							//did our max x2 match the max x of the first block?
							if (max_x2 == max_x)
							{
								//set and continue to search
								max_z = z_search;
								z_search++;
							}
							//other wise we stop the search
							else
							{
								break;
							}
						}
						else
						{
							break;
						}
					}

					for (int64_t x_mark = x; x_mark <= max_x; x_mark++)
					{
						for (int64_t z_mark = z; z_mark <= max_z; z_mark++)
						{
							if (!skip_bottom) drawn_faces[x_mark][y][z_mark][4] = true;
							if (!skip_top) drawn_faces[x_mark][y][z_mark][5] = true;
						}
					}

					int start_itr = 0;
					int max_itr = 0;

					//draw both?
					if (!skip_bottom && !skip_top)
					{
						start_itr = 24;
						max_itr = 36;
					}
					//draw only bottom
					else if (!skip_bottom && skip_top)
					{
						start_itr = 24;
						max_itr = 30;
					}
					//draw only top
					else if (skip_bottom && !skip_top)
					{
						start_itr = 30;
						max_itr = 36;
					}

					//Do we need to merge faces?
					if (max_z > z || max_x > x)
					{
						int64_t to_add_z = (max_z > 0) ? (max_z + 1) - z : 1;
						int64_t to_add_x = (max_x > 0) ? (max_x + 1) - x : 1;

						int8_t norm = 0;
						for (int i = start_itr; i < max_itr; i++)
						{

							buffer[*index].position[0] = x + (0.5f * (to_add_x - 1)) + (0.5 + CUBE_POSITION_VERTICES[i][0] * to_add_x);
							buffer[*index].position[1] = y + 0.5 + CUBE_POSITION_VERTICES[i][1];
							buffer[*index].position[2] = z + (0.5f * (to_add_z - 1)) + (0.5 + CUBE_POSITION_VERTICES[i][2] * to_add_z);

							if (i < 30)
							{
								norm = 4;
							}
							else
							{
								norm = 5;
							}
							buffer[*index].block_type = chunk->blocks[x][y][z].type;
							buffer[*index].normal = pack_norm_hp(7, norm);

							*index = *index + 1;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{
							buffer[*index].position[0] = x + 0.5 + CUBE_POSITION_VERTICES[i][0];
							buffer[*index].position[1] = y + 0.5 + CUBE_POSITION_VERTICES[i][1];
							buffer[*index].position[2] = z + 0.5 + CUBE_POSITION_VERTICES[i][2];

							int8_t norm = 0;
							if (i < 30)
							{
								norm = 4;
							}
							else
							{
								norm = 5;
							}

							buffer[*index].block_type = chunk->blocks[x][y][z].type;
							buffer[*index].normal = pack_norm_hp(7, norm);

							*index = *index + 1;
						}
					}

				}
			}

		}
	}

	if (chunk->water_blocks > 0)
	{
		ivec3 water_bounds[2];
		LC_Chunk_CalculateWaterBounds(chunk, water_bounds);
	
		int total = 0;

		int minX = water_bounds[0][0];
		int minZ = water_bounds[0][2];

		int maxX = water_bounds[1][0] + 2;
		int maxZ = water_bounds[1][2] + 2;

		size_t total_vertices = (maxX - minX) * (maxZ - minZ) * 6;
		water_vertices = calloc(total_vertices, sizeof(ChunkWaterVertex));

		if (!water_vertices)
		{
			printf("Failed to malloc cube vertices\n");
			return NULL;
		}
		water_index = 0;
		for (int x = minX; x < maxX; x++)
		{
			for (int z = minZ; z < maxZ; z++)
			{	
				total += 6;
				//FIRST TRIANGLE
				water_vertices[water_index].position[0] = x;
				water_vertices[water_index].position[1] = z;

				water_index++;

				water_vertices[water_index].position[0] = x + 1;
				water_vertices[water_index].position[1] = z;

				water_index++;

				water_vertices[water_index].position[0] = x + 1;
				water_vertices[water_index].position[1] = z + 1;

				water_index++;

				//SECOND TRIANGLE
				water_vertices[water_index].position[0] = x;
				water_vertices[water_index].position[1] = z;

				water_index++;

				water_vertices[water_index].position[0] = x;
				water_vertices[water_index].position[1] = z + 1;

				water_index++;

				water_vertices[water_index].position[0] = x + 1;
				water_vertices[water_index].position[1] = z + 1;

				water_index++;
			}
		}

		float fs = 0;
	}
	

	

	//chunk->vertex_count = vert_index;
	//chunk->transparent_vertex_count = transparent_index;
	//chunk->water_vertex_count = water_index;

	result->opaque_vertex_count = vert_index;
	result->transparent_vertex_count = transparent_index;
	result->water_vertex_count = water_index;

	result->opaque_vertices = vertices;
	result->transparent_vertices = transparent_vertices;
	result->water_vertices = water_vertices;

	//printf("%d \n", chunk->vertex_count);

	return result;
}
LC_BiomeType LC_getBiomeType(float p_x, float p_z)
{
	float noise = stb_perlin_noise3(p_x / 256.0, 0.1, p_z / 256.0, 0, 0, 0);

	float abs_noise = fabsf(noise);
	//Positive noise
	if (noise > 0)
	{
		if (noise > 20)
		{
			return LC_Biome_Desert;
		}
		if (noise > 30)
		{
			return LC_Biome_GrassyPlains;
		}
		if (noise * 3 > 50)
		{
			return LC_Biome_SnowyMountains;
		}
		else
		{
			return LC_Biome_SnowyPlains;
		}
	}


	return LC_Biome_GrassyPlains;
}

static void LC_getBiomeNoiseData(LC_BiomeType p_biome, float p_x, float p_y, float p_z, float* r_surfaceHeight)
{
	switch (p_biome)
	{
	case LC_Biome_SnowyMountains:
	{
		float surface_height_multiplier = fabsf(stb_perlin_noise3(p_x / 256.0, 0, p_z / 256.0, 0, 0, 0) * 500);
		float surface_height = fabsf(stb_perlin_noise3(p_x / 256.0, p_y / 256.0, p_z / 256.0, 0, 0, 0) * surface_height_multiplier);
		float flatness = stb_perlin_ridge_noise3(p_x / 256.0, 0, p_z / 256.0, 2.0, 0.6, 1.2, 6) * 20;
		*r_surfaceHeight = surface_height + flatness;
		return;
	}
	case LC_Biome_SnowyPlains:
	{
		float surface_height = stb_perlin_noise3(p_x / 256.0, p_y / 256.0, p_z / 256.0, 0, 0, 0) * 3;
		float flatness = stb_perlin_ridge_noise3(p_x / 256.0, 0, p_z / 256.0, 3.4, 0.9, 1.1, 3) * 20;
		*r_surfaceHeight = surface_height + flatness;
		return;
	}
	case LC_Biome_RockyMountains:
	{
		float surface_height_multiplier = fabsf(stb_perlin_noise3(p_x / 256.0, 0, p_z / 256.0, 0, 0, 0) * 1);
		float surface_height = fabsf(stb_perlin_noise3(p_x / 256.0, p_y / 256.0, p_z / 256.0, 0, 0, 0) * surface_height_multiplier);
		float flatness = stb_perlin_ridge_noise3(p_x / 256.0, 0, p_z / 256.0, 2.0, 1.8, 1.2, 3) * 12;
		*r_surfaceHeight = surface_height + flatness;
		return;
	}
	case LC_Biome_GrassyPlains:
	{
		float surface_height = stb_perlin_noise3(p_x / 256.0, p_y / 256.0, p_z / 256.0, 0, 0, 0) * 3;
		float flatness = stb_perlin_ridge_noise3(p_x / 256.0, 0, p_z / 256.0, 3.4, 0.6, 1.1, 3) * 20;
		*r_surfaceHeight = surface_height + flatness;
		return;
	}
	case LC_Biome_Desert:
	{
		float height_multiplier = 1;
		float surface_height = 1;
		float flatness = stb_perlin_fbm_noise3(p_x / 256.0, 0, p_z / 256.0, 2.34, 1.4, 4);
		*r_surfaceHeight = surface_height + flatness;
		return;
	}
	default:
		break;
	}
}

static LC_Block LC_generateBlock(float p_x, float p_y, float p_z, int p_seed)
{
	LC_BiomeType biome_type = LC_getBiomeType(p_x, p_z);
	//biome_type = LC_Biome_GrassyPlains;

	float surface_height_multiplier = 0;
	float surface_height = 0;

	LC_getBiomeNoiseData(biome_type, p_x, p_y, p_z, &surface_height);
	
	//float surface_y = fabsf(surface_height + stb_perlin_ridge_noise3(p_x / 256.0, 0, p_z / 256.0, 2.0, 0.6, 1.2, 6) * 20);

	float surface_y = surface_height;

	float sea_level = LC_WORLD_WATER_HEIGHT;
	float deep_surface = -2;

	LC_Block block;
	block.type = LC_BT__NONE;

	//Terrain shaping stage
	if (p_y < surface_y)
	{
		block.type = LC_BT__STONE;
	}
	else if (p_y < sea_level && biome_type != LC_Biome_Desert)
	{
		block.type = LC_BT__WATER;
	}
	else
	{
		block.type = LC_BT__NONE;
	}

	float cave_noise = stb_perlin_ridge_noise3(p_x / 32.0, p_y / 32.0, p_z / 32.0, 2.0, 1.5, 1.07, 2);

	if (block.type != LC_BT__WATER && cave_noise < 0.5 && cave_noise > 0.2)
	{
		if (cave_noise == 0.4)
		{
			//block.type == LC_BT__AMETHYST;
		}
		else
		{
			//block.type = LC_BT__NONE;
		}
		
	}
	else if (block.type == LC_BT__STONE)
	{
		float noise_3d = fabsf(stb_perlin_turbulence_noise3(p_x / 256.0, p_y / 256.0, p_z / 256.0, 2.0, 0.6, 1)) * 25;
		float both = surface_y + noise_3d;
		switch (biome_type)
		{
		case LC_Biome_SnowyMountains:
		{
			if (both > 50)
			{
				block.type = LC_BT__SNOW;
			}
			else if(both * 2 > 60)
			{
				block.type = LC_BT__GRASS_SNOW;
			}
			break;
		}
		case LC_Biome_SnowyPlains:
		{
			if (both > 25)
			{
				block.type = LC_BT__SNOW;
			}
			else
			{
				block.type = LC_BT__GRASS_SNOW;
			}

			break;
		}
		case LC_Biome_RockyMountains:
		{
			if (both > 35)
			{
				block.type = LC_BT__STONE;
			}
			else
			{
				block.type = LC_BT__DIRT;
			}
			break;
		}
		case LC_Biome_Desert:
		{
			block.type = LC_BT__SAND;
			break;
		}
		case LC_Biome_GrassyPlains:
		{
			if (p_y > sea_level)
			{
				block.type = LC_BT__GRASS;
			}
			else if (p_y < deep_surface)
			{
				block.type = LC_BT__STONE;
			}
			else
			{
				block.type = LC_BT__DIRT;
			}
			break;
		}
		default:
		{
			block.type = LC_BT__GRASS;
			break;
		}
		}
		
	}

	/*
	static int test = 0;
	test = Math_rand() % 5;
	if (test == 0)
	{
		block.type = LC_BT__STONE;
	}
	else if (test == 1)
	{
		block.type = LC_BT__GRASS;
	}
	else if (test == 2)
	{
		block.type = LC_BT__DIRT;
	}
	else if (test == 3)
	{
		block.type = LC_BT__DIAMOND;
	}
	else if (test == 4)
	{
		block.type = LC_BT__IRON;
	}
	*/

	return block;
}


LC_Chunk LC_Chunk_Create(int p_x, int p_y, int p_z)
{
	LC_Chunk chunk;
	memset(&chunk, 0, sizeof(LC_Chunk));

	chunk.global_position[0] = p_x;
	chunk.global_position[1] = p_y;
	chunk.global_position[2] = p_z;

	return chunk;
}

uint8_t LC_Chunk_getType(LC_Chunk* const p_chunk, int x, int y, int z)
{
	//bounds check
	if (x < 0 || x >= LC_CHUNK_WIDTH)
	{
		return LC_BT__NONE;
	}
	if (y < 0 || y >= LC_CHUNK_HEIGHT)
	{
		return LC_BT__NONE;
	}
	if (z < 0 || z >= LC_CHUNK_LENGTH)
	{
		return LC_BT__NONE;
	}

	//get block from chunk
	LC_Block* block = &p_chunk->blocks[x][y][z];

	return block->type;
}

LC_Block* LC_Chunk_GetBlock(LC_Chunk* const p_chunk, int x, int y, int z)
{
	//bounds check
	if (x < 0 || x >= LC_CHUNK_WIDTH)
	{
		return NULL;
	}
	if (y < 0 || y >= LC_CHUNK_HEIGHT)
	{
		return NULL;
	}
	if (z < 0 || z >= LC_CHUNK_LENGTH)
	{
		return NULL;
	}

	LC_Block* block = &p_chunk->blocks[x][y][z];

	return block;
}

void LC_Chunk_SetBlock(LC_Chunk* const p_chunk, int x, int y, int z, uint8_t block_type)
{
	//bounds check
	if (x < 0 || x >= LC_CHUNK_WIDTH)
	{
		return;
	}
	if (y < 0 || y >= LC_CHUNK_HEIGHT)
	{
		return;
	}
	if (z < 0 || z >= LC_CHUNK_LENGTH)
	{
		return;
	}

	//remove the old block if presents
	LC_Block* block = &p_chunk->blocks[x][y][z];

	if (block->type != LC_BT__NONE)
	{
		if (LC_isBlockSemiTransparent(block->type))
		{
			p_chunk->transparent_blocks--;
		}
		else if (LC_IsBlockWater(block->type))
		{
			p_chunk->water_blocks--;
		}
		else
		{
			p_chunk->opaque_blocks--;
		}
		if (LC_isblockEmittingLight(block->type))
		{
			p_chunk->light_blocks--;
		}
		p_chunk->alive_blocks--;
	}

	//set the new type
	block->type = block_type;
	
	if (block->type != LC_BT__NONE)
	{
		if (LC_isBlockSemiTransparent(block->type))
		{
			p_chunk->transparent_blocks++;
		}
		else if (LC_IsBlockWater(block->type))
		{
			p_chunk->water_blocks++;
		}
		else
		{
			p_chunk->opaque_blocks++;
		}
		if (LC_isblockEmittingLight(block->type))
		{
			p_chunk->light_blocks++;
		}
		p_chunk->alive_blocks++;
	}
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

static inline void LC_Chunk_GenerateAdditionalBlocks(LC_Chunk* _chunk, int p_x, int p_y, int p_z, int p_gX, int p_gY, int p_gZ)
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
		else if ((Math_rand() % 100) == 0)
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
			&& p_z < LC_CHUNK_LENGTH - 2 && up_block == LC_BT__NONE  && right_block == LC_BT__NONE && front_block == LC_BT__NONE && back_block == LC_BT__NONE)
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
				int maxH = (iy < -1 || iy > 1) ?  0 : 1;
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

void LC_Chunk_GenerateBlocks(LC_Chunk* const _chunk, int _seed)
{	
	for (int x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		for (int z = 0; z < LC_CHUNK_LENGTH; z++)
		{
			for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
			{
				int g_x = x + _chunk->global_position[0];
				int g_y = y + _chunk->global_position[1];
				int g_z = z + _chunk->global_position[2];


				//Generate block
				if (_chunk->blocks[x][y][z].type == LC_BT__NONE)
				{
					//LC_Block generated_block = LC_generateBlock(g_x, g_y, g_z, _seed);

					LC_Block generated_block;

					generated_block.type = LC_Generate_Block(g_x, g_y, g_z, _seed);

					LC_Chunk_SetBlock(_chunk, x, y, z, generated_block.type);
				}
				
				if (_chunk->blocks[x][y][z].type == LC_BT__WATER)
				{
					//LC_Chunk_SetBlock(_chunk, x, y, z, LC_BT__STONE);
				}
				
				
				if (y > 6)
				{
					//LC_Chunk_SetBlock(_chunk, x, y, z, LC_BT__WATER);
				}
				
				//Skip if no block was generated
				if (_chunk->blocks[x][y][z].type == LC_BT__NONE)
				{
					continue;
				}



				//Generate additional blocks like trees, cactuses, etc..
				LC_Chunk_GenerateAdditionalBlocks(_chunk, x, y, z, g_x, g_y, g_z);

				
			}

		}
	}
}


unsigned LC_Chunk_getAliveBlockCount(LC_Chunk* const _chunk)
{
	unsigned alive_blocks = 0;
	for (size_t x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		for (size_t y = 0; y < LC_CHUNK_HEIGHT; y++)
		{
			for (size_t z = 0; z < LC_CHUNK_LENGTH; z++)
			{
				if (_chunk->blocks[x][y][z].type != LC_BT__NONE)
				{
					alive_blocks++;
				}
			}
		}
	}

	return alive_blocks;
}



void LC_Chunk_CalculateWaterBounds(LC_Chunk* const _chunk, ivec3 dest[2])
{
	if (_chunk->water_blocks <= 0)
	{
		glm_ivec3_zero(dest[0]);
		glm_ivec3_zero(dest[1]);
		return;
	}

	int minWaterX = LC_CHUNK_WIDTH;
	int minWaterY = LC_CHUNK_HEIGHT;
	int minWaterZ = LC_CHUNK_LENGTH;

	int maxWaterX = 0;
	int maxWaterY = 0;
	int maxWaterZ = 0;

	int blocks_visited = 0;

	for (int x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
		{
			for (int z = 0; z < LC_CHUNK_LENGTH; z++)
			{
				if (LC_IsBlockWater(_chunk->blocks[x][y][z].type))
				{
					minWaterX = min(minWaterX, x);
					minWaterY = min(minWaterY, y);
					minWaterZ = min(minWaterZ, z);

					maxWaterX = max(maxWaterX, x);
					maxWaterY = max(maxWaterY, y);
					maxWaterZ = max(maxWaterZ, z);

					blocks_visited++;
				}

				
			}
		}
	}

	dest[0][0] = minWaterX;
	dest[0][1] = minWaterY;
	dest[0][2] = minWaterZ;

	dest[1][0] = maxWaterX;
	dest[1][1] = maxWaterY;
	dest[1][2] = maxWaterZ;

}