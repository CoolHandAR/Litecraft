#pragma once

//#include "lc/lc_block_defs.h"
#include "lc/lc_chunk.h"
#include "utility/Custom_Hashmap.h"
#include "utility/u_utility.h"
#include "render/r_shader.h"
#include "utility/forward_list.h"
#include "physics/p_physics_world.h"
#include "physics/p_aabb_tree.h"

#define LC_WORLD_WATER_HEIGHT 12

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

typedef struct
{
	vec3 position;
	vec3 direction_to_world_center;
	vec3 color;
	float speed;
} LC_Sun;

typedef struct
{
	//OPAQUES
	unsigned o_count;
	unsigned o_first;

	//TRANSPARENTS
	unsigned t_count;
	unsigned t_first;
} CombinedChunkDrawCmdData;



typedef struct
{
	vec4 min_point;
	unsigned vis_flags;
}  ChunkData;

typedef struct
{
	int chunk_data_index;
	int opaque_index;
	int transparent_index;
} LCTreeData;

typedef struct
{
	unsigned opaque_update_offset;
	int opaque_update_move_by;

	unsigned transparent_update_offset;
	int transparent_update_move_by;

	int skip_opaque_owner;
	int skip_transparent_owner;

	unsigned chunk_amount;

} LC_WorldUniformBuffer;

typedef struct
{
	DynamicRenderBuffer vertex_buffer;
	DynamicRenderBuffer transparents_vertex_buffer;
	RenderStorageBuffer draw_cmds_buffer;
	RenderStorageBuffer chunk_data;
	RenderStorageBuffer sorted_draw_cmds_buffer;
	unsigned draw_cmds_counters_buffers[2];
	unsigned block_data_buffer;
	unsigned visibles_ssbo;
	unsigned visibles_sorted_ssbo;
	unsigned shadow_chunk_indexes_ssbo;
	unsigned vao;
	R_Shader process_shader;
	R_Shader rendering_shader;
	R_Shader transparents_forward_pass_shader;
	R_Shader water_forward_pass_shader;
	
	int chunks_in_frustrum_count;

	LC_WorldUniformBuffer ubo_data;
	unsigned ubo_id;

	R_Texture* texture_atlas;
	R_Texture* texture_atlas_normals;
	R_Texture* texture_atlas_mer;

	AABB_Tree aabb_tree;
} WorldRenderData;

typedef struct
{
	P_PhysWorld* phys_world;
	FL_Head* entities;
	WorldRenderData render_data;
	CHMap chunk_map;
	CHMap light_hash_map;
	LC_Sun sun;

	size_t chunk_count;
	size_t chunks_vertex_loaded;
	ivec3 chunks_bounds_normalized[2];

	size_t total_num_blocks;
} LC_World;


void LC_World_Create(int p_initWidth, int p_initHeight, int p_initLength);
void LC_World_Destruct();
LC_Chunk* LC_World_GetChunk(float p_x, float p_y, float p_z);
LC_Chunk* LC_World_GetChunkByIndex(size_t p_index);
LC_Chunk* LC_World_GetChunkByNormalizedPosition(ivec3 pos);
LC_Block* LC_World_GetBlock(float p_x, float p_y, float p_z, ivec3 r_relativePos, LC_Chunk** r_chunk);
LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, int p_maxSteps, ivec3 r_pos, ivec3 r_face, LC_Chunk** r_chunk);
bool LC_World_addBlock(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, int p_bType);
bool LC_World_mineBlock(int p_gX, int p_gY, int p_gZ);
bool LC_World_setBlock(int p_gX, int p_gY, int p_gZ, LC_BlockType p_bType);
WorldRenderData* LC_World_getRenderData();
size_t LC_World_GetChunkAmount();
size_t LC_World_GetDrawCmdAmount();
P_PhysWorld* LC_World_GetPhysWorld();
void LC_World_getSunDirection(vec3 v);
int LC_World_getPrevMinedBlockHP();