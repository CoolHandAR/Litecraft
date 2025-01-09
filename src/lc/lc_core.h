#ifndef LC_CORE_H
#define LC_CORE_H
#pragma once

#include "render/r_texture.h"
#include "core/sound.h"
#include "lc/lc_chunk.h"
#include "render/r_public.h"
#include "lc/lc_world2.h"
#include "lc_common.h"


/*
* ~~~~~~~~~~~~~~~~~~~~
	CORE
* ~~~~~~~~~~~~~~~~~~~
*/
void LC_StartFrame();
void LC_EndFrame();
void LC_Draw();
void LC_PhysUpdate(float delta);
void LC_Init();
void LC_Exit();

typedef struct
{
	R_Camera cam;
} LC_CoreData;

/*
* ~~~~~~~~~~~~~~~~~~~~
	PLAYER STUFF
* ~~~~~~~~~~~~~~~~~~~
*/
#define LC_PLAYER_MAX_HOTBAR_SLOTS 9
typedef struct
{
	LC_BlockType blocks[LC_PLAYER_MAX_HOTBAR_SLOTS];
	int active_index;
} LC_Hotbar;

void PL_Update();
void LC_Player_getPosition(vec3 dest);

/*
* ~~~~~~~~~~~~~~~~~~~~~~~~
	PUBLIC DRAW FUNCTIONS
* ~~~~~~~~~~~~~~~~~~~~~~~~
*/
void LC_Draw_DrawShowPos(vec3 pos, vec3 vel, float yaw, float pitch, uint8_t held_block, uint8_t selected_block, int corner);
void LC_Draw_ChunkInfo(LC_Chunk* const chunk, int corner);
void LC_Draw_Hotbar(LC_Hotbar* const hotbar, int block_amounts[LC_BT__MAX]);
void LC_Draw_Inventory(LC_BlockType blocks[21][6], int block_amounts[LC_BT__MAX], LC_Hotbar* const hotbar);
void LC_Draw_Crosshair();
void LC_Draw_WorldInfo(LC_World* const world, int corner);
void LC_Draw_WaterOverlayScreenTexture(int water_level);
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

#endif