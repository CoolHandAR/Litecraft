#ifndef R_RENDERER_H
#define R_RENDERER_H

#include <stdbool.h>

#include "r_model.h"

#include <cglm/cglm.h>

#include "r_texture.h"
#include "lc_chunk.h"
#include "r_camera.h"

#include "utility/u_math.h"
#include "lc_world.h"
#include "r_sprite.h"

bool r_Init();

void r_drawLine(vec3 from, vec3 to, vec4 color);
void r_drawLine2(float f_x, float f_y, float f_z, float t_x, float t_y, float t_z, vec4 color);
void r_drawTriangle(vec3 p1, vec3 p2, vec3 p3, vec4 color);
void r_drawTriangle2(float p1_x, float p1_y, float p1_z, float p2_x, float p2_y, float p2_z, float p3_x, float p3_y, float p3_z, vec4 color);
void r_drawAABBWires(AABB box, vec4 color);
void r_drawAABBWires2(vec3 box[2], vec4 color);
void r_drawAABB(AABB box, vec4 color);
void r_drawCube(vec3 size, vec3 pos, vec4 color);

void r_Draw(R_Model* const p_model, vec3 pos);
void r_DrawScreenText(const char* p_string, vec2 p_position, vec2 p_size, vec4 p_color, float h_spacing, float y_spacing);
void r_DrawScreenText2(const char* p_string, float p_x, float p_y, float p_width, float p_height, float p_r, float p_g, float p_b, float p_a, float h_spacing, float y_spacing);
void r_DrawChunk(LC_Chunk* const chunk);
void r_DrawWorldChunks2(LC_World* const world);
void r_Update(R_Camera* const p_cam, ivec2 window_size);
void r_DrawCrosshair();
void r_DrawScreenSprite(vec2 pos, R_Sprite* sprite);

#endif // !R_RENDERER_H
