#ifndef R_CORE_H
#define R_CORE_H
#pragma once

#include <Windows.h>
#include <assert.h>
#include <string.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include "utility/u_math.h"
#include "utility/u_utility.h"
#include "r_shader.h"
#include "r_model.h"
#include "core/cvar.h"
#include "lc/lc_world.h"
#include "utility/u_object_pool.h"

/*
* ~~~~~~~~~~~~~~~~~~~~
	VERTEX
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	vec3 position;
	vec2 tex_coords;
	unsigned packed_texIndex_Flags;
	uint8_t color[4];
} ScreenVertex;

typedef struct
{
	vec3 position;
	uint8_t color[4];
} BasicVertex;

typedef struct
{
	vec3 position;
	uint8_t color[4];
	vec2 texOffset;
	int texture_index;
} TriangleVertex;

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
	R_Texture* texture;
	M_Rect2Df texture_region;
	vec2 position;
	vec2 scale;
	vec4 color;
	float rotation;
} R_CMD_DrawTexture;

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
	vec3 box[2];
	vec4 color;
} R_CMD_DrawCube;

typedef struct
{
	vec3 box[2];
	vec4 color;
	M_Rect2Df texture_region;
	R_Texture* texture;
} R_CMD_DrawTextureCube;

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

typedef struct
{
	int nullData;
} R_CMD_DrawLCWorld;

typedef enum R_CMD
{
	//DRAWING
	R_CMD__TEXTURE,
	R_CMD__CUBE,
	R_CMD__TEXTURED_CUBE,
	R_CMD__QUAD,
	R_CMD__TEXT,
	R_CMD__LINE,
	R_CMD__TRIANGLE,
	R_CMD__MODEL,
	R_CMD__LCWorld,

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
} RDraw_ScreenQuadData;

#define LINE_VERTICES_BUFFER_SIZE 1024

typedef struct
{
	BasicVertex vertices[LINE_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	unsigned vao, vbo;
	R_Shader shader;
} RDraw_LineData;

#define TRIANGLE_VERTICES_BUFFER_SIZE 1024

typedef struct
{
	TriangleVertex vertices[TRIANGLE_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	unsigned vao, vbo;

	unsigned texture_ids[32];
	int texture_index;
} RDraw_TriangleData;

#define TEXT_VERTICES_BUFFER_SIZE 1024

typedef struct
{
	R_Shader shader;
	TextVertex vertices[TEXT_VERTICES_BUFFER_SIZE];
	size_t vertices_count;
	size_t indices_count;
	unsigned vao, vbo, ebo;
} RDraw_TextData;

typedef struct
{
	unsigned vao, vbo;
	unsigned instance_vbo;
	int instance_count;
	dynamic_array* vertices_buffer;
	R_Shader shader;
	unsigned texture_ids[32];
	int texture_index;
	size_t allocated_size;
} RDraw_CubeData;

typedef struct
{
	bool draw;
	WorldRenderData* world_render_data;

	dynamic_array* shadow_firsts[4];
	dynamic_array* shadow_counts[4];

	dynamic_array* shadow_firsts_transparent[4];
	dynamic_array* shadow_counts_transparent[4];

	dynamic_array* shadow_sorted_chunk_indexes;
	dynamic_array* shadow_sorted_chunk_transparent_indexes;

	int shadow_sorted_chunk_offsets[4];
	int shadow_sorted_chunk_transparent_offsets[4];
} RDraw_LCWorldData;


typedef struct
{	
	R_Shader particle_render_shader;
	R_Shader particle_shadow_map_shader;
	R_Shader particle_depthPrepass_shader;
	R_Shader particle_gBuffer_shader;

	int total_particle_amount;
	int total_emitter_amount;

	unsigned texture_ids[32];
	int texture_index;
	
	int instance_count;

	unsigned vao, vbo;
	unsigned instance_vbo;

	size_t allocated_size;

	dynamic_array* instance_buffer;
} RDraw_ParticleData;

typedef struct
{
	RDraw_LCWorldData lc_world;
	RDraw_ScreenQuadData screen_quad;
	RDraw_LineData lines;
	RDraw_TriangleData triangle;
	RDraw_TextData text;
	RDraw_CubeData cube;
	RDraw_ParticleData particles;
} RDraw_DrawData;

/*
* ~~~~~~~~~~~~~~~~~~~~
	 PASS DATA
* ~~~~~~~~~~~~~~~~~~~
*/

typedef struct
{
	R_Shader depthPrepass_shader;
	R_Shader depthPrepass_semi_transparents_shader;
	R_Shader gBuffer_shader;
	R_Shader shadow_map_shader;
	R_Shader transparents_forward_shader;
	R_Shader chunk_process_shader;
	R_Shader occlussion_boxes_shader;
	R_Shader transparents_shadow_map_shader;
	R_Shader transparents_gBuffer_shader;
	R_Shader water_shader;
} RPass_LCSpecificData;

typedef struct
{
	R_Shader depthPrepass_shader;
	R_Shader gBuffer_shader;
	R_Shader shading_shader;
	unsigned FBO;
	unsigned depth_texture;
	unsigned gNormalMetal_texture;
	unsigned gColorRough_texture;
	unsigned gEmissive_texture;
} RPass_DeferredData;

typedef struct
{
	unsigned FBO;
	unsigned MainSceneColorBuffer;
	R_Shader skybox_shader;
} RPass_MainScene;

typedef struct
{
	R_Shader shader;
	unsigned ao_texture;
	unsigned blurred_ao_texture;
	unsigned noise_texture;
} RPass_AO;

#define BLOOM_MIP_COUNT 6
typedef struct
{
	unsigned FBO;
	unsigned mip_textures[BLOOM_MIP_COUNT];
	vec2 mip_sizes[BLOOM_MIP_COUNT];
} RPass_Bloom;

typedef struct
{
	unsigned dof_texture;
	R_Shader bohek_blur_shader;
} RPass_DOF;

typedef struct
{
	unsigned FBO;
	unsigned RBO;
	unsigned envCubemapTexture;
	unsigned prefilteredCubemapTexture;
	unsigned irradianceCubemapTexture;
	unsigned brdfLutTexture;
	R_Shader equirectangular_to_cubemap_shader;
	R_Shader irradiance_convulution_shader;
	R_Shader prefilter_cubemap_shader;
	R_Shader brdf_shader;
	R_Shader single_image_cubemap_convert_shader;

	mat4 cube_view_matrixes[6];
	mat4 cube_proj;
} RPass_IBL;

typedef struct
{
	R_Shader triangle_3d_shader;
	R_Shader box_blur_shader;
	R_Shader upsample_shader;
	R_Shader copy_shader;
	R_Shader downsample_shader;
	R_Shader depth_downsample_shader;

	unsigned halfsize_fbo;
	unsigned depth_halfsize_texture;
	unsigned normal_halfsize_texture;
}RPass_General;

typedef struct
{
	R_Shader post_process_shader;
	R_Shader debug_shader;
} RPass_ProcessData;

#define SHADOW_CASCADE_LEVELS 4
#define LIGHT_MATRICES_COUNT 5
#define SHADOW_MAP_SIZE 2048
typedef struct
{
	R_Shader depth_shader;
	R_Shader blur_shader;
	unsigned FBO;
	unsigned depth_maps;
	vec4 cascade_levels;
	mat4 cascade_projections[4];
	float split_bias_scales[4];
	float shadow_map_size;

	vec2 shadow_sample_kernels[32];
	int num_shadow_sample_kernels;
	float quality_radius_scale;
} RPass_ShadowMappingData;

typedef struct
{
	RPass_General general;
	RPass_LCSpecificData lc;
	RPass_DeferredData deferred;
	RPass_MainScene scene;
	RPass_AO ao;
	RPass_Bloom bloom;
	RPass_DOF dof;
	RPass_IBL ibl;
	RPass_ProcessData post;
	RPass_ShadowMappingData shadow;
} RPass_PassData;

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

	vec2 screenSize;
	vec2 halfScreenSize;

	bool renderer_display_panel;
	bool renderer_display_metrics;
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
	Cvar* r_useDirShadowMapping;
	Cvar* r_skipOclussionCulling;
	
	Cvar* r_useVolumetricLights;

	//SCREEN EFFECTS
	Cvar* r_useFxaa;
	Cvar* r_useDepthOfField;
	Cvar* r_useSsao;
	Cvar* r_useBloom;
	Cvar* r_useSsr;
	Cvar* r_ssaoBias;
	Cvar* r_ssaoRadius;
	Cvar* r_ssaoStrength;
	Cvar* r_ssaoHalfSize;
	Cvar* r_Gamma;
	Cvar* r_Exposure;
	Cvar* r_bloomStrength;
	Cvar* r_bloomThreshold;
	Cvar* r_bloomSoftThreshold;
	Cvar* r_Brightness;
	Cvar* r_Contrast;
	Cvar* r_Saturation;
	Cvar* r_TonemapMode; // 0 = Reinhard, 1 = Uncharted2 tonemapping, 2 = Aces Filmic 

	//WINDOW SPECIFIC
	Cvar* w_width;
	Cvar* w_height;
	Cvar* w_useVsync;

	Cvar* r_drawSky;

	//CAMERA SPECIFIC
	Cvar* cam_fov;
	Cvar* cam_zNear;
	Cvar* cam_zFar;
	Cvar* cam_mouseSensitivity;

	//SHADOW
	Cvar* r_allowParticleShadows;
	Cvar* r_allowTransparentShadows;
	Cvar* r_shadowNormalBias;
	Cvar* r_shadowBias;
	Cvar* r_shadowQualityLevel; //256 + (level * 256)
	Cvar* r_shadowBlurLevel; 
	Cvar* r_shadowSplits;

	//DEBUG
	Cvar* r_drawDebugTexture; //-1 disabled, 0 = Normal, 1 = Albedo, 2 = Depth, 3 = Metal, 4 = Rough, 5 = AO, 6 = BLOOM
	Cvar* r_wireframe;
	Cvar* r_drawPanel;
} R_Cvars;

/*
* ~~~~~~~~~~~~~~~~~~~~
	METRICS DATA
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	//TIMINGS
	float previous_render_time;
	float previous_fps_timed_time;
	float frame_ticks_per_second;
	float frame_time;
	float prev_frame_time;
	int fps;

	size_t total_render_frame_count;
} R_Metrics;

/*
* ~~~~~~~~~~~~~~~~~~~~
	SCENE STRUCTS
* ~~~~~~~~~~~~~~~~~~~
*/

//Must match GL struct
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
}R_SceneCameraData;

//Must match GL struct
typedef struct
{	
	vec4 dirLightColor;
	vec4 dirLightDirection;
	vec4 fogColor;

	float dirLightAmbientIntensity;
	float dirLightSpecularIntensity;

	float ambientLightInfluence;

	unsigned numPointLights;
	unsigned numSpotLights;
	
	float fogDensity;

	float depthFogCurve;
	float depthFogBegin;
	float depthFogEnd;

	float heightFogCurve;
	float heightFogMin;
	float heightFogMax;

	vec4 shadow_splits;
	vec4 shadow_bias;
	vec4 shadow_normal_bias;
	vec4 shadow_blur_factors;

	int shadow_split_count;

	mat4 shadow_matrixes[4];
}R_SceneData;

#define MAX_CULL_QUERY_BUFFER_ITEMS 5000
#define MAX_CULL_LC_SHADOW_QUERY_BUFFER_ITEMS 5000
typedef struct
{
	AABB_FrustrumCullQueryResult lc_world_frustrum_query_buffer[MAX_CULL_QUERY_BUFFER_ITEMS];
	AABB_FrustrumCullQueryResult lc_world_frustrum_sorted_query_buffer[MAX_CULL_QUERY_BUFFER_ITEMS];

	AABB_FrustrumCullQueryResultTreeData lc_world_frustrum_shadow_sorted_query_buffer[4][MAX_CULL_LC_SHADOW_QUERY_BUFFER_ITEMS];

	int lc_world_shadow_cull_count[4];
	int lc_world_shadow_cull_transparent_count[4];

	int lc_world_in_frustrum_count;

} R_SceneCullData;

#define CLUSTERS_X 16
#define CLUSTERS_Y 9
#define CLUSTERS_Z 16
#define MAX_LIGHTS_PER_CLUSTER 50
typedef struct
{
	int count;
	int offset;
	int light_indices[50];
} ClusterItem;
typedef struct
{
	vec3 cluster_bounding_boxes[CLUSTERS_X * CLUSTERS_Y * CLUSTERS_Z][2];
	vec3 min_max[2];
	uint32_t lights[CLUSTERS_X][CLUSTERS_Y][CLUSTERS_Z];
	ClusterItem light_grid[CLUSTERS_X * CLUSTERS_Y * CLUSTERS_Z];
	uint32_t light_indexes[CLUSTERS_X * CLUSTERS_Y * CLUSTERS_Z * MAX_LIGHTS_PER_CLUSTER];
	int lights_in_clusters;
	AABB_Tree light_tree;
	AABB_LightNodeData frustrum_culled_lights[1000];
	
	int culled_light_count;
} R_ClustersData;

typedef struct
{
	R_SceneCameraData camera;
	R_SceneData scene_data;

	R_SceneCullData cull_data;

	R_ClustersData clusters_data;

	unsigned camera_ubo;
	unsigned scene_ubo;
	unsigned cluster_items_ssbo;
	unsigned light_indexes_ssbo;
	unsigned light_nodes_ssbo;

	bool dirty_cam;
}R_Scene;

/*
* ~~~~~~~~~~~~~~~~~~~~
	GENERIC RESOURCES
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	unsigned quadVAO, quadVBO;
	unsigned cubeVAO, cubeVBO;
} R_RendererResources;

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
	int* particle_update_queue[256];
	int particle_update_index;

	FL_Head* particle_emitter_clients;

	Object_Pool* point_lights_pool;
	
	RenderStorageBuffer instances;
	RenderStorageBuffer point_lights;
	DynamicRenderBuffer mesh_vertices;
	DynamicRenderBuffer mesh_indices;
	RenderStorageBuffer materials;
	RenderStorageBuffer texture_handles;
	RenderStorageBuffer draw_elements_commands;
} R_StorageBuffers;


/*
* ~~~~~~~~~~~~~~~~~~~~
	INTERNAL FUNCTIONS
	r_internal.c
* ~~~~~~~~~~~~~~~~~~~
*/
void RInternal_UpdatePostProcessShader(bool p_recompile);
void RInternal_UpdateDeferredShadingShader(bool p_recompile);
void RInternal_GetShadowQualityData(int p_qualityLevel, int p_blurLevel, float* r_shadowMapSize, float* r_kernels, int* r_numKernels, float* r_qualityRadius);

/*
* ~~~~~~~~~~~~~~~~~~~~
	INTERNAL ENUMS
* ~~~~~~~~~~~~~~~~~~~
*/
typedef enum
{
	RPass__STANDART,
	RPass__DEPTH_PREPASS,
	RPass__GBUFFER,
	RPass__SHADOW_MAPPING_SPLIT1,
	RPass__SHADOW_MAPPING_SPLIT2,
	RPass__SHADOW_MAPPING_SPLIT3,
	RPass__SHADOW_MAPPING_SPLIT4,
	RPass__DEBUG_PASS,
} RenderPassState;



#endif // R_CORE_H
