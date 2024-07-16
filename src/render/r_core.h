#ifndef R_CORE_H
#define R_CORE_H
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
typedef struct
{
	vec3 position;
	vec2 tex_coords;
	float tex_index;
} ScreenVertex;

typedef struct
{
	vec3 position;
	vec4 color;
} BasicVertex;

typedef struct
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 biTangent;
	vec2 tex_coords;
} Vertex;

typedef struct
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
	R_CMD_PM__FULL, //draw the request fully
	R_CMD_PM__WIRES //draw in wireframe mode
} R_CMD__PolygonMode;

typedef struct
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

typedef struct
{
	R_CMD__PolygonMode polygon_mode;
	AABB aabb;
	vec4 color;
} R_CMD_DrawAABB;

typedef struct 
{
	R_CMD_ProjectionType proj_type;
	const char* text;
	vec3 position;
	vec3 size;
	vec4 color;
	
} R_CMD_DrawText;

typedef struct
{
	int particle_id;
	vec3 velocity;
	vec3 position;
} R_CMD_ParticleEmitCustom;

typedef struct
{
	int particle_id;
} R_CMD_ParticleGeneric;

typedef struct R_CMD_DrawModel
{
	R_Model* model;
	R_CMD__PolygonMode polygon_mode;
} R_CMD_DrawModel;

typedef enum R_CMD
{
	//DRAWING
	R_CMD__SPRITE,
	R_CMD__AABB,
	R_CMD__TEXT,
	R_CMD__LINE,
	R_CMD__TRIANGLE,
	R_CMD__MODEL,

	//PARTICLE
	R_CMD__PARTICLE_EMIT,
	R_CMD__PARTICLE_EMIT_CUSTOM,
	R_CMD__PARTICLE_RESTART,
	R_CMD__PARTICLE_PAUSE,
	R_CMD__PARTICLE_STOP
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

typedef struct
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

typedef struct
{
	BasicVertex vertices[LINE_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	unsigned vao, vbo;
	void* buffer_ptr;
} R_LineDrawData;

#define TRIANGLE_VERTICES_BUFFER_SIZE 1024

typedef struct
{
	BasicVertex vertices[TRIANGLE_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	unsigned vao, vbo;
	void* buffer_ptr;
} R_TriangleDrawData;

#define TEXT_VERTICES_BUFFER_SIZE 1024

typedef struct
{
	R_Shader shader;
	TextVertex vertices[TEXT_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	size_t indices_count;
	unsigned vao, vbo, ebo;
	void* buffer_ptr;
} R_TextDrawData;

typedef struct
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
	Cvar* r_HizCullingLevel;

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
	BUFFER STRUCTS
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	mat4 viewProjectionMatrix;
	mat4 view;
	mat4 proj;
	mat4 invView;
	mat4 invProj;
	vec4 frustrum_planes[6];
	vec4 position;
	ivec2 screen_size;

	float z_near;
	float z_far;
}R_CameraBuffer;

typedef struct
{
	int s;
}R_SceneBuffer;

/*
* ~~~~~~~~~~~~~~~~~~~~
	UNIFORM BUFFERS DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	unsigned gl_handle;
	void* data_map;
} R_BufferMapPair;
typedef struct
{
	R_BufferMapPair camera;
	R_BufferMapPair scene;
} R_UniformBuffers;

/*
* ~~~~~~~~~~~~~~~~~~~~
	DRAW COMMAND STRUCTS
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	unsigned count;
	unsigned instanceCount;
	unsigned first;
	unsigned baseInstance;
}  DrawArraysIndirectCommand;
typedef struct
{
	unsigned count;
	unsigned instanceCount;
	unsigned firstIndex;
	int      baseVertex;
	unsigned baseInstance;
} DrawElementsIndirectCommand;
/*
* ~~~~~~~~~~~~~~~~~~~~
	STORAGE BUFFERS DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	RenderStorageBuffer particles;
	RenderStorageBuffer particle_emitters;
	RenderStorageBuffer materials;
	RenderStorageBuffer texture_handles;
	RenderStorageBuffer draw_elements_commands;
} R_StorageBuffers;

#endif // R_CORE_H
