#include "lc/lc_chunk.h"

#include <string.h>
#include <assert.h>

#include <glad/glad.h>

#include "lc/lc_common.h"

#define VERTICES_PER_CUBE 36
#define FACES_PER_CUBE 6

extern void LC_GenerateAdditionalBlocks(LC_Chunk* _chunk, int p_x, int p_y, int p_z, int p_gX, int p_gY, int p_gZ);

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


static bool LC_Chunk_CompareBlocks(LC_Block const b1, LC_Block const b2)
{
	if (b1.type != b2.type)
	{
		return false;
	}

	return true;
}
//#define CULL_SKIP_TRANSPARENT_FACES
static inline bool LC_Chunk_skipCheck(LC_Block const b1, LC_Block const b2)
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

static void LC_Chunk_SetBit(uint16_t drawn_faces[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][6], int x, int y, int z, int face, bool p_bool)
{
	if (p_bool == true)
	{
		drawn_faces[x][y][face] |= (1 << z);
	}
	else
	{
		drawn_faces[x][y][face] &= ~(1 << z);
	}
}
static bool LC_Chunk_CheckBit(uint16_t drawn_faces[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][6], int x, int y, int z, int face)
{
	return drawn_faces[x][y][face] & (1 << z);
}

GeneratedChunkVerticesResult* LC_Chunk_GenerateVertices(LC_Chunk* const chunk)
{
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

	uint8_t min_water_x = LC_CHUNK_WIDTH;
	uint8_t min_water_y = LC_CHUNK_HEIGHT;
	uint8_t min_water_z = LC_CHUNK_LENGTH;

	uint8_t max_water_x = 0;
	uint8_t max_water_y = 0;
	uint8_t max_water_z = 0;

	uint16_t drawn_faces[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][6];
	memset(&drawn_faces, 0, sizeof(drawn_faces));

	ChunkVertex* buffer = NULL;
	size_t* index = NULL;

	size_t vert_index = 0;
	size_t transparent_index = 0;
	size_t water_index = 0;

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

				//The water is done seperately
				if (LC_IsBlockWater(chunk->blocks[x][y][z].type))
				{
					max_water_x = max(max_water_x, x);
					max_water_y = max(max_water_y, y);
					max_water_z = max(max_water_z, z);

					min_water_x = min(min_water_x, x);
					min_water_y = min(min_water_y, y);
					min_water_z = min(min_water_z, z);
					continue;
				}

				LC_Block_Texture_Offset_Data tex_offset_data = LC_BLOCK_TEX_OFFSET_DATA[chunk->blocks[x][y][z].type];

				bool skip_back = (z > 0 && LC_Chunk_skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y][z - 1])) || LC_Chunk_CheckBit(drawn_faces, x, y, z, 0) == true;
				bool skip_front = (next_z != 0 && LC_Chunk_skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y][next_z])) || LC_Chunk_CheckBit(drawn_faces, x, y, z, 1) == true;
				bool skip_left = (x > 0 && LC_Chunk_skipCheck(chunk->blocks[x][y][z], chunk->blocks[x - 1][y][z])) || LC_Chunk_CheckBit(drawn_faces, x, y, z, 2) == true;
				bool skip_right = (next_x != 0 && LC_Chunk_skipCheck(chunk->blocks[x][y][z], chunk->blocks[next_x][y][z])) || LC_Chunk_CheckBit(drawn_faces, x, y, z, 3) == true;
				bool skip_bottom = (y > 0 && LC_Chunk_skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y - 1][z])) || LC_Chunk_CheckBit(drawn_faces, x, y, z, 4) == true;
				bool skip_top = (next_y != 0 && LC_Chunk_skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][next_y][z])) || LC_Chunk_CheckBit(drawn_faces, x, y, z, 5) == true;


				/*
				if (z == 0 && neighbours[0])
				{
					//skip_back = skipCheck(chunk->blocks[x][y][z], neighbours[0]->blocks[x][y][LC_CHUNK_LENGTH - 1]);
				}
				else if (z == LC_CHUNK_LENGTH - 1 && neighbours[1])
				{
					//skip_front = skipCheck(chunk->blocks[x][y][z], neighbours[1]->blocks[x][y][0]);
				}
				if (x == 0 && neighbours[2])
				{
					//skip_left = skipCheck(chunk->blocks[x][y][z], neighbours[2]->blocks[LC_CHUNK_WIDTH - 1][y][z]);
				}
				else if (x == LC_CHUNK_WIDTH - 1 && neighbours[3])
				{
					//skip_right = skipCheck(chunk->blocks[x][y][z], neighbours[3]->blocks[0][y][z]);
				}
				if (y == 0 && neighbours[4])
				{
					//skip_bottom = skipCheck(chunk->blocks[x][y][z], neighbours[4]->blocks[x][LC_CHUNK_HEIGHT - 1][z]);
				}
				else if (y == LC_CHUNK_HEIGHT - 1 && neighbours[5])
				{
					//skip_top = skipCheck(chunk->blocks[x][y][z], neighbours[5]->blocks[x][0][z]);
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
						if (chunk->blocks[x][y_search][z].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z], chunk->blocks[x][y_search][z]))
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
						if (chunk->blocks[x_search][y][z].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search][y][z]))
						{
							size_t y_search = y;
							while (y_search <= max_y)
							{
								if (chunk->blocks[x_search][y_search][z].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search][y_search][z]))
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
							if (!skip_back) LC_Chunk_SetBit(drawn_faces, x_mark, y_mark, z, 0, true);
							if (!skip_front) LC_Chunk_SetBit(drawn_faces, x_mark, y_mark, z, 1, true);
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
							buffer[*index].normal = norm;

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
							buffer[*index].normal = norm;

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
						if (chunk->blocks[x][y][z_search].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z], chunk->blocks[x][y][z_search]))
						{
							size_t y_search = y;
							while (y_search <= max_y)
							{
								if (chunk->blocks[x][y_search][z_search].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z_search], chunk->blocks[x][y_search][z_search]))
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
							if (!skip_left) LC_Chunk_SetBit(drawn_faces, x, y_mark, z_mark, 2, true);
							if (!skip_right) LC_Chunk_SetBit(drawn_faces, x, y_mark, z_mark, 3, true);
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
							buffer[*index].normal = norm;

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
							buffer[*index].normal = norm;


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
						if (chunk->blocks[x_search][y][z].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search][y][z]))
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
						if (chunk->blocks[x][y][z_search].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z], chunk->blocks[x][y][z_search]))
						{
							size_t x_search_2 = x;
							while (x_search_2 <= max_x)
							{
								if (chunk->blocks[x_search_2][y][z_search].type != LC_BT__NONE && LC_Chunk_CompareBlocks(chunk->blocks[x][y][z], chunk->blocks[x_search_2][y][z_search]))
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
							if (!skip_bottom) LC_Chunk_SetBit(drawn_faces, x_mark, y, z_mark, 4, true);
							if (!skip_top) LC_Chunk_SetBit(drawn_faces, x_mark, y, z_mark, 5, true);
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
							buffer[*index].normal = norm;

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
							buffer[*index].normal = norm;

							*index = *index + 1;
						}
					}

				}
			}

		}
	}

	if (chunk->water_blocks > 0)
	{
		//add a small offset
		max_water_x += 1;
		max_water_z += 1;

		size_t total_vertices = (max_water_x - min_water_x) * (max_water_z - min_water_z) * 6;
		water_vertices = calloc(total_vertices, sizeof(ChunkWaterVertex));

		if (!water_vertices)
		{
			printf("Failed to malloc cube vertices\n");
			return NULL;
		}

		water_index = 0;

		for (int x = min_water_x; x < max_water_x; x++)
		{
			for (int z = min_water_z; z < max_water_z; z++)
			{
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
	}


	result->opaque_vertex_count = vert_index;
	result->transparent_vertex_count = transparent_index;
	result->water_vertex_count = water_index;

	result->opaque_vertices = vertices;
	result->transparent_vertices = transparent_vertices;
	result->water_vertices = water_vertices;

	return result;
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
					LC_Block generated_block;

					generated_block.type = LC_Generate_Block(g_x, g_y, g_z);

					LC_Chunk_SetBlock(_chunk, x, y, z, generated_block.type);

					if (generated_block.type != LC_BT__NONE)
					{
						LC_GenerateAdditionalBlocks(_chunk, x, y, z, g_x, g_y, g_z);
					}
				}
				
			}

		}
	}
}
