#include "lc/lc_core.h"

#include <GLFW/glfw3.h>

#include "core/cvar.h"
#include "core/input.h"
#include "core/core_common.h"
#include "physics/p_physics_defs.h"
#include "physics/physics_world.h"
#include "lc/lc_world2.h"
#include "render/r_camera.h"
#include "render/r_public.h"
#include "core/sound.h"
#include "core/resource_manager.h"

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MAIN PLAYER STUFF, MOVEMENT, ACTIONS ETC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

extern LC_ResourceList lc_resources;
extern LC_SoundGroups lc_sound_groups;
extern LC_Emitters lc_emitters;
extern ma_engine sound_engine;

#define PLAYER_AABB_WIDTH 1.1
#define PLAYER_AABB_HEIGHT 3
#define PLAYER_AABB_LENGTH 1.1

#define PLAYER_JUMP_HEIGHT 1

#define PLAYER_GROUND_ACCEL 1
#define PLAYER_AIR_ACCEL 1
#define PLAYER_FLYING_ACCEL 1

#define PLAYER_GROUND_FRICTION 1
#define PLAYER_AIR_FRICTION 10
#define PLAYER_FLYING_FRICTION 10

#define PLAYER_PLACE_COOLDOWN 0.5
#define PLAYER_MINE_COOLDOWN 0.15


typedef struct
{
	Cvar* pl_freefly;
	Cvar* pl_noclip;
	Cvar* pl_godmode;
	Cvar* pl_stop_speed;
	Cvar* pl_accel;
	Cvar* pl_air_accel;
	Cvar* pl_flying_accel;
	Cvar* pl_friction;
	Cvar* pl_air_friction;
	Cvar* pl_flying_friction;
	Cvar* pl_action_freq;
	Cvar* pl_jump_height;
	Cvar* pl_showpos;
	Cvar* pl_showchunk;
} PlayerCvars;

typedef struct
{
	LC_Chunk* chunk;
	LC_Block block;
	ivec3 face;
	ivec3 position;
} SelectedBlock;

typedef struct
{
	Kinematic_Body* k_body;
	float action_timer;
	LC_BlockType held_block_type;

	int step_sound_index;
	float step_sound_timer;

	int dig_sound_index;
	float dig_sound_timer;
	
	int emitter_index;

	float alive_timer;

	LightID flashlight_id;
	bool flashlight_active;
} Player;

typedef struct
{
	LC_BlockType inventory_blocks[21][6];
	LC_BlockType block_amounts[LC_BT__MAX];

	bool is_opened;
} Inventory;

static PlayerCvars pl_cvars;
static SelectedBlock selected_block;
static Player player;
static LC_Hotbar hotbar;
static Inventory inventory;

static bool PL_isPlayerInBlock(int p_gX, int p_gY, int p_gZ)
{
	AABB block_box;
	block_box.width = 1;
	block_box.height = 1;
	block_box.length = 1;
	block_box.position[0] = p_gX - 0.5;
	block_box.position[1] = p_gY - 0.5;
	block_box.position[2] = p_gZ - 0.5;

	return AABB_intersectsOther(&player.k_body->box, &block_box);
}

static void PL_spawnCorrect()
{
	vec3 box_extents;
	box_extents[0] = player.k_body->box.width / 2;
	box_extents[1] = player.k_body->box.height / 2;
	box_extents[2] = player.k_body->box.length / 2;

	for (int i = 0; i < 2; i++)
	{
		int minX = player.k_body->box.position[0];
		int minY = player.k_body->box.position[1];
		int minZ = player.k_body->box.position[2];

		int maxX = player.k_body->box.position[0] + box_extents[0];
		int maxY = player.k_body->box.position[1] + box_extents[1];
		int maxZ = player.k_body->box.position[2] + box_extents[2];

		for (int x = minX; x <= maxX; x++)
		{
			for (int y = minY; y <= maxY; y++)
			{
				for (int z = minZ; z <= maxZ; z++)
				{
					LC_Block* block = LC_World_GetBlock(x, y, z, NULL, NULL);

					if (block)
					{
						if (PL_isPlayerInBlock(x, y, z))
						{
							player.k_body->box.position[1] += 0.01;

							minY = player.k_body->box.position[1];
							maxY = player.k_body->box.position[1] + box_extents[1];
						}
						else
						{
							return;
						}
					}
					
				}
			}
		}
	}
	
}
static void PL_initInventoryBlocks()
{
	memset(&inventory, 0, sizeof(inventory));

	int counter = 0;
	for (int x = 0; x < 20; x++)
	{
		for (int y = 0; y < 6; y++)
		{
			counter++;

			if (counter >= LC_BT__MAX)
			{
				break;
			}
			
			inventory.inventory_blocks[x][y] = LC_BT__NONE + counter;
		}
	}

	if (LC_World_IsCreativeModeOn())
	{
		for (int i = 0; i < LC_BT__MAX; i++)
		{
			inventory.block_amounts[i] = 1000;
		}
	}
}
static void PL_initHotbar()
{
	hotbar.blocks[0] = LC_BT__GRASS;
	hotbar.blocks[1] = LC_BT__GLASS;
	hotbar.blocks[2] = LC_BT__TREELEAVES;
	hotbar.blocks[3] = LC_BT__GLOWSTONE;
	hotbar.blocks[4] = LC_BT__TRUNK;
	hotbar.blocks[5] = LC_BT__IRON;
	hotbar.blocks[6] = LC_BT__MAGMA;
	hotbar.blocks[7] = LC_BT__CACTUS;
	hotbar.blocks[8] = LC_BT__OBSIDIAN;
}

static void PL_initCvars()
{
	pl_cvars.pl_freefly = Cvar_Register("pl_freefly", "1", NULL, 0, 0, 1);
	pl_cvars.pl_noclip = Cvar_Register("pl_noclip", "0", "Disables collision", 0, 0, 1);
	pl_cvars.pl_godmode = Cvar_Register("pl_godmode", "1", "Disables all damage", NULL, 0, 1);
	pl_cvars.pl_stop_speed = Cvar_Register("pl_stop_speed", "1", "Player movement speed", 0, 0, 10000);
	pl_cvars.pl_accel = Cvar_Register("pl_accel", "1", "Player ground acceleration", 0, 0, 100);
	pl_cvars.pl_air_accel = Cvar_Register("pl_air_accel", "1", "Player air acceleration", 0, 0, 100);
	pl_cvars.pl_flying_accel = Cvar_Register("pl_flying_accel", "1", "Player flying acceleration", 0, 0, 100);
	pl_cvars.pl_friction = Cvar_Register("pl_friction", "1", "Player ground friction", 0, 0, 100);
	pl_cvars.pl_air_friction = Cvar_Register("pl_air_friction", "1", "Player air friction", 0, 0, 100);
	pl_cvars.pl_flying_friction = Cvar_Register("pl_flying_friction", "1", "Player flying friction", 0, 0, 100);
	pl_cvars.pl_action_freq = Cvar_Register("pl_action_freq", "1", NULL, 0, 0, 100);
	pl_cvars.pl_jump_height = Cvar_Register("pl_jump_height", "100", NULL, 0, 0, 100);
	pl_cvars.pl_showpos = Cvar_Register("pl_showpos", "1", NULL, 0, 0, 1);
	pl_cvars.pl_showchunk = Cvar_Register("pl_showchunk", "1", NULL, 0, 0, 1);
}

static void PL_initInputs()
{
	Input_registerAction("Jump");
	Input_registerAction("Forward");
	Input_registerAction("Backward");
	Input_registerAction("Left");
	Input_registerAction("Right");
	Input_registerAction("Place");
	Input_registerAction("Remove");
	Input_registerAction("Duck");
	Input_registerAction("ToggleFreefly");
	Input_registerAction("NextBlock");
	Input_registerAction("PrevBlock");
	Input_registerAction("Inventory");
	Input_registerAction("Flashlight");
	Input_registerAction("Block0");
	Input_registerAction("Block1");
	Input_registerAction("Block2");
	Input_registerAction("Block3");
	Input_registerAction("Block4");
	Input_registerAction("Block5");
	Input_registerAction("Block6");
	Input_registerAction("Block7");
	Input_registerAction("Block8");

	Input_setActionBinding("Jump", IT__KEYBOARD, GLFW_KEY_SPACE, 0);
	Input_setActionBinding("Forward", IT__KEYBOARD, GLFW_KEY_W, 0);
	Input_setActionBinding("Backward", IT__KEYBOARD, GLFW_KEY_S, 0);
	Input_setActionBinding("Left", IT__KEYBOARD, GLFW_KEY_A, 0);
	Input_setActionBinding("Right", IT__KEYBOARD, GLFW_KEY_D, 0);
	Input_setActionBinding("Place", IT__MOUSE, GLFW_MOUSE_BUTTON_LEFT, 0);
	Input_setActionBinding("Remove", IT__MOUSE, GLFW_MOUSE_BUTTON_RIGHT, 0);
	Input_setActionBinding("Duck", IT__KEYBOARD, GLFW_KEY_LEFT_SHIFT, 0);
	Input_setActionBinding("ToggleFreefly", IT__KEYBOARD, GLFW_KEY_F, 0);
	Input_setActionBinding("NextBlock", IT__KEYBOARD, GLFW_KEY_E, 0);
	Input_setActionBinding("PrevBlock", IT__KEYBOARD, GLFW_KEY_Q, 0);
	Input_setActionBinding("Inventory", IT__KEYBOARD, GLFW_KEY_I, 0);
	Input_setActionBinding("Flashlight", IT__KEYBOARD, GLFW_KEY_U, 0);

	Input_setActionBinding("Block0", IT__KEYBOARD, GLFW_KEY_1, 0);
	Input_setActionBinding("Block1", IT__KEYBOARD, GLFW_KEY_2, 0);
	Input_setActionBinding("Block2", IT__KEYBOARD, GLFW_KEY_3, 0);
	Input_setActionBinding("Block3", IT__KEYBOARD, GLFW_KEY_4, 0);
	Input_setActionBinding("Block4", IT__KEYBOARD, GLFW_KEY_5, 0);
	Input_setActionBinding("Block5", IT__KEYBOARD, GLFW_KEY_6, 0);
	Input_setActionBinding("Block6", IT__KEYBOARD, GLFW_KEY_7, 0);
	Input_setActionBinding("Block7", IT__KEYBOARD, GLFW_KEY_8, 0);
	Input_setActionBinding("Block8", IT__KEYBOARD, GLFW_KEY_9, 0);

	Input_registerAction("Fullscreen");
	Input_setActionBinding("Fullscreen", IT__KEYBOARD, GLFW_KEY_F6, 0);
}

void PL_initPlayer(vec3 p_spawnPos)
{
	srand(time(NULL));

	memset(&player, 0, sizeof(Player));
	memset(&selected_block, 0, sizeof(selected_block));
	memset(&hotbar, 0, sizeof(hotbar));

	ma_engine_listener_set_world_up(&sound_engine, 0, 0, 1, 0);
	ma_engine_listener_set_enabled(&sound_engine, 0, MA_TRUE);

	ma_engine_set_volume(&sound_engine, 0.7);

	PL_initCvars();
	PL_initInputs();
	PL_initHotbar();
	PL_initInventoryBlocks();

	AABB aabb;
	memset(&aabb, 0, sizeof(AABB));

	if (p_spawnPos != NULL)
	{
		glm_vec3_copy(p_spawnPos, aabb.position);
	}
	else
	{
		glm_vec3_zero(aabb.position);
	}
	aabb.width = PLAYER_AABB_WIDTH;
	aabb.height = PLAYER_AABB_HEIGHT;
	aabb.length = PLAYER_AABB_LENGTH;

	//register the physics body
	player.k_body = PhysicsWorld_AddKinematicBody(LC_World_GetPhysWorld(), &aabb, NULL);
	//setup default body config
	player.k_body->config.ducking_scale = 0.2f;
	player.k_body->config.ground_accel = 5;
	player.k_body->config.air_accel = 0.1;
	//player.k_body->config.ground_accel = 244;
	player.k_body->config.water_accel = 1.1;
	player.k_body->config.jump_height = 0.25;
	player.k_body->force_update_on_frame = true;
	player.k_body->flags |= PF__Collidable;
	player.k_body->config.air_friction = 1.2;
	player.k_body->config.ground_friction = 200.0;
	player.k_body->config.flying_friction = 1;
	player.k_body->config.water_friction = 400;
	player.k_body->config.stop_speed = 100;
	player.k_body->config.flying_speed = 50;
	
	player.held_block_type = LC_BT__GRASS;

	SpotLight light;
	memset(&light, 0, sizeof(light));

	light.color[0] = 1;
	light.color[1] = 1;
	light.color[2] = 1;
	light.color[3] = 1;

	light.ambient_intensity = 23;
	light.specular_intensity = 2;
	light.range = 12;
	light.angle_attenuation = 1;
	light.angle = 45;
	light.attenuation = 1;
	player.flashlight_id = RScene_RegisterSpotLight(light, false);
}	

static ma_sound* PL_getStepSound()
{
	float player_vel = glm_vec3_norm2(player.k_body->velocity) * 5;

	switch (player.k_body->ground_contact.block_type)
	{
	case LC_BT__NONE:
	{
		return NULL;
	}
	case LC_BT__CACTUS:
	case LC_BT__SNOWYLEAVES:
	case LC_BT__TREELEAVES:
	case LC_BT__FLOWER:
	case LC_BT__GRASS_PROP:
	case LC_BT__GRASS:
	{
		if (player.step_sound_timer > 0)
		{
			return NULL;
		}
		player.step_sound_timer = 0.4;
		player.step_sound_index = (player.step_sound_index + 1) % 6;

		return lc_resources.grass_step_sounds[player.step_sound_index];
	}
	case LC_BT__SAND:
	{
		if (player.step_sound_timer > 0)
		{
			return NULL;
		}
		player.step_sound_timer = 0.4;
		player.step_sound_index = (player.step_sound_index + 1) % 4;

		return lc_resources.sand_step_sounds[player.step_sound_index];
	}
	case LC_BT__DIRT:
	{
		if (player.step_sound_timer > 0)
		{
			return NULL;
		}
		player.step_sound_timer = 0.4;
		player.step_sound_index = (player.step_sound_index + 1) % 4;

		return lc_resources.gravel_step_sounds[player.step_sound_index];
	}
	case LC_BT__SNOW:
	case LC_BT__OBSIDIAN:
	case LC_BT__IRON:
	case LC_BT__DIAMOND:
	case LC_BT__MAGMA:
	case LC_BT__GLOWSTONE:
	case LC_BT__STONE:
	{
		if (player.step_sound_timer > 0)
		{
			return NULL;
		}
		float t = 0.4;


		player.step_sound_timer = t;
		player.step_sound_index = (player.step_sound_index + 1) % 5;

		return lc_resources.stone_step_sounds[player.step_sound_index];
	}
	case LC_BT__TRUNK:
	{
		if (player.step_sound_timer > 0)
		{
			return NULL;
		}
		player.step_sound_timer = 0.4;
		player.step_sound_index = (player.step_sound_index + 1) % 6;

		return lc_resources.wood_step_sounds[player.step_sound_index];
	}
	default:
		break;
	}
	return NULL;
}

static ma_sound* PL_getDigSound(LC_BlockType block_type, bool place_destroy)
{
	//We use step sounds when the block is being broken

	switch (block_type)
	{
	case LC_BT__CACTUS:
	case LC_BT__SNOWYLEAVES:
	case LC_BT__TREELEAVES:
	case LC_BT__FLOWER:
	case LC_BT__GRASS_PROP:
	case LC_BT__GRASS:
	{
		if (place_destroy)
		{
			player.dig_sound_index = (player.dig_sound_index + 1) % 4;

			return lc_resources.grass_dig_sounds[player.dig_sound_index];
		}

		player.dig_sound_index = (player.dig_sound_index + 1) % 6;

		return lc_resources.grass_step_sounds[player.dig_sound_index];
	}
	case LC_BT__DIRT:
	case LC_BT__SAND:
	{
		if (place_destroy)
		{
			player.dig_sound_index = (player.dig_sound_index + 1) % 4;

			return lc_resources.sand_dig_sounds[player.dig_sound_index];
		}

		player.dig_sound_index = (player.dig_sound_index + 1) % 5;

		return lc_resources.sand_step_sounds[player.dig_sound_index];
	}
	case LC_BT__SNOW:
	case LC_BT__OBSIDIAN:
	case LC_BT__IRON:
	case LC_BT__DIAMOND:
	case LC_BT__MAGMA:
	case LC_BT__GLOWSTONE:
	case LC_BT__STONE:
	{
		if (place_destroy)
		{
			player.dig_sound_index = (player.dig_sound_index + 1) % 4;

			return lc_resources.stone_dig_sounds[player.dig_sound_index];
		}

		player.dig_sound_index = (player.dig_sound_index + 1) % 6;

		return lc_resources.stone_step_sounds[player.dig_sound_index];
	}
	case LC_BT__SPRUCE_PLANKS:
	case LC_BT__TRUNK:
	{
		if (place_destroy)
		{
			player.dig_sound_index = (player.dig_sound_index + 1) % 4;

			return lc_resources.wood_dig_sounds[player.dig_sound_index];
		}

		player.dig_sound_index = (player.dig_sound_index + 1) % 6;

		return lc_resources.wood_step_sounds[player.dig_sound_index];
	}
	default:
	
		return lc_resources.grass_dig_sounds[0];
	}
	return NULL;
}

static void PL_fallCheck()
{
	if (pl_cvars.pl_freefly->int_value)
	{
		return;
	}
	
	static float fall_strength = 0;
	static bool prev_on_ground = true;

	if (player.k_body->on_ground)
	{
		if (!prev_on_ground)
		{
			ma_sound* fall_sound = NULL;
			if (fall_strength > 1.0)
			{
				fall_sound = Resource_get("assets/sounds/player/fall/fallbig.wav", RESOURCE__SOUND);
			}
			else if (fall_strength > 0.5)
			{
				fall_sound = Resource_get("assets/sounds/player/fall/fallsmall.wav", RESOURCE__SOUND);
			}
			if (fall_sound)
			{
				float sound_x = player.k_body->box.position[0];
				float sound_y = player.k_body->box.position[1];
				float sound_z = player.k_body->box.position[2];

				ma_sound_set_position(fall_sound, sound_x, sound_y, sound_z);
				ma_sound_start(fall_sound);

				ma_sound* step_sound = PL_getStepSound();

				if (step_sound)
				{
					ma_sound_set_position(step_sound, sound_x, sound_y, sound_z);
					ma_sound_start(step_sound);
				}
			}
		}
		prev_on_ground = true;
		fall_strength = 0;
	}
	else
	{
		prev_on_ground = false;
		fall_strength += Core_getDeltaTime();
	}
}

static void PL_updateModifiedCvars()
{
	if (pl_cvars.pl_freefly->modified)
	{
		if (pl_cvars.pl_freefly->int_value)
		{
			player.k_body->flags &= ~PF__AffectedByGravity;
		}
		else
		{
			player.k_body->flags |= PF__AffectedByGravity;
		}
		pl_cvars.pl_freefly->modified = false;
	}
	if (pl_cvars.pl_noclip->modified)
	{
		if (pl_cvars.pl_noclip->int_value)
		{
			player.k_body->flags &= ~PF__Collidable;
		}
		else
		{
			player.k_body->flags |= PF__Collidable;
		}
		pl_cvars.pl_noclip->modified = false;
	}
	if (pl_cvars.pl_stop_speed->modified)
	{
		player.k_body->config.stop_speed = pl_cvars.pl_stop_speed->float_value;
		pl_cvars.pl_stop_speed->modified = false;
	}
}

static void PL_UpdateSelectedBlock()
{
	R_Camera* cam = Camera_getCurrent();

	if (!cam)
	{
		return;
	}

	vec3 from_pos;

	from_pos[0] = cam->data.position[0] + 0.5;
	from_pos[1] = cam->data.position[1] + 0.5;
	from_pos[2] = cam->data.position[2] + 0.5;
	LC_Block* block = LC_World_getBlockByRay(from_pos, cam->data.camera_front, 8, selected_block.position, selected_block.face, &selected_block.chunk);

	if (block)
	{
		selected_block.block = *block;
	}
	else
	{
		selected_block.block.type = LC_BT__NONE;
	}
}

static void PL_placeBlock()
{
	//no block?
	if (selected_block.block.type == LC_BT__NONE)
	{
		return;
	}

	//is the cooldown still active?
	if (player.action_timer > 0)
	{
		return;
	}
	
	//do we have any amount of the block in inventory?
	if (!LC_World_IsCreativeModeOn())
	{
		if (inventory.block_amounts[player.held_block_type] <= 0)
		{
			return;
		}
	}

	//add the block
	//we failed to add the block(for whatever reason)
	if (!LC_World_addBlock(selected_block.position[0], selected_block.position[1], selected_block.position[2], selected_block.face, player.held_block_type))
	{
		return;
	}
	if (!LC_World_IsCreativeModeOn())
	{
		inventory.block_amounts[player.held_block_type]--;
	}
	
	//if we will possibly be stuck after placing the block, move the player by the face norma amount
	if (!LC_isBlockProp(selected_block.block.type) && !LC_isBlockProp(player.held_block_type))
	{
		if (PL_isPlayerInBlock(selected_block.position[0] + selected_block.face[0], selected_block.position[1] + selected_block.face[1], selected_block.position[2] + selected_block.face[2]))
		{
			player.k_body->box.position[0] += selected_block.face[0];
			player.k_body->box.position[1] += selected_block.face[1];
			player.k_body->box.position[2] += selected_block.face[2];
		}
	}

	//play sound
	ma_sound* place_sound = PL_getDigSound(player.held_block_type, true);

	if (place_sound)
	{
		float sound_x = selected_block.position[0];
		float sound_y = selected_block.position[1];
		float sound_z = selected_block.position[2];

		ma_sound_set_position(place_sound, sound_x, sound_y, sound_z);
		ma_sound_start(place_sound);
	}

	//Reset the timer
	player.action_timer = PLAYER_PLACE_COOLDOWN;
}

static void PL_mineBlock()
{
	//no block?
	if (selected_block.block.type == LC_BT__NONE)
	{
		return;
	}

	//is the cooldown still active?
	if (player.action_timer > 0)
	{
		return;
	}


	//did we fail to mine the block for whatever reason?
	if (!LC_World_mineBlock(selected_block.position[0], selected_block.position[1], selected_block.position[2]))
	{
		return;
	}

	//Play the dig sound
	bool destroy_or_dig = (LC_World_getPrevMinedBlockHP() == 1 || LC_isBlockProp(selected_block.block.type));

	if (destroy_or_dig)
	{
		//add to inventory
		inventory.block_amounts[selected_block.block.type]++;
	}

	ma_sound* dig_sound = PL_getDigSound(selected_block.block.type, destroy_or_dig);

	if (dig_sound)
	{
		float sound_x = selected_block.position[0];
		float sound_y = selected_block.position[1];
		float sound_z = selected_block.position[2];

		ma_sound_set_position(dig_sound, sound_x, sound_y, sound_z);
		ma_sound_start(dig_sound);
	}

	//Emit particles
	vec3 dir;
	dir[0] = selected_block.face[0];
	dir[1] = selected_block.face[1];
	dir[2] = selected_block.face[2];

	vec3 origin;
	origin[0] = selected_block.position[0];
	origin[1] = selected_block.position[1] + 0.25; //small offset
	origin[2] = selected_block.position[2];
	LC_Block_Texture_Offset_Data texture_data = LC_BLOCK_TEX_OFFSET_DATA[selected_block.block.type];
	int frame = (25 * texture_data.side_face[1]) + texture_data.side_face[0];

	//cycle through the emitters so, that we can have more alive particles
	player.emitter_index = (player.emitter_index + 1) % 4;

	lc_emitters.block_dig[player.emitter_index]->frame = frame;
	Particle_EmitTransformed(lc_emitters.block_dig[player.emitter_index], dir, origin);

	//Reset the timer
	player.action_timer = PLAYER_MINE_COOLDOWN;
}

static void PL_move(int p_dirX, int p_dirY, int p_dirZ)
{	
	R_Camera* cam = Camera_getCurrent();

	if (!cam)
	{
		return;
	}
	if (p_dirX == 0 && p_dirY == 0 && p_dirZ == 0)
	{
		return;
	}
	//STRAFING LEFT AND RIGHT
	if (p_dirX != 0)
	{
		int sign_dir = glm_sign(p_dirX);

		player.k_body->direction[0] += cam->data.camera_right[0] * sign_dir;
		player.k_body->direction[2] += cam->data.camera_right[2] * sign_dir;

		if (pl_cvars.pl_freefly->int_value == 1)
		{
			player.k_body->direction[1] += cam->data.camera_right[1] * sign_dir;
		}
	}
	//FORWARD AND BACKWARDS
	if (p_dirZ != 0)
	{
		float yaw_in_radians = glm_rad(cam->data.yaw);
		float cos_yaw = cos(yaw_in_radians);
		float sin_yaw = sin(yaw_in_radians);

		int sign_dir = glm_sign(p_dirZ);

		if (pl_cvars.pl_freefly->int_value == 1)
		{
			for (int i = 0; i < 3; i++)
			{
				player.k_body->direction[i] += cam->data.camera_front[i] * sign_dir;
			}
		}
		else
		{
			//fixes issues when looking down and moving
			player.k_body->direction[0] += cos_yaw * sign_dir;
			player.k_body->direction[2] += sin_yaw * sign_dir;
		}
	}
	//JUMPING AND DUCKING
	if (p_dirY != 0)
	{
		//Jump
		if (p_dirY > 0 && player.k_body->on_ground)
		{
			player.k_body->direction[1] = 1;
		}
		//Duck
		if (p_dirY < 0 && player.k_body->on_ground)
		{
			player.k_body->flags |= PF__Ducking;
		}
	}

	//PLAY APPROPRIATE SOUND
	if (player.k_body->on_ground && !(player.k_body->flags & PF__Ducking))
	{
		if (p_dirX != 0 || p_dirZ != 0)
		{
			player.step_sound_timer -= Core_getDeltaTime();

			ma_sound* step_sound = PL_getStepSound();
			
			if (step_sound)
			{
				float sound_x = player.k_body->box.position[0];
				float sound_y = player.k_body->box.position[1];
				float sound_z = player.k_body->box.position[2];

				ma_sound_set_position(step_sound, sound_x, sound_y, sound_z);
				ma_sound_start(step_sound);
			}
			
		}
	}
}

static void PL_updateListener()
{
	float listener_x = player.k_body->box.position[0];
	float listener_y = player.k_body->box.position[1] + (PLAYER_AABB_HEIGHT * 0.5);
	float listener_z = player.k_body->box.position[2];

	ma_engine_listener_set_position(&sound_engine, 0, player.k_body->box.position[0], player.k_body->box.position[1], player.k_body->box.position[2]);

	R_Camera* cam = Camera_getCurrent();

	if (!cam)
	{
		return;
	}

	float dir_listener_x = cam->data.camera_front[0];
	float dir_listener_y = cam->data.camera_front[1];
	float dir_listener_z = cam->data.camera_front[2];

	ma_engine_listener_set_direction(&sound_engine, 0, dir_listener_x, dir_listener_y, dir_listener_z);
}

static void PL_updateFlashlight()
{
	SpotLight* light = RScene_GetRenderInstanceData(player.flashlight_id);

	if (!player.flashlight_active)
	{
		light->ambient_intensity = 0;
		return;
	}
	R_Camera* cam = Camera_getCurrent();

	if (!cam)
	{
		return;
	}

	light->ambient_intensity = 23;

	static vec3 light_direction = { 0, 0, 0};

	light_direction[0] = glm_lerp(light_direction[0], cam->data.camera_front[0], 0.1);
	light_direction[1] = glm_lerp(light_direction[1], cam->data.camera_front[1], 0.1);
	light_direction[2] = glm_lerp(light_direction[2], cam->data.camera_front[2], 0.1);

	light->direction[0] = light_direction[0];
	light->direction[1] = light_direction[1];
	light->direction[2] = light_direction[2];

	RScene_SetRenderInstancePosition(player.flashlight_id, cam->data.position);
}

static int PL_checkForHotbarSlotInput()
{
	char block_buf[24];
	memset(block_buf, 0, sizeof(block_buf));
	for (int i = 0; i < LC_PLAYER_MAX_HOTBAR_SLOTS; i++)
	{
		sprintf(block_buf, "Block%i", i);
		if (Input_IsActionPressed(block_buf))
		{
			return i;
		}
	}

	return -1;
}

void PL_onMouseScroll(int yOffset)
{
	if (yOffset > 0)
	{
		hotbar.active_index = (hotbar.active_index + 1) % LC_PLAYER_MAX_HOTBAR_SLOTS;
	}
	else if (yOffset < 0)
	{
		hotbar.active_index = (hotbar.active_index - 1 >= 0) ? hotbar.active_index - 1 : LC_PLAYER_MAX_HOTBAR_SLOTS - 1;
	}	
}

static void PL_processInput()
{
	if (Input_IsActionJustPressed("Fullscreen"))
	{
		Window_toggleFullScreen();
	}
	
	if (Core_CheckForBlockedInputState())
	{
		return;
	}

	if (Con_isOpened())
	{
		return;
	}

	if (Input_IsActionJustPressed("Inventory"))
	{
		inventory.is_opened = !inventory.is_opened;
	}

	if (inventory.is_opened)
	{
		return;
	}

	int x_move = 0;
	int y_move = 0;
	int z_move = 0;
	if (Input_IsActionPressed("Forward"))
	{
		z_move = 1;
	}
	else if (Input_IsActionPressed("Backward"))
	{
		z_move = -1;
	}
	if (Input_IsActionPressed("Left"))
	{
		x_move = -1;
	}
	else if (Input_IsActionPressed("Right"))
	{
		x_move = 1;
	}

	if (Input_IsActionPressed("Jump"))
	{
		y_move = 1;
	}
	
	if (Input_IsActionPressed("Duck"))
	{
		y_move = -1;
	}
	PL_move(x_move, y_move, z_move);

	if (Input_IsActionPressed("Place"))
	{
		PL_placeBlock();
	}
	else if (Input_IsActionPressed("Remove"))
	{
		PL_mineBlock();
	}
	if (Input_IsActionJustPressed("NextBlock"))
	{
		hotbar.active_index = (hotbar.active_index + 1) % LC_PLAYER_MAX_HOTBAR_SLOTS;
	}
	else if (Input_IsActionJustPressed("PrevBlock"))
	{
		hotbar.active_index = (hotbar.active_index - 1 >= 0) ? hotbar.active_index - 1 : LC_PLAYER_MAX_HOTBAR_SLOTS - 1;
	}
	if (Input_IsActionJustPressed("ToggleFreefly"))
	{
		if(pl_cvars.pl_freefly->int_value)
		{
			Cvar_setValueDirect(pl_cvars.pl_freefly, "0");
		}
		else
		{
			Cvar_setValueDirect(pl_cvars.pl_freefly, "1");
		}
	}
	if (Input_IsActionJustPressed("Flashlight"))
	{
		player.flashlight_active = !player.flashlight_active;
	}
	int hotbar_input = PL_checkForHotbarSlotInput();

	if (hotbar_input >= 0)
	{
		hotbar.active_index = PL_checkForHotbarSlotInput() % LC_PLAYER_MAX_HOTBAR_SLOTS;
	}	
}

static void PL_UpdateCamera()
{
	R_Camera* cam = Camera_getCurrent();

	if (!cam)
	{
		return;
	}

	double interp = Core_getLerpFraction();

	interp = glm_smoothinterp(0.0, 1.0, interp);

	cam->data.position[0] = glm_lerp(cam->data.position[0], player.k_body->box.position[0] + player.k_body->box.width * 0.5f, interp);
	cam->data.position[1] = glm_lerp(cam->data.position[1], player.k_body->box.position[1] + player.k_body->box.height * 0.5f, interp);
	cam->data.position[2] = glm_lerp(cam->data.position[2], player.k_body->box.position[2] + player.k_body->box.length * 0.5f, interp);
}

static void PL_HandleInventory()
{
	Window_EnableCursor();

	double mouse_x_pos, mouse_y_pos;
	glfwGetCursorPos(Window_getPtr(), &mouse_x_pos, &mouse_y_pos);
	vec2 mouse_pos;
	mouse_pos[0] = mouse_x_pos;
	mouse_pos[1] = mouse_y_pos;
	Core_BlockInputThisFrame();

	ivec2 window_size;
	Window_getSize(window_size);
	
	vec2 window_scale;
	window_scale[0] = ((float)window_size[0] / LC_BASE_RESOLUTION_WIDTH);
	window_scale[1] = ((float)window_size[1] / LC_BASE_RESOLUTION_HEIGHT);

	const int NUM_X_TILES = 20;
	const int NUM_Y_TILES = 5;

	const float X_OFFSET = 72;
	const float Y_OFFSET = 165;

	const float BOX_WIDTH = 50 * window_scale[0];
	const float BOX_HEIGHT = 50 * window_scale[1];

	for (int x = 0; x <= NUM_X_TILES; x++)
	{
		for (int y = 0; y <= NUM_Y_TILES; y++)
		{
			float offseted_x = ((x * 54) + X_OFFSET) * window_scale[0];
			float offseted_y = ((y * 54) + Y_OFFSET) * window_scale[1];

			vec2 box[2];
			box[0][0] = offseted_x;
			box[0][1] = offseted_y;

			box[1][0] = offseted_x + BOX_WIDTH;
			box[1][1] = offseted_y + BOX_HEIGHT;

			if (glm_aabb2d_point(box, mouse_pos) && (Input_IsActionPressed("Place")))
			{
				LC_BlockType selected_block_type = inventory.inventory_blocks[x][y];

				int hotbar_input = PL_checkForHotbarSlotInput();

				if (hotbar_input >= 0)
				{
					hotbar.blocks[hotbar_input] = selected_block_type;
				}

				
			}
		}
	}

}

void PL_IssueDrawCmds()
{
	R_Camera* cam = Camera_getCurrent();

	if (!cam)
	{
		return;
	}

	if (pl_cvars.pl_showpos->int_value)
	{
		LC_Draw_DrawShowPos(player.k_body->box.position, player.k_body->velocity, cam->data.yaw, cam->data.pitch, player.held_block_type, selected_block.block.type, 2);
	}
	if (pl_cvars.pl_showchunk->int_value)
	{
		//LC_Draw_ChunkInfo(selected_block.chunk, 0);
	}

	if (inventory.is_opened)
	{
		LC_Draw_Inventory(inventory.inventory_blocks, inventory.block_amounts, &hotbar);
	}
	else
	{

		LC_Draw_Hotbar(&hotbar, inventory.block_amounts);
		LC_Draw_Crosshair();
	}
	if (player.k_body->in_water && player.k_body->water_level > 0)
	{
		LC_Draw_WaterOverlayScreenTexture(player.k_body->water_level);
	}
	//Draw selected block outlines
	if (selected_block.block.type != LC_BT__NONE)
	{
		vec3 box[2];
		LC_getBlockTypeAABB(selected_block.block.type, box);

		box[0][0] += selected_block.position[0];
		box[0][1] += selected_block.position[1];
		box[0][2] += selected_block.position[2];

		box[1][0] += selected_block.position[0];
		box[1][1] += selected_block.position[1];
		box[1][2] += selected_block.position[2];

		const vec4 wire_color = { 0, 0, 0, 0 };
		Draw_CubeWires(box, wire_color);

		//draw shattered effect if block is being mined
		if (LC_World_getPrevMinedBlockHP() < 7 && !LC_isBlockProp(selected_block.block.type))
		{
			//add a small offset
			for (int i = 0; i < 3; i++)
			{
				box[0][i] -= 0.01;
				box[1][i] += 0.01;
			}

			M_Rect2Df tex_Region;

			tex_Region.width = 25;
			tex_Region.height = -25;
			tex_Region.x = 24;
			tex_Region.y = (7 - LC_World_getPrevMinedBlockHP());

			Draw_TexturedCubeColored(box, Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE), &tex_Region, 1.0, 1.0, 1.0, 0.5);
		}

	}
	
}

void PL_Update()
{
	//Fall check and play sound if landed
	PL_fallCheck();

	//update cvar values if they were modified in any way
	PL_updateModifiedCvars();

	//raycast the world for the selected block
	PL_UpdateSelectedBlock();

	//update the timer
	if (player.action_timer > 0)
	{
		player.action_timer -= Core_getDeltaTime();
	}

	player.alive_timer += Core_getDeltaTime();

	//Update listener position
	PL_updateListener();

	//Update flashlight if active
	PL_updateFlashlight();

	//Process inputs
	PL_processInput();

	PL_UpdateCamera();

	if (inventory.is_opened)
	{
		PL_HandleInventory();
	}

	//Move smoothly from and to duck state
	if (player.k_body->flags & PF__Ducking)
	{
		player.k_body->box.height = glm_lerp(player.k_body->box.height, (float)PLAYER_AABB_HEIGHT * 0.31f, 0.1);
	}
	else
	{
		player.k_body->box.height = glm_lerp(player.k_body->box.height, (float)PLAYER_AABB_HEIGHT, 0.1);
	}
	
	player.held_block_type = hotbar.blocks[hotbar.active_index];

	if (player.alive_timer < 1)
	{
		//PL_spawnCorrect();
		//printf(" spawn corecting %f \n", player.alive_timer);
	}

	//printf("%f \n", LC_CalculateSurfaceHeight(player.k_body->box.position[0], player.k_body->box.position[2], ));

}

void LC_Player_getPosition(vec3 dest)
{
	if (!player.k_body)
	{
		glm_vec3_zero(dest);
		return;
	}

	dest[0] = player.k_body->box.position[0];
	dest[1] = player.k_body->box.position[1];
	dest[2] = player.k_body->box.position[2];
}