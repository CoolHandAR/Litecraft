#pragma once

#include "lc_chunk.h"
#include "physics/p_physics_world.h"
#include "lc_defs.h"
#include "utility/forward_list.h"
#include "physics/p_physics_defs.h"
#include "utility/dynamic_array.h"
#include "utility/u_object_pool.h"
#include "lc_block_defs.h"

#define LC_WORLD_INIT_WIDTH 1
#define LC_WORLD_INIT_HEIGHT 1
#define LC_WORLD_INIT_LENGTH 1
#define LC_WORLD_INIT_TOTAL_SIZE LC_WORLD_INIT_WIDTH * LC_WORLD_INIT_HEIGHT * LC_WORLD_INIT_LENGTH

#define LC_WORLD_MAX_WIDTH 64
#define LC_WORLD_MAX_HEIGHT 64
#define LC_WORLD_MAX_LENGTH 64

#define LC_WORLD_GRAVITY 3.1f

#define LC_WORLD_MIDNIGHT_THRESHOLD 1000
#define LC_WORLD_NOON_THRESHOLD 100
#define LC_WORLD_MORNING_THRESHOLD 200
#define LC_WORLD_AFTERNOON_THRESHOLD 300
#define LC_WORLD_EVENING_THRESHOLD 400
#define LC_WORLD_DAWN_THRESHOLD 500
#define LC_WORLD_DUSK_THRESHOLD 800

#define LC_WORLD_MAX_CLOUDS 124
#define LC_WORLD_CLOUD_WIDTH 24
#define LC_WORLD_CLOUD_HEIGHT 1
#define LC_WORLD_CLOUD_LENGTH 24

typedef struct LC_Sun
{
	vec3 position;
	vec3 sun_size;
	vec3 direction_to_center;

	vec4 sun_color;

	float ambient_intensity;
	float diffuse_intensity;


} LC_Sun;

typedef enum LC_TimeOfDay
{
	LC_ToD__MIDNIGHT,
	LC_ToD__NOON,
	LC_ToD__MORNING,
	LC_ToD__AFTERNOON,
	LC_ToD__EVENING,
	LC_ToD__DAWN,
	LC_ToD__DUSK

} LC_TimeOfDay;

typedef enum LC_GameType
{
	LC_GT__SURVIVAL,
	LC_GT__CREATIVE
} LC_GameType;

typedef struct LC_World
{
	size_t vertex_count_index;

	unsigned vao, vbo, copy_vbo;

	unsigned chunks_info_ssbo;

	unsigned blocks_info_ubo;

	vec3 clouds[LC_WORLD_MAX_CLOUDS];

	LC_TimeOfDay time_of_day;

	uint64_t game_time;

	LC_Sun sun;

	FL_Head* entities;

	P_PhysWorld* phys_world;

	Object_Pool* chunks;

	int chunk_indexes[LC_WORLD_MAX_WIDTH][LC_WORLD_MAX_HEIGHT][LC_WORLD_MAX_LENGTH];

	ivec3 chunks_size;

	time_t gen_seed; //Seed for world generation

	bool dirty_bit;

	LC_GameType game_type; //The current gametype ex: Survival, Creative
} LC_World;

LC_World* LC_World_Generate();
void LC_World_Destruct(LC_World* p_world);
LC_Entity* LC_World_AssignEntity();
void LC_World_DeleteEntityByID(int p_id);
void LC_World_DeleteEntity(LC_Entity* p_ent);

void LC_World_deleteBlock(int p_gX, int p_gY, int p_gZ);
void LC_World_addBlock(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, LC_BlockType p_bType);
void LC_World_mineBlock(int p_gX, int p_gY, int p_gZ);

LC_Chunk* LC_World_getChunk(float x, float y, float z);
LC_Chunk* LC_World_getNearestChunk(float g_x, float g_y, float g_z);
LC_Chunk* LC_World_getNearestChunk2(float g_x, float g_y, float g_z);
LC_Block* LC_World_getBlock(float g_x, float g_y, float g_z, ivec3 r_relativePos, LC_Chunk** r_bChunk);
LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, ivec3 r_pos, ivec3 r_face);

LC_Chunk* LC_World_CreateChunk(int p_gX, int p_gY, int p_gZ);
void LC_World_DeleteChunk(int x, int y, int z);

int LC_World_getChunkIndex(LC_Chunk* const p_chunk);

void LC_World_UpdateChunk(LC_Chunk* const p_chunk);

void LC_World_unloadFarAwayChunks(vec3 player_pos, float max_distance);
void LC_World_unloadFarAwayChunk(LC_Chunk* chunk, ivec3 player_pos, float max_distance);
void LC_World_Update(float delta);