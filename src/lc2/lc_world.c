#include "lc_world.h"
#include <LC_Block_defs.h>
#include "utility/u_utility.h"
#include "render/r_core.h"
#include <glad/glad.h>
#include "core/c_common.h"
#include "render/r_renderer.h"
#include <time.h>
#include "lc2/lc_chunk.h"

LC_World lc_world;
static WorkHandleID work_tast_id;
static LC_Chunk* work_chunk;

typedef struct ChunkData
{
	vec4 min_point;

	int opaque_draw_cmd_index;
	int transparent_draw_cmd_index;
	int water_draw_cmd_index;
}  ChunkData;

typedef enum
{
	TASK_TYPE__NONE,
	TASK_TYPE__CHUNK_UPDATE
} TaskType;

typedef struct
{
	WorkHandleID handle;
	TaskType type;
	LC_Chunk* chunk;
} WorldTask;

#define MAX_TASKS 4
typedef struct
{
	WorldTask tasks[MAX_TASKS];
	int task_pos;
} TaskQueue;

static TaskQueue task_queue;


static LC_Block_LightingData LC_getLightingData(uint8_t block_type)
{
	LC_Block_LightingData data;
	memset(&data, 0, sizeof(LC_Block_LightingData));

	int array_size = sizeof(LC_BLOCK_LIGHTING_DATA) / sizeof(LC_Block_LightingData);
	
	//could be improved without a stupid loop
	for (int i = 0; i < array_size; i++)
	{
		if(LC_BLOCK_LIGHTING_DATA[i].block_type == block_type)
		{
			return LC_BLOCK_LIGHTING_DATA[i];
		}
	}

	return data;
}

static void _getNormalizedChunkPosition(float p_gx, float p_gy, float p_gz, ivec3 dest)
{
	dest[0] = floorf((p_gx / LC_CHUNK_WIDTH));
	dest[1] = floorf((p_gy / LC_CHUNK_HEIGHT));
	dest[2] = floorf((p_gz / LC_CHUNK_LENGTH));
}


static void LC_World_UpdateDrawCommands(size_t p_opaqueOffset, size_t p_transparentOffset, size_t p_waterOffset)
{
	if (lc_world.render_data.opaque_draw_commands.used_size > 0)
	{
		DrawArraysIndirectCommand* map = RSB_MapRange(&lc_world.render_data.opaque_draw_commands, 0, lc_world.render_data.opaque_draw_commands.used_size, GL_MAP_WRITE_BIT);

		for (int i = 0; i < lc_world.chunk_map.item_data->elements_size; i++)
		{
			LC_Chunk* chunk = dA_at(lc_world.chunk_map.item_data, i);
			if (chunk->opaque_index < 0)
				continue;

			DRB_Item item = DRB_GetItem(&lc_world.render_data.vertex_buffer, chunk->opaque_index);

			DrawArraysIndirectCommand* cmd = &map[chunk->opaque_drawcmd_index];

			cmd->count = item.count / sizeof(ChunkVertex);
			cmd->first = item.offset / sizeof(ChunkVertex);
		}
		RSB_Unmap(&lc_world.render_data.opaque_draw_commands);
	}

	if (lc_world.render_data.transparent_draw_commands.used_size > 0)
	{
		DrawArraysIndirectCommand* map = RSB_MapRange(&lc_world.render_data.transparent_draw_commands, 0, lc_world.render_data.transparent_draw_commands.used_size, GL_MAP_WRITE_BIT);
		for (int i = 0; i < lc_world.chunk_map.item_data->elements_size; i++)
		{
			LC_Chunk* chunk = dA_at(lc_world.chunk_map.item_data, i);
			if (chunk->transparent_index < 0)
				continue;

			DRB_Item item = DRB_GetItem(&lc_world.render_data.transparents_vertex_buffer, chunk->transparent_index);

			DrawArraysIndirectCommand* cmd = &map[chunk->transparent_drawcmd_index];

			cmd->count = item.count / sizeof(ChunkVertex);
			cmd->first = item.offset / sizeof(ChunkVertex);
		}
		RSB_Unmap(&lc_world.render_data.transparent_draw_commands);
	}
}

static void LC_World_DeleteChunk(LC_Chunk* const p_chunk)
{
	//remove the item from vertex buffer
	if (p_chunk->opaque_index != -1)
	{
		DRB_RemoveItem(&lc_world.render_data.vertex_buffer, p_chunk->opaque_index);
		RSB_FreeItem(&lc_world.render_data.opaque_draw_commands, p_chunk->opaque_drawcmd_index, true);
	}
	if (p_chunk->transparent_index != -1)
	{
		DRB_RemoveItem(&lc_world.render_data.transparents_vertex_buffer, p_chunk->transparent_index);
		RSB_FreeItem(&lc_world.render_data.transparent_draw_commands, p_chunk->transparent_drawcmd_index, true);
	}
	
	if (p_chunk->chunk_data_index != -1)
	{
		ChunkData c_data;
		memset(&c_data, -1, sizeof(ChunkData));
		//remove from chunk data array
		glNamedBufferSubData(lc_world.render_data.chunk_data.buffer, p_chunk->chunk_data_index * sizeof(ChunkData), sizeof(ChunkData), &c_data);
		RSB_FreeItem(&lc_world.render_data.chunk_data, p_chunk->chunk_data_index, false);
	}

	ivec3 hash_key;
	_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], hash_key);

	//remove from hashmap
	CHMap_Erase(&lc_world.chunk_map, hash_key);

	LC_World_UpdateDrawCommands(p_chunk->opaque_index, p_chunk->transparent_index, p_chunk->water_index);

	//update data to gpu
	DRB_WriteDataToGpu(&lc_world.render_data.vertex_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.transparents_vertex_buffer);
	//DRB_WriteDataToGpu(&lc_world.render_data.water_vertex_buffer);
}
static void LC_World_UpdateChunkVertices(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* p_verticesResult)
{
	if (p_chunk->alive_blocks <= 0)
	{
		return;
	}

	size_t old_opaque_vertex_count = p_chunk->vertex_count;
	size_t old_transparent_vertex_count = p_chunk->transparent_vertex_count;
	size_t old_water_vertex_count = p_chunk->water_vertex_count;

	size_t opaque_offset = 0;
	size_t transparent_offset = 0;
	size_t water_offset = 0;


	if (!p_verticesResult)
	{
		return;
	}
	
	if (p_chunk->opaque_blocks > 0)
	{
		assert(p_chunk->opaque_index > -1);

		//upload to the vertex buffer
		DRB_ChangeData(&lc_world.render_data.vertex_buffer, sizeof(ChunkVertex) * p_chunk->vertex_count, p_verticesResult->opaque_vertices, p_chunk->opaque_index);

		//free the vertices buffer
		free(p_verticesResult->opaque_vertices);

		opaque_offset = max(old_opaque_vertex_count, p_chunk->vertex_count) - min(old_opaque_vertex_count, p_chunk->vertex_count);

	}
	if (p_chunk->transparent_blocks > 0)
	{
		assert(p_chunk->transparent_index > -1);

		//upload to the vertex buffer
		DRB_ChangeData(&lc_world.render_data.transparents_vertex_buffer, sizeof(ChunkVertex) * p_chunk->transparent_vertex_count, p_verticesResult->transparent_vertices, p_chunk->transparent_index);

		//free the vertices buffer
		free(p_verticesResult->transparent_vertices);

		transparent_offset = max(old_transparent_vertex_count, p_chunk->transparent_vertex_count) - min(old_transparent_vertex_count, p_chunk->transparent_vertex_count);
	}
	
	LC_World_UpdateDrawCommands(opaque_offset, transparent_offset, water_offset);

	//free the result
	free(p_verticesResult);
}

static void LC_World_UpdateChunkIndexes(LC_Chunk* const p_chunk)
{
	if (p_chunk->alive_blocks <= 0)
	{
		return;
	}

	bool data_changed = false;
	ChunkData c_data;
	memset(&c_data, -1, sizeof(c_data));

	if (p_chunk->opaque_blocks > 0 && p_chunk->opaque_index == -1)
	{
		//get item index
		p_chunk->opaque_index = DRB_EmplaceItem(&lc_world.render_data.vertex_buffer, 0, NULL);
		//Insert to draw arrays buffer
		c_data.opaque_draw_cmd_index = RSB_Request(&lc_world.render_data.opaque_draw_commands);
		p_chunk->opaque_drawcmd_index = c_data.opaque_draw_cmd_index;
		data_changed = true;
	}
	if (p_chunk->transparent_blocks > 0 && p_chunk->transparent_index == -1)
	{
		//get item index
		p_chunk->transparent_index = DRB_EmplaceItem(&lc_world.render_data.transparents_vertex_buffer, 0, NULL);

		//Insert to draw arrays buffer
		c_data.transparent_draw_cmd_index = RSB_Request(&lc_world.render_data.transparent_draw_commands);
		p_chunk->transparent_drawcmd_index = c_data.transparent_draw_cmd_index;
		data_changed = true;
	}

	c_data.min_point[0] = p_chunk->global_position[0];
	c_data.min_point[1] = p_chunk->global_position[1];
	c_data.min_point[2] = p_chunk->global_position[2];
	c_data.min_point[3] = p_chunk->opaque_index;

	if (data_changed)
	{
		if (p_chunk->chunk_data_index == -1)
		{
			//insert the position and the indexes of the chunk to the chunk databuffer
			unsigned chunk_data_index = RSB_Request(&lc_world.render_data.chunk_data);

			glNamedBufferSubData(lc_world.render_data.chunk_data.buffer, chunk_data_index * sizeof(ChunkData), sizeof(ChunkData), &c_data);

			p_chunk->chunk_data_index = chunk_data_index;
		}
		else if (p_chunk->chunk_data_index >= 0)
		{
			glNamedBufferSubData(lc_world.render_data.chunk_data.buffer, p_chunk->chunk_data_index * sizeof(ChunkData), sizeof(ChunkData), &c_data);
		}
	}
}
static void LC_World_FinishChunkUpdate(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* p_vertices)
{
	//update indexes if needed
	LC_World_UpdateChunkIndexes(p_chunk);

	//update vertices data and draw commands
	LC_World_UpdateChunkVertices(p_chunk, p_vertices);
	
	//sync data to gpu
	DRB_WriteDataToGpu(&lc_world.render_data.vertex_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.transparents_vertex_buffer);

	glBindVertexArray(lc_world.render_data.vao);
	glBindVertexBuffer(0, lc_world.render_data.vertex_buffer.buffer, 0, sizeof(ChunkVertex));
}

static void LC_World_UpdateChunk(LC_Chunk* const p_chunk)
{
	//is the chunk completely empty? delete it
	if (p_chunk->alive_blocks <= 0)
	{
		LC_World_DeleteChunk(p_chunk);
		DRB_Unmap(&lc_world.render_data.vertex_buffer);
		return;
	}
	
	//Check if we need to completely nullify vertex count in the appropiate buffer
	if (p_chunk->vertex_count > 0 && p_chunk->opaque_blocks <= 0)
	{
		DRB_ChangeData(&lc_world.render_data.vertex_buffer, 0, NULL, p_chunk->opaque_index);
	}
	if (p_chunk->transparent_vertex_count > 0 && p_chunk->transparent_blocks <= 0)
	{
		DRB_ChangeData(&lc_world.render_data.transparents_vertex_buffer, 0, NULL, p_chunk->transparent_index);
	}
	
	//generate the vertices
	GeneratedChunkVerticesResult* vertices_result = LC_Chunk_GenerateVertices(p_chunk);
	LC_World_FinishChunkUpdate(p_chunk, vertices_result);
}


static LC_Chunk* LC_World_InsertChunk(LC_Chunk* const p_chunk, int p_x, int p_y, int p_z)
{
	ivec3 chunk_key;
	_getNormalizedChunkPosition(p_x, p_y, p_z, chunk_key);

	//insert to the hash map
	LC_Chunk* chunk = CHMap_Insert(&lc_world.chunk_map, chunk_key, p_chunk);

	chunk->opaque_index = -1;
	chunk->transparent_index = -1;
	chunk->water_index = -1;
	chunk->chunk_data_index = -1;

	if (chunk->alive_blocks > 0)
	{
		GeneratedChunkVerticesResult* vertices_result = LC_Chunk_GenerateVertices(chunk);
		LC_World_FinishChunkUpdate(chunk, vertices_result);
	}

	return chunk;
}


LC_Chunk* LC_World_GetChunk(float p_x, float p_y, float p_z)
{
	ivec3 chunk_key;
	_getNormalizedChunkPosition(p_x, p_y, p_z, chunk_key);

	return CHMap_Find(&lc_world.chunk_map, chunk_key);
}

LC_Block* LC_World_GetBlock(float p_x, float p_y, float p_z, ivec3 r_relativePos, LC_Chunk** r_chunk)
{
	LC_Chunk* chunk_ptr = LC_World_GetChunk(p_x, p_y, p_z);

	//return null if we failed to find a proper chunk
	if (!chunk_ptr)
	{
		if (r_chunk)
			*r_chunk = NULL;
		return NULL;
	}

	int x_block_pos = roundf(p_x) - chunk_ptr->global_position[0];
	int y_block_pos = roundf(p_y) - chunk_ptr->global_position[1];
	int z_block_pos = roundf(p_z) - chunk_ptr->global_position[2];

	//bounds check
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
	if (r_chunk)
	{
		*r_chunk = chunk_ptr;
	}

	return block;
}

LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, ivec3 r_pos, ivec3 r_face, LC_Chunk** r_chunk)
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

	for (int i = 0; i < maxSteps; i++)
	{
		if (i > 0)
		{
			LC_Block* block = LC_World_GetBlock(px, py, pz, NULL, r_chunk);

			if (block != NULL && block->type != LC_BT__NONE && block->type != LC_BT__WATER)
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

static void _addBlock(int p_gX, int p_gY, int p_gZ, int p_cX, int p_cY, int p_cZ, LC_BlockType p_bType)
{
	LC_Chunk* chunk = LC_World_GetChunk(p_cX, p_cY, p_cZ);
	LC_Block* block2 = LC_World_GetBlock(p_gX, p_gY, p_gZ, NULL, &chunk);

	if (block2 && block2->type != LC_BT__NONE)
	{
		return;
	}

	//chunk not found? create
	if (!chunk)
	{
		LC_Chunk new_chunk = LC_Chunk_Create(p_cX * LC_CHUNK_WIDTH, p_cY * LC_CHUNK_HEIGHT, p_cZ * LC_CHUNK_LENGTH);
		chunk = LC_World_InsertChunk(&new_chunk, p_cX * LC_CHUNK_WIDTH, p_cY * LC_CHUNK_HEIGHT, p_cZ * LC_CHUNK_LENGTH);
	}

	int r_blockPosX = p_gX - chunk->global_position[0];
	int r_blockPosY = p_gY - chunk->global_position[1];
	int r_blockPosZ = p_gZ - chunk->global_position[2];

	//sanity check
	assert(r_blockPosX >= 0 && r_blockPosX < LC_CHUNK_WIDTH && r_blockPosY >= 0 && r_blockPosY < LC_CHUNK_HEIGHT && r_blockPosZ >= 0 && r_blockPosZ < LC_CHUNK_LENGTH);

	LC_Block* block = &chunk->blocks[r_blockPosX][r_blockPosY][r_blockPosZ];

	block->hp = LC_BLOCK_STARTING_HP;
	block->type = p_bType;

	if (LC_isblockEmittingLight(block->type))
	{
		LC_Block_LightingData data = LC_getLightingData(block->type);

		PointLight pl;
		memset(&pl, 0, sizeof(pl));
		pl.position[0] = p_gX + 0.5;
		pl.position[1] = p_gY + 0.5;
		pl.position[2] = p_gZ + 0.5;

		pl.ambient_intensity = data.ambient_intensity;
	
		pl.constant = data.constant;
		pl.linear = data.linear;
		pl.color[0] = data.color[0];
		pl.color[1] = data.color[1];
		pl.color[2] = data.color[2];

		pl.quadratic = data.quadratic;
		pl.specular_intensity = data.specular_intensity;
		
		unsigned index = r_registerLightSource(pl);

		ivec3 block_pos;
		block_pos[0] = p_gX;
		block_pos[1] = p_gY;
		block_pos[2] = p_gZ;

		CHMap_Insert(&lc_world.light_hash_map, block_pos, &index);
	}

	if (LC_isBlockTransparent(block->type))
	{
		chunk->transparent_blocks++;
	}
	else if (LC_isBlockOpaque(block->type))
	{
		chunk->opaque_blocks++;
	}

	chunk->alive_blocks++;

	LC_World_UpdateChunk(chunk);
}

bool LC_World_addBlock2(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, int p_bType)
{
	if (p_bType == LC_BT__NONE)
		return false;

	ivec3 r_pos;
	LC_Chunk* chunk = NULL;

	LC_Block* block = LC_World_GetBlock(p_gX, p_gY, p_gZ, r_pos, &chunk);

	if (!block)
		return false;

	ivec3 new_block_pos;
	new_block_pos[0] = r_pos[0] + (chunk->global_position[0]);
	new_block_pos[1] = r_pos[1] + (chunk->global_position[1]);
	new_block_pos[2] = r_pos[2] + (chunk->global_position[2]);

	ivec3 next_chunk_pos;
	next_chunk_pos[0] = (chunk->global_position[0] / LC_CHUNK_WIDTH);
	next_chunk_pos[1] = (chunk->global_position[1] / LC_CHUNK_HEIGHT);
	next_chunk_pos[2] = (chunk->global_position[2] / LC_CHUNK_LENGTH);

	//add to right face
	if (p_addFace[0] > 0)
	{
		next_chunk_pos[0]++;
		new_block_pos[0]++;
	}
	//add to left face
	else if (p_addFace[0] < 0)
	{
		next_chunk_pos[0]--;
		new_block_pos[0]--;
	}
	//add to top face
	else if (p_addFace[1] > 0)
	{
		next_chunk_pos[1]++;
		new_block_pos[1]++;
	}
	//add to bottom face
	else if (p_addFace[1] < 0)
	{
		next_chunk_pos[1]--;
		new_block_pos[1]--;
	}
	//add to front face
	else if (p_addFace[2] > 0)
	{
		next_chunk_pos[2]++;
		new_block_pos[2]++;
	}
	//add to back face
	else if (p_addFace[2] < 0)
	{
		next_chunk_pos[2]--;
		new_block_pos[2]--;
	}

	_addBlock(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], p_bType);

	return true;
}

bool LC_World_mineBlock2(int p_gX, int p_gY, int p_gZ)
{
	ivec3 relative_block_position;
	LC_Chunk* chunk = NULL;
	LC_Block* block = LC_World_GetBlock(p_gX, p_gY, p_gZ, relative_block_position, &chunk);

	//block not found?
	if (!block)
		return false;

	block->hp--;

	//destroy the block instantly if creative mode is on
	//if (current_world->game_type == LC_GT__CREATIVE)
	//{
		block->hp = 0;
	//}

	//delete the block
	if (block->hp <= 0)
	{
		if (LC_isBlockTransparent(block->type))
		{
			chunk->transparent_blocks--;
		}
		else if (LC_isBlockOpaque(block->type))
		{
			chunk->opaque_blocks--;
		}
		if (LC_isblockEmittingLight(block->type))
		{
			ivec3 blockpos;
			blockpos[0] = p_gX;
			blockpos[1] = p_gY;
			blockpos[2] = p_gZ;
			
			unsigned* light_index = CHMap_Find(&lc_world.light_hash_map, blockpos);
			//remove from the light hash map
			if (light_index)
			{
				r_removeLightSource(*light_index);
				CHMap_Erase(&lc_world.light_hash_map, blockpos);
			}
		}

		chunk->blocks[relative_block_position[0]][relative_block_position[1]][relative_block_position[2]].type = LC_BT__NONE;
		chunk->alive_blocks--;
	}

	//update the chunk
	LC_World_UpdateChunk(chunk);

	return true;
}

WorldRenderData* LC_World_getRenderData()
{
	return &lc_world.render_data;
}

size_t LC_World_GetChunkAmount()
{
	return lc_world.chunk_map.item_data->elements_size;
}

P_PhysWorld* LC_World_GetPhysWorld()
{
	return lc_world.phys_world;
}


void LC_World_Create(int p_initWidth, int p_initHeight, int p_initLength)
{
	memset(&lc_world, 0, sizeof(LC_World));
	work_tast_id = -1;

	lc_world.phys_world = PhysWorld_Create(3.1);

	//lc_world.entities = FL_INIT()

	//init the chunk hash map
	lc_world.chunk_map = CHMAP_INIT(Hash_ivec3, ivec3, LC_Chunk, 100000);

	lc_world.light_hash_map = CHMAP_INIT(Hash_ivec3, ivec3, unsigned, 100000);

	//init the vertex buffer
	lc_world.render_data.vertex_buffer = DRB_Create(sizeof(ChunkVertex) * 501946, 100, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE);

	lc_world.render_data.transparents_vertex_buffer = DRB_Create(sizeof(ChunkVertex) * 100000, 100, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE);

	lc_world.render_data.water_vertex_buffer = DRB_Create(sizeof(ChunkVertex) * 10000, 50, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE);

	lc_world.render_data.chunk_data = RSB_Create(10000, sizeof(ChunkData), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE);
	
	lc_world.render_data.opaque_draw_commands = RSB_Create(1000, sizeof(DrawArraysIndirectCommand), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE);
	
	lc_world.render_data.transparent_draw_commands = RSB_Create(1000, sizeof(DrawArraysIndirectCommand), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE);

	lc_world.render_data.water_draw_commands = RSB_Create(1000, sizeof(DrawArraysIndirectCommand), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE);

	//compile the shaders
	lc_world.render_data.process_shader = ComputeShader_CompileFromFile("src/shaders/world/process_chunk.comp");
	lc_world.render_data.rendering_shader = Shader_CompileFromFile("src/render/shaders/chunk_shader.vs", "src/render/shaders/chunk_shader.fs", NULL);
	lc_world.render_data.transparents_forward_pass_shader = Shader_CompileFromFile("src/shaders/world/transparents_forward.vs", "src/shaders/world/transparents_forward.fs", NULL);

	glUseProgram(lc_world.render_data.transparents_forward_pass_shader);
	Shader_SetInteger(lc_world.render_data.transparents_forward_pass_shader, "texture_atlas", 0);
	
	ChunkData* m = glMapNamedBufferRange(lc_world.render_data.chunk_data.buffer, 0, sizeof(ChunkData) * 10000, GL_MAP_WRITE_BIT);
	memset(m, -1, sizeof(ChunkData) * 10000);
	glUnmapNamedBuffer(lc_world.render_data.chunk_data.buffer);

	//setup vao and buffer attrib pointers
	glGenVertexArrays(1, &lc_world.render_data.vao);
	
	glBindVertexArray(lc_world.render_data.vao);
	glBindBuffer(GL_ARRAY_BUFFER, lc_world.render_data.vertex_buffer.buffer);
	glBindVertexBuffer(0, lc_world.render_data.vertex_buffer.buffer, 0, sizeof(ChunkVertex));

	glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, (void*)(offsetof(ChunkVertex, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribBinding(0, 0);

	glVertexAttribIFormat(1, 1, GL_BYTE, (void*)(offsetof(ChunkVertex, hp)));
	glEnableVertexAttribArray(1);
	glVertexAttribBinding(1, 0);

	glVertexAttribIFormat(2, 1, GL_BYTE, (void*)(offsetof(ChunkVertex, normal)));
	glEnableVertexAttribArray(2);
	glVertexAttribBinding(2, 0);

	glVertexAttribIFormat(3, 1, GL_SHORT, (void*)(offsetof(ChunkVertex, texture_offset)));
	glEnableVertexAttribArray(3);
	glVertexAttribBinding(3, 0);
	

	int x_offset = 0;
	int y_offset = 0;
	int z_offset = 0;

	//Generate the chunks
	struct osn_context* ctx;
	open_simplex_noise(time(NULL), &ctx);

	for (int x = 0; x < p_initWidth; x++)
	{
		z_offset = 0;
		for (int z = 0; z < p_initLength; z++)
		{
			y_offset = 0;
			for (int y = 0; y < p_initHeight; y++)
			{
				LC_Chunk stack_chunk = LC_Chunk_Create(x_offset, y_offset, z_offset);
				//Generate the chunk
				LC_Chunk_GenerateBlocks(&stack_chunk, ctx);

				//add to the hash map
				if (stack_chunk.alive_blocks > 0)
				{
					LC_World_InsertChunk(&stack_chunk, x_offset, y_offset, z_offset);
				}

				y_offset += LC_CHUNK_HEIGHT;
			}
			
			z_offset += LC_CHUNK_LENGTH;
		}
		x_offset += LC_CHUNK_WIDTH;	
	}

	glBindVertexBuffer(0, lc_world.render_data.vertex_buffer.buffer, 0, sizeof(ChunkVertex));
	
	open_simplex_noise_free(ctx);

	DRB_WriteDataToGpu(&lc_world.render_data.vertex_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.transparents_vertex_buffer);
}


void LC_World_Update2()
{
	
}

void LC_World_Destruct2()
{
	PhysWorld_Destruct(lc_world.phys_world);

	//destruct the chunk hash map
	CHMap_Destruct(&lc_world.chunk_map);

	//destruct light hash map
	CHMap_Destruct(&lc_world.light_hash_map);

	//destruct the GL Buffers
	DRB_Destruct(&lc_world.render_data.vertex_buffer);
	DRB_Destruct(&lc_world.render_data.transparents_vertex_buffer);
	DRB_Destruct(&lc_world.render_data.water_vertex_buffer);
	RSB_Destruct(&lc_world.render_data.opaque_draw_commands);
	RSB_Destruct(&lc_world.render_data.transparent_draw_commands);
	RSB_Destruct(&lc_world.render_data.water_draw_commands);
	RSB_Destruct(&lc_world.render_data.chunk_data);

	//destroy the shaders
	glDeleteProgram(lc_world.render_data.process_shader);
	glDeleteProgram(lc_world.render_data.rendering_shader);
	glDeleteProgram(lc_world.render_data.transparents_forward_pass_shader);

	glDeleteVertexArrays(1, lc_world.render_data.vao);
}



