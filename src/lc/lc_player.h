#pragma once

#include "physics/p_physics_defs.h";
#include "lc_defs.h"
#include "lc_world.h"
#include <GLFW/glfw3.h>
#include "render/r_camera.h"


#define PLAYER_HOTBAR_SLOTS 9
#define PLAYER_INVENTORY_SLOTS 27

typedef struct LC_Inventory
{
	LC_BlockType storage_slots[PLAYER_INVENTORY_SLOTS];

} LC_Inventory;

typedef struct LC_Player
{
	LC_Entity* handle;
	Kinematic_Body* k_body;
	bool free_fly;
	bool inventory_opened;
	bool mouse_click[2];
	
	float attack_cooldown_timer;

	LC_BlockType held_block_type;
	int hotbar_index;

	LC_BlockType hotbar_slots[PLAYER_HOTBAR_SLOTS];

} LC_Player;

void LC_Player_Create(LC_World* world, vec3 pos);
void LC_Player_Update(R_Camera* const cam, float delta);
void LC_Player_ProcessInput(GLFWwindow* const window, R_Camera* const cam);
void LC_Player_getPos(vec3 dest);