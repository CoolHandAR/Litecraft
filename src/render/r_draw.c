#include "r_draw.h"
#include "r_core.h"

/*
	Render functions that process draw requests to command buffers
*/

extern R_CMD_Buffers* cmdBuffers;

static bool _checkSpriteCmdLimit()
{
	if (cmdBuffers->sprite_counter + 1 >= MAX_COMMANDS_PER_CMD_BUFFER)
	{
		printf("Max sprite command buffer limit reached \n");
		return false;
	}

	return true;
}

static bool _checkAABBCmdLimit()
{
	if (cmdBuffers->aabb_counter + 1 >= MAX_COMMANDS_PER_CMD_BUFFER)
	{
		printf("Max aabb command buffer limit reached \n");
		return false;
	}
	return true;
}

void r_drawScreenSprite(R_Sprite* const p_sprite)
{
	if (!_checkSpriteCmdLimit())
		return;

	R_CMD_DrawSprite* cmd = &cmdBuffers->sprite_buf[cmdBuffers->sprite_counter];

	cmd->proj_type = R_CMD_PT__SCREEN;
	cmd->sprite_ptr = p_sprite;

	cmdBuffers->sprite_counter++;
}

void r_draw3DSprite(R_Sprite* const p_sprite)
{
	if (!_checkSpriteCmdLimit())
		return;

	R_CMD_DrawSprite* cmd = &cmdBuffers->sprite_buf[cmdBuffers->sprite_counter];

	cmd->proj_type = R_CMD_PT__3D;
	cmd->sprite_ptr = p_sprite;

	cmdBuffers->sprite_counter++;
}

void r_drawBillboardSprite(R_Sprite* const p_sprite)
{
	if (!_checkSpriteCmdLimit())
		return;

	R_CMD_DrawSprite* cmd = &cmdBuffers->sprite_buf[cmdBuffers->sprite_counter];

	cmd->proj_type = R_CMD_PT__3D;
	cmd->sprite_ptr = p_sprite;

	cmdBuffers->sprite_counter++;
}


void r_drawAABBWires1(AABB p_aabb, vec4 p_color)
{
	if (!_checkAABBCmdLimit())
		return;

	R_CMD_DrawAABB* cmd = &cmdBuffers->aabb_buf[cmdBuffers->aabb_counter];

	cmd->aabb = p_aabb;
	cmd->polygon_mode = R_CMD_PM__WIRES;
	glm_vec4_copy(p_color, cmd->color);

	cmdBuffers->aabb_counter++;
}

void r_drawAABB(AABB p_aabb, vec4 p_fillColor)
{
	if (!_checkAABBCmdLimit())
		return;

	R_CMD_DrawAABB* cmd = &cmdBuffers->aabb_buf[cmdBuffers->aabb_counter];

	cmd->aabb = p_aabb;
	cmd->polygon_mode = R_CMD_PM__FULL;
	glm_vec4_copy(p_fillColor, cmd->color);

	cmdBuffers->aabb_counter++;
}