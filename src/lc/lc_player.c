#include "lc_player.h"


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

extern R_Texture g_gui_atlas_texture;
extern R_Texture g_block_atlas;
static LC_UISprites s_uiSprites;
static R_Texture inventory_texture;

static void initUiSprites()
{
	memset(&s_uiSprites, 0, sizeof(LC_UISprites));

	Sprite_setTexture(&s_uiSprites.hotbar, &g_gui_atlas_texture);
	M_Rect2Di texture_region;
	texture_region.x = 48;
	texture_region.y = 606;
	texture_region.width = 182;
	texture_region.height = 22;
	Sprite_setTextureRegion(&s_uiSprites.hotbar, texture_region);
	s_uiSprites.hotbar.scale[0] = 2;
	s_uiSprites.hotbar.scale[1] = 2;

	Sprite_setTexture(&s_uiSprites.hotbar_highlight, &g_gui_atlas_texture);

	texture_region.x = 24;
	texture_region.y = 604;
	texture_region.width = 24;
	texture_region.height = 24;
	Sprite_setTextureRegion(&s_uiSprites.hotbar_highlight, texture_region);
	s_uiSprites.hotbar_highlight.scale[0] = 2;
	s_uiSprites.hotbar_highlight.scale[1] = 2;

	inventory_texture = Texture_Load("assets/ui/inventory.png", NULL);

	Sprite_setTexture(&s_uiSprites.inventory, &inventory_texture);
	texture_region.x = 0;
	texture_region.y = 0;
	texture_region.width = 176;
	texture_region.height = 165;
	s_uiSprites.inventory.scale[0] = 2;
	s_uiSprites.inventory.scale[1] = 2;
	s_uiSprites.inventory.flipped_y = true;
	Sprite_setTextureRegion(&s_uiSprites.inventory, texture_region);
	
}

static void drawGUI()
{
	vec2 pos;
	pos[0] = 240 + (40 * s_player.hotbar_index);
	pos[1] = 548;

	r_DrawScreenSprite(pos, &s_uiSprites.hotbar_highlight);
	
	R_Sprite sprite;
	sprite.scale[0] = 2;
	sprite.scale[1] = 2;
	sprite.rotation = 0;
	Sprite_setTexture(&sprite, &g_block_atlas);

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
		Sprite_setTextureRegion(&sprite, tex_Region);

		r_DrawScreenSprite(pos, &sprite);

		pos[0] += 40;
	}


	pos[0] = 400;
	pos[1] = 550;

	r_DrawScreenSprite(pos, &s_uiSprites.hotbar);

	pos[0] = 400;
	pos[1] = 250;
	//r_DrawScreenSprite(pos, &s_uiSprites.inventory);
	

}

static LC_Block* getSelectedBlock(ivec3 r_pos, ivec3 r_face)
{
	vec3 from_pos;

	ivec3 block_pos;
	ivec3 face;

	from_pos[0] = cam_ptr->data.position[0] + 0.5;
	from_pos[1] = cam_ptr->data.position[1] + 0.5;
	from_pos[2] = cam_ptr->data.position[2] + 0.5;

	LC_Block* block = LC_World_getBlockByRay(from_pos, cam_ptr->data.camera_front, block_pos, face);

	if (!block)
		return NULL;

	AABB aabb;
	aabb.width = 1;
	aabb.height = 1;
	aabb.length = 1;

	aabb.position[0] = block_pos[0] - 0.5;
	aabb.position[1] = block_pos[1] - 0.5;
	aabb.position[2] = block_pos[2] - 0.5;

	r_drawAABBWires(aabb, NULL);

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
			LC_World_addBlock(block_pos[0], block_pos[1], block_pos[2], faces, s_player.held_block_type);
			s_player.k_body->force_update_on_frame = true;
			s_player.attack_cooldown_timer = PLAYER_ACTION_FREQ;
		}
		else if (s_player.mouse_click[1])
		{
			LC_World_mineBlock(block_pos[0], block_pos[1], block_pos[2]);
			s_player.k_body->force_update_on_frame = true;
			s_player.attack_cooldown_timer = PLAYER_ACTION_FREQ;
		}
		
	}


	

}

void LC_Player_Create(LC_World* world, vec3 pos)
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

	player.handle = LC_World_AssignEntity();

	player.k_body = PhysWorld_AddKinematicBody(world->phys_world, &aabb, player.handle);
	player.k_body->max_speed = PLAYER_MAX_SPEED;
	player.k_body->accel = PLAYER_ACCEL;
	player.k_body->gravity_free = true;
	player.k_body->jump_height = PLAYER_JUMP_HEIGHT;
	player.k_body->collidable = true;
	player.k_body->force_update_on_frame = true;

	player.free_fly = true;
	player.held_block_type = LC_BT__STONE;
	
	player.hotbar_slots[0] = LC_BT__GRASS;
	player.hotbar_slots[1] = LC_BT__SAND;
	player.hotbar_slots[2] = LC_BT__STONE;
	player.hotbar_slots[3] = LC_BT__DIRT;
	player.hotbar_slots[4] = LC_BT__TRUNK;
	player.hotbar_slots[5] = LC_BT__TREELEAVES;
	player.hotbar_slots[6] = LC_BT__WATER;
	player.hotbar_slots[7] = LC_BT__GLASS;
	player.hotbar_slots[8] = LC_BT__FLOWER;
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
		s_player.k_body->gravity_free = true;
	}
	else
	{
		s_player.k_body->gravity_free = false;
	}

	//printf("%f \n", s_player.k_body->box.position[1]);

	//Update Camera position
	cam->data.position[0] = s_player.k_body->box.position[0] + (float)PLAYER_AABB_WIDTH * 0.5f;
	cam->data.position[1] = s_player.k_body->box.position[1] + (float)PLAYER_AABB_HEIGHT * 0.5f;
	cam->data.position[2] = s_player.k_body->box.position[2] + (float)PLAYER_AABB_LENGTH * 0.5f;

	memset(s_player.mouse_click, 0, sizeof(s_player.mouse_click));
}
void LC_Player_ProcessInput(GLFWwindow* const window, R_Camera* const cam)
{
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
		if (s_player.k_body->on_ground && !s_player.free_fly)
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

}

void LC_Player_getPos(vec3 dest)
{
	dest[0] = s_player.k_body->box.position[0];
	dest[1] = s_player.k_body->box.position[1];
	dest[2] = s_player.k_body->box.position[2];
}