
#include "lc2/lc_chunk.h"
#include "lc/lc_block_defs.h"
#include <string.h>
#include "render/r_renderer.h"

#include <stb_perlin/stb_perlin.h>


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

	if (b1.hp != b2.hp)
	{
		return false;
	}

	return true;
}

bool LC_isBlockOpaque(uint8_t block_type)
{
	return !LC_isBlockTransparent(block_type) && block_type != LC_BT__WATER;
}
bool LC_isBlockCollidable(uint8_t block_type)
{
	return LC_BLOCK_MISC_DATA[block_type].collidable >= 1;
}
bool LC_isblockEmittingLight(uint8_t block_type)
{
	return LC_BLOCK_MISC_DATA[block_type].emits_light >= 1;
}
bool LC_isBlockTransparent(uint8_t block_type)
{
	return LC_BLOCK_MISC_DATA[block_type].material_type == 1;
}
bool LC_isBlockWater(uint8_t blockType)
{
	return LC_BLOCK_MISC_DATA[blockType].material_type == 2;
}

static bool skipCheck(LC_Block const b1, LC_Block const b2)
{
	//If the other block is none, dont skip
	if (b2.type == LC_BT__NONE)
	{
		return false;
	}
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
	//if the first one isn't water and the other is don't skip
	if (!LC_isBlockWater(b1.type) && LC_isBlockWater(b2.type))
	{
		return false;
	}

	//we skip
	return true;
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
	ChunkVertex* water_vertices = NULL;
	
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

	if (chunk->water_blocks > 0)
	{
		water_vertices = calloc(chunk->water_blocks * VERTICES_PER_CUBE, sizeof(ChunkVertex));

		if (!water_vertices)
		{
			printf("Failed to malloc cube vertices\n");
			return NULL;
		}
	}


	//[0] BACK, [1] FRONT, [2] LEFT, [3] RIGHT, [4] BOTTOM, [5] TOP
	bool drawn_faces[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][LC_CHUNK_LENGTH][6];
	memset(&drawn_faces, 0, sizeof(drawn_faces));

	ChunkVertex* buffer = NULL;
	size_t* index = NULL;

	size_t vert_index = 0;
	size_t transparent_index = 0;
	size_t water_index = 0;

	float add_offset = 0;
	float prev_offset = 0;

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

				LC_Block_Texture_Offset_Data tex_offset_data = LC_BLOCK_TEX_OFFSET_DATA[chunk->blocks[x][y][z].type];

				bool skip_back = (z > 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y][z - 1])) || drawn_faces[x][y][z][0] == true || chunk->blocks[x][y][z].type == LC_BT__WATER;
				bool skip_front = (next_z != 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y][next_z])) || drawn_faces[x][y][z][1] == true || chunk->blocks[x][y][z].type == LC_BT__WATER;
				bool skip_left = (x > 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x - 1][y][z])) || drawn_faces[x][y][z][2] == true || chunk->blocks[x][y][z].type == LC_BT__WATER;
				bool skip_right = (next_x != 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[next_x][y][z])) || drawn_faces[x][y][z][3] == true || chunk->blocks[x][y][z].type == LC_BT__WATER;
				bool skip_bottom = (y > 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][y - 1][z])) || drawn_faces[x][y][z][4] == true || chunk->blocks[x][y][z].type == LC_BT__WATER;
				bool skip_top = (next_y != 0 && skipCheck(chunk->blocks[x][y][z], chunk->blocks[x][next_y][z])) || drawn_faces[x][y][z][5] == true;

				//choose buffer and index
				if (LC_isBlockTransparent(chunk->blocks[x][y][z].type))
				{
					buffer = transparent_vertices;
					index = &transparent_index;
					//add_offset = (((float)rand() / (float)RAND_MAX) / 1000.0f);
					prev_offset += 0.0001;
					add_offset = prev_offset;
				}
				else if (LC_isBlockWater(chunk->blocks[x][y][z].type))
				{
					buffer = water_vertices;
					index = &water_index;
					add_offset = 0;
				}
				else
				{
					buffer = vertices;
					index = &vert_index;
					add_offset = 0;
				}

				//float add_offset = (LC_isBlockTransparent(chunk->blocks[x][y][z].type)) ? 0.01 : 0; //add small offset to avoid z_fighting issues for transparent objects

				vec3 position;
				position[0] = (x + chunk->global_position[0]) + add_offset;
				position[1] = (y + chunk->global_position[1]) + add_offset;
				position[2] = (z + chunk->global_position[2]) + add_offset;

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
							buffer[*index].position[0] = position[0] + (0.5f * (to_add_x - 1)) + (CUBE_POSITION_VERTICES[i][0] * to_add_x);
							buffer[*index].position[1] = position[1] + (0.5f * (to_add_y - 1)) + (CUBE_POSITION_VERTICES[i][1] * to_add_y);
							buffer[*index].position[2] = position[2] + CUBE_POSITION_VERTICES[i][2];

							if (i < 6)
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.back_face[1]) + tex_offset_data.back_face[0];

								buffer[*index].normal = -3;
							}
							else
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.front_face[1]) + tex_offset_data.front_face[0];

								buffer[*index].normal = 3;
							}

							buffer[*index].hp = chunk->blocks[x][y][z].hp;
						

							*index = *index + 1;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{
							glm_vec3_add(position, CUBE_POSITION_VERTICES[i], buffer[*index].position);

							if (i < 6)
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.back_face[1]) + tex_offset_data.back_face[0];
								buffer[*index].normal = -3;
							}
							else
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.front_face[1]) + tex_offset_data.front_face[0];
								buffer[*index].normal = 3;
							}
							buffer[*index].hp = chunk->blocks[x][y][z].hp;
					

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

							buffer[*index].position[0] = position[0] + CUBE_POSITION_VERTICES[i][0];
							buffer[*index].position[1] = position[1] + (0.5f * (to_add_y - 1)) + (CUBE_POSITION_VERTICES[i][1] * to_add_y);
							buffer[*index].position[2] = position[2] + (0.5f * (to_add_z - 1)) + (CUBE_POSITION_VERTICES[i][2] * to_add_z);

							if (i < 18)
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.left_face[1]) + tex_offset_data.left_face[0];
								buffer[*index].normal = -1;
							}
							else
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.right_face[1]) + tex_offset_data.right_face[0];

								buffer[*index].normal = 1;
							}
							buffer[*index].hp = chunk->blocks[x][y][z].hp;
						

							*index = *index + 1;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{

							glm_vec3_add(position, CUBE_POSITION_VERTICES[i], buffer[*index].position);

							if (i < 18)
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.left_face[1]) + tex_offset_data.left_face[0];
								buffer[*index].normal = -1;
							}
							else
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.right_face[1]) + tex_offset_data.right_face[0];
								buffer[*index].normal = 1;
							}
							buffer[*index].hp = chunk->blocks[x][y][z].hp;
						

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

						for (int i = start_itr; i < max_itr; i++)
						{

							buffer[*index].position[0] = position[0] + (0.5f * (to_add_x - 1)) + (CUBE_POSITION_VERTICES[i][0] * to_add_x);
							buffer[*index].position[1] = position[1] + CUBE_POSITION_VERTICES[i][1];
							buffer[*index].position[2] = position[2] + (0.5f * (to_add_z - 1)) + (CUBE_POSITION_VERTICES[i][2] * to_add_z);

							if (i < 30)
							{
								
								buffer[*index].texture_offset = (64 * tex_offset_data.bottom_face[1]) + tex_offset_data.bottom_face[0];
								buffer[*index].normal = -2;
							}
							else
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.top_face[1]) + tex_offset_data.top_face[0];
								buffer[*index].normal = 2;
							}
							buffer[*index].hp = chunk->blocks[x][y][z].hp;
							

							*index = *index + 1;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{
							glm_vec3_add(position, CUBE_POSITION_VERTICES[i], buffer[*index].position);

							if (i < 30)
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.bottom_face[1]) + tex_offset_data.bottom_face[0];
								buffer[*index].normal = -2;
							}
							else
							{
								buffer[*index].texture_offset = (64 * tex_offset_data.top_face[1]) + tex_offset_data.top_face[0];
								buffer[*index].normal = 2;
							}
							buffer[*index].hp = chunk->blocks[x][y][z].hp;
							

							*index = *index + 1;
						}
					}

				}
			}

		}
	}

	chunk->vertex_count = vert_index;
	chunk->transparent_vertex_count = transparent_index;
	chunk->water_vertex_count = water_index;

	result->opaque_vertices = vertices;
	result->transparent_vertices = transparent_vertices;
	result->water_vertices = water_vertices;

	return result;
}


static LC_Block genBlock(float p_x, float p_y, float p_z, struct osn_context* osn_ctx)
{
	float surface_y = Noise_SimplexNoise2D(p_x / 16.0f, p_z / 16.0f, osn_ctx, 3, 0.4) * (12);

	surface_y = fabsf(surface_y);

	float sea_level = 12;

	LC_Block block;

	//Terrain shaping stage
	if (p_y < surface_y)
	{
		block.type = LC_BT__STONE;
	}
	else if (p_y < sea_level)
	{
		block.type = LC_BT__WATER;
	}
	else
	{
		block.type = LC_BT__NONE;
	}

	float noise_3d = Noise_SimplexNoise3Dabs(p_x / 16.0f, p_y / 16.0f, p_z / 16.0f, osn_ctx, 4, 0.5f) * 12;
	
	//what blocks to appear
	if (block.type == LC_BT__STONE)
	{
		if (surface_y + noise_3d < p_y * 4)
		{
			block.type = LC_BT__GRASS;
		}
		else if (surface_y + noise_3d > p_y * 3)
		{
			block.type = LC_BT__SAND;
		}
		if (surface_y + noise_3d > p_y * 7)
		{
			block.type = LC_BT__STONE;
		}
		else if (surface_y + noise_3d - 2 > p_y * 5)
		{
			block.type = LC_BT__DIRT;
		}
	}
	

	block.hp = LC_BLOCK_STARTING_HP;

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

static uint8_t LC_Chunk_getType(LC_Chunk* const p_chunk, int x, int y, int z)
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

	//get block from chunk
	LC_Block* block = &p_chunk->blocks[x][y][z];

	return block->type;
}

static void LC_Chunk_SetBlock(LC_Chunk* const p_chunk, int x, int y, int z, uint8_t block_type)
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

	//get block from chunk
	LC_Block* block = &p_chunk->blocks[x][y][z];

	if (block->type != LC_BT__NONE)
	{
		if (LC_isBlockTransparent(block->type))
		{
			p_chunk->transparent_blocks--;
		}
		else if (LC_isBlockWater(block->type))
		{
			p_chunk->water_blocks--;
		}
		else
		{
			p_chunk->opaque_blocks--;
		}
		p_chunk->alive_blocks--;
	}

	//set the new type
	block->type = block_type;
	
	if (block->type != LC_BT__NONE)
	{
		if (LC_isBlockTransparent(block->type))
		{
			p_chunk->transparent_blocks++;
		}
		else if (LC_isBlockWater(block->type))
		{
			p_chunk->water_blocks++;
		}
		else
		{
			p_chunk->opaque_blocks++;
		}
		p_chunk->alive_blocks++;
	}
}

void LC_Chunk_GenerateBlocks(LC_Chunk* const _chunk, struct osn_context* osn_ctx)
{
	
	for (size_t x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		for (size_t z = 0; z < LC_CHUNK_LENGTH; z++)
		{
			for (size_t y = 0; y < LC_CHUNK_HEIGHT; y++)
			{
				int g_x = x + _chunk->global_position[0];
				int g_y = y + _chunk->global_position[1];
				int g_z = z + _chunk->global_position[2];

				if (_chunk->blocks[x][y][z].type == LC_BT__NONE)
				{
					_chunk->blocks[x][y][z] = genBlock(g_x, g_y, g_z, osn_ctx);
				}
				_chunk->blocks[x][y][z].hp = 7;

				if (g_y > 15 && y > 3 && x > 3 && z > 3 &&_chunk->blocks[x][y - 1][z].type == LC_BT__GRASS && (rand() & 0xff) == 0)
				{
					int tree_height = (rand() & 0x3) + 4;

					//trunk
					for (int i = 0; i < tree_height; i++)
					{
						LC_Chunk_SetBlock(_chunk, x, y + i, z, LC_BT__TRUNK);

					}
					//leaves
					for (int ix = -2; ix <= 2; ix++)
					{
						for (int iy = -2; iy <= 2; iy++)
						{
							for (int iz = -2; iz <= 2; iz++)
							{
								if (LC_Chunk_getType(_chunk, x + ix, y + tree_height + iy, z + iz) == LC_BT__NONE)
								{
									LC_Chunk_SetBlock(_chunk, x + ix, y + tree_height + iy, z + iz, LC_BT__TREELEAVES);
								}
							}
						}
					}
				}
				else
				{
					_chunk->blocks[x][y][z].type = LC_BT__GRASS;
					if (LC_isBlockTransparent(_chunk->blocks[x][y][z].type))
					{
						_chunk->transparent_blocks++;
					}
					else if (_chunk->blocks[x][y][z].type == LC_BT__WATER)
					{
						_chunk->water_blocks++;
					}
					else
					{
						_chunk->opaque_blocks++;
					}

					if (_chunk->blocks[x][y][z].type != LC_BT__NONE)
					{
						_chunk->alive_blocks++;
					}
				}
			
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

