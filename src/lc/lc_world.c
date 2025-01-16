#include "lc/lc_world.h"

#include <Windows.h>
#include <glad/glad.h>
#include <time.h>

#include "utility/u_math.h"
#include "render/r_public.h"
#include "core/core_common.h"
#include "core/resource_manager.h"
#include "core/cvar.h"

#define LC_MAX_ACTIVE_TASKS 32

extern void LC_Player_getPosition(vec3 dest);

typedef struct
{
	Cvar* lc_static_world;
} LC_WorldCvars;

typedef enum
{
	LC_TASK_STATUS__INACTIVE,
	LC_TASK_STATUS__READY,
	LC_TASK_STATUS__WORKING,
	LC_TASK_STATUS__COMPLETED,
} LC_TaskStatus;
typedef struct
{
	LC_Chunk chunk;
	GeneratedChunkVerticesResult* vertices_result;
	LC_TaskStatus status;
} LC_Task;

typedef struct
{
	LC_Task task_list[LC_MAX_ACTIVE_TASKS];
	int lookup_index;
} LC_TaskQueue;

typedef struct
{
	bool force_exit;
	HANDLE win_handle;
	HANDLE mutex_handle;
} LC_Thread;

typedef struct
{
	LC_Block* block;
	int hp;
	float cooldown_timer;
} LC_PrevMinedBlock;

static LC_WorldCvars lc_cvars;
static LC_World lc_world;
static LC_TaskQueue lc_task_queue;
static LC_Thread lc_thread;
static LC_PrevMinedBlock lc_prev_mined_block;

static void LC_World_UpdateDrawCmds()
{
	static int counter = 0;
	if (lc_world.require_draw_cmd_update)
	{
		glNamedBufferSubData(lc_world.render_data.draw_cmds_buffer.buffer, 0,
			dA_size(lc_world.draw_cmd_backbuffer) * sizeof(LC_CombinedChunkDrawCmdData), dA_getFront(lc_world.draw_cmd_backbuffer));

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		lc_world.require_draw_cmd_update = false;
		counter++;
		printf("draw cmd update %i \n", counter);
	}
}

static LC_Chunk* LC_World_getNeighbourChunk(LC_Chunk* const p_chunk, int p_side)
{
	ivec3 normalized_chunk_pos;
	LC_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], normalized_chunk_pos);

	ivec3 chunk_pos;
	glm_ivec3_copy(normalized_chunk_pos, chunk_pos);
	switch (p_side)
	{
	case 0:
	{
		chunk_pos[2]--;
		break;
	}
	case 1:
	{
		chunk_pos[2]++;
		break;
	}
	case 3:
	{
		chunk_pos[0]--;
		break;
	}
	case 4:
	{
		chunk_pos[0]++;
		break;
	}
	case 5:
	{
		chunk_pos[1]--;
		break;
	}
	case 6:
	{
		chunk_pos[1]++;
		break;
	}
	default:
		break;
	}

	return CHMap_Find(&lc_world.chunk_map, chunk_pos);
}

static void LC_World_CreateLightBlock(int p_x, int p_y, int p_z, LC_Block_LightData light_data)
{
	PointLight point_light;
	memset(&point_light, 0, sizeof(PointLight));

	point_light.ambient_intensity = light_data.ambient_intensity;
	point_light.attenuation = light_data.attenuation;
	point_light.radius = light_data.radius;
	point_light.specular_intensity = 1;
	point_light.color[0] = light_data.color[0];
	point_light.color[1] = light_data.color[1];
	point_light.color[2] = light_data.color[2];

	point_light.position[0] = p_x;
	point_light.position[1] = p_y;
	point_light.position[2] = p_z;

	LightID index = RScene_RegisterPointLight(point_light, false);

	ivec3 block_pos;
	block_pos[0] = p_x;
	block_pos[1] = p_y;
	block_pos[2] = p_z;

	CHMap_Insert(&lc_world.light_block_map, block_pos, &index);
}

static void LC_World_DestroyLightBlock(int p_x, int p_y, int p_z)
{
	ivec3 block_pos;
	block_pos[0] = p_x;
	block_pos[1] = p_y;
	block_pos[2] = p_z;

	LightID* light_index = CHMap_Find(&lc_world.light_block_map, block_pos);
	//remove from the light hash map
	if (light_index)
	{
		RScene_DeleteRenderInstance(*light_index);
		CHMap_Erase(&lc_world.light_block_map, block_pos);
	}
}

static LC_Chunk* LC_World_InsertChunk(LC_Chunk* p_chunk)
{	
	if (p_chunk->alive_blocks > 0)
	{
		if (lc_world.num_alive_chunks >= LC_WORLD_MAX_CHUNK_LIMIT - 2)
		{
			return NULL;
		}

		lc_world.num_alive_chunks++;
	}

	ivec3 chunk_key;
	LC_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], chunk_key);

	LC_Chunk* chunk = CHMap_Insert(&lc_world.chunk_map, chunk_key, p_chunk);

	chunk->opaque_index = -1;
	chunk->transparent_index = -1;
	chunk->water_index = -1;
	chunk->chunk_data_index = -1;
	chunk->draw_cmd_index = -1;
	chunk->aabb_tree_index = -1;

	chunk->is_deleted = false;

	if (chunk->light_blocks > 0)
	{
		int light_blocks_visited = 0;

		for (int x = 0; x < LC_CHUNK_WIDTH; x++)
		{
			for (int y = 0; y < LC_CHUNK_HEIGHT; y++)
			{
				for (int z = 0; z < LC_CHUNK_LENGTH; z++)
				{
					if(light_blocks_visited >= chunk->light_blocks)
					{
						break;
					}

					if (LC_isblockEmittingLight(chunk->blocks[x][y][z].type))
					{
						LC_Block_LightData light_data = LC_getBlockLightingData(chunk->blocks[x][y][z].type);

						LC_World_CreateLightBlock(x + chunk->global_position[0], y + chunk->global_position[1], z + chunk->global_position[2], light_data);
						light_blocks_visited++;
					}
				}
			}
		}
	}

	return chunk;
}

static void LC_World_GetRenderDistanceBounds(ivec3 min_max[2])
{
	vec3 player_position;
	LC_Player_getPosition(player_position);

	ivec3 normalized_player_position;
	LC_getNormalizedChunkPosition(player_position[0], player_position[1], player_position[2], normalized_player_position);

	const int render_distance_chunks = 8;

	min_max[0][0] = normalized_player_position[0] - render_distance_chunks;
	min_max[0][1] = normalized_player_position[1] - 4;
	min_max[0][2] = normalized_player_position[2] - render_distance_chunks;

	min_max[1][0] = normalized_player_position[0] + render_distance_chunks;
	min_max[1][1] = normalized_player_position[1] + 4;
	min_max[1][2] = normalized_player_position[2] + render_distance_chunks;
}

static float LC_World_CalculateSunAngle(long time)
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

static void LC_World_CalcSunColor(float sun_angle, vec3 dest, float* r_ambientIntensity)
{
	float abs_angle = fabsf(sun_angle);

	float r = 1;
	float g = 1;
	float b = 0;
	if (sun_angle > 0)
	{
		r = glm_lerp(0.0, 1.0, abs_angle);
		g = glm_lerp(0.0, 1.0, abs_angle);
		b = glm_lerp(0.0, 0.8, abs_angle);
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

	*r_ambientIntensity = glm_lerp(0.0, 7, abs_angle);

	//its night
	if (sun_angle < 0)
	{
		*r_ambientIntensity = 0;
	}
}

static void LC_World_CalculateEnvColor(float sun_angle, vec3 sky, vec3 sky_horizon, vec3 ground_horizon, vec3 ground)
{
	float abs_angle = fabsf(sun_angle);

	//daytime 
	if (sun_angle > 0)
	{
		float sky_energy = glm_lerp(0.3, 1, abs_angle);
		sky[0] = glm_lerp(2.0, 0.0, abs_angle) * sky_energy;
		sky[1] = glm_lerp(1.3, 0.0, abs_angle) * sky_energy;
		sky[2] = glm_lerp(1.5, 0.3, abs_angle) * sky_energy;

		sky_horizon[0] = glm_lerp(3.0, 1.0, abs_angle) * sky_energy;
		sky_horizon[1] = glm_lerp(2.0, 1.0, abs_angle) * sky_energy;
		sky_horizon[2] = glm_lerp(0.0, 1.7, abs_angle) * sky_energy;

		ground_horizon[0] = glm_lerp(3.0, 1.0, abs_angle) * sky_energy;
		ground_horizon[1] = glm_lerp(2.0, 1.0, abs_angle) * sky_energy;
		ground_horizon[2] = glm_lerp(0.0, 1.7, abs_angle) * sky_energy;

		ground[0] = 1;
		ground[1] = 1;
		ground[2] = 1;
	}
	else
	{
		glm_vec3_zero(sky);
		glm_vec3_zero(sky_horizon);
		glm_vec3_zero(ground_horizon);
		glm_vec3_zero(ground);
	}
}

static void LC_World_UpdateWorldEnviroment()
{
	static const vec3 axis = { 1, 1, 1 };

	//Calculate sun angle
	float sun_angle = LC_World_CalculateSunAngle(lc_world.time);

	//Convert to a vector
	vec3 sun_dir = { 0, 1, 0 };

	//Rotate
	glm_vec3_rotate(sun_dir, sun_angle * 360.0, axis);
	glm_normalize(sun_dir);

	//Set up sun light
	DirLight dir_light;

	dir_light.direction[0] = sun_dir[0];
	dir_light.direction[1] = sun_dir[1];
	dir_light.direction[2] = sun_dir[2];

	LC_World_CalcSunColor(sun_dir[1], dir_light.color, &dir_light.ambient_intensity);
	dir_light.specular_intensity = 2;

	RScene_SetDirLight(dir_light);

	//Calculate enviroment color
	vec3 sky, sky_horizon, ground_horizon, ground;
	LC_World_CalculateEnvColor(sun_dir[1], sky, sky_horizon, ground_horizon, ground);

	RScene_SetSkyColor(sky);
	RScene_SetSkyHorizonColor(sky_horizon);
	RScene_SetGroundHorizonColor(ground_horizon);
	RScene_SetGroundColor(ground);
}


static void LC_World_ThreadProcess()
{
	int index = 0;

	while (lc_thread.force_exit == false)
	{
		LC_Task* task = &lc_task_queue.task_list[index];

		if (task->status == LC_TASK_STATUS__READY)
		{
			//WaitForSingleObject(lc_thread.mutex_handle, INFINITY);

			//lc_thread.mutex_handle = CreateMutex(NULL, true, "LC_MUTEX");

			//set status
			task->status = LC_TASK_STATUS__WORKING;

			//generate blocks
			LC_Chunk_GenerateBlocks(&task->chunk, 2);

			//generate vertices
			if (task->chunk.alive_blocks > 0)
			{
				//task->vertices_result = LC_Chunk_GenerateVertices(&task->chunk);
			}
		
			//set as completed
			task->status = LC_TASK_STATUS__COMPLETED;

			if (lc_thread.mutex_handle)
			{
				//ReleaseMutex(lc_thread.mutex_handle);
			}
		}

		index = (index + 1) % LC_MAX_ACTIVE_TASKS;
	}
}

static bool LC_World_CreateChunkAsync(int p_x, int p_y, int p_z)
{	
	bool result = false;

	LC_Task* task = &lc_task_queue.task_list[lc_task_queue.lookup_index];

	if (task->status == LC_TASK_STATUS__INACTIVE)
	{
		task->vertices_result = NULL;
		task->chunk = LC_Chunk_Create(p_x * LC_CHUNK_WIDTH, p_y * LC_CHUNK_HEIGHT, p_z * LC_CHUNK_LENGTH);

		task->status = LC_TASK_STATUS__READY;

		result = true;
	}

	lc_task_queue.lookup_index = (lc_task_queue.lookup_index + 1) % LC_MAX_ACTIVE_TASKS;

	return result;
}

static void LC_World_ProcessTaskQueue()
{
	if (lc_world.num_alive_chunks >= LC_WORLD_MAX_CHUNK_LIMIT - 2)
	{
		return;
	}
	//if (WaitForSingleObject(lc_thread.mutex_handle, 0.01) == WAIT_TIMEOUT)
	//{
	//	return;
	//}

	//lc_thread.mutex_handle = CreateMutex(NULL, true, "LC_MUTEX");

	for (int i = 0; i < LC_MAX_ACTIVE_TASKS; i++)
	{
		LC_Task* task = &lc_task_queue.task_list[i];

		if (task->status == LC_TASK_STATUS__COMPLETED)
		{
			//insert to hash map
			LC_Chunk* chunk = LC_World_InsertChunk(&task->chunk);

			if (!chunk)
			{
				task->status = LC_TASK_STATUS__INACTIVE;
				continue;
			}
			if (task->chunk.alive_blocks <= 0)
			{
				task->status = LC_TASK_STATUS__INACTIVE;
				continue;
			}

			if (!task->vertices_result)
			{
				//continue;
			}

			if (task->chunk.alive_blocks > 0 && !lc_world.player_action_this_frame)
			{
				//GeneratedChunkVerticesResult* result = LC_Chunk_GenerateVerticesTest(chunk);
				LC_World_UpdateChunk(chunk, NULL);

				task->status = LC_TASK_STATUS__INACTIVE;

				break;
			}
		}
	}
	if (lc_thread.mutex_handle)
	{
		//ReleaseMutex(lc_thread.mutex_handle);
	}
}

static void LC_World_IterateChunks()
{
	if (dA_capacity(lc_world.draw_cmd_backbuffer) <= lc_world.render_data.draw_cmds_buffer.used_size)
	{
		dA_resize(lc_world.draw_cmd_backbuffer, lc_world.render_data.draw_cmds_buffer.used_size);
	}

	memset(lc_world.draw_cmd_backbuffer->data, 0, sizeof(LC_CombinedChunkDrawCmdData) * lc_world.draw_cmd_backbuffer->elements_size);

	ivec3 min_max_bounds[2];
	LC_World_GetRenderDistanceBounds(min_max_bounds);

	for (int i = 0; i < dA_size(lc_world.chunk_map.item_data); i++)
	{
		LC_Chunk* chunk = dA_at(lc_world.chunk_map.item_data, i);

		if (chunk->is_deleted)
		{
			continue;
		}

		ivec3 normalized_chunk_pos;
		LC_getNormalizedChunkPosition(chunk->global_position[0], chunk->global_position[1], chunk->global_position[2], normalized_chunk_pos);

		if (lc_cvars.lc_static_world->int_value == 0)
		{
			//Delete chunk if it falls outside the min and max
			if (normalized_chunk_pos[0] < min_max_bounds[0][0] || normalized_chunk_pos[0] > min_max_bounds[1][0] || normalized_chunk_pos[1] < min_max_bounds[0][1] || normalized_chunk_pos[1] > min_max_bounds[1][1]
				|| normalized_chunk_pos[2] < min_max_bounds[0][2] || normalized_chunk_pos[2] > min_max_bounds[1][2])
			{
				LC_World_DeleteChunk(chunk);
				continue;
			}
		}
		if (lc_world.require_draw_cmd_update && chunk->draw_cmd_index >= 0)
		{
			LC_CombinedChunkDrawCmdData* cmd = dA_at(lc_world.draw_cmd_backbuffer, chunk->draw_cmd_index);

			if (chunk->opaque_index >= 0 && chunk->opaque_blocks > 0)
			{
				DRB_Item item = DRB_GetItem(&lc_world.render_data.opaque_buffer, chunk->opaque_index);

				cmd->o_count = item.count / sizeof(ChunkVertex);
				cmd->o_first = item.offset / sizeof(ChunkVertex);
			}
			if (chunk->transparent_index >= 0 && chunk->transparent_blocks > 0)
			{
				DRB_Item item = DRB_GetItem(&lc_world.render_data.semi_transparent_buffer, chunk->transparent_index);

				cmd->t_count = item.count / sizeof(ChunkVertex);
				cmd->t_first = item.offset / sizeof(ChunkVertex);
			}
			if (chunk->water_index >= 0 && chunk->water_blocks > 0)
			{
				DRB_Item item = DRB_GetItem(&lc_world.render_data.water_buffer, chunk->water_index);

				cmd->w_count = item.count / sizeof(ChunkWaterVertex);
				cmd->w_first = item.offset / sizeof(ChunkWaterVertex);
			}
		}
		
	}
}

static void LC_World_CreateNearbyChunks()
{
	if (lc_world.num_alive_chunks >= LC_WORLD_MAX_CHUNK_LIMIT - 2)
	{
		return;
	}

	ivec3 min_max_bounds[2];
	LC_World_GetRenderDistanceBounds(min_max_bounds);

	for (int x = min_max_bounds[0][0]; x < min_max_bounds[1][0]; x++)
	{
		for (int y = min_max_bounds[0][1]; y < min_max_bounds[1][1]; y++)
		{
			for (int z = min_max_bounds[0][2]; z < min_max_bounds[1][2]; z++)
			{
				if (!LC_World_ChunkExists(x * LC_CHUNK_WIDTH, y * LC_CHUNK_HEIGHT, z * LC_CHUNK_LENGTH))
				{
					LC_World_CreateChunkAsync(x, y, z);
					break;
				}
			}
		}
	}
}

static void LC_World_FloodWater(LC_Chunk* const p_chunk)
{
	if (p_chunk->water_blocks <= 0)
	{
		return;
	}
	//return;
	int num_none_blocks = (LC_CHUNK_WIDTH * LC_CHUNK_HEIGHT * LC_CHUNK_LENGTH) - p_chunk->alive_blocks;

	if (num_none_blocks <= 0)
	{
		return;
	}

	int minWaterX = 16;
	int minWaterY = 16;
	int minWaterZ = 16;

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
					break;
				}
				if (LC_IsBlockWater(p_chunk->blocks[x][y][z].type))
				{
					minWaterX = min(minWaterX, x);
					minWaterY = min(minWaterY, y);
					minWaterZ = min(minWaterZ, z);

					maxWaterX = max(maxWaterX, x);
					maxWaterY = max(maxWaterY, y);
					maxWaterZ = max(maxWaterZ, z);

					for (int x2 = x; x2 >= 0; x2--)
					{
						for (int y2 = y; y2 >= 0; y2--)
						{
							for (int z2 = z; z2 >= 0; z2--)
							{
								if (LC_Chunk_getType(p_chunk, x2, y2, z2) == LC_BT__NONE)
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
							if (LC_Chunk_getType(p_chunk, x2, y, z2) == LC_BT__NONE)
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
	/*
	if (minWaterZ == 0)
	{
		LC_Chunk* neigbour = LC_World_getNeighbourChunk(p_chunk, 0);
		
		if (neigbour)
		{
			for (int x = minWaterX; x < maxWaterX; x++)
			{
				if (LC_Chunk_getType(neigbour, x, LC_WORLD_WATER_HEIGHT, LC_CHUNK_LENGTH - 1) == LC_BT__NONE)
				{
					LC_Chunk_SetBlock(neigbour, x, LC_WORLD_WATER_HEIGHT, LC_CHUNK_LENGTH - 1, LC_BT__WATER);
					LC_World_UpdateChunk(neigbour, NULL);
					break;
				}
			}

		}
	}
	if (maxWaterZ == LC_CHUNK_LENGTH - 1)
	{
		LC_Chunk* neigbour = LC_World_getNeighbourChunk(p_chunk, 1);

		if (neigbour)
		{
			for (int x = minWaterX; x < maxWaterX; x++)
			{
				if (LC_Chunk_getType(neigbour, x, LC_WORLD_WATER_HEIGHT, 0) == LC_BT__NONE)
				{
					LC_Chunk_SetBlock(neigbour, x, LC_WORLD_WATER_HEIGHT, 0, LC_BT__WATER);
					LC_World_UpdateChunk(neigbour, NULL);
					break;
				}
			}

		}
	}
	if (minWaterX == 0)
	{
		LC_Chunk* neigbour = LC_World_getNeighbourChunk(p_chunk, 2);

		if (neigbour)
		{
			for (int z = minWaterZ; z < maxWaterZ; z++)
			{
				if (LC_Chunk_getType(neigbour, LC_CHUNK_WIDTH - 1, LC_WORLD_WATER_HEIGHT, z) == LC_BT__NONE)
				{
					LC_Chunk_SetBlock(neigbour, LC_CHUNK_WIDTH - 1, LC_WORLD_WATER_HEIGHT, z, LC_BT__WATER);
					LC_World_UpdateChunk(neigbour, NULL);
					break;
				}
			}

		}
	}
	if (maxWaterX == LC_CHUNK_WIDTH - 1)
	{
		LC_Chunk* neigbour = LC_World_getNeighbourChunk(p_chunk, 3);

		if (neigbour)
		{
			for (int z = minWaterZ; z < maxWaterZ; z++)
			{
				if (LC_Chunk_getType(neigbour, 0, LC_WORLD_WATER_HEIGHT, z) == LC_BT__NONE)
				{
					LC_Chunk_SetBlock(neigbour, 0, LC_WORLD_WATER_HEIGHT, z, LC_BT__WATER);
					LC_World_UpdateChunk(neigbour, NULL);
					break;
				}
			}
		}
	}
	*/
}

LC_Chunk* LC_World_GetChunk(float p_x, float p_y, float p_z)
{
	ivec3 chunk_key;
	LC_getNormalizedChunkPosition(p_x, p_y, p_z, chunk_key);

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

	return LC_Chunk_GetBlock(chunk_ptr, x_block_pos, y_block_pos, z_block_pos);
}
LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, int max_steps, ivec3 r_pos, ivec3 r_face, LC_Chunk** r_chunk)
{
	// "A Fast Voxel Traversal Algorithm for Ray Tracing" by John Amanatides, Andrew Woo */
	float big = FLT_MAX;

	int px = (int)floor(from[0]), py = (int)floor(from[1]), pz = (int)floor(from[2]);

	float dxi = 1.0f / dir[0], dyi = 1.0f / dir[1], dzi = 1.0f / dir[2];
	int sx = dir[0] > 0 ? 1 : -1, sy = dir[1] > 0 ? 1 : -1, sz = dir[2] > 0 ? 1 : -1;
	float dtx = min(dxi * sx, big), dty = min(dyi * sy, big), dtz = min(dzi * sz, big);
	float tx = fabsf((px + max(sx, 0) - from[0]) * dxi), ty = fabsf((py + max(sy, 0) - from[1]) * dyi), tz = fabsf((pz + max(sz, 0) - from[2]) * dzi);
	int maxSteps = max_steps;
	int cmpx = 0, cmpy = 0, cmpz = 0;

	for (int i = 0; i < maxSteps; i++)
	{
		if (i > 0)
		{
			LC_Block* block = LC_World_GetBlock(px, py, pz, NULL, r_chunk);

			if (block != NULL && block->type != LC_BT__NONE && !LC_IsBlockWater(block->type))
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

bool LC_World_addBlock(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, LC_BlockType block_type)
{
	if (block_type == LC_BT__NONE)
		return false;

	ivec3 relative_pos;
	LC_Chunk* chunk = NULL;

	LC_Block* block = LC_World_GetBlock(p_gX, p_gY, p_gZ, relative_pos, &chunk);

	//block or chunk not found?
	if (!block || !chunk)
		return false;

	//we can't add to props
	if (LC_isBlockProp(block->type))
	{
		return false;
	}

	int new_chunk_pos_x = p_addFace[0] + (chunk->global_position[0] / LC_CHUNK_WIDTH);
	int new_chunk_pos_y = p_addFace[1] + (chunk->global_position[1] / LC_CHUNK_HEIGHT);
	int new_chunk_pos_z = p_addFace[2] + (chunk->global_position[2] / LC_CHUNK_LENGTH);

	int new_block_pos_x = (relative_pos[0] + chunk->global_position[0]) + p_addFace[0];
	int new_block_pos_y = (relative_pos[1] + chunk->global_position[1]) + p_addFace[1];
	int new_block_pos_z = (relative_pos[2] + chunk->global_position[2]) + p_addFace[2];
	

	LC_Chunk* new_chunk = NULL;
	LC_Block* new_block = LC_World_GetBlock(new_block_pos_x, new_block_pos_y, new_block_pos_z, NULL, &new_chunk);

	if (new_block)
	{
		if (new_block->type != LC_BT__NONE)
		{
			//Allow placing blocks on top of water blocks
			if (!LC_IsBlockWater(new_block->type))
			{
				return false;
			}
		}
	}
	
	//new chunk not found? create
	if (!new_chunk)
	{	
		LC_Chunk stack_chunk = LC_Chunk_Create(new_chunk_pos_x * LC_CHUNK_WIDTH, new_chunk_pos_y * LC_CHUNK_HEIGHT, new_chunk_pos_z * LC_CHUNK_LENGTH);
		new_chunk = LC_World_InsertChunk(&stack_chunk);

		if (!new_chunk)
		{
			return false;
		}
	}

	int new_block_relative_pos_x = new_block_pos_x - new_chunk->global_position[0];
	int new_block_relative_pos_y = new_block_pos_y - new_chunk->global_position[1];
	int new_block_relative_pos_z = new_block_pos_z - new_chunk->global_position[2];

	//sanity check
	assert(new_block_relative_pos_x >= 0 && new_block_relative_pos_x < LC_CHUNK_WIDTH && new_block_relative_pos_y >= 0 && new_block_relative_pos_y < LC_CHUNK_HEIGHT &&
		new_block_relative_pos_z >= 0 && new_block_relative_pos_x < LC_CHUNK_LENGTH);

	int old_alive_blocks = new_chunk->alive_blocks;

	LC_Chunk_SetBlock(new_chunk, new_block_relative_pos_x, new_block_relative_pos_y, new_block_relative_pos_z, block_type);

	if (LC_isblockEmittingLight(block_type))
	{
		LC_Block_LightData light_data = LC_getBlockLightingData(block_type);

		LC_World_CreateLightBlock(new_block_pos_x, new_block_pos_y, new_block_pos_z, light_data);
	}

	LC_World_UpdateChunk(new_chunk, NULL);

	if (old_alive_blocks == 0 && new_chunk->alive_blocks == 1)
	{
		lc_world.num_alive_chunks++;
	}

	lc_world.player_action_this_frame = true;

	return true;
}

bool LC_World_mineBlock(int p_gX, int p_gY, int p_gZ)
{
	ivec3 relative_block_position;
	LC_Chunk* chunk = NULL;
	LC_Block* block = LC_World_GetBlock(p_gX, p_gY, p_gZ, relative_block_position, &chunk);

	//block not found?
	if (!block || !chunk)
	{
		return false;
	}

	//is water block?
	if (LC_IsBlockWater(block->type))
	{
		return false;
	}

	lc_prev_mined_block.hp--;

	//destroy instantly if creative mode is on or block is a prop
	if (lc_world.creative_mode_on || LC_isBlockProp(block->type))
	{
		lc_prev_mined_block.hp = 0;
	}

	if (lc_prev_mined_block.block)
	{
		if (lc_prev_mined_block.block != block)
		{
			lc_prev_mined_block.hp = LC_BLOCK_STARTING_HP;
		}
	}
	lc_prev_mined_block.cooldown_timer = 0;
	

	//destroy block
	if (lc_prev_mined_block.hp <= 0)
	{	
		if (LC_isblockEmittingLight(block->type))
		{
			LC_World_DestroyLightBlock(p_gX, p_gY, p_gZ);
		}

		LC_Chunk_SetBlock(chunk, relative_block_position[0], relative_block_position[1], relative_block_position[2], LC_BT__NONE);

		//update chunk
		LC_World_UpdateChunk(chunk, NULL);

		lc_prev_mined_block.block = NULL;
		lc_prev_mined_block.hp = LC_BLOCK_STARTING_HP;

		lc_world.player_action_this_frame = true;
	}

	return true;
}

bool LC_World_ChunkExists(float p_x, float p_y, float p_z)
{
	if (LC_World_GetChunk(p_x, p_y, p_z))
	{
		return true;
	}

	return false;
}

void LC_World_UpdateChunk(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* vertices_result)
{
	//flood water to nearby blocks and chunks
	if (p_chunk->water_blocks > 0)
	{
		LC_World_FloodWater(p_chunk);
	}

	LC_World_UpdateChunkIndexes(p_chunk);

	GeneratedChunkVerticesResult* vertices = NULL;

	if (p_chunk->alive_blocks > 0)
	{
		if (vertices_result)
		{
			vertices = vertices_result;
		}
		else
		{
			vertices = LC_Chunk_GenerateVertices(p_chunk);
		}
	}
	if (LC_World_UpdateChunkVertices(p_chunk, vertices))
	{
		lc_world.require_draw_cmd_update = true;
	}
}

void LC_World_UpdateChunkIndexes(LC_Chunk* const p_chunk)
{
	if (p_chunk->alive_blocks <= 0)
	{
		return;
	}

	bool data_changed = false;

	if (p_chunk->opaque_blocks > 0 && p_chunk->opaque_index == -1)
	{
		p_chunk->opaque_index = DRB_EmplaceItem(&lc_world.render_data.opaque_buffer, 0, NULL);
		data_changed = true;
	}
	if (p_chunk->transparent_blocks > 0 && p_chunk->transparent_index == -1)
	{
		p_chunk->transparent_index = DRB_EmplaceItem(&lc_world.render_data.semi_transparent_buffer, 0, NULL);
		data_changed = true;
	}
	if (p_chunk->water_blocks > 0 && p_chunk->water_index == -1)
	{
		p_chunk->water_index = DRB_EmplaceItem(&lc_world.render_data.water_buffer, 0, NULL);
		data_changed = true;
	}
	if (p_chunk->chunk_data_index == -1)
	{
		unsigned chunk_data_index = RSB_Request(&lc_world.render_data.chunk_data_buffer);

		LC_ChunkData chunk_data;
		memset(&chunk_data, 0, sizeof(LC_ChunkData));

		chunk_data.min_point[0] = p_chunk->global_position[0];
		chunk_data.min_point[1] = p_chunk->global_position[1];
		chunk_data.min_point[2] = p_chunk->global_position[2];
		chunk_data.min_point[3] = chunk_data_index;

		glNamedBufferSubData(lc_world.render_data.chunk_data_buffer.buffer, chunk_data_index * sizeof(LC_ChunkData), sizeof(LC_ChunkData), &chunk_data);

		p_chunk->chunk_data_index = chunk_data_index;

		data_changed = true;
	}
	if (p_chunk->draw_cmd_index == -1)
	{
		p_chunk->draw_cmd_index = RSB_Request(&lc_world.render_data.draw_cmds_buffer);
	}
	if (p_chunk->aabb_tree_index == -1)
	{
		LC_TreeData* tree_data = malloc(sizeof(LC_TreeData));

		if (tree_data)
		{
			tree_data->chunk_data_index = p_chunk->chunk_data_index;
			tree_data->opaque_index = p_chunk->opaque_index;
			tree_data->transparent_index = p_chunk->transparent_index;
			tree_data->water_index = p_chunk->water_index;

			vec3 box[2];
			box[0][0] = (float)p_chunk->global_position[0] - 0.5;
			box[0][1] = (float)p_chunk->global_position[1] - 0.5;
			box[0][2] = (float)p_chunk->global_position[2] - 0.5;

			box[1][0] = (float)p_chunk->global_position[0] + LC_CHUNK_WIDTH;
			box[1][1] = (float)p_chunk->global_position[1] + LC_CHUNK_HEIGHT;
			box[1][2] = (float)p_chunk->global_position[2] + LC_CHUNK_LENGTH;

			p_chunk->aabb_tree_index = BVH_Tree_Insert(&lc_world.render_data.bvh_tree, box, tree_data);
		}
	}
	else if (p_chunk->aabb_tree_index >= 0 && data_changed)
	{
		LC_TreeData* tree_data = BVH_Tree_GetData(&lc_world.render_data.bvh_tree, p_chunk->aabb_tree_index);

		assert(tree_data);

		tree_data->chunk_data_index = p_chunk->chunk_data_index;
		tree_data->opaque_index = p_chunk->opaque_index;
		tree_data->transparent_index = p_chunk->transparent_index;
		tree_data->water_index = p_chunk->water_index;
	}

	//sanity check. These should always match
	assert(p_chunk->chunk_data_index == p_chunk->draw_cmd_index);
}

bool LC_World_UpdateChunkVertices(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* p_vertices_result)
{
	bool need_draw_cmd_update = false;
	bool update_draw_cmd_cpu = false;

	LC_CombinedChunkDrawCmdData cmd;
	memset(&cmd, 0, sizeof(cmd));

	if (p_chunk->opaque_index >= 0)
	{
		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.opaque_buffer, p_chunk->opaque_index);

		//upload to the vertex buffer
		if (p_chunk->opaque_blocks > 0 && p_vertices_result->opaque_vertices)
		{
			DRB_ChangeData(&lc_world.render_data.opaque_buffer, sizeof(ChunkVertex) * p_vertices_result->opaque_vertex_count, p_vertices_result->opaque_vertices, p_chunk->opaque_index);

			DRB_Item current_item = DRB_GetItem(&lc_world.render_data.opaque_buffer, p_chunk->opaque_index);

			if (prev_item.count > 0)
			{
				need_draw_cmd_update = true;
			}
			else
			{
				update_draw_cmd_cpu = true;
				cmd.o_count = current_item.count / sizeof(ChunkVertex);
				cmd.o_first = current_item.offset / sizeof(ChunkVertex);
			}
		}
		//clean up the vertex data if we dont have any blocks left
		else if (prev_item.count > 0 && p_chunk->opaque_blocks <= 0)
		{
			DRB_ChangeData(&lc_world.render_data.opaque_buffer, 0, NULL, p_chunk->opaque_index);
			need_draw_cmd_update = true;
		}
		
	}
	if (p_chunk->transparent_index >= 0)
	{
		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.semi_transparent_buffer, p_chunk->transparent_index);

		if (p_chunk->transparent_blocks > 0 && p_vertices_result->transparent_vertices)
		{
			//upload to the vertex buffer
			DRB_ChangeData(&lc_world.render_data.semi_transparent_buffer, sizeof(ChunkVertex) * p_vertices_result->transparent_vertex_count, p_vertices_result->transparent_vertices, p_chunk->transparent_index);

			DRB_Item current_item = DRB_GetItem(&lc_world.render_data.semi_transparent_buffer, p_chunk->transparent_index);

			if (prev_item.count > 0)
			{
				need_draw_cmd_update = true;
			}
			else
			{
				update_draw_cmd_cpu = true;
				cmd.t_count = current_item.count / sizeof(ChunkVertex);
				cmd.t_first = current_item.offset / sizeof(ChunkVertex);
			}
		}
		//clean up the vertex data if we dont have any blocks left
		else if (prev_item.count > 0 && p_chunk->transparent_blocks <= 0)
		{
			DRB_ChangeData(&lc_world.render_data.semi_transparent_buffer, 0, NULL, p_chunk->transparent_index);
			need_draw_cmd_update = true;
		}

	}
	if (p_chunk->water_index >= 0)
	{
		DRB_Item prev_item = DRB_GetItem(&lc_world.render_data.water_buffer, p_chunk->water_index);

		if (p_chunk->water_blocks > 0 && p_vertices_result->water_vertices)
		{
			//upload to the vertex buffer
			DRB_ChangeData(&lc_world.render_data.water_buffer, sizeof(ChunkWaterVertex) * p_vertices_result->water_vertex_count, p_vertices_result->water_vertices, p_chunk->water_index);

			DRB_Item current_item = DRB_GetItem(&lc_world.render_data.water_buffer, p_chunk->water_index);

			if (prev_item.count > 0)
			{
				need_draw_cmd_update = true;
			}
			else
			{
				update_draw_cmd_cpu = true;
				cmd.w_count = current_item.count / sizeof(ChunkWaterVertex);
				cmd.w_first = current_item.offset / sizeof(ChunkWaterVertex);
			}
		}
		//clean up the vertex data if we dont have any blocks left
		else if (prev_item.count > 0 && p_chunk->water_blocks <= 0)
		{
			DRB_ChangeData(&lc_world.render_data.water_buffer, 0, NULL, p_chunk->water_index);
			need_draw_cmd_update = true;
		}
	}

	if (!need_draw_cmd_update && update_draw_cmd_cpu)
	{
		if (p_chunk->draw_cmd_index >= 0)
		{
			glNamedBufferSubData(lc_world.render_data.draw_cmds_buffer.buffer, sizeof(LC_CombinedChunkDrawCmdData) * p_chunk->draw_cmd_index, sizeof(LC_CombinedChunkDrawCmdData), &cmd);
		}
	}

	if (p_vertices_result)
	{
		//free the vertices buffers
		if (p_vertices_result->opaque_vertices)
		{
			free(p_vertices_result->opaque_vertices);
		}
		if (p_vertices_result->transparent_vertices)
		{
			free(p_vertices_result->transparent_vertices);
		}
		if (p_vertices_result->water_vertices)
		{
			free(p_vertices_result->water_vertices);
		}

		//free the result
		if (p_vertices_result)
		{
			free(p_vertices_result);
		}
	}
	

	return need_draw_cmd_update;
}

void LC_World_DeleteChunk(LC_Chunk* const p_chunk)
{
	assert(p_chunk->draw_cmd_index == p_chunk->chunk_data_index);

	//remove the item from vertex buffers
	if (p_chunk->opaque_index != -1)
	{
		DRB_RemoveItem(&lc_world.render_data.opaque_buffer, p_chunk->opaque_index);

		p_chunk->opaque_index = -1;
	}
	if (p_chunk->transparent_index != -1)
	{
		DRB_RemoveItem(&lc_world.render_data.semi_transparent_buffer, p_chunk->transparent_index);

		p_chunk->transparent_index = -1;
	}
	if (p_chunk->water_index != -1)
	{
		DRB_RemoveItem(&lc_world.render_data.water_buffer, p_chunk->water_index);

		p_chunk->water_index = -1;
	}
	if (p_chunk->chunk_data_index != -1)
	{
		RSB_FreeItem(&lc_world.render_data.chunk_data_buffer, p_chunk->chunk_data_index, false);

		p_chunk->chunk_data_index = -1;
	}
	if (p_chunk->draw_cmd_index != -1)
	{
		LC_CombinedChunkDrawCmdData draw_data;
		memset(&draw_data, 0, sizeof(LC_CombinedChunkDrawCmdData));
		
		glNamedBufferSubData(lc_world.render_data.draw_cmds_buffer.buffer, sizeof(LC_CombinedChunkDrawCmdData) * p_chunk->draw_cmd_index, sizeof(LC_CombinedChunkDrawCmdData), &draw_data);

		RSB_FreeItem(&lc_world.render_data.draw_cmds_buffer, p_chunk->draw_cmd_index, false);

		p_chunk->draw_cmd_index = -1;
	}
	if (p_chunk->aabb_tree_index > -1)
	{
		LC_TreeData* tree_data = BVH_Tree_GetData(&lc_world.render_data.bvh_tree, p_chunk->aabb_tree_index);

		if (tree_data)
		{
			free(tree_data);
		}

		BVH_Tree_Remove(&lc_world.render_data.bvh_tree, p_chunk->aabb_tree_index);

		p_chunk->aabb_tree_index = -1;
	}
	//remove any light blcoks
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
						LC_World_DestroyLightBlock(x + p_chunk->global_position[0], y + p_chunk->global_position[1], z + p_chunk->global_position[2]);
						p_chunk->light_blocks--;
					}
					if (p_chunk->light_blocks <= 0)
					{
						break;
					}
				}
			}
		}
	}

	if (p_chunk->alive_blocks > 0)
	{
		lc_world.num_alive_chunks--;
	}

	p_chunk->alive_blocks = 0;
	p_chunk->light_blocks = 0;
	p_chunk->water_blocks = 0;
	p_chunk->opaque_blocks = 0;
	p_chunk->transparent_blocks = 0;

	p_chunk->is_deleted = true;

	ivec3 hash_key;
	LC_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], hash_key);

	//remove from hashmap
	CHMap_Erase(&lc_world.chunk_map, hash_key);
}


static void LC_World_SetupGLBindingPoints()
{
	glBindBufferBase(GL_UNIFORM_BUFFER, 5, lc_world.render_data.block_data_buffer);
}

void LC_World_Create(int x_chunks, int y_chunks, int z_chunks)
{
	memset(&lc_world, 0, sizeof(LC_World));
	memset(&lc_task_queue, 0, sizeof(lc_task_queue));
	memset(&lc_thread, 0, sizeof(lc_thread));
	memset(&lc_prev_mined_block, 0, sizeof(lc_prev_mined_block));
	memset(&lc_cvars, 0, sizeof(lc_cvars));

	lc_cvars.lc_static_world = Cvar_Register("lc_static_world", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);

	lc_world.seed = 2;
	Math_srand(lc_world.seed);

	LC_Generate_SetSeed(lc_world.seed);

	RScene_SetNightTexture(Resource_get("assets/cubemaps/hdr/night_sky.hdr", RESOURCE__TEXTURE_HDR));

	//init the phys world
	lc_world.phys_world = PhysicsWorld_Create(1.1);

	//Init the aabb tree
	lc_world.render_data.bvh_tree = BVH_Tree_Create(0.0);

	//init the chunk hash map
	lc_world.chunk_map = CHMAP_INIT_POOLED(Hash_ivec3, NULL, ivec3, LC_Chunk, LC_WORLD_MAX_CHUNK_LIMIT);

	lc_world.light_block_map = CHMAP_INIT(Hash_ivec3, NULL, ivec3, unsigned, 1);

	lc_world.draw_cmd_backbuffer = dA_INIT(LC_CombinedChunkDrawCmdData, 0);

	lc_world.render_data.opaque_buffer = DRB_Create(sizeof(ChunkVertex) * LC_WORLD_MAX_CHUNK_LIMIT, LC_WORLD_MAX_CHUNK_LIMIT, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE | DRB_FLAG__POOLABLE_KEEP_DATA);

	lc_world.render_data.semi_transparent_buffer = DRB_Create(sizeof(ChunkVertex) * LC_WORLD_MAX_CHUNK_LIMIT, LC_WORLD_MAX_CHUNK_LIMIT, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE | DRB_FLAG__POOLABLE_KEEP_DATA);

	lc_world.render_data.water_buffer = DRB_Create(sizeof(ChunkWaterVertex) * 1000000, LC_WORLD_MAX_CHUNK_LIMIT, DRB_FLAG__WRITABLE | DRB_FLAG__RESIZABLE | DRB_FLAG__USE_CPU_BACK_BUFFER | DRB_FLAG__POOLABLE | DRB_FLAG__POOLABLE_KEEP_DATA);

	lc_world.render_data.chunk_data_buffer = RSB_Create(LC_WORLD_MAX_CHUNK_LIMIT, sizeof(LC_ChunkData), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE);

	lc_world.render_data.draw_cmds_buffer = RSB_Create(LC_WORLD_MAX_CHUNK_LIMIT, sizeof(LC_CombinedChunkDrawCmdData), RSB_FLAG__POOLABLE | RSB_FLAG__WRITABLE);
	
	glGenVertexArrays(1, &lc_world.render_data.vao);
	
	glBindVertexArray(lc_world.render_data.vao);
	glBindBuffer(GL_ARRAY_BUFFER, lc_world.render_data.opaque_buffer.buffer);
	glBindVertexBuffer(0, lc_world.render_data.opaque_buffer.buffer, 0, sizeof(ChunkVertex));

	glVertexAttribIFormat(0, 3, GL_BYTE, (void*)(offsetof(ChunkVertex, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribBinding(0, 0);

	glVertexAttribIFormat(1, 1, GL_BYTE, (void*)(offsetof(ChunkVertex, normal)));
	glEnableVertexAttribArray(1);
	glVertexAttribBinding(1, 0);

	glVertexAttribIFormat(2, 1, GL_UNSIGNED_BYTE, (void*)(offsetof(ChunkVertex, block_type)));
	glEnableVertexAttribArray(2);
	glVertexAttribBinding(2, 0);

	glGenVertexArrays(1, &lc_world.render_data.water_vao);
	glBindVertexArray(lc_world.render_data.water_vao);
	glBindBuffer(GL_ARRAY_BUFFER, lc_world.render_data.water_buffer.buffer);

	glVertexAttribIPointer(0, 2, GL_BYTE, sizeof(ChunkWaterVertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribBinding(0, 0);

	glGenBuffers(1, &lc_world.render_data.visibles_sorted_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lc_world.render_data.visibles_sorted_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * LC_WORLD_MAX_CHUNK_LIMIT * 10, NULL, GL_STREAM_DRAW);

	glGenBuffers(1, &lc_world.render_data.prev_in_frustrum_bitset_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lc_world.render_data.prev_in_frustrum_bitset_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * (LC_WORLD_MAX_CHUNK_LIMIT), NULL, GL_STREAM_DRAW);

	for (int i = 0; i < 3; i++)
	{
		glGenBuffers(1, &lc_world.render_data.atomic_counters[i]);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, lc_world.render_data.atomic_counters[i]);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned), NULL, GL_STATIC_DRAW);
	}

	glGenBuffers(1, &lc_world.render_data.draw_cmds_sorted_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lc_world.render_data.draw_cmds_sorted_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LC_CombinedChunkDrawCmdData) * (LC_WORLD_MAX_CHUNK_LIMIT * 5), NULL, GL_STATIC_DRAW);

	lc_world.render_data.block_data_buffer = LC_generateBlockInfoGLBuffer();

	LC_World_SetupGLBindingPoints();

	printf("Generating chunks...\n");
	//Generate the chunks
	for (int x = 0; x < x_chunks; x++)
	{
		for (int z = 0; z < z_chunks; z++)
		{
			for (int y = 0; y < y_chunks; y++)
			{
				LC_Chunk stack_chunk = LC_Chunk_Create(x * LC_CHUNK_WIDTH, y * LC_CHUNK_HEIGHT, z * LC_CHUNK_LENGTH);
				//Generate the chunk
				LC_Chunk_GenerateBlocks(&stack_chunk, 2);

				//add to the hash map
				if (stack_chunk.alive_blocks > 0)
				{	
					//printf("Generated chunk: %i :opaque blocks, %i :transparent blocks %i water blocks\n", stack_chunk.opaque_blocks, stack_chunk.transparent_blocks, stack_chunk.water_blocks);

					LC_Chunk* chunk = LC_World_InsertChunk(&stack_chunk, x * LC_CHUNK_WIDTH, y * LC_CHUNK_HEIGHT, z * LC_CHUNK_LENGTH);
					if (chunk)
					{
						LC_World_UpdateChunk(chunk, NULL);
					}
					
				}

			}
		}
	}



	DRB_WriteDataToGpu(&lc_world.render_data.opaque_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.semi_transparent_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.water_buffer);

	//Load the textures
	lc_world.render_data.texture_atlas = Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE);
	lc_world.render_data.texture_atlas_normals = Resource_get("assets/cube_textures/simple_block_atlas_normal.png", RESOURCE__TEXTURE);
	lc_world.render_data.texture_atlas_mer = Resource_get("assets/cube_textures/simple_block_atlas_mer.png", RESOURCE__TEXTURE);
	lc_world.render_data.texture_atlas_height = Resource_get("assets/cube_textures/simple_block_atlas_heightmap.png", RESOURCE__TEXTURE);

	lc_world.render_data.water_displacement_texture = Resource_get("assets/water/water_displacement.png", RESOURCE__TEXTURE);
	lc_world.render_data.gradient_map = Resource_get("assets/water/gradient_map.png", RESOURCE__TEXTURE);

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

	glBindTexture(GL_TEXTURE_2D, lc_world.render_data.water_displacement_texture->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	printf("Starting lc thread...\n");

	LPDWORD id = 0;
	lc_thread.win_handle = CreateThread
	(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)LC_World_ThreadProcess,
		0,
		0,
		&id
	);

	if (!lc_thread.win_handle)
	{
		return false;
	}
	lc_thread.mutex_handle = CreateMutex(NULL, false, "LC_MUTEX");

	if (!lc_thread.mutex_handle)
	{
		return false;
	}

	lc_world.creative_mode_on = true;

}

void LC_World_Exit()
{
	lc_thread.force_exit = true;

	WaitForSingleObject(lc_thread.win_handle, INFINITY);
	
	if (lc_thread.win_handle)
	{
		CloseHandle(lc_thread.win_handle);
	}
	if (lc_thread.mutex_handle)
	{
		CloseHandle(lc_thread.mutex_handle);
	}
	
	PhysicsWorld_Destruct(lc_world.phys_world);

	//destruct the chunk hash map
	CHMap_Destruct(&lc_world.chunk_map);

	//destruct light hash map
	CHMap_Destruct(&lc_world.light_block_map);

	dA_Destruct(lc_world.draw_cmd_backbuffer);

	//destruct the GL Buffers
	DRB_Destruct(&lc_world.render_data.opaque_buffer);
	DRB_Destruct(&lc_world.render_data.semi_transparent_buffer);
	DRB_Destruct(&lc_world.render_data.water_buffer);
	RSB_Destruct(&lc_world.render_data.draw_cmds_buffer);
	RSB_Destruct(&lc_world.render_data.chunk_data_buffer);

	BVH_Tree_Destruct(&lc_world.render_data.bvh_tree);
}



void LC_World_StartFrame()
{
	//lc_world.require_draw_cmd_update = true;
	//update scene enviroment, sun, sky color, etc..
	LC_World_UpdateWorldEnviroment();
	
	//create chunks nearby player
	if (lc_cvars.lc_static_world->int_value == 0)
	{
		LC_World_CreateNearbyChunks();

		//process finished tasks
		LC_World_ProcessTaskQueue();
	}

	//remove far away chunks and update the draw cmd backbuffer
	LC_World_IterateChunks();

	
	lc_world.time += Core_getDeltaTime();

	lc_prev_mined_block.cooldown_timer += Core_getDeltaTime();

	if (lc_prev_mined_block.cooldown_timer > 0.3)
	{
		lc_prev_mined_block.hp = 7;
	}
}
void LC_World_EndFrame()
{
	DRB_WriteDataToGpu(&lc_world.render_data.opaque_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.semi_transparent_buffer);
	DRB_WriteDataToGpu(&lc_world.render_data.water_buffer);

	LC_World_UpdateDrawCmds();

	lc_world.player_action_this_frame = false;
}

LC_WorldRenderData* LC_World_getRenderData()
{
	return &lc_world.render_data;
}
PhysicsWorld* LC_World_GetPhysWorld()
{
	return lc_world.phys_world;
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
		LC_getNormalizedChunkPosition(p_x, p_y, p_z, norm_position);
		int x = norm_position[0] - above_chunk->global_position[0];
		int z = norm_position[2] - above_chunk->global_position[2];

		if (above_chunk->water_blocks > 0 && LC_IsBlockWater(LC_Chunk_getType(above_chunk, x, 0, z)))
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

size_t LC_World_GetDrawCmdAmount()
{
	return lc_world.render_data.draw_cmds_buffer.used_size;
}
int LC_World_getPrevMinedBlockHP()
{
	return lc_prev_mined_block.hp;
}

bool LC_World_IsCreativeModeOn()
{
	return lc_world.creative_mode_on;
}
