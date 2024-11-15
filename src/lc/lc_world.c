#include "lc_world.h"
#include <LC_Block_defs.h>
#include "utility/u_utility.h"
#include "render/r_core.h"
#include <glad/glad.h>
#include "core/core_common.h"
#include "render/r_renderer.h"
#include <time.h>
#include "lc/lc_chunk.h"
#include "render/r_public.h"
#include "lc/lc_player.h"
#include "core/resource_manager.h"

#include "lc/lc_core.h"

typedef enum
{
	LC_TASK_TYPE__NONE,
	LC_TASK_TYPE__CHUNK_VERTEX_GENERATE,
	LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS,
	LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS_AND_GENERATE_VERTICES,
	LC_TASK_TYPE__MAX
} LCTaskType;

typedef struct
{
	WorkHandleID handle;
	LCTaskType type;
	LC_Chunk* chunk;

	size_t prev_vertex_count;
	size_t prev_transparent_vertex_count;
	size_t prev_water_vertex_count;
} LCWorldTask;

#define MAX_ACTIVE_TASKS 100
//#define LC_ASYNC
typedef struct
{
	LCWorldTask tasks[MAX_ACTIVE_TASKS];
	int task_pos;
	int tasks_in_queue;
} LCTaskQueue;

typedef struct
{
	LC_Block* block;
	float time_since_prev_mine;
	int prev_block_hp;
} PrevMinedBlock;

static LC_World lc_world;
static WorkHandleID work_tast_id;
static LC_Chunk* work_chunk;
static bool chunk_updated_this_frame;
static PrevMinedBlock prev_mined_block;
static LCTaskQueue task_queue;

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

void LC_World_GenerateBlocksWrapper(LC_Chunk* const p_chunk)
{
	LC_Chunk_GenerateBlocks(p_chunk, 2);
}

GeneratedChunkVerticesResult* LC_World_GenerateBlocksWrapperAndGenVerticesWrapper(LC_Chunk* const p_chunk)
{
	LC_Chunk_GenerateBlocks(p_chunk, 2);
	
	GeneratedChunkVerticesResult* result = NULL;

	if (p_chunk->alive_blocks > 0)
	{
		result = LC_Chunk_GenerateVerticesTest(p_chunk);
	}

	return result;
}

static bool LC_World_InsertToTaskQueue(LC_Chunk* const p_chunk, LCTaskType p_taskType)
{
	LCWorldTask* task = NULL;
	for (int i = 0; i < MAX_ACTIVE_TASKS; i++)
	{
		if (task_queue.tasks[task_queue.task_pos].type == LC_TASK_TYPE__NONE)
		{
			task = &task_queue.tasks[task_queue.task_pos];
			break;
		}
		task_queue.task_pos = (task_queue.task_pos + 1) % MAX_ACTIVE_TASKS;
	}

	if (task && task->type == LC_TASK_TYPE__NONE)
	{
		WorkHandleID handle = -1;

		if (p_taskType == LC_TASK_TYPE__CHUNK_VERTEX_GENERATE)
		{
			handle = Thread_AssignTaskIntType(LC_Chunk_GenerateVerticesTest, p_chunk, 0);
		}
		else if (p_taskType == LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS)
		{
			//handle = Thread_AssignTaskIntType(LC_World_GenerateBlocksWrapper, p_chunk, 0);
		}
		else if (p_taskType == LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS_AND_GENERATE_VERTICES)
		{
			handle = Thread_AssignTaskIntType(LC_World_GenerateBlocksWrapperAndGenVerticesWrapper, p_chunk, 0);
		}
		
		if (handle == -1)
		{
			return false;
		}

		task->type = p_taskType;
		task->chunk = p_chunk;
		task->prev_vertex_count = p_chunk->vertex_count;
		task->prev_transparent_vertex_count = p_chunk->transparent_vertex_count;
		task->prev_water_vertex_count = p_chunk->water_vertex_count;
		task->handle = handle;

		task_queue.tasks_in_queue++;

		return true;
	}

	return false;
}


static void _getNormalizedChunkPosition(float p_gx, float p_gy, float p_gz, ivec3 dest)
{
	dest[0] = floorf((p_gx / LC_CHUNK_WIDTH));
	dest[1] = floorf((p_gy / LC_CHUNK_HEIGHT));
	dest[2] = floorf((p_gz / LC_CHUNK_LENGTH));
}
static LC_Chunk* LC_World_InsertToChunkCache(LC_Chunk* const p_chunk)
{
	ivec3 chunk_key;
	_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], chunk_key);

	//insert to the hash map
	LC_Chunk* chunk = CHMap_Insert(&lc_world.chunk_cache_map, chunk_key, p_chunk);

	return chunk;
}


static void LC_World_getNeighbours(LC_Chunk* const p_chunk, LC_Chunk* r_neighbours[6])
{
	ivec3 normalized_chunk_pos;
	_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], normalized_chunk_pos);

	ivec3 back, front, left, right, bottom, top;
	glm_ivec3_copy(normalized_chunk_pos, back);
	glm_ivec3_copy(normalized_chunk_pos, front);
	glm_ivec3_copy(normalized_chunk_pos, left);
	glm_ivec3_copy(normalized_chunk_pos, right);
	glm_ivec3_copy(normalized_chunk_pos, bottom);
	glm_ivec3_copy(normalized_chunk_pos, top);

	back[2]--;
	front[2]++;
	left[0]--;
	right[0]++;
	bottom[1]--;
	top[1]++;

	r_neighbours[0] = CHMap_Find(&lc_world.chunk_map, back);
	r_neighbours[1] = CHMap_Find(&lc_world.chunk_map, front);
	r_neighbours[2] = CHMap_Find(&lc_world.chunk_map, left);
	r_neighbours[3] = CHMap_Find(&lc_world.chunk_map, right);
	r_neighbours[4] = CHMap_Find(&lc_world.chunk_map, bottom);
	r_neighbours[5] = CHMap_Find(&lc_world.chunk_map, top);
}

static void LC_World_UnloadChunkVertexData(LC_Chunk* p_chunk)
{
	if (p_chunk->opaque_index >= 0)
	{
		DRB_ChangeData(&lc_world.render_data.vertex_buffer, 0, NULL, p_chunk->opaque_index);
		p_chunk->vertex_count = 0;
		p_chunk->vertex_loaded = false;
	}
	if (p_chunk->transparent_index >= 0)
	{
		DRB_ChangeData(&lc_world.render_data.transparents_vertex_buffer, 0, NULL, p_chunk->transparent_index);
		p_chunk->transparent_vertex_count = 0;
	}

	lc_world.chunks_vertex_loaded--;
}

static void LC_World_UpdateDrawCommands(size_t p_opaqueOffset, size_t p_transparentOffset, size_t p_waterOffset)
{

	double start_Time = glfwGetTime();

	if (lc_world.render_data.draw_cmds_buffer.used_size > 0)
	{	
		dA_clear(lc_world.draw_cmds_backbuffer);
		if (dA_capacity(lc_world.draw_cmds_backbuffer) <= lc_world.render_data.draw_cmds_buffer.used_size)
		{
			//dA_reserve(lc_world.draw_cmds_backbuffer, lc_world.render_data.draw_cmds_buffer.used_size - dA_capacity(lc_world.draw_cmds_backbuffer));
			dA_resize(lc_world.draw_cmds_backbuffer, lc_world.render_data.draw_cmds_buffer.used_size);
			memset(lc_world.draw_cmds_backbuffer->data, 0, sizeof(CombinedChunkDrawCmdData) * lc_world.draw_cmds_backbuffer->elements_size);
		}
		for (int i = 0; i < lc_world.chunk_map.item_data->elements_size; i++)
		{
			LC_Chunk* chunk = dA_at(lc_world.chunk_map.item_data, i);
			
			if (chunk->draw_cmd_index < 0 || chunk->is_deleted)
				continue;

			CombinedChunkDrawCmdData* cmd = dA_at(lc_world.draw_cmds_backbuffer, chunk->draw_cmd_index);

			if (chunk->opaque_index >= 0)
			{
				DRB_Item item = DRB_GetItem(&lc_world.render_data.vertex_buffer, chunk->opaque_index);

				cmd->o_count = item.count / sizeof(ChunkVertex);
				cmd->o_first = item.offset / sizeof(ChunkVertex);
			}
			if (chunk->transparent_index >= 0)
			{
				DRB_Item item = DRB_GetItem(&lc_world.render_data.transparents_vertex_buffer, chunk->transparent_index);

				cmd->t_count = item.count / sizeof(ChunkVertex);
				cmd->t_first = item.offset / sizeof(ChunkVertex);
			}
			if (chunk->water_index >= 0)
			{
				DRB_Item item = DRB_GetItem(&lc_world.render_data.water_vertex_buffer, chunk->water_index);

				cmd->w_count = item.count / sizeof(ChunkWaterVertex);
				cmd->w_first = item.offset / sizeof(ChunkWaterVertex);
			}
		}
		
		lc_world.render_data.ubo_data.opaque_update_offset = 0;
		lc_world.render_data.ubo_data.opaque_update_move_by = 0;
		lc_world.render_data.ubo_data.transparent_update_offset = 0;
		lc_world.render_data.ubo_data.transparent_update_move_by = 0;
		lc_world.render_data.ubo_data.water_update_move_by = 0;
		lc_world.render_data.ubo_data.water_update_offset = 0;
		lc_world.render_data.ubo_data.skip_opaque_owner = -1;
		lc_world.render_data.ubo_data.skip_transparent_owner = -1;
		lc_world.render_data.ubo_data.skip_water_owner = -1;

		glNamedBufferSubData(lc_world.render_data.draw_cmds_buffer.buffer, 0, 
			lc_world.render_data.draw_cmds_buffer.used_size * sizeof(CombinedChunkDrawCmdData), dA_getFront(lc_world.draw_cmds_backbuffer));
	}

	double end_time = glfwGetTime();

	printf("%f draw cmd update \n", end_time - start_Time);
}

static void LC_World_DeleteChunk(LC_Chunk* const p_chunk, bool p_cpuMassDelete)
{
	if (chunk_updated_this_frame && !p_cpuMassDelete)
	{
		return;
	}

	//remove the item from vertex buffers
	if (p_chunk->opaque_index != -1)
	{
		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.vertex_buffer, p_chunk->opaque_index);

		DRB_RemoveItem(&lc_world.render_data.vertex_buffer, p_chunk->opaque_index);

		//free the vertices buffer
		int64_t to_move_by = prev_item.count / sizeof(ChunkVertex);

		to_move_by = -to_move_by;

		lc_world.render_data.ubo_data.opaque_update_offset = prev_item.offset / sizeof(ChunkVertex);
		lc_world.render_data.ubo_data.opaque_update_move_by = to_move_by;

		lc_world.chunks_vertex_loaded--;

		p_chunk->opaque_index = -1;
	}
	if (p_chunk->transparent_index != -1)
	{
		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.transparents_vertex_buffer, p_chunk->transparent_index);

		DRB_RemoveItem(&lc_world.render_data.transparents_vertex_buffer, p_chunk->transparent_index);

		int64_t to_move_by = prev_item.count / sizeof(ChunkVertex);

		to_move_by = -to_move_by;

		lc_world.render_data.ubo_data.transparent_update_offset = prev_item.offset / sizeof(ChunkVertex);
		lc_world.render_data.ubo_data.transparent_update_move_by = to_move_by;

		lc_world.chunks_vertex_loaded--;

		p_chunk->transparent_index = -1;
	}
	if (p_chunk->water_index != -1)
	{
		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.water_vertex_buffer, p_chunk->water_index);

		DRB_RemoveItem(&lc_world.render_data.water_vertex_buffer, p_chunk->water_index);

		int64_t to_move_by = prev_item.count / sizeof(ChunkWaterVertex);

		to_move_by = -to_move_by;

		lc_world.render_data.ubo_data.water_update_offset = prev_item.offset / sizeof(ChunkWaterVertex);
		lc_world.render_data.ubo_data.water_update_move_by = to_move_by;

		lc_world.chunks_vertex_loaded--;

		p_chunk->water_index = -1;
	}
	
	assert(p_chunk->draw_cmd_index == p_chunk->chunk_data_index);

	if (p_chunk->chunk_data_index != -1)
	{
		ChunkData c_data;
		memset(&c_data, 0, sizeof(ChunkData));
		//remove from chunk data array
		glNamedBufferSubData(lc_world.render_data.chunk_data.buffer, p_chunk->chunk_data_index * sizeof(ChunkData), sizeof(ChunkData), &c_data);
		RSB_FreeItem(&lc_world.render_data.chunk_data, p_chunk->chunk_data_index, false);

		p_chunk->chunk_data_index = -1;
	}
	if (p_chunk->draw_cmd_index != -1)
	{
		CombinedChunkDrawCmdData data;
		memset(&data, 0, sizeof(data));

		glNamedBufferSubData(lc_world.render_data.draw_cmds_buffer.buffer, p_chunk->draw_cmd_index * sizeof(CombinedChunkDrawCmdData), sizeof(CombinedChunkDrawCmdData), &data);
		RSB_FreeItem(&lc_world.render_data.draw_cmds_buffer, p_chunk->draw_cmd_index, false);

		p_chunk->draw_cmd_index = -1;
	}
	if (p_chunk->aabb_tree_index > -1)
	{
		LCTreeData* tree_data = AABB_Tree_GetIndexData(&lc_world.render_data.aabb_tree, p_chunk->aabb_tree_index);

		if (tree_data)
		{
			free(tree_data);
		}
		tree_data = NULL;

		AABB_Tree_Remove(&lc_world.render_data.aabb_tree, p_chunk->aabb_tree_index);

		p_chunk->aabb_tree_index = -1;
	}
	if (p_chunk->light_blocks > 0)
	{
		for (int x = 0; x < LC_CHUNK_WIDTH; x++)
		{
			for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
			{
				for (int z = 0; z < LC_CHUNK_LENGTH; z++)
				{
					if (LC_isblockEmittingLight(p_chunk->blocks[x][y][z].type))
					{
						int gX = x + p_chunk->global_position[0];
						int gY = y + p_chunk->global_position[1];
						int gZ = z + p_chunk->global_position[2];

						ivec3 blockpos;
						blockpos[0] = gX;
						blockpos[1] = gY;
						blockpos[2] = gZ;

						unsigned* light_index = CHMap_Find(&lc_world.light_hash_map, blockpos);
						//remove from the light hash map
						if (light_index)
						{
							RSCene_DeletePointLight(*light_index);
							CHMap_Erase(&lc_world.light_hash_map, blockpos);
						}

						if (p_chunk->light_blocks > 0)
						{
							p_chunk->light_blocks--;
						}
					}
					
					if (p_chunk->light_blocks <= 0)
					{
						break;
					}
				}
			}
		}
	}
	p_chunk->alive_blocks = 0;
	p_chunk->light_blocks = 0;
	p_chunk->water_blocks = 0;
	p_chunk->opaque_blocks = 0;
	p_chunk->transparent_blocks = 0;

	p_chunk->is_deleted = true;

	ivec3 hash_key;
	_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], hash_key);

	//remove from hashmap
	CHMap_Erase(&lc_world.chunk_map, hash_key);

	if(lc_world.chunk_count > 0)
		lc_world.chunk_count--;

	if(!p_cpuMassDelete)
		chunk_updated_this_frame = true;
}
static void LC_World_UpdateChunkVertices(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* p_verticesResult, size_t p_oldVertexCount, size_t p_oldTransparentVertexCount, size_t p_oldWaterVertexCount)
{
	if (p_chunk->alive_blocks <= 0 || !p_verticesResult)
	{
		return;
	}

	bool update_drawcmds_cpu = false;

	CombinedChunkDrawCmdData cmd;
	memset(&cmd, 0, sizeof(cmd), 0);

	if (p_chunk->opaque_blocks > 0 || (p_oldVertexCount > 0 && p_chunk->opaque_blocks <= 0))
	{
		assert(p_chunk->opaque_index > -1);

		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.vertex_buffer, p_chunk->opaque_index);

		//upload to the vertex buffer
		DRB_ChangeData(&lc_world.render_data.vertex_buffer, sizeof(ChunkVertex) * p_chunk->vertex_count, p_verticesResult->opaque_vertices, p_chunk->opaque_index);

		int64_t to_move_by = max(prev_item.count / sizeof(ChunkVertex), p_chunk->vertex_count) - min(prev_item.count / sizeof(ChunkVertex), p_chunk->vertex_count);

		DRB_Item item = DRB_GetItem(&lc_world.render_data.vertex_buffer, p_chunk->opaque_index);

		if (prev_item.count > item.count)
		{
			to_move_by = -to_move_by;
		}

		if (prev_item.count == 0)
		{
			update_drawcmds_cpu = true;
			lc_world.render_data.ubo_data.skip_opaque_owner = p_chunk->chunk_data_index;
		}

		cmd.o_count = item.count / sizeof(ChunkVertex);
		cmd.o_first = item.offset / sizeof(ChunkVertex);
		
		lc_world.render_data.ubo_data.opaque_update_offset = item.offset / sizeof(ChunkVertex);
		lc_world.render_data.ubo_data.opaque_update_move_by = to_move_by;

		if (p_chunk->vertex_count > 0)
		{
			lc_world.chunks_vertex_loaded++;
			p_chunk->vertex_loaded = true;
		}
	}

	if (p_chunk->transparent_blocks > 0 || (p_oldTransparentVertexCount > 0 && p_chunk->transparent_blocks <= 0))
	{
		assert(p_chunk->transparent_index > -1);

		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.transparents_vertex_buffer, p_chunk->transparent_index);

		//upload to the vertex buffer
		DRB_ChangeData(&lc_world.render_data.transparents_vertex_buffer, sizeof(ChunkVertex) * p_chunk->transparent_vertex_count, p_verticesResult->transparent_vertices, p_chunk->transparent_index);

		DRB_Item item = DRB_GetItem(&lc_world.render_data.transparents_vertex_buffer, p_chunk->transparent_index);

		int64_t to_move_by = max(prev_item.count / sizeof(ChunkVertex), p_chunk->transparent_vertex_count) - min(prev_item.count / sizeof(ChunkVertex), p_chunk->transparent_vertex_count);

		if (prev_item.count > item.count)
		{
			to_move_by = -to_move_by;
		}

		if (prev_item.count == 0)
		{
			update_drawcmds_cpu = true;
			lc_world.render_data.ubo_data.skip_transparent_owner = p_chunk->chunk_data_index;
		}

		cmd.t_count = item.count / sizeof(ChunkVertex);
		cmd.t_first = item.offset / sizeof(ChunkVertex);

		lc_world.render_data.ubo_data.transparent_update_offset = item.offset / sizeof(ChunkVertex);
		lc_world.render_data.ubo_data.transparent_update_move_by = to_move_by;
	} 
	if (p_chunk->water_blocks > 0 || (p_oldWaterVertexCount > 0 && p_chunk->water_blocks <= 0))
	{
		assert(p_chunk->water_index > -1);

		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.water_vertex_buffer, p_chunk->water_index);

		//upload to the vertex buffer
		DRB_ChangeData(&lc_world.render_data.water_vertex_buffer, sizeof(ChunkWaterVertex) * p_chunk->water_vertex_count, p_verticesResult->water_vertices, p_chunk->water_index);

		DRB_Item item = DRB_GetItem(&lc_world.render_data.water_vertex_buffer, p_chunk->water_index);

		int64_t to_move_by = max(prev_item.count / sizeof(ChunkWaterVertex), p_chunk->water_vertex_count) - min(prev_item.count / sizeof(ChunkWaterVertex), p_chunk->water_vertex_count);

		if (prev_item.count > item.count)
		{
			to_move_by = -to_move_by;
		}
		if (prev_item.count == 0)
		{
			update_drawcmds_cpu = true;
			lc_world.render_data.ubo_data.skip_water_owner = p_chunk->chunk_data_index;
		}

		cmd.w_count = item.count / sizeof(ChunkWaterVertex);
		cmd.w_first = item.offset / sizeof(ChunkWaterVertex);

		lc_world.render_data.ubo_data.water_update_offset = item.offset / sizeof(ChunkWaterVertex);
		lc_world.render_data.ubo_data.water_update_move_by = to_move_by;
	}
	if (update_drawcmds_cpu)
	{
		glNamedBufferSubData(lc_world.render_data.draw_cmds_buffer.buffer, sizeof(CombinedChunkDrawCmdData) * p_chunk->draw_cmd_index, sizeof(CombinedChunkDrawCmdData), &cmd);
	}

	//free the vertices buffers
	if (p_verticesResult->opaque_vertices)
	{
		free(p_verticesResult->opaque_vertices);
	}
	if (p_verticesResult->transparent_vertices)
	{
		free(p_verticesResult->transparent_vertices);
	}
	if (p_verticesResult->water_vertices)
	{
		free(p_verticesResult->water_vertices);
	}

	//free the result
	free(p_verticesResult);
}

static void LC_World_UpdateChunkIndexes(LC_Chunk* const p_chunk)
{
	if (p_chunk->alive_blocks <= 0)
	{
		return;
	}

	ChunkData c_data;
	memset(&c_data, -1, sizeof(c_data));

	bool data_changed = false;

	if (p_chunk->opaque_blocks > 0 && p_chunk->opaque_index == -1)
	{
		p_chunk->opaque_index = DRB_EmplaceItem(&lc_world.render_data.vertex_buffer, 0, NULL);
		data_changed = true;

	}
	if (p_chunk->transparent_blocks > 0 && p_chunk->transparent_index == -1)
	{
		p_chunk->transparent_index = DRB_EmplaceItem(&lc_world.render_data.transparents_vertex_buffer, 0, NULL);
		data_changed = true;
	}
	if (p_chunk->water_blocks > 0 && p_chunk->water_index == -1)
	{
		p_chunk->water_index = DRB_EmplaceItem(&lc_world.render_data.water_vertex_buffer, 0, NULL);
	}
	if (p_chunk->draw_cmd_index == -1)
	{
		p_chunk->draw_cmd_index = RSB_Request(&lc_world.render_data.draw_cmds_buffer);
	}

	c_data.min_point[0] = p_chunk->global_position[0];
	c_data.min_point[1] = p_chunk->global_position[1];
	c_data.min_point[2] = p_chunk->global_position[2];
	c_data.min_point[3] = 1;
	c_data.vis_flags = 0;
	

	if (p_chunk->chunk_data_index == -1)
	{
		//insert the position and the indexes of the chunk to the chunk databuffer
		unsigned chunk_data_index = RSB_Request(&lc_world.render_data.chunk_data);

		glNamedBufferSubData(lc_world.render_data.chunk_data.buffer, chunk_data_index * sizeof(ChunkData), sizeof(ChunkData), &c_data);

		p_chunk->chunk_data_index = chunk_data_index;
	}
	if (p_chunk->aabb_tree_index == -1)
	{
		//Insert into the aabb tree
		AABB aabb;
		aabb.width = LC_CHUNK_WIDTH;
		aabb.height = LC_CHUNK_HEIGHT;
		aabb.length = LC_CHUNK_LENGTH;

		aabb.position[0] = (float)p_chunk->global_position[0] - 0.5;
		aabb.position[1] = (float)p_chunk->global_position[1] - 0.5;
		aabb.position[2] = (float)p_chunk->global_position[2] - 0.5;

		LCTreeData* tree_data = malloc(sizeof(LCTreeData));
		
		if (tree_data)
		{
			tree_data->chunk_data_index = p_chunk->chunk_data_index;
			tree_data->opaque_index = p_chunk->opaque_index;
			tree_data->transparent_index = p_chunk->transparent_index;
			p_chunk->aabb_tree_index = AABB_Tree_Insert(&lc_world.render_data.aabb_tree, &aabb, tree_data);
		}
	}
	else if (p_chunk->aabb_tree_index >= 0 && data_changed)
	{
		LCTreeData* tree_data = AABB_Tree_GetIndexData(&lc_world.render_data.aabb_tree, p_chunk->aabb_tree_index);

		assert(tree_data);

		tree_data->chunk_data_index = p_chunk->chunk_data_index;
		tree_data->opaque_index = p_chunk->opaque_index;
		tree_data->transparent_index = p_chunk->transparent_index;
	}

	//sanity check. These should always match
	assert(p_chunk->chunk_data_index == p_chunk->draw_cmd_index);
}
static void LC_World_FinishChunkUpdate(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* p_vertices, size_t p_oldVertexCount, size_t p_oldTransparentVertexCount, size_t p_oldWaterVertexCount)
{
	//update indexes if needed
	LC_World_UpdateChunkIndexes(p_chunk);

	//update vertices data and draw commands
	LC_World_UpdateChunkVertices(p_chunk, p_vertices, p_oldVertexCount, p_oldTransparentVertexCount, p_oldWaterVertexCount);

	lc_world.total_num_blocks += p_chunk->alive_blocks;
}
static void LC_World_GenVerticesAndUpdateChunk(LC_Chunk* const p_chunk)
{
	size_t old_opaque_vertex_count = p_chunk->vertex_count;
	size_t old_transparent_vertex_count = p_chunk->transparent_vertex_count;
	size_t old_water_vertex_count = p_chunk->water_vertex_count;

	//generate the vertices
	GeneratedChunkVerticesResult* vertices_result = LC_Chunk_GenerateVerticesTest(p_chunk, NULL);
	LC_World_FinishChunkUpdate(p_chunk, vertices_result, old_opaque_vertex_count, old_transparent_vertex_count, old_water_vertex_count);

	chunk_updated_this_frame = true;
}
static void LC_World_FloodWater(LC_Chunk* const p_chunk)
{
	if (p_chunk->water_blocks <= 0)
	{
		return;
	}

	int num_none_blocks = (LC_CHUNK_WIDTH * LC_CHUNK_HEIGHT * LC_CHUNK_LENGTH) - p_chunk->alive_blocks;

	if (num_none_blocks <= 0)
	{
		return;
	}

	int minWaterX = 32;
	int minWaterY = 32;
	int minWaterZ = 32;

	int maxWaterX = 0;
	int maxWaterY = 0;
	int maxWaterZ = 0;

	for (int x = 0; x < LC_CHUNK_WIDTH; x++)
	{
		for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
		{
			for (int z = 0; z < LC_CHUNK_LENGTH; z++)
			{
				num_none_blocks = (LC_CHUNK_WIDTH * LC_CHUNK_HEIGHT * LC_CHUNK_LENGTH) - p_chunk->alive_blocks;
				if (num_none_blocks <= 0)
				{
					return;
				}
				if (LC_isBlockWater(p_chunk->blocks[x][y][z].type))
				{
					for (int x2 = x; x2 >= 0; x2--)
					{
						for (int y2 = y; y2 >= 0; y2--)
						{
							for (int z2 = z; z2 >= 0; z2--)
							{
								if (p_chunk->blocks[x2][y2][z2].type == LC_BT__NONE)
								{
									minWaterX = min(minWaterX, x2);
									minWaterY = min(minWaterY, y2);
									minWaterZ = min(minWaterZ, z2);

									maxWaterX = max(maxWaterX, x2);
									maxWaterY = max(maxWaterY, y2);
									maxWaterZ = max(maxWaterZ, z2);

									LC_Chunk_SetBlock(p_chunk, x2, y2, z2, LC_BT__WATER);
								}
							}
						}
					}
					for (int x2 = x; x2 < LC_CHUNK_WIDTH; x2++)
					{
						for (int z2 = z; z2 < LC_CHUNK_LENGTH; z2++)
						{
							if (p_chunk->blocks[x2][y][z2].type == LC_BT__NONE)
							{
								minWaterX = min(minWaterX, x2);
								minWaterY = min(minWaterY, y);
								minWaterZ = min(minWaterZ, z2);

								maxWaterX = max(maxWaterX, x2);
								maxWaterY = max(maxWaterY, y);
								maxWaterZ = max(maxWaterZ, z2);


								LC_Chunk_SetBlock(p_chunk, x2, y, z2, LC_BT__WATER);
							}
						}
					}
				}
			}
		}
	}

	if (minWaterX == 0 || minWaterZ == 0 || minWaterY == 0)
	{
		LC_Chunk* chunk = NULL;
		LC_Block* block = LC_World_GetBlock(p_chunk->global_position[0] - 1, p_chunk->global_position[1] + minWaterY,
			p_chunk->global_position[2] -1, NULL, &chunk);
		if (block && block->type == LC_BT__NONE)
		{
			LC_World_setBlock(chunk, p_chunk->global_position[0] - 1, p_chunk->global_position[1] + minWaterY,
				p_chunk->global_position[2] - 1, LC_BT__WATER);
		}
	}
	else if (maxWaterX == LC_CHUNK_WIDTH - 1 || maxWaterY == LC_CHUNK_HEIGHT - 1 || maxWaterZ == LC_CHUNK_LENGTH - 1)
	{
		LC_Chunk* chunk = NULL;
		LC_Block* block = LC_World_GetBlock(p_chunk->global_position[0] + maxWaterX + 1, p_chunk->global_position[1] + maxWaterY,
			p_chunk->global_position[2] + 1, NULL, &chunk);
		if (block && block->type == LC_BT__NONE)
		{
			LC_World_setBlock(chunk, p_chunk->global_position[0]  + 1, p_chunk->global_position[1] + maxWaterY,
				p_chunk->global_position[2] + 1, LC_BT__WATER);
		}
	}
}
static void LC_World_UpdateChunk(LC_Chunk* const p_chunk, bool p_async)
{
	//is the chunk completely empty? delete it
	if (p_chunk->alive_blocks <= 0)
	{
		lc_world.total_num_blocks -= p_chunk->alive_blocks;
		LC_World_DeleteChunk(p_chunk, false);
		return;
	}
	
	//flood water to nearby blocks and chunks
	if (p_chunk->water_blocks > 0)
	{
		LC_World_FloodWater(p_chunk);
	}

	LC_Chunk* neighbours[6];
	//LC_World_getNeighbours(p_chunk, neighbours);

#ifdef LC_ASYNC
	if (p_async)
	{
		//Did we fail to insert into the task queue?
		if (!LC_World_InsertToTaskQueue(p_chunk, LC_TASK_TYPE__CHUNK_VERTEX_GENERATE))
		{
			//perform on this thread then
			LC_World_GenVerticesAndUpdateChunk(p_chunk);
		}

	}
	else
	{
		LC_World_GenVerticesAndUpdateChunk(p_chunk);
	}
	
#else
	LC_World_GenVerticesAndUpdateChunk(p_chunk);
#endif
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
	chunk->draw_cmd_index = -1;
	chunk->aabb_tree_index = -1;

	if (chunk->alive_blocks > 0)
	{
		LC_Chunk* neighbours[6];
		LC_World_getNeighbours(chunk, neighbours);

		LC_World_UpdateChunk(chunk, true);
	}

	lc_world.chunk_count++;
	
	for (int i = 0; i < 3; i++)
	{
		lc_world.chunks_bounds_normalized[0][i] = min(chunk_key[i], lc_world.chunks_bounds_normalized[0][i]);
		lc_world.chunks_bounds_normalized[1][i] = max(chunk_key[i], lc_world.chunks_bounds_normalized[1][i]);
	}

	return chunk;
}


LC_Chunk* LC_World_GetChunk(float p_x, float p_y, float p_z)
{
	ivec3 chunk_key;
	_getNormalizedChunkPosition(p_x, p_y, p_z, chunk_key);

	return CHMap_Find(&lc_world.chunk_map, chunk_key);
}

LC_Chunk* LC_World_GetChunkByIndex(size_t p_index)
{
	if (p_index >= LC_World_GetChunkAmount())
	{
		return NULL;
	}

	return dA_at(lc_world.chunk_map.item_data, p_index);
}

LC_Chunk* LC_World_GetChunkByNormalizedPosition(ivec3 pos)
{
	return CHMap_Find(&lc_world.chunk_map, pos);
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


LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, int p_maxSteps, ivec3 r_pos, ivec3 r_face, LC_Chunk** r_chunk)
{
	// "A Fast Voxel Traversal Algorithm for Ray Tracing" by John Amanatides, Andrew Woo */
	float big = FLT_MAX;

	int px = (int)floor(from[0]), py = (int)floor(from[1]), pz = (int)floor(from[2]);

	float dxi = 1.0f / dir[0], dyi = 1.0f / dir[1], dzi = 1.0f / dir[2];
	int sx = dir[0] > 0 ? 1 : -1, sy = dir[1] > 0 ? 1 : -1, sz = dir[2] > 0 ? 1 : -1;
	float dtx = min(dxi * sx, big), dty = min(dyi * sy, big), dtz = min(dzi * sz, big);
	float tx = fabsf((px + max(sx, 0) - from[0]) * dxi), ty = fabsf((py + max(sy, 0) - from[1]) * dyi), tz = fabsf((pz + max(sz, 0) - from[2]) * dzi);
	int maxSteps = p_maxSteps;
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

	if (r_chunk)
	{
		*r_chunk = NULL;
	}

	return NULL;
}

static bool LC_World_addBlockInternal(int p_gX, int p_gY, int p_gZ, int p_cX, int p_cY, int p_cZ, LC_BlockType p_bType)
{
	LC_Chunk* chunk = LC_World_GetChunk(p_cX, p_cY, p_cZ);
	LC_Block* block2 = LC_World_GetBlock(p_gX, p_gY, p_gZ, NULL, &chunk);

	if (block2)
	{
		if (block2->type != LC_BT__NONE)
		{
			//Allow placing blocks on top of water blocks
			if (!LC_isBlockWater(block2->type))
			{
				return false;
			}
		}
	}

	//chunk not found? create
	if (!chunk)
	{
		LC_Chunk new_chunk = LC_Chunk_Create(p_cX * LC_CHUNK_WIDTH, p_cY * LC_CHUNK_HEIGHT, p_cZ * LC_CHUNK_LENGTH);
		chunk = LC_World_InsertChunk(&new_chunk, p_cX * LC_CHUNK_WIDTH, p_cY * LC_CHUNK_HEIGHT, p_cZ * LC_CHUNK_LENGTH);
	}

	chunk->user_modified = true;

	int r_blockPosX = p_gX - chunk->global_position[0];
	int r_blockPosY = p_gY - chunk->global_position[1];
	int r_blockPosZ = p_gZ - chunk->global_position[2];

	//sanity check
	assert(r_blockPosX >= 0 && r_blockPosX < LC_CHUNK_WIDTH && r_blockPosY >= 0 && r_blockPosY < LC_CHUNK_HEIGHT && r_blockPosZ >= 0 && r_blockPosZ < LC_CHUNK_LENGTH);

	LC_Block* block = &chunk->blocks[r_blockPosX][r_blockPosY][r_blockPosZ];

	//block->hp = LC_BLOCK_STARTING_HP;
	block->type = p_bType;

	if (LC_isblockEmittingLight(block->type))
	{
		LC_Block_LightingData data = LC_getLightingData(block->type);

		PointLight2 pl;
		memset(&pl, 0, sizeof(pl));
		pl.position[0] = p_gX;
		pl.position[1] = p_gY;
		pl.position[2] = p_gZ;

		pl.ambient_intensity = data.ambient_intensity;
	
		pl.constant = data.constant;
		pl.linear = data.linear;
		pl.color[0] = data.color[0];
		pl.color[1] = data.color[1];
		pl.color[2] = data.color[2];

		pl.quadratic = data.quadratic;
		pl.specular_intensity = data.specular_intensity;
		
		LightID index = RScene_RegisterPointLight(pl);

		ivec3 block_pos;
		block_pos[0] = p_gX;
		block_pos[1] = p_gY;
		block_pos[2] = p_gZ;

		CHMap_Insert(&lc_world.light_hash_map, block_pos, &index);

		chunk->light_blocks++;
	}

	if (LC_isBlockTransparent(block->type))
	{
		chunk->transparent_blocks++;
	}
	else if (LC_isBlockOpaque(block->type))
	{
		chunk->opaque_blocks++;
	}
	else if (LC_isBlockWater(block->type))
	{
		chunk->water_blocks++;
	}

	chunk->alive_blocks++;

	LC_World_UpdateChunk(chunk, false);

	return true;
}

bool LC_World_addBlock(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, int p_bType)
{
	if (p_bType == LC_BT__NONE)
		return false;

	ivec3 r_pos;
	LC_Chunk* chunk = NULL;

	LC_Block* block = LC_World_GetBlock(p_gX, p_gY, p_gZ, r_pos, &chunk);

	if (!block)
		return false;

	//we can't stack props vertically
	if (p_addFace[1] > 0 && LC_isBlockProp(block->type))
	{
		return false;
	}

	ivec3 new_block_pos;
	new_block_pos[0] = r_pos[0] + (chunk->global_position[0]);
	new_block_pos[1] = r_pos[1] + (chunk->global_position[1]);
	new_block_pos[2] = r_pos[2] + (chunk->global_position[2]);

	ivec3 next_chunk_pos;
	next_chunk_pos[0] = (chunk->global_position[0] / LC_CHUNK_WIDTH);
	next_chunk_pos[1] = (chunk->global_position[1] / LC_CHUNK_HEIGHT);
	next_chunk_pos[2] = (chunk->global_position[2] / LC_CHUNK_LENGTH);
	
	ivec3 next_pos;

	for (int i = 0; i < 3; i++)
	{
		next_pos[i] = glm_clamp(p_addFace[i], -1, 1);
	}

	next_chunk_pos[0] += p_addFace[0];
	next_chunk_pos[1] += p_addFace[1];
	next_chunk_pos[2] += p_addFace[2];

	new_block_pos[0] += p_addFace[0];
	new_block_pos[1] += p_addFace[1];
	new_block_pos[2] += p_addFace[2];

	return LC_World_addBlockInternal(new_block_pos[0], new_block_pos[1], new_block_pos[2], next_chunk_pos[0], next_chunk_pos[1], next_chunk_pos[2], p_bType);
}

bool LC_World_mineBlock(int p_gX, int p_gY, int p_gZ)
{
	ivec3 relative_block_position;
	LC_Chunk* chunk = NULL;
	LC_Block* block = LC_World_GetBlock(p_gX, p_gY, p_gZ, relative_block_position, &chunk);

	//block not found?
	if (!block)
		return false;

	prev_mined_block.prev_block_hp--;

	chunk->user_modified = true;

	//destroy the block instantly if creative mode is on or the object is a prop (flowers, etc..)
	if (LC_isBlockProp(block->type))
	{
		prev_mined_block.prev_block_hp = 0;
	}
	//prev_mined_block.prev_block_hp = 0;

	if (prev_mined_block.block)
	{
		if (prev_mined_block.block != block)
		{
			prev_mined_block.prev_block_hp = 7;
		}
	}
	else
	{
		prev_mined_block.prev_block_hp = 7;
	}
	prev_mined_block.time_since_prev_mine = 0;
	
	prev_mined_block.block = block;

	//delete the block
	if (prev_mined_block.prev_block_hp <= 0)
	{
		if (LC_isBlockTransparent(block->type))
		{
			chunk->transparent_blocks--;
		}
		else if (LC_isBlockOpaque(block->type))
		{
			chunk->opaque_blocks--;
		}
		else if (LC_isBlockWater(block->type))
		{
			chunk->water_blocks--;
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
				//r_removeLightSource(*light_index);
				RSCene_DeletePointLight(*light_index);
				CHMap_Erase(&lc_world.light_hash_map, blockpos);
			}

			if (chunk->light_blocks > 0)
			{
				chunk->light_blocks--;
			}
			
		}

		chunk->blocks[relative_block_position[0]][relative_block_position[1]][relative_block_position[2]].type = LC_BT__NONE;
		chunk->alive_blocks--;

		//update the chunk
		LC_World_UpdateChunk(chunk, false);

		prev_mined_block.block = NULL;
		prev_mined_block.prev_block_hp = 7;
	}


	return true;
}

bool LC_World_setBlock(int p_gX, int p_gY, int p_gZ, LC_BlockType p_bType)
{
	ivec3 relative_block_position;
	LC_Chunk* chunk = NULL;
	LC_Block* block = LC_World_GetBlock(p_gX, p_gY, p_gZ, relative_block_position, &chunk);

	//block not found?
	if (!block || !chunk)
		return false;

	chunk->blocks[relative_block_position[0]][relative_block_position[1]][relative_block_position[2]].type = p_bType;

	if (p_bType == LC_BT__NONE)
	{
		chunk->alive_blocks--;
	}

	//update the chunk
	LC_World_UpdateChunk(chunk, false);

	return true;
}

static int LC_World_calcWaterLevelInsideChunk(LC_Chunk* const p_chunk, float p_y)
{
	if (p_chunk->water_blocks <= 0)
	{
		return 0;
	}

	int water_level = 0;

	if (p_y <= p_chunk->global_position[1] + (LC_CHUNK_HEIGHT * 0.8))
	{
		water_level++;
	}
	if (p_y <= p_chunk->global_position[1] + (LC_CHUNK_HEIGHT * 0.7))
	{
		water_level++;
	}
	if (p_y <= p_chunk->global_position[1] + (LC_CHUNK_HEIGHT * 0.6))
	{
		water_level++;
	}
	if (p_y <= p_chunk->global_position[1] + (LC_CHUNK_HEIGHT * 0.5))
	{
		water_level++;
	}

	return water_level;
}


int LC_World_calcWaterLevelFromPoint(float p_x, float p_y, float p_z)
{	
	LC_Chunk* point_chunk = LC_World_GetChunk(p_x, p_y, p_z);

	if (!point_chunk)
	{
		return 0;
	}
	if (point_chunk->water_blocks <= 0)
	{
		return 0;
	}
	int water_level = LC_World_calcWaterLevelInsideChunk(point_chunk, p_y);

	LC_Chunk* above_chunk = NULL;

	for (int i = 0; i < 4; i++)
	{
		above_chunk = LC_World_GetChunk(p_x, p_y + (i * LC_CHUNK_HEIGHT), p_z);

		if (!above_chunk)
		{
			break;
		}
		ivec3 norm_position;
		_getNormalizedChunkPosition(p_x, p_y, p_z, norm_position);
		int x = norm_position[0] - above_chunk->global_position[0];
		int z = norm_position[2] - above_chunk->global_position[2];

		if (above_chunk->water_blocks > 0 && LC_isBlockWater(LC_Chunk_getType(above_chunk, x, 0, z)))
		{
			water_level += LC_World_calcWaterLevelInsideChunk(above_chunk, p_y);
		}
		else
		{
			break;
		}
	}

	return water_level;
}

WorldRenderData* LC_World_getRenderData()
{
	return &lc_world.render_data;
}

size_t LC_World_GetChunkAmount()
{
	return lc_world.chunk_count;
}

size_t LC_World_GetDrawCmdAmount()
{
	return lc_world.render_data.draw_cmds_buffer.used_size;
}

P_PhysWorld* LC_World_GetPhysWorld()
{
	return lc_world.phys_world;
}

void LC_World_getSunDirection(vec3 v)
{
	glm_vec3_copy(lc_world.sun.direction_to_world_center, v);
	glm_normalize(v);
}

int LC_World_getPrevMinedBlockHP()
{
	return prev_mined_block.prev_block_hp;
}

static float sunAngle(long time)
{
	int i = (int)(time % 24000L);
	float f = ((float)i + 1) / 24000.0F - 0.25F;

	if (f < 0.0F)
	{
		++f;
	}

	if (f > 1.0F)
	{
		--f;
	}
	float f1 = 1.0F - (float)((cos((double)f * Math_PI) + 1.0) / 2.0);
	f = f + (f1 - f) / 3.0F;
	return f;
}

static void sunColor(float sun_angle, vec3 dest, float* r_ambientIntensity)
{
	float abs_angle = fabsf(sun_angle);
	//printf("%f angle \n", sun_angle);

	float r = 1;
	float g = 1;
	float b = 0;
	if (sun_angle > 0)
	{
		g = glm_lerp(0.0, 1.0, abs_angle);
		b = glm_lerp(0.0, 0.5, abs_angle);
	}
	else
	{
		r = 0;
		g = 0;
		b = 0;
	}


	dest[0] = r;
	dest[1] = g;
	dest[2] = b;

	*r_ambientIntensity = glm_lerp(0.0, 10, abs_angle);
}


void LC_World_Draw()
{	
	//return;
	vec3 box[2];
	box[0][0] = 0;
	box[0][1] = 516;
	box[0][2] = 0;
	box[1][0] = 16;
	box[1][1] = 532;
	box[1][2] = 16;

	vec3 axis;
	axis[0] = 1;
	axis[1] = 1;
	axis[2] = 1;
	static float prev_angle = 0;

	float angle = sunAngle(glfwGetTime() * 12);
	
	glm_vec3_rotate(box[0], angle * 360, axis);
	glm_vec3_rotate(box[1], angle * 360, axis);
	

	
	

	vec3 dir;
	dir[0] = box[0][0];
	dir[1] = box[0][1];
	dir[2] = box[0][2];
	glm_normalize(dir);

	float ambient_intensity = 1;

	vec3 sun_color;
	sunColor(dir[1], sun_color, &ambient_intensity);

	DirLight light;
	glm_vec3_copy(dir, light.direction);

	if (dir[1] < 0 && prev_angle > 0)
	{
		//RScene_SetSkyboxTexturePanorama(Resource_get("assets/cubemaps/hdr/night_sky.hdr", RESOURCE__TEXTURE_HDR));
		printf("set to night \n");
	}
	else if (dir[1] > 0 && prev_angle < 0)
	{
		//RScene_SetSkyboxTexturePanorama(Resource_get("assets/cubemaps/hdr/industrial.hdr", RESOURCE__TEXTURE_HDR));
		printf("set to day \n");
	}

	prev_angle = dir[1];

	glm_vec3_copy(sun_color, light.color);

	light.ambient_intensity = ambient_intensity;
	light.specular_intensity = 0;

	RScene_SetDirLight(light);
	//RScene_SetDirLightDirection(dir);


	Draw_TexturedCube(box, Resource_get("assets/cube_textures/sun.png", RESOURCE__TEXTURE), NULL);
}

static void LC_World_TaskQueueProcess()
{
	printf("%i tasks in queue\n", task_queue.tasks_in_queue);

	for (int i = 0; i < MAX_ACTIVE_TASKS; i++)
	{
		if (task_queue.tasks_in_queue <= 0)
		{
			return;
		}
		LCWorldTask* task = &task_queue.tasks[i];
		if ((task->type == LC_TASK_TYPE__CHUNK_VERTEX_GENERATE || task->type == LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS_AND_GENERATE_VERTICES) && !chunk_updated_this_frame)
		{
			if (Thread_IsCompleted(task->handle))
			{
				GeneratedChunkVerticesResult* result = Thread_WaitForResult(task->handle, 0).mem_value;

				if (result)
				{
					LC_World_FinishChunkUpdate(task->chunk, result, task->prev_vertex_count, task->prev_transparent_vertex_count, task->prev_water_vertex_count);
					chunk_updated_this_frame = true;
				}

				task->type = LC_TASK_TYPE__NONE;

				task_queue.tasks_in_queue--;
			}
		}
		else if (task->type == LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS)
		{
			if (Thread_IsCompleted(task->handle))
			{
				Thread_WaitForResult(task->handle, 0);

				task->chunk->is_generated = true;

				task->type = LC_TASK_TYPE__NONE;
				task_queue.tasks_in_queue--;
			}
		}
	}
}

static void LC_World_SetupGLBindingPoints()
{
	glBindBufferBase(GL_UNIFORM_BUFFER, 5, lc_world.render_data.block_data_buffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, 6, lc_world.render_data.ubo_id);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, lc_world.render_data.chunk_data.buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, lc_world.render_data.draw_cmds_buffer.buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, lc_world.render_data.visibles_sorted_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 19, lc_world.render_data.visibles_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, lc_world.render_data.sorted_draw_cmds_buffer.buffer);
}

bool ivec3Compare(ivec3 v1, ivec3 v2)
{
	for (int i = 0; i < 3; i++)
	{
		if (v1[i] != v2[i])
		{
			return false;
		}
	}
	return true;
}

void LC_World_Create(int p_initWidth, int p_initHeight, int p_initLength)
{
	memset(&lc_world, 0, sizeof(LC_World));
	work_tast_id = -1;

	//Generate seed
	Math_srand(time(NULL));

	//Load the textures
	lc_world.render_data.texture_atlas = Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE);
	lc_world.render_data.texture_atlas_normals =  Resource_get("assets/cube_textures/simple_block_atlas_normal.png", RESOURCE__TEXTURE);
	lc_world.render_data.texture_atlas_mer = Resource_get("assets/cube_textures/simple_block_atlas_mer.png", RESOURCE__TEXTURE);
	
	lc_world.render_data.texture_dudv = Resource_get("assets/water/waterDUDV.png", RESOURCE__TEXTURE);
	lc_world.render_data.water_normal_map = Resource_get("assets/water/normal_map.png", RESOURCE__TEXTURE);

	//setup mipmaps for the textures
	glBindTexture(GL_TEXTURE_2D, lc_world.render_data.texture_atlas->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 2);
	glGenerateTextureMipmap(lc_world.render_data.texture_atlas->id);

	glBindTexture(GL_TEXTURE_2D, lc_world.render_data.texture_atlas_normals->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glGenerateTextureMipmap(lc_world.render_data.texture_atlas_normals->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 2);

	glBindTexture(GL_TEXTURE_2D, lc_world.render_data.texture_atlas_mer->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glGenerateTextureMipmap(lc_world.render_data.texture_atlas_mer->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 2);

	size_t approx_init_chunk_amount = p_initWidth * p_initHeight * p_initLength;
	size_t aprrox_chunk_vertex_count = approx_init_chunk_amount * 36;

	//init the phys world
	lc_world.phys_world = PhysWorld_Create(1.1);

	//Init the aabb tree
	lc_world.render_data.aabb_tree = AABB_Tree_Create();

	//init the chunk hash map
	lc_world.chunk_map = CHMAP_INIT_POOLED(Hash_ivec3, ivec3Compare, ivec3, LC_Chunk, 100000);

	//lc_world.chunk_cache_map = CHMAP_INIT_POOLED(Hash_ivec3, ivec3Compare, ivec3, LC_Chunk, 100000);

	lc_world.light_hash_map = CHMAP_INIT(Hash_ivec3, NULL, ivec3, unsigned, 100000);

	lc_world.draw_cmds_backbuffer = dA_INIT(CombinedChunkDrawCmdData, 0);

	//init buffers
	lc_world.render_data.vertex_buffer = DRB_Create(sizeof(ChunkVertex) * aprrox_chunk_vertex_count, approx_init_chunk_amount, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE);

	lc_world.render_data.transparents_vertex_buffer = DRB_Create(sizeof(ChunkVertex) * aprrox_chunk_vertex_count, approx_init_chunk_amount, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE);

	lc_world.render_data.water_vertex_buffer = DRB_Create(sizeof(ChunkWaterVertex) * aprrox_chunk_vertex_count, approx_init_chunk_amount, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE);

	lc_world.render_data.chunk_data = RSB_Create(100000, sizeof(ChunkData), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE | RSB_FLAG__READABLE | RSB_FLAG__PERSISTENT);
	
	lc_world.render_data.draw_cmds_buffer = RSB_Create(100000, sizeof(CombinedChunkDrawCmdData), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE);

	lc_world.render_data.sorted_draw_cmds_buffer = RSB_Create(100000, sizeof(DrawArraysIndirectCommand), RSB_FLAG__WRITABLE);

	DRB_setResizeChunkSize(&lc_world.render_data.vertex_buffer, sizeof(ChunkVertex) * 10000);
	DRB_setResizeChunkSize(&lc_world.render_data.transparents_vertex_buffer, sizeof(ChunkVertex) * 10000);

	glGenBuffers(1, &lc_world.render_data.visibles_sorted_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lc_world.render_data.visibles_sorted_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * 50000, NULL, GL_STREAM_DRAW);

	glGenBuffers(1, &lc_world.render_data.visibles_ssbo);;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lc_world.render_data.visibles_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * 5000, NULL, GL_STREAM_DRAW);

	glGenBuffers(1, &lc_world.render_data.ubo_id);
	glBindBuffer(GL_UNIFORM_BUFFER, lc_world.render_data.ubo_id);
	glBufferStorage(GL_UNIFORM_BUFFER, sizeof(LC_WorldUniformBuffer), NULL, GL_DYNAMIC_STORAGE_BIT);

	glGenBuffers(1, &lc_world.render_data.shadow_chunk_indexes_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lc_world.render_data.shadow_chunk_indexes_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * 50000, NULL, GL_STREAM_DRAW);

	for (int i = 0; i < 3; i++)
	{
		glGenBuffers(1, &lc_world.render_data.draw_cmds_counters_buffers[i]);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lc_world.render_data.draw_cmds_counters_buffers[i]);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned), NULL, GL_STATIC_DRAW);
	}
	

	lc_world.render_data.block_data_buffer = LC_generateBlockInfoGLBuffer();

	LC_World_SetupGLBindingPoints();

	
	ChunkData* m = glMapNamedBufferRange(lc_world.render_data.chunk_data.buffer, 0, sizeof(ChunkData) * 10000, GL_MAP_WRITE_BIT);
	memset(m, -1, sizeof(ChunkData) * 10000);
	glUnmapNamedBuffer(lc_world.render_data.chunk_data.buffer);

	//setup vao and buffer attrib pointers
	glGenVertexArrays(1, &lc_world.render_data.vao);
	
	glBindVertexArray(lc_world.render_data.vao);
	glBindBuffer(GL_ARRAY_BUFFER, lc_world.render_data.vertex_buffer.buffer);
	glBindVertexBuffer(0, lc_world.render_data.vertex_buffer.buffer, 0, sizeof(ChunkVertex));

	glVertexAttribIFormat(0, 3, GL_BYTE, (void*)(offsetof(ChunkVertex, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribBinding(0, 0);

	glVertexAttribIFormat(1, 1, GL_BYTE, (void*)(offsetof(ChunkVertex, packed_norm_hp)));
	glEnableVertexAttribArray(1);
	glVertexAttribBinding(1, 0);

	glVertexAttribIFormat(2, 1, GL_UNSIGNED_BYTE, (void*)(offsetof(ChunkVertex, block_type)));
	glEnableVertexAttribArray(2);
	glVertexAttribBinding(2, 0);
	
	glGenVertexArrays(1, &lc_world.render_data.water_vao);
	glBindVertexArray(lc_world.render_data.water_vao);
	glBindBuffer(GL_ARRAY_BUFFER, lc_world.render_data.water_vao);
	glBindVertexBuffer(0, lc_world.render_data.water_vertex_buffer.buffer, 0, sizeof(ChunkWaterVertex));

	glVertexAttribIFormat(0, 3, GL_BYTE, (void*)(offsetof(ChunkWaterVertex, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribBinding(0, 0);

	int x_offset = 0;
	int y_offset = 0;
	int z_offset = 0;

	//Generate the chunks
	int num_chunk = 0;
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
				LC_Chunk_GenerateBlocks(&stack_chunk, 2);

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


	DRB_WriteDataToGpu(&lc_world.render_data.vertex_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.transparents_vertex_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.water_vertex_buffer);
}

static void LC_Chunk_GenerateAndInsertToCache(int p_normX, int p_normY, int p_normZ)
{
	LC_Chunk stack_chunk = LC_Chunk_Create(p_normX * LC_CHUNK_WIDTH, p_normY * LC_CHUNK_HEIGHT, p_normZ * LC_CHUNK_LENGTH);
	//Generate the chunk
#ifdef LC_ASYNC
	LC_Chunk* chunk_ptr = LC_World_InsertToChunkCache(&stack_chunk);

	if (!LC_World_InsertToTaskQueue(chunk_ptr, LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS))
	{
		LC_Chunk_GenerateBlocks(chunk_ptr, 2);
		chunk_ptr->is_generated = true;
	}
#else
	LC_Chunk_GenerateBlocks(&stack_chunk, 2);
	LC_World_InsertToChunkCache(&stack_chunk);
#endif // LC_ASYNC
}

static bool LC_Chunk_InsertChunkIfInChunkCache(int p_normX, int p_normY, int p_normZ)
{
	ivec3 chunk_key;
	chunk_key[0] = p_normX;
	chunk_key[1] = p_normY;
	chunk_key[2] = p_normZ;

	//look in visible chunk map
	LC_Chunk* vis_chunk = CHMap_Find(&lc_world.chunk_map, chunk_key);

	if (vis_chunk)
	{
		return false;
	}

	LC_Chunk* chunk = CHMap_Find(&lc_world.chunk_cache_map, chunk_key);
	if (!chunk)
	{
		return false;
	}
	if (!chunk->is_generated)
	{
		return false;
	}

	LC_World_InsertChunk(chunk, chunk->global_position[0], chunk->global_position[1], chunk->global_position[2]);

	return true;
}

static bool LC_Chunk_GenerateAndInsertChunk(int p_normX, int p_normY, int p_normZ)
{
	ivec3 chunk_key;
	chunk_key[0] = p_normX;
	chunk_key[1] = p_normY;
	chunk_key[2] = p_normZ;

	//look in visible chunk map
	if (CHMap_Has(&lc_world.chunk_map, chunk_key))
	{
		return false;
	}
	LC_Chunk stack_chunk = LC_Chunk_Create(p_normX * LC_CHUNK_WIDTH, p_normY * LC_CHUNK_HEIGHT, p_normZ * LC_CHUNK_LENGTH);

#ifdef LC_ASYNC
	LC_Chunk* ptr = LC_World_InsertChunk(&stack_chunk, stack_chunk.global_position[0], stack_chunk.global_position[1], stack_chunk.global_position[2]);
	if (!LC_World_InsertToTaskQueue(ptr, LC_TASK_TYPE__CHUNK_GENERATE_BLOCKS_AND_GENERATE_VERTICES))
	{
		LC_Chunk_GenerateBlocks(ptr, 2);
	}
#else
	LC_Chunk_GenerateBlocks(&stack_chunk, 2);
	LC_World_InsertChunk(&stack_chunk, stack_chunk.global_position[0], stack_chunk.global_position[1], stack_chunk.global_position[2]);
#endif // LC_ASYNC

	return true;
}

static void LC_World_ChunkLiveGenerate()
{
	vec3 player_position;
	LC_Player_getPosition(player_position);

	ivec3 normalized_player_position;
	_getNormalizedChunkPosition(player_position[0], player_position[1], player_position[2], normalized_player_position);

	static int prev_visited_index = 0;

	const int MAX_ITR = lc_world.chunk_map.item_data->elements_size;;

	bool max_chunk_threshold_reached = (LC_World_GetChunkAmount() > 1000);

	max_chunk_threshold_reached = false;
	int render_distance_chunks = 8;

	int minX2 = normalized_player_position[0] - render_distance_chunks;
	int minY2 = normalized_player_position[1] - 4;
	int minZ2 = normalized_player_position[2] - render_distance_chunks;

	int maxX2 = normalized_player_position[0] + render_distance_chunks;
	int maxY2 = normalized_player_position[1] + 2;
	int maxZ2 = normalized_player_position[2] + render_distance_chunks;



	if (task_queue.tasks_in_queue == 0)
	{
		//Add chunks to chunk cache
		for (int x = minX2; x < maxX2; x++)
		{
			for (int y = minY2; y < maxY2; y++)
			{
				for (int z = minZ2; z < maxZ2; z++)
				{
					if (chunk_updated_this_frame)
					{
						break;
					}

					if (LC_Chunk_GenerateAndInsertChunk(x, y, z))
					{
						
					}
				}
			}
		}
	}


	for (int i = 0; i < lc_world.chunk_map.item_data->elements_size; i++)
	{
		LC_Chunk* chunk = dA_at(lc_world.chunk_map.item_data, i);

		if (chunk->is_deleted)
		{
			continue;
		}
		if (!max_chunk_threshold_reached)
		{
			if (chunk->alive_blocks > 0 && chunk_updated_this_frame)
			{
				continue;
			}
		}
		else if (chunk_updated_this_frame)
		{
			break;
		}
		
		

		ivec3 normalized_chunk_pos;
		_getNormalizedChunkPosition(chunk->global_position[0], chunk->global_position[1], chunk->global_position[2], normalized_chunk_pos);

		//Delete chunk if it falls outside the min and max
		if (normalized_chunk_pos[0] < minX2 || normalized_chunk_pos[0] > maxX2 || normalized_chunk_pos[1] < minY2 || normalized_chunk_pos[1] > maxY2
			|| normalized_chunk_pos[2] < minZ2 || normalized_chunk_pos[2] > maxZ2)
		{
			//printf(" chunk deleted \n");
			bool break_out_of_this = (chunk->alive_blocks > 0);
			if (max_chunk_threshold_reached)
			{
				break_out_of_this = false;
			}

			LC_World_DeleteChunk(chunk, max_chunk_threshold_reached);
			if (break_out_of_this)
			{
				prev_visited_index = i;
				break;
			}
		}

		prev_visited_index = 0;
	}
	if (max_chunk_threshold_reached)
	{
		LC_World_UpdateDrawCommands(0, 0, 0);
	}

}


void LC_World_StartFrame()
{
	LC_Draw_WorldInfo(&lc_world, 3);

	lc_world.sun.position[1] = -10000;
	lc_world.sun.position[0] += Core_getDeltaTime() * 0.1;
	prev_mined_block.time_since_prev_mine += Core_getDeltaTime();

	if (prev_mined_block.time_since_prev_mine > 0.3)
	{
		prev_mined_block.prev_block_hp = 7;
	}
	
	vec3 world_center;
	memset(world_center, 0, sizeof(vec3));

	float sun_angle = sunAngle(glfwGetTime() * 1000);

	vec3 dir;
	memset(dir, 0, sizeof(vec3));
	dir[0] = 0.5;
	dir[1] = 1;
	//Math_vec3_dir_to(lc_world.sun.position, world_center, lc_world.sun.direction_to_world_center);
	vec3 axis;
	axis[0] = 0;
	axis[1] = -1;
	axis[2] = 0;
	glm_vec3_rotate(dir, sun_angle, axis);

	//RScene_SetDirLightDirection(dir);
	

	vec3 player_position;
	LC_Player_getPosition(player_position);

	static bool block_new = false;

	bool allow_flyby_generation = false;

	R_Camera* cam = Camera_getCurrent();

	vec3 signed_camera_dir;
	
	if (cam)
	{
		glm_vec3_sign(cam->data.camera_front, signed_camera_dir);
	}

	static int prev_index = 0;

	if (chunk_updated_this_frame)
	{
		//return;
	}
	//LC_World_ChunkLiveGenerate();

#ifdef LC_ASYNC
	LC_World_TaskQueueProcess();
#endif // LC_ASYNC

	

}

void LC_World_EndFrame()
{
	DRB_WriteDataToGpu(&lc_world.render_data.vertex_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.transparents_vertex_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.water_vertex_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, lc_world.render_data.ubo_id);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LC_WorldUniformBuffer), &lc_world.render_data.ubo_data);

	lc_world.render_data.ubo_data.opaque_update_offset = 0;
	lc_world.render_data.ubo_data.opaque_update_move_by = 0;
	lc_world.render_data.ubo_data.skip_opaque_owner = -1;

	lc_world.render_data.ubo_data.transparent_update_offset = 0;
	lc_world.render_data.ubo_data.transparent_update_move_by = 0;
	lc_world.render_data.ubo_data.skip_transparent_owner = -1;

	lc_world.render_data.ubo_data.water_update_offset = 0;
	lc_world.render_data.ubo_data.water_update_move_by = 0;
	lc_world.render_data.ubo_data.skip_water_owner = -1;

	chunk_updated_this_frame = false;
}

void LC_World_Destruct()
{
	PhysWorld_Destruct(lc_world.phys_world);

	//destruct the chunk hash map
	CHMap_Destruct(&lc_world.chunk_map);

	//destruct light hash map
	CHMap_Destruct(&lc_world.light_hash_map);

	dA_Destruct(lc_world.draw_cmds_backbuffer);

	//destruct the GL Buffers
	DRB_Destruct(&lc_world.render_data.vertex_buffer);
	DRB_Destruct(&lc_world.render_data.transparents_vertex_buffer);
	RSB_Destruct(&lc_world.render_data.draw_cmds_buffer);
	RSB_Destruct(&lc_world.render_data.chunk_data);

	glDeleteVertexArrays(1, lc_world.render_data.vao);

	glDeleteBuffers(1, lc_world.render_data.block_data_buffer);


}



