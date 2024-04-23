#include "r_sprite.h"

#include <assert.h>

void _updateTexCoords(R_Sprite* sprite)
{
	 float rect_width = sprite->texture_region.width;
	 float rect_height = sprite->texture_region.height;

	 float rect_x = sprite->texture_region.x;
	 float rect_y = sprite->texture_region.y;

	if (sprite->flipped_x)
		rect_width = -rect_width;

	if (sprite->flipped_y)
		rect_height = -rect_height;

	const float left = rect_x;
	const float right = rect_x + rect_width;
	const float top = rect_y;
	const float bottom = rect_y + rect_height;

	const int texture_width = sprite->texture->width;
	const int texture_height = sprite->texture->height;

	sprite->texture_coords[0][0] = right / texture_width;
	sprite->texture_coords[0][1] = bottom / texture_height;

	sprite->texture_coords[1][0] = right / texture_width;
	sprite->texture_coords[1][1] = top / texture_height;

	sprite->texture_coords[2][0] = left / texture_width;
	sprite->texture_coords[2][1] = top / texture_height;

	sprite->texture_coords[3][0] = left / texture_width;
	sprite->texture_coords[3][1] = bottom / texture_height;
}

void Sprite_setTexture(R_Sprite* sprite, R_Texture* tex)
{
	sprite->texture = tex;

	sprite->texture_region.width = tex->width;
	sprite->texture_region.height = tex->height;
	sprite->texture_region.x = 0;
	sprite->texture_region.y = 0;

	_updateTexCoords(sprite);

}

void Sprite_setTextureRegion(R_Sprite* sprite, M_Rect2Di tex_region)
{
	assert(sprite->texture || sprite->texture->id > 0&& "Sprite Texture must be set \n");

	sprite->texture_region = tex_region;

	_updateTexCoords(sprite);
}

void Sprite_getSize(R_Sprite* sprite, vec2 dest)
{
	dest[0] = sprite->texture_region.width * sprite->scale[0];
	dest[1] = sprite->texture_region.height * sprite->scale[1];
}
