/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Process all draw calls, UI drawing associated with the game
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <cglm/cglm.h>

#include "core/core_common.h"
#include "core/resource_manager.h"
#include "lc/lc_core.h"
#include "lc/lc_world.h"

extern NK_Data nk;

static void LC_Draw_CornerIndexToScreenPosition(int corner, vec2 dest)
{
	ivec2 window_size;
	Window_getSize(window_size);

	float x_pos = 0;
	float y_pos = 0;

	//TOP LEFT AND BOTTOM LEFT
	if (corner == 0 || corner == 2)
	{
		x_pos = 10;
		y_pos = (corner == 0) ? 10 : window_size[1] - 100;
	}
	//TOP RIGHT AND BOTTOM RIGHT
	else if (corner == 1 || corner == 3)
	{
		x_pos = window_size[0] - 200;
		y_pos = (corner == 1) ? 10 : window_size[1] - 100;
	}

	dest[0] = x_pos;
	dest[1] = y_pos;
}

void LC_Draw_DrawShowPos(vec3 pos, vec3 vel, float yaw, float pitch, uint8_t held_block, uint8_t selected_block, int corner)
{
	if (!nk.enabled)
		return;

	vec2 win_pos;
	LC_Draw_CornerIndexToScreenPosition(corner, win_pos);

	nk_style_push_color(nk.ctx, &nk.ctx->style.window.fixed_background.data.color, nk_rgba(0, 0, 0, 0));
	if (!nk_begin(nk.ctx, "ShowPos", nk_rect(win_pos[0], win_pos[1] - 20, 240, 122), NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT))
	{
		nk_end(nk.ctx);
		return;
	}
	
	nk_style_push_color(nk.ctx, &nk.ctx->style.text.color, nk_rgba(255, 255, 255, 255));
	nk_layout_row_dynamic(nk.ctx, 15, 1);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Position: %.2f %.2f %.2f", pos[0], pos[1], pos[2]);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Velocity: %.2f", glm_vec3_norm(vel));
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Yaw: %.2f", yaw);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Pitch: %.2f", pitch);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Held block: %s", LC_getBlockName(held_block));
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Selected block: %s", LC_getBlockName(selected_block));
	nk_style_pop_color(nk.ctx);
	nk_style_pop_color(nk.ctx);
	nk_end(nk.ctx);
}

void LC_Draw_ChunkInfo(LC_Chunk* const chunk, int corner)
{
	if (!nk.enabled)
		return;

	if (!chunk)
	{
		return;
	}
	vec2 win_pos;
	LC_Draw_CornerIndexToScreenPosition(corner, win_pos);

	nk_style_push_color(nk.ctx, &nk.ctx->style.window.fixed_background.data.color, nk_rgba(255, 255, 255, 80));
	if (!nk_begin(nk.ctx, "ChunkInfo", nk_rect(win_pos[0], win_pos[1], 200, 120), NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT))
	{
		nk_end(nk.ctx);
		return;
	}
	nk_style_push_color(nk.ctx, &nk.ctx->style.text.color, nk_rgba(255, 255, 255, 255));
	nk_layout_row_dynamic(nk.ctx, 15, 1);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Global Position: %i %i %i", chunk->global_position[0], chunk->global_position[1], chunk->global_position[2]);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Alive blocks: %i", chunk->alive_blocks);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Opaque blocks: %i", chunk->opaque_blocks);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Transparent blocks: %i", chunk->transparent_blocks);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Water blocks: %i", chunk->water_blocks);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "DRAW CMD INDEX: %i", chunk->draw_cmd_index);
	nk_style_pop_color(nk.ctx);
	nk_style_pop_color(nk.ctx);

	nk_end(nk.ctx);
}

void LC_Draw_WorldInfo(LC_World* const world, int corner)
{
	if (!nk.enabled)
		return;

	if (!world)
	{
		return;
	}
	vec2 win_pos;
	LC_Draw_CornerIndexToScreenPosition(corner, win_pos);

	const int num_items = 4;

	nk_style_push_color(nk.ctx, &nk.ctx->style.window.fixed_background.data.color, nk_rgba(255, 255, 255, 80));
	if (!nk_begin(nk.ctx, "WorldInfo", nk_rect(win_pos[0], win_pos[1], 200, num_items * 20), NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT))
	{
		nk_end(nk.ctx);
		return;
	}

	nk_style_push_color(nk.ctx, &nk.ctx->style.text.color, nk_rgba(255, 255, 255, 255));
	nk_layout_row_dynamic(nk.ctx, 15, 1);

	nk_style_pop_color(nk.ctx);
	nk_style_pop_color(nk.ctx);

	nk_end(nk.ctx);
}

void LC_Draw_WaterOverlayScreenTexture(int water_level)
{
	float alpha = 0.0;
	if (water_level > 2)
	{
		alpha = 0.05 * water_level;
	}

	if (alpha > 0)
	{
		Draw_ScreenTextureColored(Resource_get("assets/ui/water_overlay.png", RESOURCE__TEXTURE), NULL, 640, 360, 80, 45, 0, 1, 1, 1, alpha);
	}	
}


void LC_Draw_Hotbar(LC_Hotbar* const hotbar, int block_amounts[LC_BT__MAX])
{
	float x_position = LC_BASE_RESOLUTION_WIDTH / 2.0;
	float y_position = LC_BASE_RESOLUTION_HEIGHT - 35;
	float scale = 2;
	
	R_Texture* gui_texture = Resource_get("assets/ui/hotbar.png", RESOURCE__TEXTURE);

	M_Rect2Df texture_region;
	texture_region.width = 182;
	texture_region.height = 22;
	texture_region.x = 24;
	texture_region.y = 0;

	Draw_ScreenTexture(gui_texture, &texture_region, x_position, y_position, scale, scale, 0);

	texture_region.width = 24;
	texture_region.height = 24;
	texture_region.x = 207;
	texture_region.y = 1;

	Draw_ScreenTexture(gui_texture, &texture_region, (x_position - 160) + (40 * hotbar->active_index), y_position + 1, scale, scale, 0);

	R_Texture* block_atlas_texture = Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE);

	texture_region.width = 16;
	texture_region.height = 16;

	float xPosOffset = 0;
	for (int i = 0; i < LC_PLAYER_MAX_HOTBAR_SLOTS; i++)
	{
		if (hotbar->blocks[i] == LC_BT__NONE)
		{
			xPosOffset += 40;
			continue;
		}
		LC_Block_Texture_Offset_Data tex_data = LC_BLOCK_TEX_OFFSET_DATA[hotbar->blocks[i]];

		texture_region.x = tex_data.top_face[0] * 16;
		texture_region.y = -tex_data.top_face[1] * 16;

		float alpha = 0.2;

		if (block_amounts[hotbar->blocks[i]] > 0)
		{
			alpha = 1.0;
		}

		Draw_ScreenTextureColored(block_atlas_texture, &texture_region, (x_position - 160) + xPosOffset, y_position, scale, scale, 0, 1, 1, 1, alpha);
		xPosOffset += 20 * scale;
	}

}

void LC_Draw_Inventory(LC_BlockType blocks[21][6], int block_amounts[LC_BT__MAX], LC_Hotbar* const hotbar)
{
	float x_position = LC_BASE_RESOLUTION_WIDTH / 2.0;
	float y_position = LC_BASE_RESOLUTION_HEIGHT - 360;

	//Render inventory
	Draw_ScreenTexture(Resource_get("assets/ui/new_inventory.png", RESOURCE__TEXTURE), NULL, x_position, y_position, 3, 3, 0);

	const int NUM_X_TILES = 20;
	const int NUM_Y_TILES = 5;

	const float X_OFFSET = 100;
	const float Y_OFFSET = 195;

	const float BOX_WIDTH = 50;
	const float BOX_HEIGHT = 50;

	//Render texture block slots
	R_Texture* block_atlas_texture = Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE);

	M_Rect2Df texture_region;
	texture_region.width = 16;
	texture_region.height = 16;

	for (int x = 0; x <= NUM_X_TILES; x++)
	{
		for (int y = 0; y <= NUM_Y_TILES; y++)
		{
			float offseted_x = ((x * 54) + X_OFFSET);
			float offseted_y = ((y * 54) + Y_OFFSET);

			vec2 box[2];
			box[0][0] = offseted_x;
			box[0][1] = offseted_y;

			box[1][0] = offseted_x + BOX_WIDTH;
			box[1][1] = offseted_y + BOX_HEIGHT;

			LC_Block_Texture_Offset_Data tex_data = LC_BLOCK_TEX_OFFSET_DATA[blocks[x][y]];

			texture_region.x = tex_data.top_face[0] * 16;
			texture_region.y = -tex_data.top_face[1] * 16;
			
			float alpha = 0.1;
			if (block_amounts[blocks[x][y]] > 0)
			{
				alpha = 1.0;
			}

			Draw_ScreenTextureColored(block_atlas_texture, &texture_region, offseted_x, offseted_y, 3, 3, 0, 1, 1, 1, alpha);
			
		}
	}

	//Render player's hotbar
	float xPosOffset = 0;
	for (int i = 0; i < LC_PLAYER_MAX_HOTBAR_SLOTS; i++)
	{
		if (hotbar->blocks[i] == LC_BT__NONE)
		{
			xPosOffset += 54;
			continue;
		}
		LC_Block_Texture_Offset_Data tex_data = LC_BLOCK_TEX_OFFSET_DATA[hotbar->blocks[i]];

		texture_region.x = tex_data.top_face[0] * 16;
		texture_region.y = -tex_data.top_face[1] * 16;

		Draw_ScreenTexture(block_atlas_texture, &texture_region, 424 + xPosOffset, 530, 3.0, 3.0, 0);
		xPosOffset += 54;
	}

}

void LC_Draw_Crosshair()
{
	vec2 center_point;
	center_point[0] = LC_BASE_RESOLUTION_WIDTH / 2.0;
	center_point[1] = LC_BASE_RESOLUTION_HEIGHT / 2.0;
	
	R_Texture* crosshair_texture = Resource_get("assets/ui/crosshair.png", RESOURCE__TEXTURE);
	Draw_ScreenTexture(crosshair_texture, NULL, center_point[0], center_point[1], 1.5, 2, 0);
}
