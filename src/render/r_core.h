#pragma once

#include <threads.h>
#include <cglm/cglm.h>
#include <glad/glad.h>

#include "r_sprite.h"
#include "r_shader.h"
#include "core/c_cvars.h"

/*
* ~~~~~~~~~~~~~~~~~~~~
	VERTEX
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct ScreenVertex
{
	vec2 position;
	vec2 tex_coords;
	float tex_index;
} ScreenVertex;

typedef struct LineVertex
{
	vec3 position;
	vec4 color;
} LineVertex;

typedef struct TextVertex
{
	vec2 position;
	vec2 tex_coords;
	vec4 color;
} TextVertex;
/*
* ~~~~~~~~~~~~~~~~~~~~
	COMMANDS
* ~~~~~~~~~~~~~~~~~~~
*/

typedef enum
{
	R_CMD_PT__SCREEN, //render using screen coordinates
	R_CMD_PT__2D, //render using 2d world coordinates
	R_CMD_PT__3D, //render using 3d world coordinates
	R_CMD_PT__BILLBOARD_3D //render 2d sprites in 3d that always face the came
}  R_CMD_ProjectionType;

typedef enum
{
	R_CMD_PM__FULL, //draw the request
	R_CMD_PM__WIRES //draw in wireframe mode
} R_CMD__PolygonMode;

typedef struct R_CMD_DrawSprite
{
	R_CMD_ProjectionType proj_type;
	R_Sprite* sprite_ptr;
} R_CMD_DrawSprite;

typedef struct  R_CMD_DrawAABB
{
	R_CMD__PolygonMode polygon_mode;
	AABB aabb;
	vec4 color;
} R_CMD_DrawAABB;

typedef struct R_CMD_DrawText
{
	R_CMD_ProjectionType proj_type;
	const char* text;
	vec3 position;
	vec3 size;
	vec4 color;
	
} R_CMD_DrawText;

#define MAX_COMMANDS_PER_CMD_BUFFER 1024
typedef struct R_CMD_Buffers
{
	R_CMD_DrawSprite sprite_buf[MAX_COMMANDS_PER_CMD_BUFFER];
	R_CMD_DrawAABB aabb_buf[MAX_COMMANDS_PER_CMD_BUFFER];
	R_CMD_DrawText text_buf[MAX_COMMANDS_PER_CMD_BUFFER];

	int sprite_counter;
	int aabb_counter;
	int text_counter;
} R_CMD_Buffers;

/*
* ~~~~~~~~~~~~~~~~~~~~
	DRAWING DATA
* ~~~~~~~~~~~~~~~~~~~
*/
#define SCREEN_QUAD_VERTICES_BUFFER_SIZE 1024
#define SCREEN_QUAD_MAX_TEXTURES_IN_ARRAY 32

typedef struct R_ScreenQuadDrawData
{
	R_Shader shader;
	ScreenVertex vertices[SCREEN_QUAD_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	size_t indices_count;
	unsigned tex_array[SCREEN_QUAD_MAX_TEXTURES_IN_ARRAY];
	unsigned white_texture;
	int tex_index;
	unsigned vao, vbo, ebo;
} R_ScreenQuadDrawData;

#define LINE_VERTICES_BUFFER_SIZE 1024

typedef struct R_LineDrawData
{
	R_Shader shader;
	LineVertex vertices[LINE_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	unsigned vao, vbo;
} R_LineDrawData;

#define TEXT_VERTICES_BUFFER_SIZE 1024

typedef struct R_TextDrawData
{
	R_Shader shader;
	TextVertex vertices[TEXT_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	size_t indices_count;
	unsigned vao, vbo, ebo;
} R_TextDrawData;

typedef struct R_DrawData
{
	R_ScreenQuadDrawData screen_quad;
	R_LineDrawData lines;
	R_TextDrawData text;
} R_DrawData;

/*
* ~~~~~~~~~~~~~~~~~~~~
	BACKEND DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	thrd_t render_thread;
	bool render_thread_active;
	bool skip_frame;

} R_BackendData;

/*
* ~~~~~~~~~~~~~~~~~~~~
	CVARS DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	C_Cvar* r_multithread;
	C_Cvar* r_limitFPS;
	C_Cvar* r_maxFPS;
	C_Cvar* r_useVsync;
	C_Cvar* r_mssaLevel;
	C_Cvar* r_useDirShadowMapping;
	C_Cvar* r_DirShadowMapResolution;
} R_Cvars;

/*
* ~~~~~~~~~~~~~~~~~~~~
	METRICS DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	float previous_render_time;
	float previous_fps_timed_time;
	float frame_ticks_per_second;
	int fps;
} R_Metrics;

/*
* ~~~~~~~~~~~~~~~~~~~~
	INIT AND CLEANUP FUNCTIONS
* ~~~~~~~~~~~~~~~~~~~
*/
