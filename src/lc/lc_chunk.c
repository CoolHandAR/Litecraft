#include "lc_chunk.h"

#include <string.h>
#include <glad/glad.h>
#include "utility/dynamic_array.h"

#include "render/r_renderDefs.h"
#include <stdio.h>
#include <time.h>

#include "lc_block_defs.h"

#include "perlin_noise/noise1234.h"
#include "render/r_renderer.h"


#define VERTICES_PER_CUBE 36
#define FACES_PER_CUBE 6

static unsigned global_vbo;
static unsigned s_copy_vbo;

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

bool compareBlocks(LC_Block* const b1, LC_Block* const b2)
{
	if (b1->hp != b2->hp)
	{
		return false;
	}

	if (b1->type != b2->type)
	{
		return false;
	}

	return true;
}

Vertex* genCubeVertices(LC_Chunk* const chunk)
{
	Vertex* vertices = calloc(LC_CHUNK_TOTAL_SIZE * VERTICES_PER_CUBE, sizeof(Vertex));

	if (!vertices)
	{
		printf("Failed to malloc cube vertices\n");
		return NULL;
	}
	
	//[0] BACK, [1] FRONT, [2] LEFT, [3] RIGHT, [4] BOTTOM, [5] TOP
	bool drawn_faces[LC_CHUNK_WIDTH][LC_CHUNK_HEIGHT][LC_CHUNK_LENGTH][6];
	memset(&drawn_faces, 0, sizeof(drawn_faces));

	size_t vert_index = 0;

	for (size_t x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		const size_t next_x = (x + 1) % LC_CHUNK_WIDTH;

		for (size_t y = 0; y < LC_CHUNK_HEIGHT; y++)
		{
			const size_t next_y = (y + 1) % LC_CHUNK_HEIGHT;

			for (size_t z = 0; z < LC_CHUNK_LENGTH; z++)
			{
				const size_t next_z = (z + 1) % LC_CHUNK_LENGTH;
					
				//SKIP IF NOT ALIVE
				if (chunk->blocks[x][y][z].type == LC_BT__NONE)
					continue;

				LC_Block_Texture_Offset_Data tex_offset_data = LC_BLOCK_TEX_OFFSET_DATA[chunk->blocks[x][y][z].type];

				bool skip_back = (z > 0 && chunk->blocks[x][y][z - 1].type != LC_BT__NONE) || drawn_faces[x][y][z][0] == true;
				bool skip_front = (next_z != 0 && chunk->blocks[x][y][next_z].type != LC_BT__NONE) || drawn_faces[x][y][z][1] == true;
				bool skip_left = (x > 0 && chunk->blocks[x - 1][y][z].type != LC_BT__NONE) || drawn_faces[x][y][z][2] == true;
				bool skip_right = (next_x != 0 && chunk->blocks[next_x][y][z].type != LC_BT__NONE) || drawn_faces[x][y][z][3] == true;
				bool skip_bottom = (y > 0 && chunk->blocks[x][y - 1][z].type != LC_BT__NONE) || drawn_faces[x][y][z][4] == true;
				bool skip_top = (next_y != 0 && chunk->blocks[x][next_y][z].type != LC_BT__NONE) || drawn_faces[x][y][z][5] == true;


				vec3 position;
				position[0] = x + chunk->global_position[0];
				position[1] = y + chunk->global_position[1];
				position[2] = z + chunk->global_position[2];

				size_t max_x = x;
				size_t max_y = y;
				size_t max_z = z;

				if (chunk->blocks[x][y][z].type == LC_BT__GLOWSTONE)
				{
					R_LightData light_data;
					light_data.color[0] = 1;
					light_data.color[1] = 0.84;
					light_data.color[2] = 0;

					light_data.constant = 0.4;
					light_data.diffuse_intesity = 0.1;
					light_data.linear = 0.09f;
					light_data.quadratic = 0.032f;
					light_data.ambient_intesity = 0.1;
					light_data.position[0] = position[0] - 1;
					light_data.position[1] = position[1] - 1;
					light_data.position[2] = position[2] - 1;
					light_data.radius = 777;

					light_data.light_type = LT__POINT;

					r_registerLightSource(light_data);

				}
				
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
						if (chunk->blocks[x][y_search][z].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z], &chunk->blocks[x][y_search][z]))
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
						if (chunk->blocks[x_search][y][z].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z], &chunk->blocks[x_search][y][z]))
						{
							size_t y_search = y;
							while (y_search <= max_y)
							{
								if (chunk->blocks[x_search][y_search][z].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z], &chunk->blocks[x_search][y_search][z]))
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
					else if(skip_back && !skip_front)
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
							vertices[vert_index].position[0] = position[0] + (0.5f * (to_add_x - 1)) + (CUBE_POSITION_VERTICES[i][0] * to_add_x);
							vertices[vert_index].position[1] = position[1] + (0.5f * (to_add_y - 1)) + (CUBE_POSITION_VERTICES[i][1] * to_add_y);
							vertices[vert_index].position[2] = position[2] + CUBE_POSITION_VERTICES[i][2];

							vertices[vert_index].tex_coords[0] = CUBE_TEX_COORDS[i][0] * to_add_x;
							vertices[vert_index].tex_coords[1] = CUBE_TEX_COORDS[i][1] * to_add_y;

							if (i < 6)
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.back_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.back_face[1];

								glm_vec3_copy(CUBE_NORMALS[0], vertices[vert_index].normal);
							}
							else
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.front_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.front_face[1];

								glm_vec3_copy(CUBE_NORMALS[1], vertices[vert_index].normal);
							}
							
							vertices[vert_index].hp = chunk->blocks[x][y][z].hp;

							vert_index++;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{
							glm_vec3_add(position, CUBE_POSITION_VERTICES[i], vertices[vert_index].position);

							vertices[vert_index].tex_coords[0] = CUBE_TEX_COORDS[i][0];
							vertices[vert_index].tex_coords[1] = CUBE_TEX_COORDS[i][1];

							if (i < 6)
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.back_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.back_face[1];
								glm_vec3_copy(CUBE_NORMALS[0], vertices[vert_index].normal);
							}
							else
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.front_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.front_face[1];
								glm_vec3_copy(CUBE_NORMALS[1], vertices[vert_index].normal);
							}
							vertices[vert_index].hp = chunk->blocks[x][y][z].hp;

							vert_index++;
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
						if (chunk->blocks[x][y][z_search].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z], &chunk->blocks[x][y][z_search]))
						{
							size_t y_search = y;
							while (y_search <= max_y)
							{
								if (chunk->blocks[x][y_search][z_search].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z_search], &chunk->blocks[x][y_search][z_search]))
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
							if(!skip_left) drawn_faces[x][y_mark][z_mark][2] = true;
							if(!skip_right) drawn_faces[x][y_mark][z_mark][3] = true;
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
			
							vertices[vert_index].position[0] = position[0] + CUBE_POSITION_VERTICES[i][0];
							vertices[vert_index].position[1] = position[1] + (0.5f * (to_add_y - 1)) + (CUBE_POSITION_VERTICES[i][1] * to_add_y);
							vertices[vert_index].position[2] = position[2] + (0.5f * (to_add_z - 1)) + (CUBE_POSITION_VERTICES[i][2] * to_add_z);

							vertices[vert_index].tex_coords[0] = CUBE_TEX_COORDS[i][0] * to_add_z;
							vertices[vert_index].tex_coords[1] = CUBE_TEX_COORDS[i][1] * to_add_y;

							if (i < 18)
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.left_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.left_face[1];
								glm_vec3_copy(CUBE_NORMALS[2], vertices[vert_index].normal);
							}
							else
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.right_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.right_face[1];

								glm_vec3_copy(CUBE_NORMALS[3], vertices[vert_index].normal);
							}
							vertices[vert_index].hp = chunk->blocks[x][y][z].hp;

							vert_index++;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{

							glm_vec3_add(position, CUBE_POSITION_VERTICES[i], vertices[vert_index].position);

							vertices[vert_index].tex_coords[0] = CUBE_TEX_COORDS[i][0];
							vertices[vert_index].tex_coords[1] = CUBE_TEX_COORDS[i][1];

							if (i < 18)
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.left_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.left_face[1];
								glm_vec3_copy(CUBE_NORMALS[2], vertices[vert_index].normal);
							}
							else
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.right_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.right_face[1];
								glm_vec3_copy(CUBE_NORMALS[3], vertices[vert_index].normal);
							}
							vertices[vert_index].hp = chunk->blocks[x][y][z].hp;

							vert_index++;
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
						if (chunk->blocks[x_search][y][z].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z], &chunk->blocks[x_search][y][z]))
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
						if (chunk->blocks[x][y][z_search].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z], &chunk->blocks[x][y][z_search]))
						{
							size_t x_search_2 = x;
							while (x_search_2 <= max_x)
							{
								if (chunk->blocks[x_search_2][y][z_search].type != LC_BT__NONE && compareBlocks(&chunk->blocks[x][y][z], &chunk->blocks[x_search_2][y][z_search]))
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
							
							vertices[vert_index].position[0] = position[0] + (0.5f * (to_add_x - 1)) + (CUBE_POSITION_VERTICES[i][0] * to_add_x);
							vertices[vert_index].position[1] = position[1] + CUBE_POSITION_VERTICES[i][1];
							vertices[vert_index].position[2] = position[2] + (0.5f * (to_add_z - 1)) + (CUBE_POSITION_VERTICES[i][2] * to_add_z);

							vertices[vert_index].tex_coords[0] = CUBE_TEX_COORDS[i][0] * to_add_x;
							vertices[vert_index].tex_coords[1] = CUBE_TEX_COORDS[i][1] * to_add_z;

							if (i < 30)
							{	
								vertices[vert_index].tex_offset[0] = tex_offset_data.bottom_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.bottom_face[1];
								glm_vec3_copy(CUBE_NORMALS[4], vertices[vert_index].normal);
							}
							else
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.top_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.top_face[1];
								glm_vec3_copy(CUBE_NORMALS[5], vertices[vert_index].normal);
							}
							vertices[vert_index].hp = chunk->blocks[x][y][z].hp;

							vert_index++;
						}
					}
					//Otherwise do a normal vertice transfer
					else
					{
						for (int i = start_itr; i < max_itr; i++)
						{
							glm_vec3_add(position, CUBE_POSITION_VERTICES[i], vertices[vert_index].position);

							vertices[vert_index].tex_coords[0] = CUBE_TEX_COORDS[i][0];
							vertices[vert_index].tex_coords[1] = CUBE_TEX_COORDS[i][1];

							if (i < 30)
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.bottom_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.bottom_face[1];
								glm_vec3_copy(CUBE_NORMALS[4], vertices[vert_index].normal);
							}
							else
							{
								vertices[vert_index].tex_offset[0] = tex_offset_data.top_face[0];
								vertices[vert_index].tex_offset[1] = tex_offset_data.top_face[1];
								glm_vec3_copy(CUBE_NORMALS[5], vertices[vert_index].normal);
							}
							vertices[vert_index].hp = chunk->blocks[x][y][z].hp;

							vert_index++;
						}
					}
					
				}
			}
				
		}
	}
	
	chunk->vertex_count = vert_index;

	return vertices;
}


static LC_Block genBlock(int x, int y, int z, struct osn_context* osn_ctx)
{
	LC_Block block;
	float surface_y = Noise_SimplexNoise2D(x / 256.0f, z /256.0f, osn_ctx, 4, 0.5) * (24);
	
	surface_y = fabsf(surface_y);

	float sea_level = 12;

	if (y < surface_y)
	{
		block.type = LC_BT__STONE;
	}
	else if (y < sea_level)
	{
		block.type = LC_BT__WATER;
	}
	else
	{
		block.type = LC_BT__NONE;
	}


	float noise_3d = Noise_SimplexNoise3Dabs(x / 256.0f, y / 256.0f, z / 256.0f, osn_ctx, 4, 0.5f) * 24;


	if (surface_y + noise_3d < y * 4)
	{
		if (block.type == LC_BT__STONE)
		{
			block.type = LC_BT__GRASS;
		}
		
	}
	
	block.hp = LC_BLOCK_STARTING_HP;

	return block;
}

void LC_Chunk_CreateEmpty(int p_gX, int p_gY, int p_gZ, LC_Chunk* chunk)
{
	memset(chunk, 0, sizeof(LC_Chunk));

	//MIN POINT
	chunk->box[0][0] = p_gX - 0.5;
	chunk->box[0][1] = p_gY - 0.5;
	chunk->box[0][2] = p_gZ - 0.5;

	//MAX POINT
	chunk->box[1][0] = p_gX + LC_CHUNK_WIDTH;
	chunk->box[1][1] = p_gY + LC_CHUNK_HEIGHT;
	chunk->box[1][2] = p_gZ + LC_CHUNK_LENGTH;

	//SET GLOBAL POS
	chunk->global_position[0] = p_gX;
	chunk->global_position[1] = p_gY;
	chunk->global_position[2] = p_gZ;
}

void LC_Chunk_Generate(int p_gX, int p_gY, int p_gZ, struct osn_context* osn_ctx, LC_Chunk* chunk, int chunk_index, unsigned vbo, unsigned coby_vbo, unsigned* vertex_count)
{
	LC_Chunk_CreateEmpty(p_gX, p_gY, p_gZ, chunk);

	int blocks = 0;

	for (size_t x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		for (size_t z = 0; z < LC_CHUNK_LENGTH; z++)
		{
			float noise = Noise_SimplexNoise2D((x + p_gX * LC_CHUNK_WIDTH) / 256.0f, (z + p_gZ * LC_CHUNK_HEIGHT) / 256.0f, osn_ctx, 5, 4.8) * 4;

			for (size_t y = 0; y < LC_CHUNK_HEIGHT; y++)
			{
				blocks++;
					
				chunk->blocks[x][y][z] = genBlock(x + p_gX, y + p_gY, z + p_gZ, osn_ctx);

				//chunk->blocks[x][y][z].type = LC_BT__GRASS;

				if (chunk->blocks[x][y][z].type != LC_BT__NONE)
				{
					chunk->alive_blocks++;
				}
				
				if (chunk->blocks[x][y][z].type == LC_BT__GLOWSTONE)
				{
					R_LightData light_data;
					light_data.color[0] = 1;
					light_data.color[1] = 1;
					light_data.color[2] = 1;

					light_data.constant = 1;
					light_data.diffuse_intesity = 0.1;
					light_data.linear = 0;
					light_data.ambient_intesity = 1;
					light_data.position[0] = x + p_gX;
					light_data.position[1] = y + p_gY;
					light_data.position[2] = z + p_gZ;

					light_data.light_type = LT__POINT;

					r_registerLightSource(light_data);
				}
				

			}
			
			

		}
	}

	if (chunk->alive_blocks == 0)
		return;

	Vertex* vertices = genCubeVertices(chunk);

	//glBindVertexArray(chunk->vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);


	global_vbo = vbo;
	s_copy_vbo = coby_vbo;


	glBufferSubData(GL_ARRAY_BUFFER, *vertex_count * sizeof(Vertex), sizeof(Vertex) * chunk->vertex_count, vertices);


	chunk->vertex_start = *vertex_count;

	*vertex_count += chunk->vertex_count;

	free(vertices);

}

void LC_Chunk_Update(LC_Chunk* const p_chunk, int* last_index)
{
	size_t prev_vertex_count = p_chunk->vertex_count;

	Vertex* vertices = genCubeVertices(p_chunk);
	
	if (!vertices)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, global_vbo);
	
	//no previous vertex data? emplace to the back
	if (prev_vertex_count == 0 && p_chunk->vertex_start == 0)
	{
		glNamedBufferSubData(global_vbo, *last_index * sizeof(Vertex), p_chunk->vertex_count * sizeof(Vertex), vertices);
		//copy to copy vbo
		glCopyNamedBufferSubData(global_vbo, s_copy_vbo, *last_index * sizeof(Vertex), *last_index * sizeof(Vertex), p_chunk->vertex_count * sizeof(Vertex));
		p_chunk->vertex_start = *last_index;
		*last_index += p_chunk->vertex_count;
	}
	//else we update the data
	else
	{
		int64_t size_to_move = (*last_index - (p_chunk->vertex_start + prev_vertex_count)) * sizeof(Vertex);

		assert(size_to_move >= 0);
		
		//move the data of the others chunks forward or backward
		if (prev_vertex_count != p_chunk->vertex_count)
		{
			//copy from copy vbo to the main vbo
			glCopyNamedBufferSubData(s_copy_vbo, global_vbo, (p_chunk->vertex_start + prev_vertex_count) * sizeof(Vertex), (p_chunk->vertex_start + p_chunk->vertex_count) * sizeof(Vertex), size_to_move);
			
			if (prev_vertex_count > p_chunk->vertex_count)
			{
				unsigned remaineder = prev_vertex_count - p_chunk->vertex_count;
				*last_index -= remaineder;
			}
			else if (prev_vertex_count < p_chunk->vertex_count)
			{
				unsigned remaineder = p_chunk->vertex_count - prev_vertex_count;
				*last_index += remaineder;
			}
			glCopyNamedBufferSubData(global_vbo, s_copy_vbo, (p_chunk->vertex_start + p_chunk->vertex_count) * sizeof(Vertex), (p_chunk->vertex_start + p_chunk->vertex_count) * sizeof(Vertex), size_to_move);
		}
		//update the vertex data
		glNamedBufferSubData(global_vbo, p_chunk->vertex_start * sizeof(Vertex), p_chunk->vertex_count * sizeof(Vertex), vertices);
		glNamedBufferSubData(s_copy_vbo, p_chunk->vertex_start * sizeof(Vertex), p_chunk->vertex_count * sizeof(Vertex), vertices);
	
	}
	free(vertices);
}


void LC_Chunk_Destroy(LC_Chunk* chunk)
{
	
	chunk->vertex_count = 0;

	chunk->flags = 0;

}
