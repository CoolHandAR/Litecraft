#include "lc_world.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin/stb_perlin.h"

#include <glad/glad.h>

#include "render/r_renderer.h"

#include "render/r_renderDefs.h"

static vec3 SUN_SIZE = { 200, 20, 200 };
static LC_World* current_world = NULL;
static struct osn_context* OSN_S;
static vec3 WORLD_CENTER;

typedef struct LC_ChunksInfo
{
	vec4 min_point;
	unsigned vertex_start;
	unsigned vertex_count;
} LC_ChunksInfo;


static void _updateClouds(float delta)
{
	for (int i = 0; i < LC_WORLD_MAX_CLOUDS; i++)
	{
		current_world->clouds[i][0] += 0.01;
		current_world->clouds[i][2] += 0.01;
	}
}

static void _updateSun(float delta)
{
	current_world->sun.position[0] += 300 * delta;

	Math_vec3_dir_to(current_world->sun.position, WORLD_CENTER, current_world->sun.direction_to_center);
	glm_vec3_negate(current_world->sun.direction_to_center);
	glm_vec3_normalize(current_world->sun.direction_to_center);
	
}

static void _updateChunksInfoSSBO()
{
	if (current_world->dirty_bit == false)
	{
		return;
	}

	LC_Chunk* chunk_array = current_world->chunks->pool->data;
	LC_ChunksInfo* c_info = malloc(sizeof(LC_ChunksInfo) * current_world->chunks->pool->elements_size);

	if (!c_info)
	{
		printf("MALLOC FAILURE \n");
		return;
	}
	memset(c_info, 0, sizeof(LC_ChunksInfo) * current_world->chunks->pool->elements_size);

	for (int i = 0; i < current_world->chunks->pool->elements_size; i++)
	{
		LC_Chunk* chunk = &chunk_array[i];
		LC_ChunksInfo* current_chunk_info = &c_info[i];

		current_chunk_info->min_point[0] = chunk->global_position[0] - 0.5;
		current_chunk_info->min_point[1] = chunk->global_position[1] - 0.5;
		current_chunk_info->min_point[2] = chunk->global_position[2] - 0.5;
		
		current_chunk_info->vertex_count = (chunk->vertex_count > 0) ? chunk->vertex_count : 0;
		current_chunk_info->vertex_start = chunk->vertex_start;
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, current_world->chunks_info_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LC_ChunksInfo) * current_world->chunks->pool->elements_size, c_info, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, current_world->chunks_info_ssbo);

	free(c_info);

	current_world->dirty_bit = false;
}

static void _updateBlocksInfoUBO()
{

}

static LC_Chunk* _getChunk(int p_index)
{
	if (p_index <= -1 || p_index >= current_world->chunks->pool->elements_size)
	{
		return NULL;
	}

	LC_Chunk* array = current_world->chunks->pool->data;
	LC_Chunk* chunk = &array[p_index];

	return chunk;
}

LC_World* LC_World_Generate()
{
	LC_World* world = malloc(sizeof(LC_World));

	if (!world)
	{
		printf("MALLOC FAILURE \n");
		return NULL;
	}

	memset(world, 0, sizeof(LC_World));

	world->dirty_bit = true;

	world->sun.position[0] = -3000;
	world->sun.position[1] = 2000;
	world->sun.position[2] = 0;

	world->sun.sun_size[0] = 200;
	world->sun.sun_size[1] = 20;
	world->sun.sun_size[2] = 200;
	
	world->sun.sun_color[0] = 0.6;
	world->sun.sun_color[1] = 0.6;
	world->sun.sun_color[2] = 0.5;
	world->sun.sun_color[3] = 0.97;
	
	world->sun.ambient_intensity = 0.3;
	world->sun.diffuse_intensity = 0.3;

	WORLD_CENTER[0] = (LC_WORLD_MAX_WIDTH / 2) * LC_CHUNK_WIDTH;
	WORLD_CENTER[1] = (LC_WORLD_MAX_HEIGHT / 2) * LC_CHUNK_HEIGHT;
	WORLD_CENTER[2] = (LC_WORLD_MAX_LENGTH / 2) * LC_CHUNK_LENGTH;

	world->time_of_day = LC_ToD__AFTERNOON;

	//create phys world instance
	world->phys_world = PhysWorld_Create(LC_WORLD_GRAVITY);

	//Init data structures
	world->entities = FL_INIT(LC_Entity);
	world->chunks = Object_Pool_INIT(LC_Chunk, 0);
	
	//Reserve for faster generation
	dA_reserve(world->chunks->pool, LC_WORLD_INIT_TOTAL_SIZE);

	current_world = world;

	//generate seed by system time
	world->gen_seed = time(NULL);

	memset(world->chunk_indexes, -1, sizeof(world->chunk_indexes));

	//Generate simplex noise context
	struct osn_context* ctx;
	open_simplex_noise(world->gen_seed, &ctx);
	
	world->game_type = LC_GT__SURVIVAL;


	//GEN VAO
	glGenVertexArrays(1, &world->vao);

	//GEN VBO
	glGenBuffers(1, &world->vbo);
	glGenBuffers(1, &world->copy_vbo);

	glBindVertexArray(world->vao);
	glBindBuffer(GL_ARRAY_BUFFER, world->vbo);

	//BUFFER THE VERTICIES
	glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vertex), NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, world->copy_vbo);

	glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vertex), NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, world->vbo);

	//SETUP ATTRIB POINTERS
	const int STRIDE_SIZE = sizeof(Vertex);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(Vertex, position)));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(Vertex, normal)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(Vertex, tex_coords)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 2, GL_BYTE, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(Vertex, tex_offset)));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(Vertex, hp)));
	glEnableVertexAttribArray(4);

	int x_offset = 0;
	int y_offset = 0;
	int z_offset = 0;
	
	for (int x = 0; x < LC_WORLD_INIT_WIDTH; x++)
	{
		z_offset = 0;
		for (int z = 0; z < LC_WORLD_INIT_LENGTH; z++)
		{
			y_offset = 0;
			for (int y = 0; y < LC_WORLD_INIT_HEIGHT; y++)
			{
				//emplace the chunk in the array
				int chunk_index = Object_Pool_Request(world->chunks);
				LC_Chunk* chunk_array = world->chunks->pool->data;
				LC_Chunk* chunk = &chunk_array[chunk_index];
				//set the index
				world->chunk_indexes[x][y][z] = chunk_index;

				//Generate the chunk
				LC_Chunk_Generate(x_offset, y_offset, z_offset, ctx, chunk, chunk_index, world->vbo, &world->vertex_count_index);
				
				if (chunk->alive_blocks == 0)
				{
					LC_Chunk_Destroy(chunk);
					Object_Pool_Free(world->chunks, chunk_index, true);
				}

				y_offset += LC_CHUNK_HEIGHT;

				world->chunks_size[1]++;
			}
			world->chunks_size[2]++;
			z_offset += LC_CHUNK_LENGTH;
		}
		x_offset += LC_CHUNK_WIDTH;
		world->chunks_size[0]++;
	}
	
	//copy buffer
	glCopyNamedBufferSubData(world->vbo, world->copy_vbo, 0, 0, 100000 * sizeof(Vertex));


	//CHUNKS INFO BUFFER
	glGenBuffers(1, &world->chunks_info_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, world->chunks_info_ssbo);
	_updateChunksInfoSSBO();

	//BLOCKS INFO UBO BUFFER
	glGenBuffers(1, &world->blocks_info_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, world->blocks_info_ubo);


	//gen clouds
	int index = 0;
	float c_x_offset = -200;
	float c_z_offset = -700;
	for (int x = 0; x < 40; x += 2)
	{
		if (index >= LC_WORLD_MAX_CLOUDS)
			break;

		for (int z = 0; z < 40; z += 5)
		{
			c_x_offset = x * LC_WORLD_CLOUD_WIDTH;
			c_z_offset = z * LC_WORLD_CLOUD_LENGTH;

			float noise = Noise_SimplexNoise2D(c_x_offset, c_z_offset, ctx, 1, 0.8);

			if (noise > 0.1)
			{
				c_x_offset += x * LC_WORLD_CLOUD_WIDTH;
			}
			else if (noise < 0.1)
			{
				c_z_offset += z * LC_WORLD_CLOUD_LENGTH;
			}
			else
			{
				c_x_offset -= (x * LC_WORLD_CLOUD_WIDTH) * 1;
				c_z_offset -= (z * LC_WORLD_CLOUD_LENGTH) * 1;
			}

			world->clouds[index][0] = c_x_offset;
			world->clouds[index][1] = 100;
			world->clouds[index][2] = c_z_offset;
			
			index++;

			if (index >= LC_WORLD_MAX_CLOUDS)
				break;

		}
	}



	open_simplex_noise_free(ctx);

	

	return world;
}

void LC_World_Destruct(LC_World* p_world)
{
	PhysWorld_Destruct(p_world->phys_world);

	FL_Destruct(p_world->entities);

	Object_Pool_Destruct(p_world->chunks);

	free(p_world);

}

LC_Entity* LC_World_AssignEntity()
{
	assert(current_world != NULL && "World not set");

	static int id_gen = 1;
	LC_Entity* entity = FL_emplaceFront(current_world->entities)->value;
	entity->id = id_gen;
	id_gen++;

	return entity;
}

void LC_World_DeleteEntityByID(int p_id)
{
	assert(current_world != NULL && "World not set");

	FL_Node* node = current_world->entities->next;
	while (node != NULL)
	{
		LC_Entity* cast = node->value;

		//found 
		if (cast->id == p_id)
		{
			FL_remove(current_world->entities, node);
			break;
		}

		node = node->next;
	}

}

void LC_World_DeleteEntity(LC_Entity* p_ent)
{
	assert(current_world != NULL && "World not set");

	FL_Node* node = current_world->entities->next;
	while (node != NULL)
	{
		LC_Entity* cast = node->value;

		//found 
		if (cast == p_ent)
		{
			FL_remove(current_world->entities, node);
			break;
		}
		node = node->next;
	}

}

void LC_World_deleteBlock(int p_gX, int p_gY, int p_gZ)
{
	ivec3 r_pos;
	LC_Chunk* chunk = NULL;

	LC_Block* block = LC_World_getBlock(p_gX, p_gY, p_gZ, r_pos, &chunk);

	if (!block)
		return;
	
	if (chunk->alive_blocks == 0)
		return;

	chunk->blocks[r_pos[0]][r_pos[1]][r_pos[2]].hp = 0;
	chunk->blocks[r_pos[0]][r_pos[1]][r_pos[2]].type = LC_BT__NONE;
	
	chunk->alive_blocks--;

	//Update the chunk if there are more alive blocks
	if (chunk->alive_blocks > 0)
	{
		LC_World_UpdateChunk(chunk);
	}
	//Otherewise we delete
	else
	{
		LC_World_UpdateChunk(chunk);
		LC_World_DeleteChunk(p_gX / LC_CHUNK_WIDTH, p_gY / LC_CHUNK_HEIGHT, p_gZ / LC_CHUNK_LENGTH);
	}
	
}

static void _addBlock(int p_gX, int p_gY, int p_gZ, int p_cX, int p_cY, int p_cZ, bool p_createChunk, LC_BlockType p_bType)
{
	assert(p_cX >= 0 && p_cY >= 0 && p_cZ >= 0);

	//create another chunk
	if (p_createChunk)
	{
		//do we have a chunk already
		int c_index = current_world->chunk_indexes[p_cX][p_cY][p_cZ];
		if (c_index > -1)
		{
			LC_Chunk* chunk = _getChunk(c_index);

			int r_blockPosX = p_gX - chunk->global_position[0];
			int r_blockPosY = p_gY - chunk->global_position[1];
			int r_blockPosZ = p_gZ - chunk->global_position[2];
			
			assert(r_blockPosX >= 0 && r_blockPosX < LC_CHUNK_WIDTH && r_blockPosY >= 0 && r_blockPosY < LC_CHUNK_HEIGHT && r_blockPosZ >= 0 && r_blockPosZ < LC_CHUNK_LENGTH);

			LC_Block* block = &chunk->blocks[r_blockPosX][r_blockPosY][r_blockPosZ];

			block->hp = LC_BLOCK_STARTING_HP;
			block->type = p_bType;
			
			chunk->alive_blocks++;

			//LC_Chunk_Update(chunk);

			LC_World_UpdateChunk(chunk);
		}
		//otherwise create a new chunk
		else
		{
			//otherwise we will create a new chunk
			LC_Chunk* new_chunk = LC_World_CreateChunk(p_cX, p_cY, p_cZ);

			int r_blockPosX = p_gX - new_chunk->global_position[0];
			int r_blockPosY = p_gY - new_chunk->global_position[1];
			int r_blockPosZ = p_gZ - new_chunk->global_position[2];

			assert(r_blockPosX >= 0 && r_blockPosX < LC_CHUNK_WIDTH&& r_blockPosY >= 0 && r_blockPosY < LC_CHUNK_HEIGHT&& r_blockPosZ >= 0 && r_blockPosZ < LC_CHUNK_LENGTH);

			LC_Block* block = &new_chunk->blocks[r_blockPosX][r_blockPosY][r_blockPosZ];

			block->hp = LC_BLOCK_STARTING_HP;
			block->type = p_bType;

			new_chunk->alive_blocks++;

			//LC_Chunk_Update(new_chunk);

			LC_World_UpdateChunk(new_chunk);
		}
	}
	//we don't need to create another chunk
	else
	{
		LC_Chunk* chunk = NULL;
		LC_Block* block = LC_World_getBlock(p_gX, p_gY, p_gZ, NULL, &chunk);

		if (!block)
			return;

		block->hp = LC_BLOCK_STARTING_HP;
		block->type = p_bType;
		chunk->alive_blocks++;
		
		//LC_Chunk_Update(chunk);

		LC_World_UpdateChunk(chunk);
	}
}

void LC_World_addBlock(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, LC_BlockType p_bType)
{
	if (p_bType == LC_BT__NONE)
		return;

	ivec3 r_pos;
	LC_Chunk* chunk = NULL;

	LC_Block* block = LC_World_getBlock(p_gX, p_gY, p_gZ, r_pos, &chunk);

	if (!block)
		return;

	ivec3 new_block_pos;
	new_block_pos[0] = r_pos[0] + (chunk->global_position[0]);
	new_block_pos[1] = r_pos[1] + (chunk->global_position[1]);
	new_block_pos[2] = r_pos[2] + (chunk->global_position[2]);

	ivec3 next_chunk_pos;
	next_chunk_pos[0] = (chunk->global_position[0] / LC_CHUNK_WIDTH);
	next_chunk_pos[1] = (chunk->global_position[1] / LC_CHUNK_HEIGHT);
	next_chunk_pos[2] = (chunk->global_position[2] / LC_CHUNK_LENGTH);

	current_world->dirty_bit = true;

	//add to right face
	if (p_addFace[0] > 0)
	{
		next_chunk_pos[0]++;
		new_block_pos[0]++;
		//Do we need to create another chunk?
		if (r_pos[0] == LC_CHUNK_WIDTH - 1)
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], true, p_bType);
		}
		else
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], 0, 0, 0, false, p_bType);
		}
	}
	//add to left face
	else if (p_addFace[0] < 0)
	{
		next_chunk_pos[0]--;
		new_block_pos[0]--;
		//Do we need to create another chunk?
		if (r_pos[0] == 0)
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], true, p_bType);
		}
		else
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], 0, 0, 0, false, p_bType);
		}
	}
	//add to top
	else if (p_addFace[1] > 0)
	{
		next_chunk_pos[1]++;
		new_block_pos[1]++;
		//Do we need to create another chunk?
		if (r_pos[1] == LC_CHUNK_HEIGHT - 1)
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], true, p_bType);
		}
		else
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], 0, 0, 0, false, p_bType);
		}
	}
	//add to bottom
	else if (p_addFace[1] < 0)
	{
		next_chunk_pos[1]--;
		new_block_pos[1]--;
		//Do we need to create another chunk?
		if (r_pos[1] == 0)
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], true, p_bType);
		}
		else
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], 0, 0, 0, false, p_bType);
		}
	}
	//add to front
	else if (p_addFace[2] > 0)
	{
		next_chunk_pos[2]++;
		new_block_pos[2]++;
		//Do we need to create another chunk?
		if (r_pos[2] == LC_CHUNK_LENGTH - 1)
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], true, p_bType);
		}
		else
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], 0, 0, 0, false, p_bType);
		}
	}
	//add to back
	else if (p_addFace[2] < 0)
	{
		next_chunk_pos[2]--;
		new_block_pos[2]--;
		//Do we need to create another chunk?
		if (r_pos[2] == 0)
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], true, p_bType);
		}
		else
		{
			_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], 0, 0, 0, false, p_bType);
		}
	}
}

void LC_World_mineBlock(int p_gX, int p_gY, int p_gZ)
{
	LC_Chunk* chunk = NULL;
	LC_Block* block = LC_World_getBlock(p_gX, p_gY, p_gZ, NULL, &chunk);

	current_world->dirty_bit = true;

	if (!block)
		return;


	block->hp--;


	//destroy the block instantly if creative mode is on
	if (current_world->game_type == LC_GT__CREATIVE)
	{
		block->hp = 0;
	}

	if (block->hp <= 0)
	{
		LC_World_deleteBlock(p_gX, p_gY, p_gZ);
	}
	else
	{
		LC_World_UpdateChunk(chunk);
	}
}

LC_Chunk* LC_World_getChunk(float x, float y, float z)
{
	//round, floor and cast to int
	int x_chunk_pos = (int)floorf((x / (float)LC_CHUNK_WIDTH));
	int y_chunk_pos = (int)floorf((y / (float)LC_CHUNK_HEIGHT));
	int z_chunk_pos = (int)floorf((z / (float)LC_CHUNK_LENGTH));

	//Make sure the pos is valid
	if (x_chunk_pos >= LC_WORLD_MAX_WIDTH || x_chunk_pos < 0)
	{
		return NULL;
	}
	if (y_chunk_pos >= LC_WORLD_MAX_HEIGHT || y_chunk_pos < 0)
	{
		return NULL;
	}
	if (z_chunk_pos >= LC_WORLD_MAX_LENGTH || z_chunk_pos < 0)
	{
		return NULL;
	}
	
	//get the index
	int index = current_world->chunk_indexes[x_chunk_pos][y_chunk_pos][z_chunk_pos];
	
	//is the index valid?
	if (index >= current_world->chunks->pool->elements_size || index == -1)
		return NULL;

	LC_Chunk* chunk_array = current_world->chunks->pool->data;

	LC_Chunk* chunk = &chunk_array[index];
	
	return chunk;
}

LC_Chunk* LC_World_getNearestChunk(float g_x, float g_y, float g_z)
{
	LC_Chunk* array = current_world->chunks->pool->data;

	//round to and get the position of the possible nearest chunk
	int x_chunk_pos = (int)roundf((g_x / (float)LC_CHUNK_WIDTH));
	int y_chunk_pos = (int)roundf((g_y / (float)LC_CHUNK_HEIGHT));
	int z_chunk_pos = (int)roundf((g_z / (float)LC_CHUNK_LENGTH));

	ivec3 distance_test_pos;
	distance_test_pos[0] = roundf(g_x);
	distance_test_pos[1] = roundf(g_y);
	distance_test_pos[2] = roundf(g_z);
	
	//try to find the chunk immediately
	if (x_chunk_pos >= 0 && x_chunk_pos < LC_WORLD_INIT_WIDTH && y_chunk_pos >= 0 && y_chunk_pos < LC_WORLD_INIT_HEIGHT &&
		z_chunk_pos >= 0 && z_chunk_pos < LC_WORLD_MAX_LENGTH)
	{
		int index = current_world->chunk_indexes[x_chunk_pos][y_chunk_pos][z_chunk_pos];

		if (index > -1 && index < current_world->chunks->pool->elements_size)
		{
			LC_Chunk* chunk = &array[index];

			ivec3 pos;
			pos[0] = chunk->global_position[0] + (LC_CHUNK_WIDTH / 2);
			pos[1] = chunk->global_position[1] + (LC_CHUNK_HEIGHT / 2);
			pos[2] = chunk->global_position[2] + (LC_CHUNK_LENGTH / 2);

			float d = glm_ivec3_distance(pos, distance_test_pos);

			if (d < 12)
			{
				return chunk;
			}
		}
	}

	//otherwise we do a deeper search

	int min_x = (x_chunk_pos - 1 < 0) ? 0 : x_chunk_pos - 1;
	int min_y = (y_chunk_pos - 1 < 0) ? 0 : y_chunk_pos - 1;
	int min_z = (z_chunk_pos - 1 < 0) ? 0 : z_chunk_pos - 1;
	
	int max_x = (x_chunk_pos + 1 > LC_WORLD_INIT_WIDTH) ? LC_WORLD_INIT_WIDTH - 1 : x_chunk_pos + 1;
	int max_y = (y_chunk_pos + 1 > LC_WORLD_INIT_HEIGHT) ? LC_WORLD_INIT_HEIGHT - 1 : y_chunk_pos + 1;
	int max_z = (z_chunk_pos + 1 > LC_WORLD_INIT_LENGTH) ? LC_WORLD_INIT_LENGTH - 1 : z_chunk_pos + 1;

	
	float min_distance = FLT_MAX;
	int min_index = -1;
	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			for (int z = min_z; z <= max_z; z++)
			{
				int index = current_world->chunk_indexes[x][y][z];

				if (index > -1 && index < current_world->chunks->pool->elements_size)
				{
					LC_Chunk* chunk = &array[index];

					ivec3 pos;
					pos[0] = chunk->global_position[0] + (LC_CHUNK_WIDTH / 2);
					pos[1] = chunk->global_position[1] + (LC_CHUNK_HEIGHT / 2);
					pos[2] = chunk->global_position[2] + (LC_CHUNK_LENGTH / 2);

					float d = glm_ivec3_distance(pos, distance_test_pos);

					//sort by distance
					if (d < min_distance)
					{
						LC_Chunk* chunk = &array[index];
	
						min_distance = d;
						min_index = index;
					}

				}
			}
		}
	}

	if (min_index >= 0)
	{
		LC_Chunk* chunk = &array[min_index];

		return chunk;
	}

	return NULL;
}

LC_Chunk* LC_World_getNearestChunk2(float g_x, float g_y, float g_z)
{
	int x_chunk_pos = (int)roundf((g_x / (float)LC_CHUNK_WIDTH));
	int y_chunk_pos = (int)roundf((g_y / (float)LC_CHUNK_HEIGHT));
	int z_chunk_pos = (int)roundf((g_z / (float)LC_CHUNK_LENGTH));
	

	if (x_chunk_pos < 0)
	{
		x_chunk_pos = 0;
	}
	else if (x_chunk_pos > LC_WORLD_MAX_WIDTH - 1)
	{
		x_chunk_pos = LC_WORLD_MAX_WIDTH - 1;
	}

	if (y_chunk_pos < 0)
	{
		y_chunk_pos = 0;
	}
	else if (y_chunk_pos > LC_WORLD_MAX_HEIGHT - 1)
	{
		y_chunk_pos = LC_WORLD_MAX_HEIGHT - 1;
	}

	if (z_chunk_pos < 0)
	{
		z_chunk_pos = 0;
	}
	else if (z_chunk_pos > LC_WORLD_MAX_LENGTH - 1)
	{
		z_chunk_pos = LC_WORLD_MAX_LENGTH - 1;
	}
	
	int chunk_index = -1;

	chunk_index = current_world->chunk_indexes[x_chunk_pos][y_chunk_pos][z_chunk_pos];

	if (chunk_index != -1)
	{
		LC_Chunk* array = current_world->chunks->pool->data;

		LC_Chunk* chunk = &array[chunk_index];

		return chunk;
	}

	while (chunk_index == -1 && x_chunk_pos >= 0 && y_chunk_pos >= 0 && z_chunk_pos >= 0)
	{
		chunk_index = current_world->chunk_indexes[x_chunk_pos][y_chunk_pos][z_chunk_pos];
		x_chunk_pos--;
		y_chunk_pos--;
		z_chunk_pos--;

	}


	if (chunk_index != -1)
	{
		LC_Chunk* array = current_world->chunks->pool->data;

		LC_Chunk* chunk = &array[chunk_index];

		return chunk;
	}

	return NULL;
}

LC_Block* LC_World_getBlock(float g_x, float g_y, float g_z, ivec3 r_relativePos, LC_Chunk** r_bChunk)
{
	LC_Chunk* chunk_ptr = LC_World_getChunk(g_x, g_y, g_z);
	
	//return null if we failed to find a proper chunk
	if (!chunk_ptr)
	{
		return NULL;
	}

	int x_block_pos = (int)roundf(g_x) - chunk_ptr->global_position[0];
	int y_block_pos = (int)roundf(g_y) - chunk_ptr->global_position[1];
	int z_block_pos = (int)roundf(g_z) - chunk_ptr->global_position[2];

	if (x_block_pos < 0 || x_block_pos >= LC_CHUNK_WIDTH)
	{
		return NULL;
	}
	if (y_block_pos < 0 || y_block_pos >= LC_CHUNK_HEIGHT)
	{
		return NULL;
	}
	if (z_block_pos < 0 || z_block_pos >= LC_CHUNK_LENGTH)
	{
		return NULL;
	}

	LC_Block* block = &chunk_ptr->blocks[x_block_pos][y_block_pos][z_block_pos];

	if (r_relativePos)
	{
		r_relativePos[0] = x_block_pos;
		r_relativePos[1] = y_block_pos;
		r_relativePos[2] = z_block_pos;
	}
	if (r_bChunk)
	{
		*r_bChunk = chunk_ptr;
	}

	return block;
}

LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, ivec3 r_pos, ivec3 r_face)
{
	// "A Fast Voxel Traversal Algorithm for Ray Tracing" by John Amanatides, Andrew Woo */
	float big = FLT_MAX;

	int px = (int)floor(from[0]), py = (int)floor(from[1]), pz = (int)floor(from[2]);

	float dxi = 1.0f / dir[0], dyi = 1.0f / dir[1], dzi = 1.0f / dir[2];
	int sx = dir[0] > 0 ? 1 : -1, sy = dir[1] > 0 ? 1 : -1, sz = dir[2] > 0 ? 1 : -1;
	float dtx = min(dxi * sx, big), dty = min(dyi * sy, big), dtz = min(dzi * sz, big);
	float tx = fabsf((px + max(sx, 0) - from[0]) * dxi), ty = fabsf((py + max(sy, 0) - from[1]) * dyi), tz = fabsf((pz + max(sz, 0) - from[2]) * dzi);
	int maxSteps = 16;
	int cmpx = 0, cmpy = 0, cmpz = 0;

	for (int i = 0; i < maxSteps && py >= 0; i++) 
	{
		if (i > 0 && py < (LC_WORLD_MAX_HEIGHT * LC_CHUNK_HEIGHT))
		{
			LC_Block* block = LC_World_getBlock(px, py, pz, NULL, NULL);

			if (block != NULL && block->type != LC_BT__NONE)
			{
				if (r_pos)
				{
					r_pos[0] = px;
					r_pos[1] = py;
					r_pos[2] = pz;
				}

				if (r_face)
				{
					r_face[0] = -cmpx * sx;
					r_face[1] = -cmpy * sy;
					r_face[2] = -cmpz * sz;
				}
				return block;
			}
		}
		cmpx = Math_step(tx, tz) * Math_step(tx, ty);
		cmpy = Math_step(ty, tx) * Math_step(ty, tz);
		cmpz = Math_step(tz, ty) * Math_step(tz, tx);
		tx += dtx * cmpx;
		ty += dty * cmpy;
		tz += dtz * cmpz;
		px += sx * cmpx;
		py += sy * cmpy;
		pz += sz * cmpz;
	}
	
	return NULL;
}

LC_Chunk* LC_World_CreateChunk(int p_gX, int p_gY, int p_gZ)
{
	if (current_world->chunk_indexes[p_gX][p_gY][p_gZ] != -1)
	{
		assert(false);
		return NULL;
	}

	//emplace the chunk in the array
	int chunk_index = Object_Pool_Request(current_world->chunks);
	LC_Chunk* array = current_world->chunks->pool->data;
	LC_Chunk* chunk = &array[chunk_index];

	//we reuse the vertex data
	size_t stored_vertex_start = chunk->vertex_start;
	size_t stored_vertec_count = chunk->vertex_count;

	//set the index
	current_world->chunk_indexes[p_gX][p_gY][p_gZ] = chunk_index;

	int posX = p_gX * LC_CHUNK_WIDTH;
	int posY = p_gY * LC_CHUNK_HEIGHT;
	int posZ = p_gZ * LC_CHUNK_LENGTH;

	LC_Chunk_CreateEmpty(posX, posY, posZ, chunk);

	chunk->vertex_start = stored_vertex_start;
	chunk->vertex_count = stored_vertec_count;

	if (posX >= current_world->chunks_size[0])
	{
		current_world->chunks_size[0] = posX + 1;
	}
	if (posY / LC_CHUNK_HEIGHT >= current_world->chunks_size[1])
	{
		current_world->chunks_size[1] = (posY / LC_CHUNK_HEIGHT) + 1;
	}
	if (posZ >= current_world->chunks_size[2])
	{
		current_world->chunks_size[2] = posZ + 1;
	}

	return chunk;
}

void LC_World_DeleteChunk(int x, int y, int z)
{
	int chunk_index = current_world->chunk_indexes[x][y][z];

	assert(chunk_index > -1);

	LC_Chunk* array = current_world->chunks->pool->data;

	LC_Chunk* chunk = &array[chunk_index];

	//LC_Chunk_UnloadVertexData(chunk);

	Object_Pool_Free(current_world->chunks, chunk_index, false);

	current_world->chunk_indexes[x][y][z] = -1;
}

int LC_World_getChunkIndex(LC_Chunk* const p_chunk)
{
	int x_norm_pos = p_chunk->global_position[0] / LC_CHUNK_WIDTH;
	int y_norm_pos = p_chunk->global_position[1] / LC_CHUNK_HEIGHT;
	int z_norm_pos = p_chunk->global_position[2] / LC_CHUNK_HEIGHT;

	assert((x_norm_pos >= 0 && x_norm_pos < LC_WORLD_MAX_WIDTH&& y_norm_pos >= 0 && y_norm_pos < LC_WORLD_MAX_HEIGHT
		&& z_norm_pos >= 0 && z_norm_pos < LC_WORLD_MAX_LENGTH) && "Chunk pos out of bounds");


	int chunk_index = current_world->chunk_indexes[x_norm_pos][y_norm_pos][z_norm_pos];

	assert(chunk_index > -1);

	return chunk_index;
}

void LC_World_UpdateChunk(LC_Chunk* const p_chunk)
{
	size_t old_vertex_count = p_chunk->vertex_count;

	LC_Chunk_Update(p_chunk, &current_world->vertex_count_index);

	size_t new_vertex_count = p_chunk->vertex_count;

	int to_move = 0;

	if (new_vertex_count > old_vertex_count)
	{
		to_move = new_vertex_count - old_vertex_count;
	}
	else
	{
		to_move = old_vertex_count - new_vertex_count;
		to_move = -to_move;
	}

	if (to_move == 0)
		return;
	
	int chunk_index = LC_World_getChunkIndex(p_chunk);


	LC_Chunk* array = current_world->chunks->pool->data;
	for (int i = chunk_index + 1; i < current_world->chunks->pool->elements_size; i++)
	{
		LC_Chunk* chunk = &array[i];

		chunk->vertex_start += to_move;
	}

	current_world->dirty_bit = true;
}

void LC_World_unloadFarAwayChunks(vec3 player_pos, float max_distance)
{
	LC_Chunk* array = current_world->chunks->pool->data;

	ivec3 p;
	p[0] = player_pos[0];
	p[1] = player_pos[1];
	p[2] = player_pos[2];

	for (int i = 0; i < current_world->chunks->pool->elements_size; i++)
	{
		LC_Chunk* chunk = &array[i];

		float distance = glm_ivec3_distance(p, chunk->global_position);

		int loaded_flag = chunk->flags & LC_CF__LOADED_VERTEX_DATA;

		if (distance < max_distance)
		{
			//load
			//if (chunk->vbo == 0 && chunk->vertex_count == 0)
			//{
			//	LC_Chunk_LoadVertexData(chunk);
			//}
		}
		//else
		//{
			//if (chunk->vbo != 0 && chunk->vertex_count > 0)
			//{
			//	LC_Chunk_UnloadVertexData(chunk);
			//}
		//}
	}

}

void LC_World_unloadFarAwayChunk(LC_Chunk* chunk, ivec3 player_pos, float max_distance)
{
	ivec3 chunk_center;
	chunk_center[0] = chunk->global_position[0] + (LC_CHUNK_WIDTH / 2);
	chunk_center[1] = chunk->global_position[1] + (LC_CHUNK_HEIGHT / 2);
	chunk_center[2] = chunk->global_position[2] + (LC_CHUNK_LENGTH / 2);

	float distance = glm_ivec3_distance(player_pos, chunk_center);
	
//	if (distance < max_distance)
//	{
		//if (!chunk->vertex_loaded)
		//.{
		//	LC_Chunk_LoadVertexData(chunk);
		//}
	//}
	//else
	//{
	//	if (chunk->vertex_loaded)
//		{
//			LC_Chunk_UnloadVertexData(chunk);
//		}
//}

}

void LC_World_Update(float delta)
{
	r_drawCube(SUN_SIZE, current_world->sun.position, current_world->sun.sun_color);
	
	
	for (int i = 0; i < LC_WORLD_MAX_CLOUDS; i++)
	{
		vec3 pos;
		
		
		vec3 size;
		size[0] = LC_WORLD_CLOUD_WIDTH;
		size[1] = LC_WORLD_CLOUD_HEIGHT;
		size[2] = LC_WORLD_CLOUD_LENGTH;

		//r_drawCube(size, current_world->clouds[i], sun_color);
		
	}
	
	if (current_world->dirty_bit)
	{
		_updateChunksInfoSSBO();
	}

	//_updateClouds(delta);
	_updateSun(delta);


}




