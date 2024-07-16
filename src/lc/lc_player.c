#include "lc_player.h"
#include "core/c_common.h"

#define PLAYER_AABB_WIDTH 1.1
#define PLAYER_AABB_HEIGHT 3
#define PLAYER_AABB_LENGTH 1.1

#define PLAYER_MAX_SPEED 100
#define PLAYER_MAX_DUCK_SPEED 12
#define PLAYER_ACCEL 10
#define PLAYER_AIR_ACCEL 100
#define PLAYER_JUMP_HEIGHT 1

#define PLAYER_ACTION_FREQ 0.4f

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

static LC_Player s_player;
R_Camera* cam_ptr;

#include "render/r_renderer.h"

typedef struct LC_UISprites
{
	R_Sprite hotbar;
	R_Sprite hotbar_highlight;
	R_Sprite inventory;
} LC_UISprites;

#include "lc_block_defs.h"
#include "lc2/lc_world.h"

extern R_Texture g_gui_atlas_texture;
extern R_Texture g_block_atlas;
static LC_UISprites s_uiSprites;
static R_Texture inventory_texture;

static bool isPlayerInBlock(int p_gX, int p_gY, int p_gZ)
{
	AABB block_box;
	block_box.width = 1;
	block_box.height = 1;
	block_box.length = 1;
	block_box.position[0] = p_gX - 0.5;
	block_box.position[1] = p_gY - 0.5;
	block_box.position[2] = p_gZ - 0.5;

	return AABB_intersectsOther(&s_player.k_body->box, &block_box);
}

static void initUiSprites()
{
	memset(&s_uiSprites, 0, sizeof(LC_UISprites));

	Sprite_setTexture(&s_uiSprites.hotbar, &g_gui_atlas_texture);
	M_Rect2Di texture_region;
	texture_region.x = 48;
	texture_region.y = 606;
	texture_region.width = 182;
	texture_region.height = 22;
	//Sprite_setTextureRegion(&s_uiSprites.hotbar, texture_region);
	s_uiSprites.hotbar.scale[0] = 2;
	s_uiSprites.hotbar.scale[1] = 2;
	vec2 scale;
	scale[0] = 1.8;
	scale[1] = 2;

	vec3 position;
	position[0] = 400;
	position[1] = 550;
	position[2] = 23;

	Sprite_setScale(&s_uiSprites.hotbar, scale);
	Sprite_setPosition(&s_uiSprites.hotbar, position);


	Sprite_setTexture(&s_uiSprites.hotbar_highlight, &g_gui_atlas_texture);

	texture_region.x = 24;
	texture_region.y = 604;
	texture_region.width = 24;
	texture_region.height = 24;
	//Sprite_setTextureRegion(&s_uiSprites.hotbar_highlight, texture_region);
	s_uiSprites.hotbar_highlight.scale[0] = 2;
	s_uiSprites.hotbar_highlight.scale[1] = 2;

	inventory_texture = Texture_Load("assets/ui/inventory.png", NULL);

	Sprite_setTexture(&s_uiSprites.inventory, &inventory_texture);
	texture_region.x = 0;
	texture_region.y = 0;
	texture_region.width = 176;
	texture_region.height = 165;
	position[0] = 454;
	position[1] = 550;
	position[2] = 22;
	s_uiSprites.inventory.scale[0] = 2;
	s_uiSprites.inventory.scale[1] = 2;
	s_uiSprites.inventory.flipped_y = true;
	//Sprite_setTextureRegion(&s_uiSprites.inventory, texture_region);
	Sprite_setPosition(&s_uiSprites.inventory, position);
}

static void drawGUI()
{
	vec2 pos;
	pos[0] = 240 + (40 * s_player.hotbar_index);
	pos[1] = 548;

	//r_DrawScreenSprite(&s_uiSprites.hotbar_highlight);
	
	R_Sprite sprite;
	sprite.scale[0] = 2;
	sprite.scale[1] = 2;
	sprite.rotation = 0;
	//Sprite_setTexture(&sprite, &g_block_atlas);

	M_Rect2Di tex_Region;
	tex_Region.width = 16;
	tex_Region.height = 16;
	pos[0] = 240;
	pos[1] = 550;
	for (int i = 0; i < 9; i++)
	{
		if (s_player.hotbar_slots[i] == LC_BT__NONE)
			continue;

		LC_Block_Texture_Offset_Data tex_data = LC_BLOCK_TEX_OFFSET_DATA[s_player.hotbar_slots[i]];

		tex_Region.x = (tex_data.top_face[0] * 16) + 16;
		tex_Region.y = -(tex_data.top_face[1] * 16);
		//Sprite_setTextureRegion(&sprite, tex_Region);

		//r_DrawScreenSprite(&sprite);

		pos[0] += 40;
	}



	//r_DrawScreenSprite(&s_uiSprites.hotbar);


	//r_DrawScreenSprite(&s_uiSprites.inventory);
	

}

static LC_Block* getSelectedBlock(ivec3 r_pos, ivec3 r_face)
{
	vec3 from_pos;

	ivec3 block_pos;
	ivec3 face;

	from_pos[0] = cam_ptr->data.position[0] + 0.5;
	from_pos[1] = cam_ptr->data.position[1] + 0.5;
	from_pos[2] = cam_ptr->data.position[2] + 0.5;

	//LC_Block* block = LC_World_getBlockByRay(from_pos, cam_ptr->data.camera_front, block_pos, face);
	LC_Block* block = LC_World_getBlockByRay(from_pos, cam_ptr->data.camera_front, block_pos, face, NULL);

	if (!block)
		return NULL;

	AABB aabb;
	aabb.width = 1;
	aabb.height = 1;
	aabb.length = 1;

	aabb.position[0] = block_pos[0] - 0.5;
	aabb.position[1] = block_pos[1] - 0.5;
	aabb.position[2] = block_pos[2] - 0.5;


	r_pos[0] = block_pos[0];
	r_pos[1] = block_pos[1];
	r_pos[2] = block_pos[2];

	r_face[0] = face[0];
	r_face[1] = face[1];
	r_face[2] = face[2];

	return block;
}

void handleAction(float delta)
{
	ivec3 block_pos;
	ivec3 faces;

	LC_Block* block = getSelectedBlock(block_pos, faces);

	//No block found?
	if (!block)
	{
		return;
	}

	if (s_player.attack_cooldown_timer > 0)
	{
		s_player.attack_cooldown_timer -= delta;
	}
	
	if (s_player.attack_cooldown_timer <= 0.0)
	{
		if (s_player.mouse_click[0])
		{
			if (LC_World_addBlock2(block_pos[0], block_pos[1], block_pos[2], faces, s_player.held_block_type))
			{
				if (isPlayerInBlock(block_pos[0] + faces[0], block_pos[1] + faces[1], block_pos[2] + faces[2]))
				{
					s_player.k_body->box.position[0] += faces[0];
					s_player.k_body->box.position[1] += faces[1];
					s_player.k_body->box.position[2] += faces[2];
				}
				
	
			}
			s_player.k_body->force_update_on_frame = true;
			s_player.attack_cooldown_timer = PLAYER_ACTION_FREQ;
		}
		else if (s_player.mouse_click[1])
		{
			LC_World_mineBlock2(block_pos[0], block_pos[1], block_pos[2]);
			s_player.k_body->force_update_on_frame = true;
			s_player.attack_cooldown_timer = PLAYER_ACTION_FREQ;
		}
		
	}


	

}

void LC_Player_Create(vec3 pos)
{
	LC_Player player;
	memset(&player, 0, sizeof(LC_Player));

	AABB aabb;

	if (pos != NULL)
	{
		aabb.position[0] = pos[0];
		aabb.position[1] = pos[1];
		aabb.position[2] = pos[2];
	}
	else
	{
		aabb.position[0] = 0;
		aabb.position[1] = 0;
		aabb.position[2] = 0;
	}
	aabb.width = PLAYER_AABB_WIDTH;
	aabb.height = PLAYER_AABB_HEIGHT;
	aabb.length = PLAYER_AABB_LENGTH;

	//player.handle = LC_World_AssignEntity();

	//PHYSICS BODY
	//player.k_body = PhysWorld_AddKinematicBody(world->phys_world, &aabb, player.handle);
	player.k_body = PhysWorld_AddKinematicBody(LC_World_GetPhysWorld(), &aabb, NULL);
	player.k_body->config.ducking_scale = 0.2f;
	player.k_body->config.ground_accel = 100;
	player.k_body->config.air_accel = 1;
	player.k_body->config.water_accel = 0.3;
	player.k_body->config.jump_height = 0.07;
	player.k_body->force_update_on_frame = true;
	player.k_body->flags = PF__Collidable | PF__ForceUpdateOnFrame;
	player.k_body->config.air_friction = 0.2;
	player.k_body->config.ground_friction = 6.0;
	player.k_body->config.flying_friction = 5;
	player.k_body->config.water_friction = 1000;
	player.k_body->config.stop_speed = 100;

	player.free_fly = true;
	player.held_block_type = LC_BT__STONE;
	
	player.hotbar_slots[0] = LC_BT__GRASS;
	player.hotbar_slots[1] = LC_BT__SAND;
	player.hotbar_slots[2] = LC_BT__STONE;
	player.hotbar_slots[3] = LC_BT__GLOWSTONE;
	player.hotbar_slots[4] = LC_BT__TRUNK;
	player.hotbar_slots[5] = LC_BT__TREELEAVES;
	player.hotbar_slots[6] = LC_BT__MAGMA;
	player.hotbar_slots[7] = LC_BT__GLASS;
	player.hotbar_slots[8] = LC_BT__OBSIDIAN;
	player.hotbar_index = 0;


	initUiSprites();



	s_player = player;
}

void LC_Player_Update(R_Camera* const cam, float delta)
{
	
	cam_ptr = cam;

	handleAction(delta);

	//r_DrawCrosshair();

	drawGUI();

	if (s_player.free_fly)
	{
		s_player.k_body->flags &= ~PF__AffectedByGravity;
	}
	else
	{
		s_player.k_body->flags |= PF__AffectedByGravity;
	}

	//printf("%f \n", s_player.k_body->box.position[1]);

	//Update Camera position
	cam->data.position[0] = s_player.k_body->box.position[0] + (float)PLAYER_AABB_WIDTH * 0.5f;
	cam->data.position[1] = s_player.k_body->box.position[1] + s_player.k_body->box.height * 0.5f;
	cam->data.position[2] = s_player.k_body->box.position[2] + (float)PLAYER_AABB_LENGTH * 0.5f;

	if (s_player.k_body->flags & PF__Ducking)
	{
		s_player.k_body->box.height = glm_lerp(s_player.k_body->box.height, (float)PLAYER_AABB_HEIGHT * 0.31f, 0.1);
	}
	else
	{
		s_player.k_body->box.height = glm_lerp(s_player.k_body->box.height, (float)PLAYER_AABB_HEIGHT, 0.1);
	}

	memset(s_player.mouse_click, 0, sizeof(s_player.mouse_click));
}
void LC_Player_ProcessInput(GLFWwindow* const window, R_Camera* const cam)
{
	if (Con_isOpened())
		return;

	float yaw_in_radians = glm_rad(cam->data.yaw);
	float cos_yaw = cos(yaw_in_radians);
	float sin_yaw = sin(yaw_in_radians);

	

	//MOVING
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		
		s_player.k_body->direction[0] += cam->data.camera_right[0];
		s_player.k_body->direction[2] += cam->data.camera_right[2];

		if (s_player.free_fly)
			s_player.k_body->direction[1] += cam->data.camera_right[1];
	}
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{

		s_player.k_body->direction[0] += -cam->data.camera_right[0];
		s_player.k_body->direction[2] += -cam->data.camera_right[2];

		if (s_player.free_fly)
			s_player.k_body->direction[1] += -cam->data.camera_right[1];
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{	

		if (s_player.free_fly)
		{
			s_player.k_body->direction[0] += cam->data.camera_front[0];
			s_player.k_body->direction[1] += cam->data.camera_front[1];
			s_player.k_body->direction[2] += cam->data.camera_front[2];
		}
		else
		{
			//fixes issues when looking down and moving
			s_player.k_body->direction[0] += cos_yaw;
			s_player.k_body->direction[2] += sin_yaw;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		if (s_player.free_fly)
		{
			s_player.k_body->direction[0] += -cam->data.camera_front[0];
			s_player.k_body->direction[1] += -cam->data.camera_front[1];
			s_player.k_body->direction[2] += -cam->data.camera_front[2];
		}
		else
		{
			//fixes issues when looking down and moving
			s_player.k_body->direction[0] += -cos_yaw;
			s_player.k_body->direction[2] += -sin_yaw;
		}
	}
	//JUMPING
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		if (!s_player.free_fly)
		{
			s_player.k_body->direction[1] = 1;
		}
	}
	//PLACE BLOCK
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		s_player.mouse_click[0] = true;
	}
	//REMOVE BLOCK
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		s_player.mouse_click[1] = true;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		s_player.k_body->flags |= PF__Ducking;
	}
	else
	{
		//s_player.k_body->flags = s_player.k_body->flags & ~PF__Ducking;
	}
	//CHANGE HELD BLOCK TYPE
	//HOTBAR BUTTONS
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[0];
		s_player.hotbar_index = 0;
	}
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[1];
		s_player.hotbar_index = 1;
	}
	else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[2];
		s_player.hotbar_index = 2;
	}
	else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[3];
		s_player.hotbar_index = 3;
	}
	else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[4];
		s_player.hotbar_index = 4;
	}
	else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[5];
		s_player.hotbar_index = 5;
	}
	else if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[6];
		s_player.hotbar_index = 6;
	}
	else if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[7];
		s_player.hotbar_index = 7;
	}
	else if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS)
	{
		s_player.held_block_type = s_player.hotbar_slots[8];
		s_player.hotbar_index = 8;
	}
	//ENABLE FREE FLY
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
	{
		s_player.free_fly = !s_player.free_fly;
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
	{
		s_player.k_body->flags |= PF__Collidable;
	}
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
	{
		s_player.k_body->flags = s_player.k_body->flags & ~PF__Collidable;
	}
	
}

void LC_Player_getPos(vec3 dest)
{
	dest[0] = s_player.k_body->box.position[0];
	dest[1] = s_player.k_body->box.position[1];
	dest[2] = s_player.k_body->box.position[2];
}