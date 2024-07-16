#pragma once

//#include "lc/lc_block_defs.h"
#include "lc2/lc_chunk.h"
#include "utility/Custom_Hashmap.h"
#include "utility/u_utility.h"
#include "render/r_shader.h"
#include "utility/forward_list.h"
#include "physics/p_physics_world.h"

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
	DynamicRenderBuffer vertex_buffer;
	DynamicRenderBuffer transparents_vertex_buffer;
	DynamicRenderBuffer water_vertex_buffer;
	RenderStorageBuffer opaque_draw_commands;
	RenderStorageBuffer transparent_draw_commands;
	RenderStorageBuffer water_draw_commands;
	RenderStorageBuffer chunk_data;
	unsigned vao;
	R_Shader process_shader;
	R_Shader rendering_shader;
	R_Shader transparents_forward_pass_shader;
} WorldRenderData;

typedef struct
{
	P_PhysWorld* phys_world;
	FL_Head* entities;
	WorldRenderData render_data;
	CHMap chunk_map;
	CHMap light_hash_map;
} LC_World;


void LC_World_Create(int p_initWidth, int p_initHeight, int p_initLength);
void LC_World_Destruct2();
void LC_World_Update2();
LC_Chunk* LC_World_GetChunk(float p_x, float p_y, float p_z);
LC_Block* LC_World_GetBlock(float p_x, float p_y, float p_z, ivec3 r_relativePos, LC_Chunk** r_chunk);
LC_Block* LC_World_getBlockByRay(vec3 from, vec3 dir, ivec3 r_pos, ivec3 r_face, LC_Chunk** r_chunk);
bool LC_World_addBlock2(int p_gX, int p_gY, int p_gZ, ivec3 p_addFace, int p_bType);
bool LC_World_mineBlock2(int p_gX, int p_gY, int p_gZ);
WorldRenderData* LC_World_getRenderData();
size_t LC_World_GetChunkAmount();
P_PhysWorld* LC_World_GetPhysWorld();