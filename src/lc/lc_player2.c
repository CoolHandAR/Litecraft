#include "core/cvar.h"
#include "input.h"
#include "physics/p_physics_defs.h"
#include "physics/p_physics_world.h"
#include "lc2/lc_world.h"

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MAIN PLAYER STUFF, MOVEMENT, ACTIONS ETC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


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
	LC_Block* block;
	ivec3 face;
	ivec3 position;
} SelectedBlock;

typedef struct
{
	Kinematic_Body* k_body;
	float action_timer;
	LC_BlockType held_block_type;
} Player;

PlayerCvars pl_cvars;
static SelectedBlock selected_block;
Player player;


static void PL_initCvars()
{
	pl_cvars.pl_freefly = Cvar_Register("pl_freefly", "1", 0, 0, 0, 1);
	pl_cvars.pl_noclip = Cvar_Register("pl_noclip", "1", "Disables collision", 0, 0, 0, 1);
	pl_cvars.pl_godmode = Cvar_Register("pl_godmode", "1", "Disables all damage", 0, 0, 0, 0);
	pl_cvars.pl_stop_speed = Cvar_Register("pl_stop_speed", "1", "Player movement speed", 0, 0, 10000);
	pl_cvars.pl_accel = Cvar_Register("pl_accel", "1", "Player ground acceleration", 0, 0, 100);
	pl_cvars.pl_air_accel = Cvar_Register("pl_air_accel", "1", "Player air acceleration", 0, 0, 100);
	pl_cvars.pl_flying_accel = Cvar_Register("pl_flying_accel", "1", "Player flying acceleration", 0, 0, 100);
	pl_cvars.pl_friction = Cvar_Register("pl_friction", "1", "Player ground friction", 0, 0, 100);
	pl_cvars.pl_air_friction = Cvar_Register("pl_air_friction", "1", "Player air friction", 0, 0, 100);
	pl_cvars.pl_flying_friction = Cvar_Register("pl_flying_friction", "1", "Player flying friction", 0, 0, 100);
	pl_cvars.pl_action_freq = Cvar_Register("pl_action_freq", "1", 0, 0, 0, 100);
	pl_cvars.pl_jump_height = Cvar_Register("pl_jump_height", "100", 0, 0, 0, 100);
	pl_cvars.pl_showpos = Cvar_Register("pl_showpos", "0", NULL, 0, 0, 1);
	pl_cvars.pl_showchunk = Cvar_Register("pl_showchunk", "0", NULL, 0, 0, 1);
}

static void PL_initInputs()
{
	Input_registerAction("Jump");
	Input_registerAction("Forward");
	Input_registerAction("Backward");
	Input_registerAction("Left");
	Input_registerAction("Right");
	Input_registerAction("Attack");
	Input_registerAction("Special");
	Input_registerAction("Duck");
}

static void PL_initPlayer(vec3 p_initPos)
{
	PL_initCvars();
	PL_initInputs();
	memset(&player, 0, sizeof(Player));
	memset(&selected_block, 0, sizeof(selected_block));
	
	AABB aabb;
	memset(&aabb, 0, sizeof(AABB));

	if (p_initPos != NULL)
	{
		glm_vec3_copy(p_initPos, aabb.position);
	}
	else
	{
		glm_vec3_zero(aabb.position);
	}
	aabb.width = PLAYER_AABB_WIDTH;
	aabb.height = PLAYER_AABB_HEIGHT;
	aabb.length = PLAYER_AABB_LENGTH;

	//register the physics body
	player.k_body = PhysWorld_AddKinematicBody(LC_World_GetPhysWorld(), &aabb, NULL);
	//setup default body config
	player.k_body->config.jump_height = PLAYER_JUMP_HEIGHT;
	player.k_body->config.ground_accel = PLAYER_GROUND_ACCEL;
	player.k_body->config.air_accel = PLAYER_AIR_ACCEL;
	player.k_body->config.flying_accel = PLAYER_FLYING_ACCEL;
	player.k_body->config.ground_friction = PLAYER_GROUND_FRICTION;
	player.k_body->config.ducking_scale = 1;
	player.k_body->config.stop_speed = 10;
	
	
}

static void PL_updateModifiedCvars()
{
	if (pl_cvars.pl_freefly->modified)
	{
		if (pl_cvars.pl_freefly->int_value)
		{
			player.k_body->flags |= PF__AffectedByGravity;
		}
		else
		{
			player.k_body->flags &= ~PF__AffectedByGravity;
		}
		pl_cvars.pl_freefly->modified = false;
	}
	if (pl_cvars.pl_noclip->modified)
	{
		if (pl_cvars.pl_noclip->int_value)
		{
			player.k_body->flags |= PF__Collidable;
		}
		else
		{
			player.k_body->flags &= ~PF__Collidable;
		}
		pl_cvars.pl_noclip->modified = false;
	}
	if (pl_cvars.pl_stop_speed->modified)
	{
		player.k_body->config.stop_speed = pl_cvars.pl_stop_speed->float_value;
		pl_cvars.pl_stop_speed->modified = false;
	}
}


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

static void PL_UpdateSelectedBlock()
{
	vec3 from_pos;

	//from_pos[0] = cam_ptr->data.position[0] + 0.5;
	//from_pos[1] = cam_ptr->data.position[1] + 0.5;
	//from_pos[2] = cam_ptr->data.position[2] + 0.5;
	//LC_Block* block = LC_World_getBlockByRay(from_pos, cam_ptr->data.camera_front, block_pos, face);
	selected_block.block = LC_World_getBlockByRay(from_pos, NULL, selected_block.position, selected_block.face, &selected_block.chunk);
}

static void PL_placeBlock()
{
	//no block?
	if (!selected_block.block)
	{
		return;
	}

	//is the cooldown still active?
	if (player.action_timer > 0)
	{
		return;
	}

	//add the block
	//we failed to add the block(for whatever reason)
	if (!LC_World_addBlock2(selected_block.position[0], selected_block.position[1], selected_block.position[2], selected_block.face, player.held_block_type))
	{
		return;
	}

	//if we will possibly be stuck after placing the block, move the player by the amount
	glm_normalize(selected_block.face);
	if (PL_isPlayerInBlock(selected_block.position[0] + selected_block.face[0], selected_block.position[1] + selected_block.face[1], selected_block.position[2] + selected_block.face[2]))
	{
		player.k_body->box.position[0] += selected_block.face[0];
		player.k_body->box.position[1] += selected_block.face[1];
		player.k_body->box.position[2] += selected_block.face[2];
	}

	//Reset the timer
	player.action_timer = 8;
}

static void PL_mineBlock()
{
	//no block?
	if (!selected_block.block)
	{
		return;
	}

	//is the cooldown still active?
	if (player.action_timer > 0)
	{
		return;
	}

	//did we fail to mine the block for whatever reason?
	if (!LC_World_mineBlock2(selected_block.position[0], selected_block.position[1], selected_block.position[2]))
	{
		return;
	}

	//Reset the timer
	player.action_timer = 8;
}

static void PL_move(int p_dirX, int p_dirY, int p_dirZ)
{
	//STRAFING LEFT AND RIGHT
	if (p_dirX != 0)
	{
		player.k_body->direction[0] += 0.1 * glm_sign(p_dirX);
		player.k_body->direction[2] += 0.1 * glm_sign(p_dirX);

		if (pl_cvars.pl_freefly->int_value == 1)
		{
			player.k_body->direction[1] += 0.1 * glm_sign(p_dirX);
		}
	}
	//JUMPING AND DUCKING
	if (p_dirY != 0)
	{

	}
	//FORWARD AND BACKWARDS
	if (p_dirZ != 0)
	{

	}
}

static void PL_inventoryInput()
{

}

static void PL_processInput()
{
	float yaw_in_radians = 2;
	float cos_yaw = cos(yaw_in_radians);
	float sin_yaw = sin(yaw_in_radians);

	vec3 direction;
	glm_vec3_copy(player.k_body->direction, direction);

	if (Input_IsActionPressed("Forward"))
	{
		PL_move(0, 0, 1);
	}
	else if (Input_IsActionPressed("Backwards"))
	{
		PL_move(0, 0, -1);
	}
	if (Input_IsActionPressed("Left"))
	{
		PL_move(-1, 0, 0);
	}
	else if (Input_IsActionPressed("Right"))
	{
		PL_move(1, 0, 0);
	}

	if (Input_IsActionPressed("Jump"))
	{
		PL_move(0, 1, 0);
	}
	
	if (Input_IsActionPressed("Duck"))
	{
		PL_move(0, -1, 0);
	}
	else
	{
		//unduck
	}

	if (Input_IsActionPressed("Place"))
	{
		PL_placeBlock();
	}
	else if (Input_IsActionPressed("Remove"))
	{
		PL_mineBlock();
	}
	else if (Input_IsActionJustPressed("Inventory"))
	{
		PL_inventoryInput();
	}

	if (Input_IsActionJustPressed("NextBlock"))
	{

	}
	else if (Input_IsActionJustPressed("PrevBlock"))
	{

	}
}

void PL_Update()
{
	//update cvar values if they were modified in any way
	PL_updateModifiedCvars();

	//Update the selected block
	PL_UpdateSelectedBlock();

	//update the timer
	if (player.action_timer > 0)
	{
		player.action_timer -= 0.1;
	}


}