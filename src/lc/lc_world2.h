#ifndef LC_WORLD_H
#define LC_WORLD_H
#pragma once

#include "lc_chunk.h"
#include "lc_common.h"

#include "utility/Custom_Hashmap.h"
#include "utility/dynamic_array.h"
#include "utility/u_utility.h"
#include "utility/BVH_Tree.h"
#include "physics/physics_world.h"
#include "render/r_texture.h"

typedef struct
{
	DynamicRenderBuffer opaque_buffer;
	DynamicRenderBuffer semi_transparent_buffer;
	DynamicRenderBuffer water_buffer;

	RenderStorageBuffer draw_cmds_buffer;
	RenderStorageBuffer chunk_data_buffer;

	BVH_Tree bvh_tree;

	unsigned vao;
	unsigned water_vao;
	unsigned visibles_buffer;
	unsigned visibles_sorted_buffer;
	unsigned atomic_counters[3];
	unsigned block_data_buffer;
	unsigned draw_cmds_sorted_buffer;

	R_Texture* texture_atlas;
	R_Texture* texture_atlas_normals;
	R_Texture* texture_atlas_mer;
	R_Texture* texture_atlas_height;

	R_Texture* water_displacement_texture;
	R_Texture* gradient_map;

	float water_height;
} LC_WorldRenderData;

typedef struct
{
	uint64_t seed;
	CHMap chunk_map;
	CHMap light_block_map;

	size_t num_alive_chunks;

	LC_WorldRenderData render_data;

	dynamic_array* draw_cmd_backbuffer;

	bool require_draw_cmd_update;
	float time;

	PhysicsWorld* phys_world;

	bool creative_mode_on;
} LC_World;

typedef struct
{
	int chunk_data_index;
	int opaque_index;
	int transparent_index;
	int water_index;
} LC_TreeData;

typedef struct
{
	vec4 min_point;
	unsigned vis_flags;
}  LC_ChunkData;

typedef struct
{
	//OPAQUES
	unsigned o_count;
	unsigned o_first;

	//TRANSPARENTS
	unsigned t_count;
	unsigned t_first;

	//WATER
	unsigned w_count;
	unsigned w_first;
} LC_CombinedChunkDrawCmdData;

LC_Chunk* LC_World_GetChunk(float p_x, float p_y, float p_z);
LC_Block* LC_World_GetBlock(float p_x, float p_y, float p_z, ivec3 r_relativePos, LC_Chunk** r_chunk);
LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, int max_steps, ivec3 r_pos, ivec3 r_face, LC_Chunk** r_chunk);
bool LC_World_addBlock(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, LC_BlockType block_type);
bool LC_World_mineBlock(int p_gX, int p_gY, int p_gZ);

bool LC_World_ChunkExists(float p_x, float p_y, float p_z);
void LC_World_UpdateChunk(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* vertices_result);
void LC_World_UpdateChunkIndexes(LC_Chunk* const p_chunk);
bool LC_World_UpdateChunkVertices(LC_Chunk* const p_chunk, GeneratedChunkVerticesResult* p_vertices_result);
void LC_World_DeleteChunk(LC_Chunk* const p_chunk);

void LC_World_Create(int x_chunks, int y_chunks, int z_chunks);
void LC_World_Exit();

void LC_World_StartFrame();
void LC_World_EndFrame();

LC_WorldRenderData* LC_World_getRenderData();
PhysicsWorld* LC_World_GetPhysWorld();

int LC_World_calcWaterLevelFromPoint(float p_x, float p_y, float p_z);

size_t LC_World_GetDrawCmdAmount();
int LC_World_getPrevMinedBlockHP();
bool LC_World_IsCreativeModeOn();

#endif