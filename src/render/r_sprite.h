#pragma once

#include <cglm/cglm.h>
#include "r_texture.h"
#include "utility/u_math.h"

typedef struct R_Sprite
{
	R_Texture* texture;
	M_Rect2Di texture_region;
	mat4x2 texture_coords;
	mat4 model_matrix;
	
	vec3 position;
	vec2 scale;

	float rotation;

	bool flipped_x;
	bool flipped_y;

} R_Sprite;

void Sprite_Init(R_Sprite* sprite, vec3 position, vec2 scale, float rotation, R_Texture* tex);
void Sprite_setTexture(R_Sprite* sprite, R_Texture* tex);
void Sprite_setTextureRegion(R_Sprite* sprite, M_Rect2Di tex_region);
void Sprite_setPosition(R_Sprite* sprite, vec3 position);
void Sprite_setScale(R_Sprite* sprite, vec2 scale);
void Sprite_getSize(R_Sprite* sprite, vec2 dest);