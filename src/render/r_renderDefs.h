#pragma once

#include <cglm/cglm.h>
#include "r_texture.h"

typedef struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 tex_coords;
	uint8_t tex_offset[2];
	float hp;
} Vertex;

typedef struct ModelVertex
{
	vec3 position;
	vec3 normal;
	vec2 tex_coords;
	vec3 tangent;
	vec3 bitangent;
}  ModelVertex;

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

typedef struct ScreenVertex
{
	vec3 position;
	vec2 tex_coords;
	float tex_index;
} ScreenVertex;

typedef struct BitmapTextVertex
{
	vec2 position;
	vec2 tex_coords;
	vec4 color;
} BitmapTextVertex;

typedef struct TextureData
{
	R_Texture texture;
	char path[256];
	char type[256];

} TextureData;

typedef enum R_MSSA_Setting
{
	R_MSSA_NONE = 0,
	R_MSSA_2 = 2,
	R_MSSA_4 = 4,
	R_MSSA_8 = 8

} R_MSSA_Setting;

typedef enum R_ShadowMapResolution
{
	R_SM__256 = 256,
	R_SM__512 = 512,
	R_SM__1024 = 1024,
	R_SM__2048 = 2048,
	R_SM__4096 = 4096

} R_ShadowMapResolution;

typedef enum R_RenderFlags
{
	RF__ENABLE_SHADOW_MAPPING = 1 << 0
} R_RenderFlags;

typedef struct R_RenderSettings
{
	int flags;
	R_MSSA_Setting mssa_setting;
	R_ShadowMapResolution shadow_map_resolution;

} R_RenderSettings;