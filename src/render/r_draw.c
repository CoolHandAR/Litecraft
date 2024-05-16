#include "r_draw.h"
#include "r_core.h"

/*
	Render functions that process draw requests to command buffers
*/

extern R_CMD_Buffer* cmdBuffer;

static const vec4 DEFAULT_COLOR = { 1.0, 1.0, 1.0, 1.0 };

static bool _checkCmdLimit(unsigned int p_cmdSize)
{
	if (cmdBuffer->cmds_counter + 1 >= MAX_RENDER_COMMANDS || cmdBuffer->byte_count + p_cmdSize >= RENDER_BUFFER_COMMAND_ALLOC_SIZE)
	{
		printf("Max render command buffer limit reached \n");
		return false;
	}
	return true;
}

static void _copyToCmdBuffer(R_CMD p_cmdEnum, void* p_cmdData, unsigned int p_cmdSize)
{
	if (!_checkCmdLimit(p_cmdSize))
		return;

	cmdBuffer->cmd_enums[cmdBuffer->cmds_counter] = p_cmdEnum;

	memcpy(cmdBuffer->cmds_ptr, p_cmdData, p_cmdSize);
	(char*)cmdBuffer->cmds_ptr += p_cmdSize;

	cmdBuffer->byte_count += p_cmdSize;
	cmdBuffer->cmds_counter++;
}

void Draw_ScreenSprite(R_Sprite* const p_sprite)
{
	R_CMD_DrawSprite cmd;

	cmd.proj_type = R_CMD_PT__SCREEN;
	cmd.sprite_ptr = p_sprite;

	_copyToCmdBuffer(R_CMD__SPRITE, &cmd, sizeof(R_CMD_DrawSprite));
}

void Draw_3DSprite(R_Sprite* const p_sprite)
{
	R_CMD_DrawSprite cmd;

	cmd.proj_type = R_CMD_PT__3D;
	cmd.sprite_ptr = p_sprite;

	_copyToCmdBuffer(R_CMD__SPRITE, &cmd, sizeof(R_CMD_DrawSprite));
}

void Draw_BillboardSprite(R_Sprite* const p_sprite)
{
	R_CMD_DrawSprite cmd;

	cmd.proj_type = R_CMD_PT__BILLBOARD_3D;
	cmd.sprite_ptr = p_sprite;

	_copyToCmdBuffer(R_CMD__SPRITE, &cmd, sizeof(R_CMD_DrawSprite));
}


void Draw_AABBWires1(AABB p_aabb, vec4 p_color)
{
	R_CMD_DrawAABB cmd;

	cmd.aabb = p_aabb;
	cmd.polygon_mode = R_CMD_PM__WIRES;

	if (!p_color)
		glm_vec4_copy(DEFAULT_COLOR, cmd.color);
	else
		glm_vec4_copy(p_color, cmd.color);

	_copyToCmdBuffer(R_CMD__AABB, &cmd, sizeof(R_CMD_DrawAABB));
}

void Draw_AABB(AABB p_aabb, vec4 p_fillColor)
{
	R_CMD_DrawAABB cmd;

	cmd.aabb = p_aabb;
	cmd.polygon_mode = R_CMD_PM__FULL;
	if (!p_fillColor)
		glm_vec4_copy(DEFAULT_COLOR, cmd.color);
	else
		glm_vec4_copy(p_fillColor, cmd.color);

	_copyToCmdBuffer(R_CMD__AABB, &cmd, sizeof(R_CMD_DrawAABB));
}

void Draw_Line(vec3 p_from, vec3 p_to, vec4 p_color)
{
	R_CMD_DrawLine cmd;
	
	cmd.proj_type = R_CMD_PT__3D;
	glm_vec3_copy(p_from, cmd.from);
	glm_vec3_copy(p_to, cmd.to);
	if (!p_color)
		glm_vec4_copy(DEFAULT_COLOR, cmd.color);
	else
		glm_vec4_copy(p_color, cmd.color);

	_copyToCmdBuffer(R_CMD__LINE, &cmd, sizeof(R_CMD_DrawLine));
}

void Draw_Triangle(vec3 p1, vec3 p2, vec3 p3, vec4 p_color)
{
	R_CMD_DrawTriangle cmd;

	cmd.proj_type = R_CMD_PT__3D;
	glm_vec3_copy(p1, cmd.points[0]);
	glm_vec3_copy(p2, cmd.points[1]);
	glm_vec3_copy(p3, cmd.points[2]);

	if (!p_color)
		glm_vec4_copy(DEFAULT_COLOR, cmd.color);
	else
		glm_vec4_copy(p_color, cmd.color);

	_copyToCmdBuffer(R_CMD__TRIANGLE, &cmd, sizeof(R_CMD_DrawTriangle));
}

void Draw_Model(R_Model* const p_model, vec3 p_position)
{

}

void Draw_ModelWires(R_Model* const p_model, vec3 p_position)
{
}
