#pragma once

#include "r_sprite.h"

void r_drawScreenSprite(R_Sprite* const p_sprite);
void r_draw3DSprite(R_Sprite* const p_sprite);
void r_drawBillboardSprite(R_Sprite* const p_sprite);
void r_drawAABBWires1(AABB p_aabb, vec4 p_color);
void r_drawAABB(AABB p_aabb, vec4 p_fillColor);
