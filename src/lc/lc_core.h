#pragma once


#include "utility/dynamic_array.h"
#include "render/r_camera.h"
#include "core/cvar.h"
#include "lc/lc_chunk.h"
#include "lc_block_defs.h"
#include "render/r_texture.h"
#include "sound/sound.h"
#include "render/r_public.h"
#include "lc/lc_world.h"

typedef struct GLFWwindow GLFWwindow;




/*
* ~~~~~~~~~~~~~~~~~~~~
	CVARS
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	Cvar* lc_timeofday;
} LC_Cvars;

/*
* ~~~~~~~~~~~~~~~~~~~~
	CORE
* ~~~~~~~~~~~~~~~~~~~
*/
void LC_Init();
void LC_Draw();
void LC_Loop(float delta);
void LC_PhysLoop(float delta);
void LC_Cleanup();

/*
* ~~~~~~~~~~~~~~~~~~~~
	PLAYER, INVENTORY STUFF
* ~~~~~~~~~~~~~~~~~~~
*/
#define LC_PLAYER_MAX_HOTBAR_SLOTS 9
typedef struct
{
	int x;
} LC_Inventory;

typedef struct
{
	LC_BlockType blocks[LC_PLAYER_MAX_HOTBAR_SLOTS];
	int active_index;
} LC_Hotbar;

/*
* ~~~~~~~~~~~~~~~~~~~~~~~~
	PUBLIC DRAW FUNCTIONS
* ~~~~~~~~~~~~~~~~~~~~~~~~
*/
void LC_Draw_DrawShowPos(vec3 pos, vec3 vel, float yaw, float pitch, uint8_t held_block, int corner);
void LC_Draw_ChunkInfo(LC_Chunk* const chunk, int corner);
void LC_Draw_Hotbar(LC_Hotbar* const hotbar);
void LC_Draw_Inventory(LC_BlockType blocks[21][6], LC_Hotbar* const hotbar);
void LC_Draw_Crosshair();
void LC_Draw_WorldInfo(LC_World* const world, int corner);
/*
* ~~~~~~~~~~~~~~~~~~~~
	RESOURCES
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	ma_sound* fall_sound;
	ma_sound* big_fall_sound;
	ma_sound* grass_step_sounds[6];
	ma_sound* sand_step_sounds[5];
	ma_sound* stone_step_sounds[6];
	ma_sound* wood_step_sounds[6];
	ma_sound* gravel_step_sounds[4];

	ma_sound* grass_dig_sounds[4];
	ma_sound* sand_dig_sounds[4];
	ma_sound* stone_dig_sounds[4];
	ma_sound* wood_dig_sounds[4];

} LC_ResourceList;

typedef struct
{
	ma_sound_group step;
} LC_SoundGroups;

typedef struct
{
	ParticleEmitterSettings* block_break;
	ParticleEmitterSettings* block_dig[5];
} LC_Emitters;