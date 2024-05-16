#pragma once

#include <Windows.h>
#include <assert.h>
#include <string.h>
#include <cglm/cglm.h>
#include <glad/glad.h>

#include "utility/u_utility.h"
#include "r_sprite.h"
#include "r_shader.h"
#include "r_model.h"
#include "core/cvar.h"

/*
* ~~~~~~~~~~~~~~~~~~~~
	VERTEX
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct ScreenVertex
{
	vec3 position;
	vec2 tex_coords;
	float tex_index;
} ScreenVertex;

typedef struct LineVertex
{
	vec3 position;
	vec4 color;
} LineVertex;

typedef struct TriangleVertex
{
	vec3 position;
	vec4 color;
} TriangleVertex;

typedef struct TextVertex
{
	vec3 position;
	vec2 tex_coords;
	vec4 color;
	int effect_flags;
	float tex_index;
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

typedef struct
{
	R_CMD_ProjectionType proj_type;
	vec3 from;
	vec3 to;
	vec4 color;
} R_CMD_DrawLine;

typedef struct
{
	R_CMD_ProjectionType proj_type;
	vec3 points[3];
	vec4 color;
} R_CMD_DrawTriangle;

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

typedef struct R_CMD_DrawModel
{
	R_Model* model;
	R_CMD__PolygonMode polygon_mode;
} R_CMD_DrawModel;

typedef enum R_CMD
{
	R_CMD__SPRITE,
	R_CMD__AABB,
	R_CMD__TEXT,
	R_CMD__LINE,
	R_CMD__TRIANGLE,
	R_CMD__MODEL
} R_CMD;

typedef struct
{
	void* data;
	R_CMD cmd;
} R_CMD_T;

#define MAX_RENDER_COMMANDS 1024
#define RENDER_BUFFER_COMMAND_ALLOC_SIZE MAX_RENDER_COMMANDS * 64 //The multiplier is the byte size of the biggest cmd struct
typedef struct
{
	R_CMD cmd_enums[MAX_RENDER_COMMANDS];
	void* cmds_data;
	void* cmds_ptr;

	size_t byte_count;
	int cmds_counter;
} R_CMD_Buffer;

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
	void* buffer_ptr;
} R_ScreenQuadDrawData;

#define LINE_VERTICES_BUFFER_SIZE 1024

typedef struct R_LineDrawData
{
	R_Shader shader;
	LineVertex vertices[LINE_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	unsigned vao, vbo;
	void* buffer_ptr;
} R_LineDrawData;

#define TRIANGLE_VERTICES_BUFFER_SIZE 1024

typedef struct
{
	R_Shader shader;
	TriangleVertex vertices[TRIANGLE_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	unsigned vao, vbo;
	void* buffer_ptr;
} R_TriangleDrawData;

#define TEXT_VERTICES_BUFFER_SIZE 1024

typedef struct R_TextDrawData
{
	R_Shader shader;
	TextVertex vertices[TEXT_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	size_t indices_count;
	unsigned vao, vbo, ebo;
	void* buffer_ptr;
} R_TextDrawData;


typedef struct R_DrawData
{
	R_ScreenQuadDrawData screen_quad;
	R_LineDrawData lines;
	R_TriangleDrawData triangle;
	R_TextDrawData text;
} R_DrawData;

/*
* ~~~~~~~~~~~~~~~~~~~~
	RENDER PASS DATA
* ~~~~~~~~~~~~~~~~~~~
*/

typedef struct
{
	R_Shader shader;
	unsigned FBO;
	unsigned msFBO, msRBO;
	unsigned ms_color_buffer_texture;
	unsigned color_buffer_texture;
	unsigned depth_texture;
} R_PostProcessData;

#define SHADOW_CASCADE_LEVELS 4
#define LIGHT_MATRICES_COUNT 5
typedef struct
{
	R_Shader depth_shader;
	unsigned FBO;
	unsigned depth_maps;
	unsigned light_space_matrices_ubo;
	float cascade_levels[SHADOW_CASCADE_LEVELS];
} R_ShadowMappingData;

typedef struct
{
	R_PostProcessData post;
	R_ShadowMappingData shadow;
} R_RenderPassData;

/*
* ~~~~~~~~~~~~~~~~~~~~
	BACKEND DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	DWORD id;
	HANDLE handle;

	HANDLE event_active;
	HANDLE event_completed;
	HANDLE event_work_permssion;

	bool boolean_active;

} R_Thread;

typedef struct
{
	R_Thread thread;

	bool skip_frame;

} R_BackendData;

/*
* ~~~~~~~~~~~~~~~~~~~~
	CVARS DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	Cvar* r_multithread;
	Cvar* r_limitFPS;
	Cvar* r_maxFPS;
	Cvar* r_mssaLevel;
	Cvar* r_useDirShadowMapping;
	Cvar* r_DirShadowMapResolution;

	//WINDOW SPECIFIC
	Cvar* w_width;
	Cvar* w_height;
	Cvar* w_useVsync;
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
	BUFFERS DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	RenderStorageBuffer particles;
	RenderStorageBuffer particle_emitters;
} R_StorageBuffers;