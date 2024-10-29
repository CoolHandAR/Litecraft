#include <glad/glad.h>
#include "r_renderer.h"
#include "r_shader.h"

#include "core/c_common.h"
#include <cglm/clipspace/persp_rh_zo.h>


#include "r_renderDefs.h"


#include "lc/lc_player.h"

#include "render/r_font.h"

#include "core/cvar.h"
#include <stdarg.h>
#include "utility/u_utility.h"
#include "r_particle.h"
#include "lc/lc_world.h"
#include <time.h>
#include <stdlib.h>

#define W_WIDTH 1280
#define W_HEIGHT 720

#define SHADOW_MAP_RESOLUTION 1280


#define LIGHT_MATRICES_COUNT 5
#define SHADOW_CASCADE_LEVELS 4

#define BITMAP_TEXT_VERTICES_BUFFER_SIZE 1024
#define LINE_VERTICES_BUFFER_SIZE 16384
#define TRIANGLE_VERTICES_BUFFER_SIZE 8192
#define SCREEN_QUAD_VERTICES_BUFFER_SIZE 8192
#define SCREEN_QUAD_INDICES_BUFFER_SIZE 12288
#define SCREEN_QUAD_MAX_TEXTURES 32
const vec4 DEFAULT_COLOR = { 1, 1, 1, 1 };

extern GLFWwindow* glfw_window;


typedef struct
{
    unsigned fbo;
    unsigned mip_textures[6];
    vec2 mip_sizes[6];
    R_Shader downsample_shader;
    R_Shader upsample_shader;
} BloomRenderData;

typedef struct
{
    R_Texture hdrTexture;
    R_Shader equirectangular_to_cubemap_shader;
    R_Shader irradiance_convulution_shader;
    R_Shader prefilter_shader;
    R_Shader brdf_shader;
    unsigned envCubemapTexture;
    unsigned irradianceCubemapTexture;
    unsigned preFilteredCubemapTexture;
    unsigned brdfLutTexture;
    unsigned captureFBO, captureRBO;
} PBRIraddianceData;

typedef struct
{
    R_Shader cluster_build_shader;
    R_Shader cluster_cull_shader;
    R_Shader cluster_actives_shader;
    R_Shader cluster_compact_shader;

    unsigned clusters_SSBO;
    unsigned lights_SSBO;
    unsigned light_indexes_SSBO;
    unsigned light_grid_SSBO;
    unsigned global_index_count_SSBO;
    unsigned active_clusters_ssbo;
    unsigned global_active_clusters_count_ssbo;
    unsigned unique_active_clusters_data_ssbo;
    RenderStorageBuffer light_buffer;

} ClusteredLightingData;

typedef struct
{
    unsigned reflection_fbo, refraction_fbo;
    unsigned cbuffer_reflection, cbuffer_refraction;
    unsigned dbuffer_reflection, dbuffer_refraction;
    R_Shader shader;
    R_Texture dudv_map;
    R_Texture normal_map;
    float move_factor;
    float wave_speed;
} WaterRenderData;

typedef struct
{
    unsigned flags;
    int albedo_map;
    int metalic_map;
    int roughness_map;
    int normal_map;
    int ao_map;

    float metallic_ratio;
    float roughness_ratio;

    vec4 color;
} GL_Material;

typedef struct
{
    GL_Material gl_material;
} Client_Material;

typedef struct MeshRenderData
{
    vec4 bounding_box[2];
    unsigned material_index;
}  MeshRenderData;
typedef struct RenderRequest
{
    mat4 xform;
    unsigned mesh_index;
} RenderRequest;


typedef struct DrawArraysIndirectCommand
{
    unsigned  count;
    unsigned  instanceCount;
    unsigned  first;
    unsigned  baseInstance;
} DrawElementsIndirectCommand;

typedef  struct {
    unsigned  count;
    unsigned  instanceCount;
    unsigned  firstIndex;
    int  baseVertex;
    unsigned  baseInstance;
} DrawElementsCMD;

typedef struct R_ParticleDrawData
{
    R_Shader transform_shader;
    R_Shader compute_shader;
    R_Shader render_shader;
    RenderStorageBuffer particles;
    RenderStorageBuffer emitters;
    RenderStorageBuffer transforms;
    
    ParticleEmitterClient emitter_clients[4];

    R_ParticleEmitter* emitters_buffer;

    unsigned draw_cmds;
    unsigned atomic_counter_buffer;
    unsigned vao, vbo, ebo;

} R_ParticleDrawData;

typedef struct
{
    unsigned render_requests;
    unsigned mesh_data;
    unsigned element_draW_cmds;
    unsigned vao, vbo, ebo;
    unsigned last_request_index;
    unsigned process_shader;
    unsigned tranforms;
    unsigned textures_ssbo;
    unsigned materials_ssbo;
} R_MeshRenderData;


typedef struct R_ScreenQuadDrawData
{
    R_Shader shader;
    ScreenVertex screen_quad_vertices[SCREEN_QUAD_VERTICES_BUFFER_SIZE];
    size_t vertices_count;
    size_t indices_count;
    unsigned tex_array[SCREEN_QUAD_MAX_TEXTURES];
    unsigned white_texture;
    int tex_index;
    unsigned vao, vbo, ebo;
    unsigned indices_offset;
} R_ScreenQuadDrawData;

typedef struct R_BitmapTextDrawData
{
    R_Shader shader;
    BitmapTextVertex vertices[BITMAP_TEXT_VERTICES_BUFFER_SIZE];
    size_t vertices_count;
    size_t indices_count;
    unsigned vao, vbo, ebo;
} R_BitmapTextDrawData;

typedef struct R_ShadowMapData
{
    unsigned frame_buffer;
    unsigned light_depth_maps;

    unsigned light_space_matrices_ubo;

    R_Shader shadow_depth_shader;
    vec4 shadow_cascade_levels;

} R_ShadowMapData;



typedef struct R_DeferredShadingData
{
    R_Shader g_shader;
    unsigned g_fbo;
    unsigned g_position, g_normal, g_colorSpec;
    unsigned g_depth_buffer;

    R_Shader lighting_shader;

    unsigned screen_quad_vao, screen_quad_vbo, screen_quad_ebo;
    unsigned indexed_screen_quad_vao, indexed_screen_quad_vbo, indexed_screen_quad_ebo;

    R_Shader light_box_shader;
    unsigned light_box_vao, light_box_vbo;

    R_Shader fbo_debug_shader;
    
    R_Shader simple_lighting_shader;

    unsigned depth_mapFBO;
    unsigned depth_map;

    R_Shader shadow_depth_shader;

    unsigned SSBO;
    
    unsigned QUERY;

    unsigned color_buffer_texture;
    unsigned bloom_threshold_texture;
    unsigned depth_map_texture;
    

    R_Shader hi_z_shader;
    unsigned frame_buffer;

    R_Shader depth_visual_shader;

    R_Shader cull_shader;

    unsigned cull_vao;
    unsigned cull_vbo;

    R_Shader post_process_shader;

    unsigned transform_feedback_buffer;
    unsigned transform_feedback_buffer2;

    R_Shader cull_compute_shader;

    unsigned bounds_ssbo;

    unsigned sync;

    bool use_hi_z_cull;

    unsigned draw_buffer;

    unsigned chunks_info_ssbo;

    unsigned atomic_counter;

    R_Shader bounding_box_cull_shader;

    R_Shader hiz_debug_shader;

    unsigned cull_test_vao;

    R_Shader frustrum_select_shader;

    R_Shader downsample_shader;

    unsigned camera_info_ubo;

    unsigned draw_buffer_disoccluded;

    unsigned atomic_counter_disoccluded;

    unsigned instanced_positions_vbo;

    unsigned light_depth_maps;

   unsigned light_space_matrixes_ubo;

    R_Shader skybox_shader;

    unsigned chunk_block_info_ubo;

    R_Shader model_shader;

} R_DeferredShadingData;


typedef struct R_GeneralRenderData
{
    mat4 orthoProj;
    unsigned camera_buffer;
    R_Camera* camera;
    vec4 camera_frustrum[6];
    int window_width;
    int window_height;
    float window_scale_width;
    float window_scale_height;
    
    R_Shader depth_prepass_shader;
    R_Shader reflected_uv_shader;

    R_Shader FXAA_shader;
    R_Shader gaussian_blur_shader;

    R_Shader deffered_compute_shader;
    R_Shader deffered_lighting_shader;
    R_Shader ssao_pass_deffered_shader;
    R_Shader ssao_pass_shader;
    R_Shader ssao_blur_shader;
    R_Shader ssao_compute_shader;

    R_Shader dof_shader;
    R_Shader box_blur_shader;

    unsigned dof_fbo;
    unsigned dof_unfocus_texture;

    unsigned ssao_noise_texture;

    unsigned ssaoFBO, ssaoColorBuffer;
    unsigned ssaoBlurFBO, ssaoBlurColorBuffer;

    unsigned reflectedUV_FBO, reflectedUV_ColorBuffer;

    unsigned msFBO, FBO;
    unsigned msRBO;
    unsigned color_buffer_multisampled_texture;
    unsigned color_buffer_texture;
    unsigned depth_texture;
    unsigned hdrFBO;
    unsigned hdrColorBuffer;
    unsigned hdrRBO;

} R_GeneralRenderData;

typedef struct
{
    R_Texture albedo;
    R_Texture metallic;
    R_Texture normal;
    R_Texture roughness;
} PBR_TEXTURES;

R_RenderSettings render_settings;
static BloomRenderData bloom_data;
static PBRIraddianceData irradiance_data;
static ClusteredLightingData clustered_data;
static R_ParticleDrawData s_particleDrawData;
static R_FontData s_fontData;
static R_BitmapTextDrawData s_bitmapTextData;
static R_ShadowMapData s_shadowMapData;
static R_ScreenQuadDrawData s_screenQuadDrawData;
static R_DeferredShadingData s_deferredShadingData;
static R_GeneralRenderData s_RenderData;
static R_Texture tex_atlas;
static R_Texture tex_atlas_normal;
static R_Texture tex_atlas_specular;
static R_Texture tex_atlas_mer;
static R_Texture cubemap_texture;
static R_Texture bitmap_texture;
static R_Texture dirst_texture;
static R_Model model;
static R_Model cube_model;
static R_Model sponza_model;
static R_Model sphere_model;
static R_Texture particle_spritesheet_texture;
static R_MeshRenderData mesh_Render_data;
static PBR_TEXTURES pbr_textures;
static WaterRenderData water_data;
mat4 s_view_proj;
Cvar* r_mssaLevel;
Cvar* r_shadowMapping;

bool use_hdr;

static PointLight s_lightDataBuffer[32];
int s_lightDataIndex;



void r_onWindowResize(ivec2 window_size)
{
    s_RenderData.window_width = window_size[0];
    s_RenderData.window_height = window_size[1];

    //update the ortho proj matrix
    glm_ortho(0.0f, s_RenderData.window_width, s_RenderData.window_height, 0.0f, -1.0f, 1.0f, s_RenderData.orthoProj);
    s_RenderData.window_scale_width = (float)s_RenderData.window_width / (float)W_WIDTH;
    s_RenderData.window_scale_height = (float)s_RenderData.window_height / (float)W_HEIGHT;
    
    //Update framebuffer textures
    if (render_settings.mssa_setting != R_MSSA_NONE)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, s_RenderData.color_buffer_multisampled_texture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, r_mssaLevel->int_value, GL_RGB, s_RenderData.window_width, s_RenderData.window_height, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, s_RenderData.msRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, r_mssaLevel->int_value, GL_DEPTH24_STENCIL8, s_RenderData.window_width, s_RenderData.window_height);
    }
   
    glBindTexture(GL_TEXTURE_2D, s_RenderData.color_buffer_texture);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, s_RenderData.window_width, s_RenderData.window_height, 0,
      //  GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, s_RenderData.depth_texture);
   // glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, s_RenderData.window_width, s_RenderData.window_height, 0,
     //   GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

    glUseProgram(s_screenQuadDrawData.shader);
    Shader_SetVector2f(s_screenQuadDrawData.shader, "u_windowScale", s_RenderData.window_scale_width, s_RenderData.window_scale_height);
    Shader_SetMatrix4(s_screenQuadDrawData.shader, "u_projection", s_RenderData.orthoProj);

    glUseProgram(s_bitmapTextData.shader);
    Shader_SetVector2f(s_bitmapTextData.shader, "u_windowScale", s_RenderData.window_scale_width, s_RenderData.window_scale_height);
    Shader_SetMatrix4(s_bitmapTextData.shader, "u_orthoProjection", s_RenderData.orthoProj);
}

static void _initBloomData()
{
    memset(&bloom_data, 0, sizeof(bloom_data));

    bloom_data.upsample_shader = Shader_CompileFromFile("src/shaders/sample_common.vs", "src/shaders/upsample_bloom.fs", NULL);
    assert(bloom_data.upsample_shader > 0);
    bloom_data.downsample_shader = Shader_CompileFromFile("src/shaders/sample_common.vs", "src/shaders/downsample_bloom.fs", NULL);
    assert(bloom_data.downsample_shader > 0);

    glUseProgram(bloom_data.upsample_shader);
    Shader_SetInteger(bloom_data.upsample_shader, "srcTexture", 0);

    glUseProgram(bloom_data.downsample_shader);
    Shader_SetInteger(bloom_data.downsample_shader, "srcTexture", 0);

    glGenFramebuffers(1, &bloom_data.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, bloom_data.fbo);

    float mipWidth = W_WIDTH;
    float mipHeight = W_HEIGHT;

    for (int i = 0; i < 6; i++)
    {
        mipWidth *= 0.5;
        mipHeight *= 0.5;

        glGenTextures(1, &bloom_data.mip_textures[i]);
        glBindTexture(GL_TEXTURE_2D, bloom_data.mip_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mipWidth, mipHeight, 0, GL_RGB, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        bloom_data.mip_sizes[i][0] = mipWidth;
        bloom_data.mip_sizes[i][1] = mipHeight;
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, bloom_data.mip_textures[0], 0);

    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }
 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void _initIrradianceData()
{
    memset(&irradiance_data, 0, sizeof(irradiance_data));
    //load hdr texture
    irradiance_data.hdrTexture = HDRTexture_Load("assets/cubemaps/hdr/sky.hdr", NULL);

    //load shaders
    irradiance_data.equirectangular_to_cubemap_shader = Shader_CompileFromFile("src/shaders/cubemap.vs", "src/shaders/equirectangular_to_cubemap.fs", NULL);
    assert(irradiance_data.equirectangular_to_cubemap_shader > 0);

    irradiance_data.irradiance_convulution_shader = Shader_CompileFromFile("src/shaders/cubemap.vs", "src/shaders/irradiance_convolution.fs", NULL);
    assert(irradiance_data.irradiance_convulution_shader > 0);
    
    irradiance_data.prefilter_shader = Shader_CompileFromFile("src/shaders/cubemap.vs", "src/shaders/prefilter_cubemap.fs", NULL);
    assert(irradiance_data.prefilter_shader > 0);

    irradiance_data.brdf_shader = Shader_CompileFromFile("src/shaders/brdf.vs", "src/shaders/brdf.fs", NULL);
    assert(irradiance_data.brdf_shader > 0);

    glGenTextures(1, &irradiance_data.envCubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_data.envCubemapTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &irradiance_data.irradianceCubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_data.irradianceCubemapTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &irradiance_data.preFilteredCubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_data.preFilteredCubemapTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glGenTextures(1, &irradiance_data.brdfLutTexture);
    glBindTexture(GL_TEXTURE_2D, irradiance_data.brdfLutTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &irradiance_data.captureFBO);
    glGenRenderbuffers(1, &irradiance_data.captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, irradiance_data.captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, irradiance_data.captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, irradiance_data.captureRBO);


    //Convert hdr map to a cubemap
    mat4 captureProj;
    glm_perspective(glm_rad(90.0), 1.0, 0.1, 10.0, captureProj);
    glUseProgram(irradiance_data.equirectangular_to_cubemap_shader);
    Shader_SetInteger(irradiance_data.equirectangular_to_cubemap_shader, "equirectangularMap", 0);
    Shader_SetMatrix4(irradiance_data.equirectangular_to_cubemap_shader, "u_proj", captureProj);
    vec3 zero, center, up;
    glm_vec3_zero(zero);
    glm_vec3_zero(center);
    glm_vec3_zero(up);

    mat4 viewMatrixes[6];

    center[0] = 1.0;
    up[1] = -1.0;
    glm_lookat(zero, center, up, viewMatrixes[0]);
    center[0] = -1.0;
    glm_lookat(zero, center, up, viewMatrixes[1]);
    center[0] = 0.0;
    center[1] = 1.0;
    up[1] = 0;
    up[2] = 1.0;
    glm_lookat(zero, center, up, viewMatrixes[2]);
    center[1] = -1.0;
    up[2] = -1.0;
    glm_lookat(zero, center, up, viewMatrixes[3]);
    center[1] = 0;
    center[2] = 1.0;
    up[1] = -1.0;
    up[2] = 0;
    glm_lookat(zero, center, up, viewMatrixes[4]);
    center[2] = -1.0;
    glm_lookat(zero, center, up, viewMatrixes[5]);

    glBindTextureUnit(0, irradiance_data.hdrTexture.id);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, irradiance_data.captureFBO);
    
    glBindVertexArray(s_deferredShadingData.light_box_vao);
    for (int i = 0; i < 6; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance_data.envCubemapTexture, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        

        Shader_SetMatrix4(irradiance_data.equirectangular_to_cubemap_shader, "u_view", viewMatrixes[i]);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    //Cubemap to covluted irradiance map
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    glUseProgram(irradiance_data.irradiance_convulution_shader);
    Shader_SetInteger(irradiance_data.irradiance_convulution_shader, "environmentMap", 0);
    Shader_SetMatrix4(irradiance_data.irradiance_convulution_shader, "u_proj", captureProj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_data.envCubemapTexture);

    glViewport(0, 0, 32, 32);
    for (int i = 0; i < 6; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance_data.irradianceCubemapTexture, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        Shader_SetMatrix4(irradiance_data.irradiance_convulution_shader, "u_view", viewMatrixes[i]);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    //prefilter cubemap
    glUseProgram(irradiance_data.prefilter_shader);
    Shader_SetInteger(irradiance_data.prefilter_shader, "environmentMap", 0);
    Shader_SetMatrix4(irradiance_data.prefilter_shader, "u_proj", captureProj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_data.envCubemapTexture);

    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = (unsigned)(128 * pow(0.5, mip));
        unsigned int mipHeight = (unsigned)(128 * pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, irradiance_data.captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        Shader_SetFloat(irradiance_data.prefilter_shader, "roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            Shader_SetMatrix4(irradiance_data.prefilter_shader, "u_view", viewMatrixes[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance_data.preFilteredCubemapTexture, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    //bdrf texture generation
    glUseProgram(irradiance_data.brdf_shader);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, irradiance_data.brdfLutTexture, 0);
    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.screen_quad_vbo);
    glDisable(GL_BLEND);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void _initClusteredData()
{
    memset(&clustered_data, 0, sizeof(clustered_data));

    clustered_data.cluster_build_shader = ComputeShader_CompileFromFile("src/shaders/cluster/cluster_build.comp");
    clustered_data.cluster_cull_shader = ComputeShader_CompileFromFile("src/shaders/cluster/cluster_cull.comp");
    clustered_data.cluster_actives_shader = ComputeShader_CompileFromFile("src/shaders/cluster/cluster_actives.comp");
    clustered_data.cluster_compact_shader = ComputeShader_CompileFromFile("src/shaders/cluster/cluster_compact.comp");

    glGenBuffers(1, &clustered_data.clusters_SSBO);
    glGenBuffers(1, &clustered_data.lights_SSBO);
    glGenBuffers(1, &clustered_data.light_indexes_SSBO);
    glGenBuffers(1, &clustered_data.light_grid_SSBO);
    glGenBuffers(1, &clustered_data.global_index_count_SSBO);
    glGenBuffers(1, &clustered_data.active_clusters_ssbo);
    glGenBuffers(1, &clustered_data.global_active_clusters_count_ssbo);
    glGenBuffers(1, &clustered_data.unique_active_clusters_data_ssbo);

    clustered_data.light_buffer = RSB_Create(1000, sizeof(PointLight), RSB_FLAG__POOLABLE | RSB_FLAG__RESIZABLE | RSB_FLAG__WRITABLE);
    

    int num_clusters = 3456;
    unsigned total_num_lights = num_clusters * 1000;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.lights_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PointLight) * 1000, NULL, GL_STATIC_DRAW);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.light_grid_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * num_clusters, NULL, GL_STREAM_DRAW);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.clusters_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 32 * num_clusters, NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.light_indexes_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * total_num_lights, NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.global_index_count_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned), NULL, GL_STREAM_DRAW);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.active_clusters_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 32 * num_clusters, NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.global_active_clusters_count_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned), NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, clustered_data.unique_active_clusters_data_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 32 * num_clusters, NULL, GL_STREAM_DRAW);


    glUseProgram(clustered_data.cluster_actives_shader);
    Shader_SetInteger(clustered_data.cluster_actives_shader, "depth_texture", 0);
}

#define WATER_REFLECTION_WIDTH 1024
#define WATER_REFLECTION_HEIGHT 720
#define WATER_REFRACTION_WIDTH 1024
#define WATER_REFRACTION_HEIGHT 720
static void _initWaterRenderData()
{
    memset(&water_data, 0, sizeof(water_data));

    water_data.wave_speed = 0.02;

    water_data.shader = Shader_CompileFromFile("src/shaders/water_shader.vs", "src/shaders/water_shader.fs", NULL);
    assert(water_data.shader > 0);

    water_data.dudv_map = Texture_Load("assets/water/waterDUDV.png", NULL);
 
    water_data.normal_map = Texture_Load("assets/water/normal_map.png", NULL);

    glBindTexture(GL_TEXTURE_2D, water_data.dudv_map.id);


    glUseProgram(water_data.shader);
    Shader_SetInteger(water_data.shader, "reflection_texture", 0);
    Shader_SetInteger(water_data.shader, "refraction_texture", 1);
    Shader_SetInteger(water_data.shader, "dudv_map", 2);
    Shader_SetInteger(water_data.shader, "normal_map", 3);
    Shader_SetInteger(water_data.shader, "refraction_depth_map", 4);

    glGenFramebuffers(1, &water_data.reflection_fbo);
    glGenFramebuffers(1, &water_data.refraction_fbo);
    glGenTextures(1, &water_data.cbuffer_reflection);
    glGenTextures(1, &water_data.cbuffer_refraction);
    glGenTextures(1, &water_data.dbuffer_reflection);
    glGenTextures(1, &water_data.dbuffer_refraction);

    //WATER REFLECTION
    glBindFramebuffer(GL_FRAMEBUFFER, water_data.reflection_fbo);
    glBindTexture(GL_TEXTURE_2D, water_data.cbuffer_reflection);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WATER_REFLECTION_WIDTH, WATER_REFLECTION_HEIGHT, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, water_data.dbuffer_reflection);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WATER_REFLECTION_WIDTH, WATER_REFLECTION_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, water_data.cbuffer_reflection, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, water_data.dbuffer_reflection, 0);

    //WATER REFRACTION
    glBindFramebuffer(GL_FRAMEBUFFER, water_data.refraction_fbo);
    glBindTexture(GL_TEXTURE_2D, water_data.cbuffer_refraction);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WATER_REFRACTION_WIDTH, WATER_REFRACTION_HEIGHT, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, water_data.dbuffer_refraction);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WATER_REFRACTION_WIDTH, WATER_REFRACTION_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, water_data.cbuffer_refraction, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, water_data.dbuffer_refraction, 0);
}

static void _initMeshRenderData()
{
    memset(&mesh_Render_data, 0, sizeof(mesh_Render_data));

    mesh_Render_data.process_shader = ComputeShader_CompileFromFile("src/shaders/mesh_batching.comp");
    assert(mesh_Render_data.process_shader > 0);
    

    glGenBuffers(1, &mesh_Render_data.mesh_data);
    glGenBuffers(1, &mesh_Render_data.element_draW_cmds);
    glGenBuffers(1, &mesh_Render_data.render_requests);
    glGenBuffers(1, &mesh_Render_data.vbo);
    glGenBuffers(1, &mesh_Render_data.ebo);
    glGenBuffers(1, &mesh_Render_data.tranforms);
    glGenBuffers(1, &mesh_Render_data.textures_ssbo);
    glGenBuffers(1, &mesh_Render_data.materials_ssbo);
    glGenVertexArrays(1, &mesh_Render_data.vao);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_Render_data.textures_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * 15, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_Render_data.mesh_data);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(MeshRenderData) * 400, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_Render_data.element_draW_cmds);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DrawElementsCMD) * 400, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_Render_data.render_requests);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(RenderRequest) * 400, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_Render_data.materials_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GL_Material) * 15, NULL, GL_STATIC_DRAW);

    glBindVertexArray(mesh_Render_data.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh_Render_data.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ModelVertex) * 100000, NULL, GL_STATIC_DRAW);
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_Render_data.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * 10000, NULL, GL_STATIC_DRAW);

    const int STRIDE_SIZE = sizeof(ModelVertex);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, tex_coords)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, tangent)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, bitangent)));
    glEnableVertexAttribArray(4);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_Render_data.tranforms);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * 1000, NULL, GL_STATIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, mesh_Render_data.mesh_data);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, mesh_Render_data.element_draW_cmds);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, mesh_Render_data.render_requests);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, mesh_Render_data.tranforms);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 26, mesh_Render_data.textures_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 27, mesh_Render_data.materials_ssbo);
}



static void _initShadowMapData()
{   
    memset(&s_shadowMapData, 0, sizeof(R_ShadowMapData));

    //SETUP SHADER
    s_shadowMapData.shadow_depth_shader = Shader_CompileFromFile("src/shaders/shadow_depth_shader.vs", "src/shaders/shadow_depth_shader.fs", "src/shaders/shadow_depth_shader.gs");
    assert(s_shadowMapData.shadow_depth_shader > 0);

    //SETUP FRAMEBUFFER
    glGenFramebuffers(1, &s_shadowMapData.frame_buffer);

   
    
    glGenTextures(1, &s_shadowMapData.light_depth_maps);
    glBindTexture(GL_TEXTURE_2D_ARRAY, s_shadowMapData.light_depth_maps);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT16, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, SHADOW_CASCADE_LEVELS + 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, s_shadowMapData.frame_buffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_shadowMapData.light_depth_maps, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //MATRICES UBO
    glGenBuffers(1, &s_shadowMapData.light_space_matrices_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, s_shadowMapData.light_space_matrices_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * LIGHT_MATRICES_COUNT, NULL, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, s_shadowMapData.light_space_matrices_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
   
    float z_far = 1500;

    s_shadowMapData.shadow_cascade_levels[0] = z_far / 75.0f;
    s_shadowMapData.shadow_cascade_levels[1] = z_far / 50.0f;
    s_shadowMapData.shadow_cascade_levels[2] = z_far / 20.0f;
    s_shadowMapData.shadow_cascade_levels[3] = z_far / 15.0f;
}

static void _initBitmapTextDrawData()
{
    memset(&s_bitmapTextData, 0, sizeof(R_BitmapTextDrawData));

    s_bitmapTextData.shader = Shader_CompileFromFile("src/shaders/bitmap_text.vs", "src/shaders/bitmap_text.fs", NULL);
    assert(s_bitmapTextData.shader > 0);

    glUseProgram(s_bitmapTextData.shader);
    Shader_SetInteger(s_bitmapTextData.shader, "font_atlas_texture", 0);

    glGenVertexArrays(1, &s_bitmapTextData.vao);
    glGenBuffers(1, &s_bitmapTextData.vbo);
    glGenBuffers(1, &s_bitmapTextData.ebo);

    glBindVertexArray(s_bitmapTextData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_bitmapTextData.vbo);

    glBufferStorage(GL_ARRAY_BUFFER, sizeof(s_bitmapTextData.vertices), NULL, GL_DYNAMIC_STORAGE_BIT);

    const size_t indices_count = BITMAP_TEXT_VERTICES_BUFFER_SIZE * 6;

    unsigned* indices = malloc(sizeof(unsigned) * indices_count);

    unsigned offset = 0;
    if (indices)
    {
        for (int i = 0; i < indices_count / sizeof(unsigned); i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_bitmapTextData.ebo);
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices_count, indices, GL_CLIENT_STORAGE_BIT);

        free(indices);
    }


    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BitmapTextVertex), (void*)(offsetof(BitmapTextVertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BitmapTextVertex), (void*)(offsetof(BitmapTextVertex, tex_coords)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(BitmapTextVertex), (void*)(offsetof(BitmapTextVertex, color)));
    glEnableVertexAttribArray(2);
    
}

static void _initParticleData()
{
    memset(&s_particleDrawData, 0, sizeof(R_ParticleDrawData));

    s_particleDrawData.compute_shader = ComputeShader_CompileFromFile("src/shaders/particle_update.comp");
    assert(s_particleDrawData.compute_shader > 0);

    s_particleDrawData.render_shader = Shader_CompileFromFile("src/shaders/particle_render.vs", "src/shaders/particle_render.fs", NULL);
    assert(s_particleDrawData.render_shader);


    glGenVertexArrays(1, &s_particleDrawData.vao);
    glGenBuffers(1, &s_particleDrawData.vbo);
    glGenBuffers(1, &s_particleDrawData.ebo);


    glBindVertexArray(s_particleDrawData.vao);
  

    float quad_vertices[] =
    {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0, 1.0f, 0.0f,
    };
    float quad_vertices2[] =
    {
        // positions        // texture Coords
        -1.0f, -1.0f, 0.0, 0.0f, 0.0f,
         -1.0f,  1.0f, 0.0, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0, 1.0f, 0.0f,
    };

   

    unsigned indices[6];

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_particleDrawData.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * 1000, NULL, GL_STREAM_DRAW);

    typedef struct
    {
        vec3 pos;
        vec3 norm;
        vec2 texCoords;
    } Vert;

    //glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);
    glBindBuffer(GL_ARRAY_BUFFER, s_particleDrawData.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 1000, NULL, GL_STREAM_DRAW);
  //  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad_vertices), quad_vertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, norm));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)offsetof(Vert, texCoords));
    glEnableVertexAttribArray(2);

    s_particleDrawData.particles = RSB_Create(100, sizeof(R_Particle), RSB_FLAG__READABLE | RSB_FLAG__WRITABLE);
    s_particleDrawData.emitters = RSB_Create(4, sizeof(R_ParticleEmitter), RSB_FLAG__READABLE | RSB_FLAG__WRITABLE);
    s_particleDrawData.transforms = RSB_Create(200, sizeof(vec4), RSB_FLAG__READABLE | RSB_FLAG__WRITABLE);
    
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, s_particleDrawData.emitters.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, s_particleDrawData.particles.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, s_particleDrawData.transforms.buffer);

    unsigned init = 0;
    glGenBuffers(1, &s_particleDrawData.atomic_counter_buffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, s_particleDrawData.atomic_counter_buffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned), init, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, s_particleDrawData.atomic_counter_buffer);


    DrawElementsCMD cmd;
    cmd.count = 6;
    cmd.baseInstance = 0;
    cmd.baseVertex = 0;
    cmd.firstIndex = 0;
    cmd.instanceCount = 64;

    glGenBuffers(1, &s_particleDrawData.draw_cmds);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_particleDrawData.draw_cmds);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DrawElementsCMD) * 1, &cmd, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, s_particleDrawData.draw_cmds);
}

static void _initGLStates()
{
    glEnable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void _initGeneralRenderData()
{
    use_hdr = false;

    memset(&s_RenderData, 0, sizeof(R_GeneralRenderData));

    s_RenderData.depth_prepass_shader = Shader_CompileFromFile("src/shaders/depth_prepass.vs", "src/shaders/depth_prepass.fs", NULL);
    assert(s_RenderData.depth_prepass_shader > 0);

    s_RenderData.deffered_compute_shader = ComputeShader_CompileFromFile("src/shaders/deffered_lighting.comp");
    assert(s_RenderData.deffered_compute_shader > 0);
    s_RenderData.deffered_lighting_shader = Shader_CompileFromFile("src/shaders/deffered_lighting.vs", "src/shaders/deffered_lighting_pbr.fs", NULL);
    s_RenderData.ssao_pass_deffered_shader = Shader_CompileFromFile("src/shaders/ssao_deffered.vs", "src/shaders/ssao_deffered.fs", NULL);
    s_RenderData.ssao_pass_shader = Shader_CompileFromFile("src/shaders/ssao.vs", "src/shaders/ssao.fs", NULL);
    s_RenderData.ssao_blur_shader = Shader_CompileFromFile("src/shaders/ssao_blur.vs", "src/shaders/ssao_blur.fs", NULL);
    s_RenderData.ssao_compute_shader = ComputeShader_CompileFromFile("src/shaders/ssao.comp");
    s_RenderData.reflected_uv_shader = Shader_CompileFromFile("src/shaders/reflected_uv.vs", "src/shaders/backup_ssr.fs", NULL);
    assert(s_RenderData.reflected_uv_shader > 0);
    s_RenderData.box_blur_shader = Shader_CompileFromFile("src/shaders/box_blur.vs", "src/shaders/box_blur.fs", NULL);
    s_RenderData.dof_shader = Shader_CompileFromFile("src/shaders/dof_shader.vs", "src/shaders/dof_shader.fs", NULL);
    
    s_RenderData.FXAA_shader = Shader_CompileFromFile("src/shaders/fxaa.vs", "src/shaders/fxaa.fs", NULL);
    assert(s_RenderData.FXAA_shader > 0);

    s_RenderData.gaussian_blur_shader = Shader_CompileFromFile("src/shaders/gaussian_blur.vs", "src/shaders/gaussian_blur.fs", NULL);
    assert(s_RenderData.gaussian_blur_shader > 0);

    //CAMERA UBO
    glGenBuffers(1, &s_RenderData.camera_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, s_RenderData.camera_buffer);
    glBufferStorage(GL_UNIFORM_BUFFER, sizeof(mat4), NULL, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, s_RenderData.camera_buffer);

    //SETUP FRAMEBUFFERS AND RENDERBUFFERS
    glGenFramebuffers(1, &s_RenderData.reflectedUV_FBO);
    glGenFramebuffers(1, &s_RenderData.ssaoFBO);
    glGenFramebuffers(1, &s_RenderData.ssaoBlurFBO);
    glGenFramebuffers(1, &s_RenderData.FBO);
    glGenFramebuffers(1, &s_RenderData.msFBO);
    glGenRenderbuffers(1, &s_RenderData.msRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.msFBO);

    glGenTextures(1, &s_RenderData.color_buffer_multisampled_texture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, s_RenderData.color_buffer_multisampled_texture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, W_WIDTH, W_HEIGHT, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, s_RenderData.color_buffer_multisampled_texture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, s_RenderData.msRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, W_WIDTH, W_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s_RenderData.msRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }
   
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.FBO);
    //COLOR BUFFER TEXTURE
    glGenTextures(1, &s_RenderData.color_buffer_texture);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.color_buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W_WIDTH, W_HEIGHT, 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_RenderData.color_buffer_texture, 0);
    //DEPTH BUFFER TEXTURE
    glGenTextures(1, &s_RenderData.depth_texture);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, W_WIDTH, W_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_RenderData.depth_texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //HDR
    glGenFramebuffers(1, &s_RenderData.hdrFBO);
    glGenRenderbuffers(1, &s_RenderData.hdrRBO);
    glGenTextures(1, &s_RenderData.hdrColorBuffer);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W_WIDTH, W_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.hdrFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, s_RenderData.hdrRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, W_WIDTH, W_HEIGHT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_RenderData.hdrColorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_RenderData.hdrRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //DOF
    glGenFramebuffers(1, &s_RenderData.dof_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.dof_fbo);
    glGenTextures(1, &s_RenderData.dof_unfocus_texture);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.dof_unfocus_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W_WIDTH, W_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_RenderData.dof_unfocus_texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //REFLECTED UV
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.reflectedUV_FBO);
    glGenTextures(1, &s_RenderData.reflectedUV_ColorBuffer);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.reflectedUV_ColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W_WIDTH, W_HEIGHT, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_RenderData.reflectedUV_ColorBuffer, 0);

    glUseProgram(s_RenderData.reflected_uv_shader);
    Shader_SetInteger(s_RenderData.reflected_uv_shader, "depth_texture", 0);
    Shader_SetInteger(s_RenderData.reflected_uv_shader, "g_normal", 1);
    Shader_SetInteger(s_RenderData.reflected_uv_shader, "g_color", 2);

    //SSAO
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.ssaoFBO);
    glGenTextures(1, &s_RenderData.ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, W_WIDTH / 2, W_HEIGHT / 2, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_RenderData.ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        assert(false);

    //SSAO BLUR
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.ssaoBlurFBO);
    glGenTextures(1, &s_RenderData.ssaoBlurColorBuffer);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.ssaoBlurColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, W_WIDTH / 2, W_HEIGHT / 2, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_RenderData.ssaoBlurColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        assert(false);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //SSAO NOISE
    vec3 ssao_noise[16];
    srand(time(NULL));
    for (int i = 0; i < 16; i++)
    {
        ssao_noise[i][0] = ((float)rand() / (float)RAND_MAX) * 2.0 - 1.0;
        ssao_noise[i][1] = ((float)rand() / (float)RAND_MAX) * 2.0 - 1.0;
        ssao_noise[i][2] = 0.0f;
    }

    glGenTextures(1, &s_RenderData.ssao_noise_texture);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.ssao_noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssao_noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUseProgram(s_RenderData.ssao_pass_shader);
    Shader_SetInteger(s_RenderData.ssao_pass_shader, "depth_texture", 0);
    Shader_SetInteger(s_RenderData.ssao_pass_shader, "noise_texture", 1);

    
   
    glUseProgram(s_RenderData.ssao_blur_shader);
    Shader_SetInteger(s_RenderData.ssao_blur_shader, "ssao_texture", 0);

    glUseProgram(s_RenderData.ssao_pass_deffered_shader);
    Shader_SetInteger(s_RenderData.ssao_pass_deffered_shader, "g_position", 0);
    Shader_SetInteger(s_RenderData.ssao_pass_deffered_shader, "g_normal", 0);
    Shader_SetInteger(s_RenderData.ssao_pass_deffered_shader, "noise_texture", 1);
    Shader_SetInteger(s_RenderData.ssao_pass_deffered_shader, "depth_texture", 2);

    glUseProgram(s_RenderData.ssao_compute_shader);
    Shader_SetInteger(s_RenderData.ssao_compute_shader, "g_normal", 0);
    Shader_SetInteger(s_RenderData.ssao_compute_shader, "noise_texture", 1);
    Shader_SetInteger(s_RenderData.ssao_compute_shader, "depth_texture", 2);

    glUseProgram(s_RenderData.deffered_lighting_shader);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "depth_texture", 0);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "gNormal", 1);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "gAlbedo", 2);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "ssao", 3);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "shadowMap", 4);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "irradianceMap", 5);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "preFilterMap", 6);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "brdfLUT", 7);

    glUseProgram(s_RenderData.deffered_compute_shader);
    Shader_SetInteger(s_RenderData.deffered_compute_shader, "depth_texture", 0);
    Shader_SetInteger(s_RenderData.deffered_compute_shader, "gNormal", 1);
    Shader_SetInteger(s_RenderData.deffered_compute_shader, "gAlbedo", 2);
    Shader_SetInteger(s_RenderData.deffered_compute_shader, "ssao", 3);

    glUseProgram(s_RenderData.box_blur_shader);
    Shader_SetInteger(s_RenderData.box_blur_shader, "input_texture", 0);
   
    glUseProgram(s_RenderData.dof_shader);
    Shader_SetInteger(s_RenderData.dof_shader, "depth_texture", 0);
    Shader_SetInteger(s_RenderData.dof_shader, "noise_texture", 1);
    Shader_SetInteger(s_RenderData.dof_shader, "focus_texture", 2);
    Shader_SetInteger(s_RenderData.dof_shader, "unFocus_texture", 3);
}   

static void _initDeferredShadingData()
{
    glGenFramebuffers(1, &s_deferredShadingData.g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.g_fbo);
    // normal color buffer
    glGenTextures(1, &s_deferredShadingData.g_normal);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W_WIDTH, W_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_deferredShadingData.g_normal, 0);
    // color + specular color buffer
    glGenTextures(1, &s_deferredShadingData.g_colorSpec);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_colorSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W_WIDTH, W_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s_deferredShadingData.g_colorSpec, 0);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    //depth
    glGenTextures(1, &s_deferredShadingData.g_depth_buffer);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, W_WIDTH, W_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer, 0);



    glGenFramebuffers(1, &s_deferredShadingData.depth_mapFBO);
    glGenTextures(1, &s_deferredShadingData.light_depth_maps);
    glBindTexture(GL_TEXTURE_2D_ARRAY, s_deferredShadingData.light_depth_maps);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, W_WIDTH, W_HEIGHT, SHADOW_CASCADE_LEVELS + 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.depth_mapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_deferredShadingData.light_depth_maps, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }

    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    //lights ubo
    glGenBuffers(1, &s_deferredShadingData.light_space_matrixes_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, s_deferredShadingData.light_space_matrixes_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * 16, NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, s_deferredShadingData.light_space_matrixes_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    //COMPILE SHADERS
    s_deferredShadingData.g_shader = Shader_CompileFromFile("src/shaders/g_buffer_shader.vs", "src/shaders/g_buffer_shader.fs", NULL);
    assert(s_deferredShadingData.g_shader > 0);

    s_deferredShadingData.lighting_shader = Shader_CompileFromFile("src/shaders/lighting_shader.vs", "src/shaders/lighting_shader.fs", NULL);
    assert(s_deferredShadingData.lighting_shader > 0);

    s_deferredShadingData.light_box_shader = Shader_CompileFromFile("src/shaders/light_box_shader.vs", "src/shaders/light_box_shader.fs", NULL);
    assert(s_deferredShadingData.light_box_shader > 0);

    s_deferredShadingData.simple_lighting_shader = Shader_CompileFromFile("src/shaders/lighting_shader_simple.vs", "src/shaders/lighting_shader_simple.fs", NULL);
    assert(s_deferredShadingData.simple_lighting_shader > 0);

    s_deferredShadingData.shadow_depth_shader = Shader_CompileFromFile("src/shaders/shadow_depth_shader.vs", "src/shaders/shadow_depth_shader.fs", "src/shaders/shadow_depth_shader.gs");
    assert(s_deferredShadingData.shadow_depth_shader > 0);

    s_deferredShadingData.hi_z_shader = Shader_CompileFromFile("src/shaders/hi-z.vs", "src/shaders/hi-z_min.fs", NULL);
    assert(s_deferredShadingData.hi_z_shader > 0);

    s_deferredShadingData.cull_shader = Shader_CompileFromFile("src/shaders/cull2.vs", "src/shaders/cull.fs", NULL);
    assert(s_deferredShadingData.cull_shader > 0);

    s_deferredShadingData.post_process_shader = Shader_CompileFromFile("src/shaders/post_process.vs", "src/shaders/post_process.fs", NULL);
    assert(s_deferredShadingData.post_process_shader > 0);

    s_deferredShadingData.cull_compute_shader = ComputeShader_CompileFromFile("src/shaders/cull_compute2.comp");
    assert(s_deferredShadingData.cull_compute_shader > 0);

    s_deferredShadingData.bounding_box_cull_shader = Shader_CompileFromFile("src/shaders/cull_bound_box.vs", "src/shaders/cull_bound_box.fs", NULL);
    assert(s_deferredShadingData.bounding_box_cull_shader > 0);

    s_deferredShadingData.frustrum_select_shader = ComputeShader_CompileFromFile("src/shaders/frustrum_select.comp");
    assert(s_deferredShadingData.frustrum_select_shader > 0);

    s_deferredShadingData.skybox_shader = Shader_CompileFromFile("src/shaders/skybox.vs", "src/shaders/skybox.fs", NULL);
    assert(s_deferredShadingData.skybox_shader > 0);

    s_deferredShadingData.model_shader = Shader_CompileFromFile("src/shaders/model_shader.vs", "src/shaders/model_shader.fs", NULL);
    assert(s_deferredShadingData.model_shader > 0);

    //s_deferredShadingData.hiz_debug_shader = Shader_CompileFromFile("src/shaders/hiz_debug.vs", "src/shaders/hiz_debug.fs", "src/shaders/hiz_debug.gs");
    //assert(s_deferredShadingData.hiz_debug_shader > 0);
    
    s_deferredShadingData.downsample_shader = Shader_CompileFromFile("src/shaders/downsample.vs", "src/shaders/downsample.fs", NULL);

    //SETUP SHADER
    glUseProgram(s_deferredShadingData.g_shader);
    Shader_SetInteger(s_deferredShadingData.g_shader, "texture_atlas", 0);
    Shader_SetInteger(s_deferredShadingData.g_shader, "texture_atlas_normal", 1);
    Shader_SetInteger(s_deferredShadingData.g_shader, "texture_atlas_mer", 2);


    glUseProgram(s_deferredShadingData.lighting_shader);
    Shader_SetInteger(s_deferredShadingData.lighting_shader, "u_position", 0);
    Shader_SetInteger(s_deferredShadingData.lighting_shader, "u_normal", 1);
    Shader_SetInteger(s_deferredShadingData.lighting_shader, "u_colorSpec", 2);

    glUseProgram(s_deferredShadingData.simple_lighting_shader);
    Shader_SetInteger(s_deferredShadingData.simple_lighting_shader, "atlas_texture", 0);
    Shader_SetInteger(s_deferredShadingData.simple_lighting_shader, "shadowMap", 1);
    Shader_SetInteger(s_deferredShadingData.simple_lighting_shader, "skybox_texture", 2);
    Shader_SetInteger(s_deferredShadingData.simple_lighting_shader, "ssao_texture", 3);
    Shader_SetVector4f(s_deferredShadingData.simple_lighting_shader, "u_Fog.color", 0.6, 0.8, 1.0, 1.0);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_Fog.density", 0.00009);


    glUseProgram(s_deferredShadingData.skybox_shader);
    Shader_SetInteger(s_deferredShadingData.skybox_shader, "skybox_texture", 0);

    glUseProgram(s_deferredShadingData.model_shader);
    Shader_SetInteger(s_deferredShadingData.model_shader, "texture_diffuse1", 0);
    Shader_SetInteger(s_deferredShadingData.model_shader, "skybox_texture", 1);

    glUseProgram(s_deferredShadingData.post_process_shader);
    Shader_SetInteger(s_deferredShadingData.post_process_shader, "ColorBuffer", 0);
    Shader_SetInteger(s_deferredShadingData.post_process_shader, "BloomBuffer", 1);

    //SETUP SCREEN DATA
    float quad_vertices[] = 
    {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
    };

    glGenVertexArrays(1, &s_deferredShadingData.screen_quad_vao);
    glGenBuffers(1, &s_deferredShadingData.screen_quad_vbo);
   
    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.screen_quad_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    float quad_vertices2[] =
    {
        // positions        // texture Coords
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
          1.0f,  1.0f, 1.0f, 1.0f,
          -1.0f,  1.0f, 0.0f, 1.0f,
    };


    unsigned indices2[6];

    indices2[0] = 0;
    indices2[1] = 1;
    indices2[2] = 2;

    indices2[3] = 2;
    indices2[4] = 3;
    indices2[5] = 0;

    glGenVertexArrays(1, &s_deferredShadingData.indexed_screen_quad_vao);
    glGenBuffers(1, &s_deferredShadingData.indexed_screen_quad_vbo);
    glGenBuffers(1, &s_deferredShadingData.indexed_screen_quad_ebo);

    glBindVertexArray(s_deferredShadingData.indexed_screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.indexed_screen_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices2), quad_vertices2, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_deferredShadingData.indexed_screen_quad_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2), indices2, GL_STATIC_DRAW);

    //SETUP LIGHTCUBE DATA
    glGenVertexArrays(1, &s_deferredShadingData.light_box_vao);
    glGenBuffers(1, &s_deferredShadingData.light_box_vbo);

    glBindVertexArray(s_deferredShadingData.light_box_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.light_box_vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);

    vec3 positions[4];
    positions[0][0] = 5.0;
    positions[0][1] = 0.0;
    positions[0][2] = 0.0;

    positions[1][0] = 15.0;
    positions[1][1] = 0.0;
    positions[1][2] = 0.0;

    positions[2][0] = 20.0;
    positions[2][1] = 0.0;
    positions[2][2] = 0.0;

    positions[3][0] = 26.0;
    positions[3][1] = 0.0;
    positions[3][2] = 2.0;
    
    vec3 size;
    size[0] = 16;
    size[1] = 16;
    size[2] = 16;

    mat4 models[4];
    Math_Model(positions[0], size, 0, models[0]);
    Math_Model(positions[1], size, 0, models[1]);
    Math_Model(positions[2], size, 0, models[2]);
    Math_Model(positions[3], size, 0, models[3]);

    vec3 CUBE_POSITION_VERTICES[] =
    {
        //back face
        -0.5f, -0.5f, -0.5f,  // Bottom-left
         0.5f,  0.5f, -0.5f,   // top-right
         0.5f, -0.5f, -0.5f,   // bottom-right         
         0.5f,  0.5f, -0.5f,   // top-right
        -0.5f, -0.5f, -0.5f,   // bottom-left
        -0.5f,  0.5f, -0.5f,   // top-left
        // Front face
        -0.5f, -0.5f,  0.5f,   // bottom-left
         0.5f, -0.5f,  0.5f,   // bottom-right
         0.5f,  0.5f,  0.5f,   // top-right
         0.5f,  0.5f,  0.5f,   // top-right
        -0.5f,  0.5f,  0.5f,   // top-left
        -0.5f, -0.5f,  0.5f,   // bottom-left
        // Left face
        -0.5f,  0.5f,  0.5f,  // top-right
        -0.5f,  0.5f, -0.5f,   // top-left
        -0.5f, -0.5f, -0.5f,   // bottom-left
        -0.5f, -0.5f, -0.5f,   // bottom-left
        -0.5f, -0.5f,  0.5f,   // bottom-right
        -0.5f,  0.5f,  0.5f,   // top-right
        // Right face
         0.5f,  0.5f,  0.5f,   // top-left
         0.5f, -0.5f, -0.5f,   // bottom-right
         0.5f,  0.5f, -0.5f,  // top-right         
         0.5f, -0.5f, -0.5f,   // bottom-right
         0.5f,  0.5f,  0.5f,  // top-left
         0.5f, -0.5f,  0.5f,  // bottom-left     
         // Bottom face
         -0.5f, -0.5f, -0.5f,   // top-right
          0.5f, -0.5f, -0.5f,   // top-left
          0.5f, -0.5f,  0.5f,   // bottom-left
          0.5f, -0.5f,  0.5f,   // bottom-left
         -0.5f, -0.5f,  0.5f,  // bottom-right
         -0.5f, -0.5f, -0.5f,   // top-right
         // Top face
         -0.5f,  0.5f, -0.5f,   // top-left
          0.5f,  0.5f,  0.5f,   // bottom-right
          0.5f,  0.5f, -0.5f,   // top-right     
          0.5f,  0.5f,  0.5f,   // bottom-right
         -0.5f,  0.5f, -0.5f,   // top-left
         -0.5f,  0.5f,  0.5f // bottom-left   
    };

    vec3 skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };


    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);


  

 

    

    

    glGenBuffers(1, &s_deferredShadingData.instanced_positions_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.instanced_positions_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(models), models, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(sizeof(vec4)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(2 * sizeof(vec4)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(3 * sizeof(vec4)));


    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenQueries(1, &s_deferredShadingData.QUERY);
    
    //color buffer texture
    glGenTextures(1, &s_deferredShadingData.color_buffer_texture);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.color_buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W_WIDTH, W_HEIGHT, 0,
        GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //bloom threshold texture
    glGenTextures(1, &s_deferredShadingData.bloom_threshold_texture);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.bloom_threshold_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W_WIDTH, W_HEIGHT, 0,
        GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //depth texture for occlussion culling
    glGenTextures(1, &s_deferredShadingData.depth_map_texture);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.depth_map_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, W_WIDTH, W_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    //framebuffer for occullsion culling
    glGenFramebuffers(1, &s_deferredShadingData.frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
    // setup color and depth buffer texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_deferredShadingData.color_buffer_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s_deferredShadingData.bloom_threshold_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_deferredShadingData.depth_map_texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer creation failure \n");
        assert(false);
    }
    unsigned int attachments2[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments2);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenVertexArrays(1, &s_deferredShadingData.cull_vao);
    glBindVertexArray(s_deferredShadingData.cull_vao);
    glGenBuffers(1, &s_deferredShadingData.cull_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.cull_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4), NULL, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*)0);

    glGenTransformFeedbacks(1, &s_deferredShadingData.transform_feedback_buffer);

  

  //  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, this->instanceBO[i]);

    glGenBuffers(1, &s_deferredShadingData.transform_feedback_buffer2);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, s_deferredShadingData.transform_feedback_buffer2);

    const char* vars[] = { "InstPosLOD0", "gl_NextBuffer", "InstPosLOD1", "gl_NextBuffer", "InstPosLOD2" };
    glTransformFeedbackVaryings(s_deferredShadingData.cull_shader, 5, vars, GL_INTERLEAVED_ATTRIBS);
 

    int data[2000];
    memset(data, -1, sizeof(data));

    glGenBuffers(1, &s_deferredShadingData.SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_deferredShadingData.SSBO);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 100000, NULL , GL_MAP_READ_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_deferredShadingData.SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


    glGenBuffers(1, &s_deferredShadingData.bounds_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_deferredShadingData.bounds_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * 2000, NULL, GL_STREAM_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, s_deferredShadingData.bounds_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


    glGenBuffers(1, &s_deferredShadingData.draw_buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_deferredShadingData.draw_buffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, 16 * 2000, NULL, GL_STATIC_DRAW);
    

    glGenBuffers(1, &s_deferredShadingData.chunks_info_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_deferredShadingData.chunks_info_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 16, NULL, GL_STREAM_DRAW);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, s_deferredShadingData.bounds_ssbo);


    unsigned data_atomic = 0;

    glGenBuffers(1, &s_deferredShadingData.atomic_counter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, s_deferredShadingData.atomic_counter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned), data_atomic, GL_STATIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, s_deferredShadingData.atomic_counter);

    glGenBuffers(1, &s_deferredShadingData.camera_info_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, s_deferredShadingData.camera_info_ubo);
    glBufferStorage(GL_UNIFORM_BUFFER, sizeof(vec4) * 6, NULL, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &s_deferredShadingData.draw_buffer_disoccluded);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_deferredShadingData.draw_buffer_disoccluded);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 12, NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &s_deferredShadingData.atomic_counter_disoccluded);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, s_deferredShadingData.atomic_counter_disoccluded);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned), data_atomic, GL_STATIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, s_deferredShadingData.atomic_counter_disoccluded);


    glBindBufferBase(GL_UNIFORM_BUFFER, 2, s_deferredShadingData.camera_info_ubo);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, s_deferredShadingData.draw_buffer);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, s_deferredShadingData.atomic_counter);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, s_deferredShadingData.cull_vbo);
}

static void Register_ParticleEmitter(ParticleEmitterClient emitter)
{
    unsigned emitter_index = RSB_Request(&s_particleDrawData.emitters);
    void* map = RSB_MapRange(&s_particleDrawData.emitters, 0, s_particleDrawData.emitters.used_size, GL_MAP_WRITE_BIT);
    void* emitter_data = (char*)map + (s_particleDrawData.emitters.item_size * emitter_index);
    memcpy(emitter_data, &emitter.emitter_gl, sizeof(R_ParticleEmitter));
    //RSB_Unmap(&s_particleDrawData.emitters);


    //assign particle data
    if (emitter.amount > s_particleDrawData.particles.reserve_size)
    {
        RSB_IncreaseSize(&s_particleDrawData.particles, emitter.amount - s_particleDrawData.particles.reserve_size);
    }
    R_Particle* particle_data = RSB_Map(&s_particleDrawData.particles, GL_MAP_WRITE_BIT);
    mat4 model;
    glm_mat4_identity(model);

    for (int i = 0; i < emitter.amount; i++)
    {
        unsigned particle_index = RSB_Request(&s_particleDrawData.particles);
        R_Particle* particle = &particle_data[particle_index];
        glm_mat4_copy(model, particle->xform);
        particle->emitter_index = emitter_index;
        particle->local_index = i;
    }
    RSB_Unmap(&s_particleDrawData.particles);

    emitter.storage_index = emitter_index;

    s_particleDrawData.emitter_clients[0] = emitter;
}

static void Particle_Emitter_Play(ParticleEmitterClient* emitter)
{
   
}

static void Particle_Emitter_AnimationUpdate(ParticleEmitterClient* emitter, R_ParticleEmitter* gl_emit, float delta)
{
    double remaining = delta;
    int i = 0;

    while (remaining)
    {
        double speed = emitter->anim_speed_scale;
        double abs_speed = fabs(speed);

        if (speed == 0)
        {
            return;
        }

        int frame_count = emitter->frame_count;

        int last_frame = frame_count - 1;

        if (!signbit(speed))
        {
            if (emitter->frame_progress >= 1.0)
            {
                //anim restart
                if (gl_emit->anim_frame >= last_frame)
                {
                    gl_emit->anim_frame = 0;
                }
                else
                {
                    gl_emit->anim_frame++;
                }
                emitter->frame_progress = 0.0;
            }
            
            double to_process = min((1.0 - emitter->frame_progress) / abs_speed, remaining);
            emitter->frame_progress += to_process * abs_speed;
            remaining -= to_process;
        }

        i++;
        if (i > frame_count) 
        {
            return; 
        }
    }

}


typedef enum EMITTER_FLAGS
{
    EMITTER_SKIP_DRAW = 1 << 0,
    EMITTER_ACTIVE = 1 << 1,
    EMITTER_CLEAR = 1 << 2,
    EMITTER_FORCE_RESTART = 1 << 3

} EMITTER_FLAGS;

static void Particle_Emitter_Update()
{


    float delta = C_getDeltaTime();

    R_ParticleEmitter* gl_map = RSB_MapRange(&s_particleDrawData.emitters, 0, s_particleDrawData.emitters.used_size, GL_MAP_WRITE_BIT);

    for (int i = 0; i < 1; i++)
    {
        ParticleEmitterClient* client = &s_particleDrawData.emitter_clients[0];
        R_ParticleEmitter* gl = &gl_map[client->storage_index];

        float local_delta = delta;

        
        gl->prev_time = gl->time;
        local_delta = delta * client->speed_scale;
        client->time += local_delta;

        if (client->time > client->life_time)
        {
            client->time = fmodf(client->time, client->life_time);
            client->cycles++;
            gl->cycle = client->cycles;

            if (client->one_shot && client->cycles > 0)
            {
                client->playing = false;
                client->cycles = 0;
            }
        }

        gl->delta = local_delta;

        gl->time = client->time / client->life_time;
        
        if(client->anim_speed_scale > 0)
            Particle_Emitter_AnimationUpdate(client, gl, local_delta);

        gl->flags = 0;

        //perform frustrum cull
        if (!glm_aabb_frustum(client->aabb, s_RenderData.camera_frustrum))
        {
            gl->flags |= EMITTER_SKIP_DRAW;
        }

     
  
        if (client->playing)
        {
            gl->flags |= EMITTER_ACTIVE;
        }
       


        bool clear = false;

        if (clear)
        {
            gl->flags |= EMITTER_CLEAR;
        }
       

    }
    RSB_Unmap(&s_particleDrawData.emitters);


}

static void renderMeshRequest(float x, float y, float z)
{
    RenderRequest request;
    request.mesh_index = 0;
    glm_mat4_identity(request.xform);
    request.xform[3][0] = x;
    request.xform[3][1] = y;
    request.xform[3][2] = z;
    

    glNamedBufferSubData(mesh_Render_data.render_requests, mesh_Render_data.last_request_index * sizeof(RenderRequest), sizeof(RenderRequest), &request);
    mesh_Render_data.last_request_index++;
}

extern void BSP_InitAllRenderData();
extern void BSP_RenderAll(vec3 viewPos);
extern void LoadBSPFile(const char* filename);


bool r_Init()
{
    //LoadBSPFile("assets/bsp/test_level.bsp");
    //LoadBSPFile("assets/bsp/q3dm1.bsp");
   // BSP_InitAllRenderData();


    render_settings.flags = RF__ENABLE_SHADOW_MAPPING;
    render_settings.mssa_setting = R_MSSA_NONE;
    render_settings.shadow_map_resolution = R_SM__1024;

    _initGLStates();
    _initGeneralRenderData();
    _initBitmapTextDrawData();
    _initDeferredShadingData();
    _initShadowMapData();
    _initWaterRenderData();
    _initParticleData();
    _initClusteredData();
    _initMeshRenderData();
    _initIrradianceData();
    _initBloomData();
    
    ParticleEmitterClient emiiter;

    
    
    memset(&emiiter, 0, sizeof(ParticleEmitterClient));

    emiiter.gravity = 3;

    //emiiter.emitter_gl.velocity[0] = 1;
    //emiiter.emitter_gl.velocity[1] -= emiiter.gravity;

    emiiter.life_time = 111;
    emiiter.emitter_gl.life_time = 111;

    emiiter.one_shot = false;
    
    emiiter.playing = true;
    
    emiiter.speed_scale = 1;
    emiiter.emitter_gl.explosivness = 0;
    emiiter.emitter_gl.emission_shape = 2;
    emiiter.emitter_gl.emission_size[0] = 5;
    emiiter.emitter_gl.emission_size[1] = 5;
    emiiter.emitter_gl.emission_size[2] = 5;

    emiiter.emitter_gl.randomness = 0;
    emiiter.frame_count = 25;
    emiiter.anim_speed_scale = 24;

    emiiter.emitter_gl.color[0] = 1;
    emiiter.emitter_gl.color[1] = 1;
    emiiter.emitter_gl.color[2] = 1;
    emiiter.emitter_gl.color[3] = 1;

    emiiter.amount = 12;
    emiiter.emitter_gl.particle_amount = 12;
    emiiter.emitter_gl.anim_offset = 2;
    glm_mat4_identity(emiiter.emitter_gl.xform);
    

    emiiter.aabb[0][0] = -20;
    emiiter.aabb[0][1] = -20;
    emiiter.aabb[0][2] = -20;

    emiiter.aabb[1][0] = 22;
    emiiter.aabb[1][1] = 22;
    emiiter.aabb[1][2] = 22;

    emiiter.emitter_gl.transform_align = 1;
    emiiter.direction[0] = 1;
    emiiter.direction[1] = -1;
    emiiter.direction[2] = 0;

    float inital_velocity = 5;
    float linear_accel = 5;
    vec3 force;
    force[0] = 0;
    force[1] = emiiter.gravity;
    force[2] = 0;

    force[0] += emiiter.direction[0] * inital_velocity;
    force[1] += emiiter.direction[1] * inital_velocity;
    force[2] += emiiter.direction[2] * inital_velocity;

   

    emiiter.emitter_gl.velocity[0] = force[0];
    emiiter.emitter_gl.velocity[1] = force[1];
    emiiter.emitter_gl.velocity[2] = force[2];

    Register_ParticleEmitter(emiiter);
    Particle_Emitter_Play(&emiiter);


    r_mssaLevel = Cvar_Register("r_mssaLevel", "0", "HELP TEXT", 0, 0, 8);
    r_shadowMapping = Cvar_Register("r_shadowMapping", "1", NULL, 0, 0, 1);
        
    ivec2 window_size;
    window_size[0] = W_WIDTH;
    window_size[1] = W_HEIGHT;
    r_onWindowResize(window_size);

    cube_model = Model_Load("assets/meshes/quad_mesh.dae");
    sphere_model = Model_Load("assets/meshes/sphere_mesh.dae");
    //sponza_model = Model_Load("assets/sponza/sponza2/NewSponza_Main_Zup_002.fbx");
    
    
    R_Mesh* mesh = cube_model.meshes->data;
    ModelVertex* verticies = mesh->vertices->data;
    
    float vert[450];
    
    int offset = 0;
    for (int i = 0; i < mesh->vertices->elements_size; i++)
    {
        ModelVertex* v = &verticies[i];

        vert[i + 0 + offset] = v->position[0];
        vert[i + 1 + offset] = v->position[1];
        vert[i + 2 + offset] = v->position[2];

        vert[i + 3 + offset] = v->normal[0];
        vert[i + 4 + offset] = v->normal[1];
        vert[i + 5 + offset] = v->normal[2];

        vert[i + 6 + offset] = v->tex_coords[0];
        vert[i + 7 + offset] = v->tex_coords[1];
        offset += 7;
    }

    glNamedBufferSubData(s_particleDrawData.vbo, 0, (sizeof(float) * 8) * mesh->vertices->elements_size, vert);
    unsigned* indices = mesh->indices->data;

    glNamedBufferSubData(s_particleDrawData.ebo, 0, sizeof(unsigned) * mesh->indices->elements_size, indices);

    glNamedBufferSubData(mesh_Render_data.vbo, 0, sizeof(ModelVertex) * mesh->vertices->elements_size, mesh->vertices->data);
    glNamedBufferSubData(mesh_Render_data.ebo, 0, sizeof(unsigned) * mesh->indices->elements_size, indices);
    
    MeshRenderData msd[2];
    glm_vec3_copy(mesh->bounding_box[0], msd[0].bounding_box[0]);
    glm_vec3_copy(mesh->bounding_box[1], msd[0].bounding_box[1]);

    int v_offset = sizeof(ModelVertex) * mesh->vertices->elements_size;
    int i_offset = sizeof(unsigned) * mesh->indices->elements_size;

    R_Mesh* mesh2 = sphere_model.meshes->data;
    glm_vec3_copy(mesh2->bounding_box[0], msd[1].bounding_box[0]);
    glm_vec3_copy(mesh2->bounding_box[1], msd[1].bounding_box[1]);
    glNamedBufferSubData(mesh_Render_data.vbo, v_offset, sizeof(ModelVertex) * mesh2->vertices->elements_size, mesh2->vertices->data);
    glNamedBufferSubData(mesh_Render_data.ebo, i_offset, sizeof(unsigned) * mesh2->indices->elements_size, mesh2->indices->data);
    

  
        

    glNamedBufferSubData(mesh_Render_data.mesh_data, 0, sizeof(MeshRenderData) * 2, msd);

    DrawElementsCMD cmd[2];
    cmd[0].baseInstance = 0;
    cmd[0].baseVertex = 0;
    cmd[0].count = mesh->indices->elements_size;
    cmd[0].firstIndex = 0;
    cmd[0].instanceCount = 0;
    
    cmd[1].baseInstance = 0;
    cmd[1].baseVertex = mesh->vertices->elements_size;
    cmd[1].count = mesh2->indices->elements_size;
    cmd[1].firstIndex = mesh->indices->elements_size;
    cmd[1].instanceCount = 0;

    glNamedBufferSubData(mesh_Render_data.element_draW_cmds, 0, sizeof(DrawElementsCMD) * 2, cmd);
    

    GL_Material material;
    memset(&material, -1, sizeof(material));
    material.albedo_map = 0;
    material.metalic_map = 1;
    material.normal_map = 2;
    material.roughness_map = 3;
    material.metallic_ratio = 1;
    material.color[0] = 0;
    material.color[1] = 0;
    material.color[2] = 1;
    material.color[3] = 1;
    glNamedBufferSubData(mesh_Render_data.materials_ssbo, 0, sizeof(GL_Material), &material);

    pbr_textures.albedo = Texture_Load("assets/pbr/rustediron2_basecolor.png", NULL);
    pbr_textures.metallic = Texture_Load("assets/pbr/rustediron2_metallic.png", NULL);
    pbr_textures.normal = Texture_Load("assets/pbr/rustediron2_normal.png", NULL);
    pbr_textures.roughness = Texture_Load("assets/pbr/rustediron2_roughness.png", NULL);

   

    particle_spritesheet_texture = Texture_Load("assets/Rocket Fire 2-Sheet.png", NULL);

    bitmap_texture = Texture_Load("assets/ui/bigchars.tga", NULL);

    tex_atlas = Texture_Load("assets/cube_textures/simple_block_atlas.png", NULL);
    tex_atlas_normal = Texture_Load("assets/cube_textures/simple_block_atlas_normal.png", NULL);
    tex_atlas_specular = Texture_Load("assets/cube_textures/simple_block_atlas_specular.png", NULL);
    tex_atlas_mer = Texture_Load("assets/cube_textures/simple_block_atlas_mer.png", NULL);
    
    dirst_texture = Texture_Load("assets/cube_textures/dirt.png", NULL);


    GLuint64 handles[4];
    //handles[0] = glGetTextureHandleARB(pbr_textures.albedo.id);
    //handles[1] = glGetTextureHandleARB(pbr_textures.metallic.id);
    //handles[2] = glGetTextureHandleARB(pbr_textures.normal.id);
    //handles[3] = glGetTextureHandleARB(pbr_textures.roughness.id);

    for (int i = 0; i < 4; i++)
    {
        //glMakeTextureHandleResidentARB(handles[i]);
    }

   // glNamedBufferSubData(mesh_Render_data.textures_ssbo, 0, sizeof(GLuint64) * 4, handles);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    s_fontData = R_loadFontData("assets/ui/myFont4.json", "assets/ui/myFont4.bmp");
    

   //
   // 
   // model = Model_Load("assets/backpack2/backpack.obj");

    float z_far = 1500;

    memset(s_lightDataBuffer, 0, sizeof(s_lightDataBuffer));
    s_lightDataIndex = 0;
  
    glUseProgram(s_deferredShadingData.simple_lighting_shader);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[0]", s_shadowMapData.shadow_cascade_levels[0]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[1]", s_shadowMapData.shadow_cascade_levels[1]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[2]", s_shadowMapData.shadow_cascade_levels[2]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[3]", s_shadowMapData.shadow_cascade_levels[3]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[4]", 1500);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_camerafarPlane", z_far);

    Cubemap_Faces_Paths cubemap_paths;
    cubemap_paths.right_face = "assets/cubemaps/skybox/right.jpg";
    cubemap_paths.left_face = "assets/cubemaps/skybox/left.jpg";
    cubemap_paths.bottom_face = "assets/cubemaps/skybox/bottom.jpg";
    cubemap_paths.top_face = "assets/cubemaps/skybox/top.jpg";
    cubemap_paths.back_face = "assets/cubemaps/skybox/back.jpg";
    cubemap_paths.front_face = "assets/cubemaps/skybox/front.jpg";
    cubemap_texture = CubemapTexture_Load(cubemap_paths);



    //LOAD HDR MAP






	return true;
}




void r_DrawScreenText(const char* p_string, vec2 p_position, vec2 p_size, vec4 p_color, float h_spacing, float y_spacing)
{
    const int str_len = strlen(p_string);

    //flush the batch??
    if (s_bitmapTextData.vertices_count + (4 * str_len) >= BITMAP_TEXT_VERTICES_BUFFER_SIZE - 1)
    {
        
    }

    const char* itr_string = p_string;

    const double fs_scale = 1.0 / (s_fontData.metrics_data.ascender - s_fontData.metrics_data.descender);
    double x = 0;
    double y = 0;

    const float texel_width = 1.0 / s_fontData.atlas_data.width;
    const float texel_height = 1.0 / s_fontData.atlas_data.height;
    
    R_FontGlyphData space_glyph_data = R_getFontGlyphData(&s_fontData, ' ');

    mat4 model_matrix;
    Math_Model2D(p_position, p_size, 0, model_matrix);

    vec4 color;
    if (p_color)
    {
        glm_vec4_copy(p_color, color);
    }
    else
    {
        glm_vec4_copy(DEFAULT_COLOR, color);
    }

    for (int i = 0; i < str_len; i++)
    {
        char ch = itr_string[i];

        //Handle escape characters
        //new line
        if (ch == '\n')
        {
            x = -0.1;
            y += fs_scale * (s_fontData.metrics_data.line_height + y_spacing);
            continue;
        }
        //space
        if (ch == ' ')
        {
            if (i < str_len - 1)
            {
                x += fs_scale * space_glyph_data.advance;
            }
            continue;
        }
        //horizontal tab
        if (ch == '\t')
        {
            if (i < str_len - 1)
            {
                x += (fs_scale * (space_glyph_data.advance * 2));
            }
            continue;
        }
        //carriage return
        if (ch == '\r')
        {
            x = 0;
            continue;
        }

        R_FontGlyphData glyph_data = R_getFontGlyphData(&s_fontData, ch);

        vec2 tex_coord_min, tex_coord_max;
        tex_coord_min[0] = (glyph_data.atlas_bounds.left * texel_width);
        tex_coord_min[1] = (glyph_data.atlas_bounds.top * texel_height);

        tex_coord_max[0] = (glyph_data.atlas_bounds.right * texel_width);
        tex_coord_max[1] = (glyph_data.atlas_bounds.bottom * texel_height);

        vec3 quad_min, quad_max;
        quad_min[0] = (glyph_data.plane_bounds.left * fs_scale) + x;
        quad_min[1] = -(glyph_data.plane_bounds.top * fs_scale) + y;
        quad_min[2] = 0;

        quad_max[0] = (glyph_data.plane_bounds.right * fs_scale) + x;
        quad_max[1] = -(glyph_data.plane_bounds.bottom * fs_scale) + y;
        quad_max[2] = 0;

        vec2 pos_bottom, tex_coord_bottom;
        pos_bottom[0] = quad_min[0];
        pos_bottom[1] = quad_max[1];

        tex_coord_bottom[0] = tex_coord_min[0];
        tex_coord_bottom[1] = tex_coord_max[1];

        vec2 pos_right, tex_coord_right;
        pos_right[0] = quad_max[0];
        pos_right[1] = quad_min[1];

        tex_coord_right[0] = tex_coord_max[0];
        tex_coord_right[1] = tex_coord_min[1];

        Math_mat4_mulv2(model_matrix, quad_min, 0.0, 1.0, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].position);
        glm_vec2_copy(tex_coord_min, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].tex_coords);
        glm_vec4_copy(color, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].color);
        s_bitmapTextData.vertices_count++;

        Math_mat4_mulv2(model_matrix, pos_bottom, 0.0, 1.0, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].position);
        glm_vec2_copy(tex_coord_bottom, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].tex_coords);
        glm_vec4_copy(color, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].color);
        s_bitmapTextData.vertices_count++;

        Math_mat4_mulv2(model_matrix, quad_max, 0.0, 1.0, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].position);
        glm_vec2_copy(tex_coord_max, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].tex_coords);
        glm_vec4_copy(color, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].color);
        s_bitmapTextData.vertices_count++;

        Math_mat4_mulv2(model_matrix, pos_right, 0.0, 1.0, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].position);
        glm_vec2_copy(tex_coord_right, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].tex_coords);
        glm_vec4_copy(color, s_bitmapTextData.vertices[s_bitmapTextData.vertices_count].color);
        s_bitmapTextData.vertices_count++;

        s_bitmapTextData.indices_count += 6;

        //advanced to the next character
        if (i < str_len - 1)
        {
            double glyph_advance = glyph_data.advance;
            x += fs_scale * (glyph_advance + h_spacing);
        }

    }


}




void _drawSkybox()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glUseProgram(s_deferredShadingData.skybox_shader);
    mat4 view_no_translation;
    glm_mat4_zero(view_no_translation);
    
    glm_mat4_identity(view_no_translation);
    glm_mat3_make(s_RenderData.camera->data.view_matrix, view_no_translation);

    view_no_translation[2][0] = s_RenderData.camera->data.view_matrix[2][0];
    view_no_translation[2][1] = s_RenderData.camera->data.view_matrix[2][1];
    view_no_translation[2][2] = s_RenderData.camera->data.view_matrix[2][2];
    view_no_translation[2][3] = s_RenderData.camera->data.view_matrix[2][3];
    
    static float rotation = 0;
    rotation += 0.00010 * C_getDeltaTime();

    vec3 axis;
    axis[0] = 0;
    axis[1] = 1;
    axis[2] = 0;

    //glm_rotate(view_no_translation, glm_deg(rotation), axis);

    Shader_SetMatrix4(s_deferredShadingData.skybox_shader, "u_view", view_no_translation);
    Shader_SetMatrix4(s_deferredShadingData.skybox_shader, "u_projection", s_RenderData.camera->data.proj_matrix);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBindVertexArray(s_deferredShadingData.light_box_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.light_box_vbo);
    glActiveTexture(GL_TEXTURE0);
   // glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_data.envCubemapTexture);
    glDepthMask(GL_TRUE);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);
}






void r_Update(R_Camera* const p_cam)
{

    mat4 view_proj;
    glm_mat4_mul(p_cam->data.proj_matrix, p_cam->data.view_matrix, view_proj);


    glBindBuffer(GL_UNIFORM_BUFFER, s_RenderData.camera_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), view_proj);

    s_RenderData.camera = p_cam;

    //Math_GLM_FrustrumPlanes_ReverseZ(view_proj, s_RenderData.camera_frustrum);
    glm_frustum_planes(view_proj, s_RenderData.camera_frustrum);

    glm_mat4_copy(view_proj, s_view_proj);

    glBindBufferBase(GL_UNIFORM_BUFFER, 1, s_deferredShadingData.camera_info_ubo);
    glNamedBufferSubData(s_deferredShadingData.camera_info_ubo, 0, sizeof(vec4) * 6, s_RenderData.camera_frustrum);

    
  
}


void r_DrawCrosshair()
{
    unsigned vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)(0));
    glEnableVertexAttribArray(0);


    vec2 line[4];
    line[0][0] = -0.03;
    line[0][1] = 0.0;

    line[1][0] = 0.03;
    line[1][1] = 0.0;
    
    line[2][0] = 0.0;
    line[2][1] = -0.03;

    line[3][0] = 0.0;
    line[3][1] = 0.03;

    //glUseProgram(s_RenderData.screen_shader);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 4, line[0], GL_STATIC_DRAW);

    glLineWidth(1);

    glDrawArrays(GL_LINES, 0, 4);

}

void hizcullPass()
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.g_fbo);
    glUseProgram(s_deferredShadingData.hi_z_shader);
    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    // disable color buffer as we will render only a depth image
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer);
    // we have to disable depth testing but allow depth writes
    glDepthFunc(GL_ALWAYS);
    // calculate the number of mipmap levels for NPOT texture
    int numLevels = 1 + (int)floorf(log2f(fmaxf(W_WIDTH, W_HEIGHT)));
   // int numLevels = 1 + 2;
    int currentWidth = W_WIDTH;
    int currentHeight = W_HEIGHT;
    for (int i = 1; i < numLevels; i++) {
        glUniform2i(glGetUniformLocation(s_deferredShadingData.hi_z_shader, "LastMipSize"), currentWidth, currentHeight);
        // calculate next viewport size
        currentWidth /= 2;
        currentHeight /= 2;
        // ensure that the viewport size is always at least 1x1
        currentWidth = currentWidth > 0 ? currentWidth : 2;
        currentHeight = currentHeight > 0 ? currentHeight : 2;
        glViewport(0, 0, currentWidth, currentHeight);
        // bind next level for rendering but first restrict fetches only to previous level
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, i - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i - 1);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer, i);
        // dummy draw command as the full screen quad is generated completely by a geometry shader
       // glDrawArrays(GL_POINTS, 0, 1);
        glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    // reset mipmap level range for the depth image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
    // reset the framebuffer configuration
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_deferredShadingData.g_colorSpec, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer, 0);
    // reenable color buffer writes, reset viewport and reenable depth test
    glDepthFunc(GL_LEQUAL); //is this right?
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
}

void compLights()
{   
    mat4 inverse_proj;
    glm_mat4_inv(s_RenderData.camera->data.proj_matrix, inverse_proj);

    glClearNamedBufferSubData(clustered_data.light_grid_SSBO, GL_R32UI, 0, 8 * 500, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glClearNamedBufferSubData(clustered_data.global_active_clusters_count_ssbo, GL_R32UI, 0, sizeof(unsigned), GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, clustered_data.clusters_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, clustered_data.active_clusters_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, clustered_data.light_buffer.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, clustered_data.light_indexes_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, clustered_data.light_grid_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, clustered_data.global_index_count_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, clustered_data.global_active_clusters_count_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, clustered_data.unique_active_clusters_data_ssbo);
    

   
    glUseProgram(clustered_data.cluster_build_shader);
    Shader_SetMatrix4(clustered_data.cluster_build_shader, "u_InverseProj", inverse_proj);
    glDispatchCompute(16, 9, 24);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    glUseProgram(clustered_data.cluster_cull_shader);
    Shader_SetMatrix4(clustered_data.cluster_cull_shader, "u_view", s_RenderData.camera->data.view_matrix);
   // Shader_SetMatrix4(clustered_data.cluster_cull_shader, "u_proj", s_RenderData.camera->data.proj_matrix);
    glDispatchCompute(1, 1, 6);
   // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
 
}


void ShadowMapDraw(int max)
{
    vec3 down;
    down[0] = -1;
    down[1] = 1;
    down[2] = 0;
    //SET UP_LIGHT MATRICES
    mat4 light_matrixes[5];
    float cascades[5];
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
   // CascadeShadow_genMatrix(down, 0.1, 500, s_RenderData.camera->data.view_matrix, s_RenderData.camera->data.proj_matrix, light_matrixes, cascades);
    glUseProgram(s_RenderData.deffered_lighting_shader);
    Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_cascadePlaneDistances[0]", s_shadowMapData.shadow_cascade_levels[0]);
    Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_cascadePlaneDistances[1]", s_shadowMapData.shadow_cascade_levels[1]);
    Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_cascadePlaneDistances[2]", s_shadowMapData.shadow_cascade_levels[2]);
    Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_cascadePlaneDistances[3]", s_shadowMapData.shadow_cascade_levels[3]);
    //Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_cascadePlaneDistances[4]", s_shadowMapData.shadow_cascade_levels[4]);

    //Math_getLightSpacesMatrixesForFrustrum(s_RenderData.camera, s_RenderData.window_width, s_RenderData.window_height, 12, s_shadowMapData.shadow_cascade_levels, down, light_matrixes);
    glBindBuffer(GL_UNIFORM_BUFFER, s_shadowMapData.light_space_matrices_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(light_matrixes), light_matrixes);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, s_shadowMapData.light_space_matrices_ubo);

    glUseProgram(s_shadowMapData.shadow_depth_shader);
    glBindFramebuffer(GL_FRAMEBUFFER, s_shadowMapData.frame_buffer);
    glViewport(0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
    glClear(GL_DEPTH_BUFFER_BIT);

    //disable cull face for peter panning
    glEnable(GL_CULL_FACE);


    glEnable(GL_DEPTH_CLAMP);

    //RENDER FROM LIGHTS PERSPECTIVE
    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, max, 32);
   

    glDisable(GL_DEPTH_CLAMP);

    glEnable(GL_CULL_FACE);
    glViewport(0, 0, s_RenderData.window_width, s_RenderData.window_height);
}

void downSample(int width, int height)
{
    int level = 0;
    int dim = width > height ? width : height;
    int twidth = width;
    int theight = height;
    int wasEven = 0;

    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.g_fbo);
    glDepthFunc(GL_ALWAYS);
    glUseProgram(s_deferredShadingData.downsample_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer);
    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    while (dim)
    {
        if (level)
        {
            twidth = twidth < 1 ? 1 : twidth;
            theight = theight < 1 ? 1 : theight;
            glViewport(0, 0, twidth, theight);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer, level);
            glUniform1i(0, level - 1);
            glUniform1i(1, wasEven);

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        wasEven = (twidth % 2 == 0) && (theight % 2 == 0);

        dim /= 2;
        twidth /= 2;
        theight /= 2;
        level++;
    }
   // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
   // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
    // reset the framebuffer configuration
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glUseProgram(0);
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDepthFunc(GL_LESS);
    glViewport(0, 0, width, height);
}   

static void RenderBloomTexture(unsigned srcTexture)
{
    glBindFramebuffer(GL_FRAMEBUFFER, bloom_data.fbo);
    glClear(GL_COLOR_BUFFER_BIT);
    
    float filterRadius = 0;

    //Render downsamples
    glUseProgram(bloom_data.downsample_shader);
    Shader_SetVector2f(bloom_data.downsample_shader, "srcResolution", W_WIDTH, W_HEIGHT);

    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.screen_quad_vbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);
    glDisable(GL_BLEND);

    Shader_SetInteger(bloom_data.downsample_shader, "mipLevel", 0);

    for (int i = 0; i < 6; i++)
    {
        glViewport(0, 0, bloom_data.mip_sizes[i][0], bloom_data.mip_sizes[i][1]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, bloom_data.mip_textures[i], 0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        Shader_SetVector2f(bloom_data.downsample_shader, "srcResolution", bloom_data.mip_sizes[i][0], bloom_data.mip_sizes[i][1]);

        glBindTexture(GL_TEXTURE_2D, bloom_data.mip_textures[i]);

        if (i == 0) Shader_SetInteger(bloom_data.downsample_shader, "mipLevel", 1);
    }

    //Render upsamples
    glUseProgram(bloom_data.upsample_shader);
    Shader_SetInteger(bloom_data.upsample_shader, "filterRadius", filterRadius);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    for (int i = 5; i > 0; i--)
    {

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloom_data.mip_textures[i]);

        glViewport(0, 0, bloom_data.mip_sizes[i - 1][0], bloom_data.mip_sizes[i - 1][1]);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, bloom_data.mip_textures[i - 1], 0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, W_WIDTH, W_HEIGHT);
}

void r_startFrame()
{
    //update camera stuff
   // r_Update(p_cam);

    Particle_Emitter_Update();

    if (r_mssaLevel->int_value != 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.msFBO);
    }
    else if (use_hdr)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.hdrFBO);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.FBO);
    }
   
    glEnable(GL_DEPTH_TEST);
    //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        
   vec3 pos;
  
   //draw occluder

    // bind offscreen framebuffer
   glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


   
    vec4 bounds;
    bounds[0] = 8;
    bounds[1] = 4;
    bounds[2] = 8;
    bounds[3] = 0.1;

    glNamedBufferSubData(s_deferredShadingData.bounds_ssbo, 0, sizeof(vec4) * 1, bounds);
    
    unsigned queyr = 0;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_deferredShadingData.bounds_ssbo);
    //glBindTextureUnit(0, s_deferredShadingData.depth_map_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer);
    glUseProgram(s_deferredShadingData.cull_compute_shader);
    
  

    Shader_SetMatrix4(s_deferredShadingData.cull_compute_shader, "u_mvp", s_view_proj);
    Shader_SetVector3f_2(s_deferredShadingData.cull_compute_shader, "u_viewPos", s_RenderData.camera->data.position);
   
    //renderMeshRequest(0, 0, 0);
   // renderMeshRequest(32, 0, 0);
 


    //glUseProgram(mesh_Render_data.process_shader);
    //glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.depth_map_texture);
   // glDispatchCompute(1, 1, 1);
    //glFinish();



    WorldRenderData* world_render_data = LC_World_getRenderData();

    


    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glFinish();


    //glm_sphere_fru


   
   



    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_deferredShadingData.draw_buffer);
    glBindBuffer(GL_PARAMETER_BUFFER, s_deferredShadingData.atomic_counter);

    //NORMAL RENDER PASS
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_atlas.id);
    
    //glUseProgram(s_deferredShadingData.simple_lighting_shader);
    //glActiveTexture(GL_TEXTURE2);
   // glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture.id);
    glUseProgram(s_deferredShadingData.simple_lighting_shader);
    //Shader_SetMatrix4(s_deferredShadingData.simple_lighting_shader, "u_view", s_RenderData.camera->data.view_matrix);
  //  Shader_SetVector3f(s_deferredShadingData.simple_lighting_shader, "u_viewPos", s_RenderData.camera->data.position[0], s_RenderData.camera->data.position[1], s_RenderData.camera->data.position[2]);



    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_atlas.id);
    glBindVertexArray(world_render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, world_render_data->vertex_buffer.buffer);

   
    size_t vertex_count = world_render_data->vertex_buffer.used_bytes / sizeof(ChunkVertex);
    size_t t_vertex_count = world_render_data->transparents_vertex_buffer.used_bytes / sizeof(ChunkVertex);
    
  
   
    mat4 inverse_proj;
    glm_mat4_inv(s_RenderData.camera->data.proj_matrix, inverse_proj);
    mat4 inverse_view;
    glm_mat4_inv(s_RenderData.camera->data.view_matrix, inverse_view);
    
    vec3 aabb[2];


    aabb[0][0] = (s_RenderData.camera->data.position[0] - 500) * s_RenderData.camera->data.camera_front[0];
    aabb[0][1] = s_RenderData.camera->data.position[1] - 500;
    aabb[0][2] = s_RenderData.camera->data.position[2] - 500;

    aabb[1][0] = (s_RenderData.camera->data.position[0] + 500) * s_RenderData.camera->data.camera_front[0];
    aabb[1][1] = s_RenderData.camera->data.position[1] + 500;
    aabb[1][2] = s_RenderData.camera->data.position[2] + 500;

    aabb[0][0] = min((s_RenderData.camera->data.position[0] - 500), (s_RenderData.camera->data.position[0] + 500));
    aabb[1][0] = max((s_RenderData.camera->data.position[0] - 500), (s_RenderData.camera->data.position[0] + 500));


    //Process particles
    glUseProgram(s_particleDrawData.compute_shader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, s_particleDrawData.emitters.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, s_particleDrawData.particles.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, s_particleDrawData.transforms.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, s_particleDrawData.draw_cmds);
    Shader_SetVector3f_2(s_particleDrawData.compute_shader, "u_viewPos", s_RenderData.camera->data.position);
    glDispatchCompute(1, 1, 1);
    

    //Process Chunks
    int chunk_amount = world_render_data->chunk_data.used_size;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, world_render_data->chunk_data.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, world_render_data->draw_cmds_buffer.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, s_deferredShadingData.SSBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, s_deferredShadingData.camera_info_ubo);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, s_deferredShadingData.atomic_counter);
    r_Update(s_RenderData.camera);
    glUseProgram(world_render_data->process_shader);
    int num_x_groups = ceilf((float)LC_World_GetChunkAmount() / 16.0f);
    Shader_SetInteger(world_render_data->process_shader, "u_chunkAmount", chunk_amount);
   // Shader_SetUnsigned(world_render_data->process_shader, "u_opaqueUpdateOffset", world_render_data->opaque_update_offset);
   // Shader_SetInteger(world_render_data->process_shader, "u_opaqueUpdateMoveBy", world_render_data->opaque_update_move_by);
   // Shader_SetUnsigned(world_render_data->process_shader, "u_transparentUpdateOffset", world_render_data->transparent_update_offset);
    //Shader_SetInteger(world_render_data->process_shader, "u_transparentUpdateMoveBy", world_render_data->transparent_update_move_by);

    //Shader_SetInteger(world_render_data->process_shader, "hiz_map_texture", 0);
   // Shader_SetVector2f(world_render_data->process_shader, "u_minMax", aabb[0][0], aabb[1][0]);
    glFinish();
    glBindTextureUnit(0, s_deferredShadingData.g_depth_buffer);
    glDispatchCompute(num_x_groups, 1, 1);
    glFinish();

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, world_render_data->draw_cmds_buffer.buffer);
    
    glBindVertexArray(world_render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, world_render_data->vertex_buffer.buffer);
    glBindVertexBuffer(0, world_render_data->vertex_buffer.buffer, 0, sizeof(ChunkVertex));
   
    
   // world_render_data->opaque_update_offset = 0;
  //  world_render_data->opaque_update_move_by = 0;
   // world_render_data->transparent_update_offset = 0;
  //  world_render_data->transparent_update_move_by = 0;
    Camera_UpdateMatrices(s_RenderData.camera, 1280, 720);
    ShadowMapDraw(chunk_amount);
   
  

    //DEPTH PRE PASS
    unsigned query = 0;
    glGenQueries(1, &query);
    glBeginQuery(GL_PRIMITIVES_GENERATED, query);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, world_render_data->block_data_buffer);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);

    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.g_fbo);
    glClearColor(0, 0, 0, 0);//must be black
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   

    glDisable(GL_BLEND);   //Disable so we can put extra information in alpha channel and it wont cause any issues
    glDepthFunc(GL_LESS);
    glBindVertexArray(world_render_data->vao);
    glUseProgram(s_RenderData.depth_prepass_shader);
    // disable color buffer as we will render only a depth image
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, chunk_amount, 32);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_LESS);

    //glEndQuery(GL_PRIMITIVES_GENERATED);
    
   // GLuint64 query_result = 0;
    //glGetQueryObjectui64v(query, GL_QUERY_RESULT, &query_result);
  
    //printf("primitives : %i \n", query_result);

   
    //GBUFFER PASS
    glDisable(GL_BLEND);
    glDepthFunc(GL_EQUAL); //Skip occluded pixels since we did pre pass above
    glDepthMask(GL_FALSE);
    glUseProgram(s_deferredShadingData.g_shader);
   // Shader_SetMatrix4(s_deferredShadingData.g_shader, "u_view", s_RenderData.camera->data.view_matrix);
    glBindTextureUnit(0, tex_atlas.id);
    glBindTextureUnit(1, tex_atlas_normal.id);
    glBindTextureUnit(2, tex_atlas_mer.id);
    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, chunk_amount, 32);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
   
    

    //SSAO PASS
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.ssaoFBO);
    glViewport(0, 0, W_WIDTH / 2, W_HEIGHT / 2);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTextureUnit(0, s_deferredShadingData.g_normal);
    glBindTextureUnit(1, s_RenderData.ssao_noise_texture);
    glBindTextureUnit(2, s_deferredShadingData.g_depth_buffer);
    glUseProgram(s_RenderData.ssao_pass_deffered_shader);
    

    Shader_SetMatrix4(s_RenderData.ssao_pass_deffered_shader, "u_proj", s_RenderData.camera->data.proj_matrix);
    Shader_SetMatrix4(s_RenderData.ssao_pass_deffered_shader, "u_InverseProj", inverse_proj);
    Shader_SetMatrix4(s_RenderData.ssao_pass_deffered_shader, "u_InverseView", inverse_view);
    Shader_SetMatrix4(s_RenderData.ssao_pass_deffered_shader, "u_view", s_RenderData.camera->data.view_matrix);

    //glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glBindVertexArray(s_deferredShadingData.indexed_screen_quad_vao);

    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    //SSAO BLUR PASS
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTextureUnit(0, s_RenderData.ssaoColorBuffer);
    glUseProgram(s_RenderData.ssao_blur_shader);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
    glEnable(GL_BLEND);
   // hizcullPass();
    //
     //downSample(W_WIDTH, W_HEIGHT);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(s_deferredShadingData.screen_quad_vao);

    //REFLECTED UV PASS
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.reflectedUV_FBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTextureUnit(0, s_deferredShadingData.g_depth_buffer);
    glBindTextureUnit(1, s_deferredShadingData.g_normal);
    glBindTextureUnit(2, s_deferredShadingData.g_colorSpec);
    glUseProgram(s_RenderData.reflected_uv_shader);
    Shader_SetMatrix4(s_RenderData.reflected_uv_shader, "u_InverseProj", inverse_proj);
    Shader_SetMatrix4(s_RenderData.reflected_uv_shader, "u_proj", s_RenderData.camera->data.proj_matrix);
    //Shader_SetMatrix4(s_RenderData.reflected_uv_shader, "u_view", inverse_view);
   // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
   

   

    compLights();

    //DEFFERED LIGHTING PASS
    //glClearNamedBufferSubData(clustered_data.global_index_count_SSBO, GL_R32UI, 0, sizeof(unsigned), GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //_drawSkybox();
    
    glDisable(GL_DEPTH_TEST);
   


    glUseProgram(s_RenderData.deffered_lighting_shader);
    
    static float time = 0;
    time += 0.04 * C_getDeltaTime();
    time = time - (int)time;
   // if (time >= 10.0)
     //   time = 0;


    Shader_SetVector3f_2(s_RenderData.deffered_lighting_shader, "u_viewPos", s_RenderData.camera->data.position);
    Shader_SetMatrix4(s_RenderData.deffered_lighting_shader, "u_InverseProj", inverse_proj);
    Shader_SetMatrix4(s_RenderData.deffered_lighting_shader, "u_InverseView", inverse_view);
    //Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_delta", time);
   // Shader_SetMatrix4(s_RenderData.deffered_lighting_shader, "u_view", s_RenderData.camera->data.view_matrix);
    Shader_SetVector3f(s_RenderData.deffered_lighting_shader, "u_dirLight.direction", -1, 1, 0);
    Shader_SetVector3f(s_RenderData.deffered_lighting_shader, "u_dirLight.color", 3, 2, 1);
   // Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_dirLight.ambient_intensity", 0.5);
    //Shader_SetFloat(s_RenderData.deffered_lighting_shader, "u_dirLight.specular_intensity", 0.5);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, clustered_data.clusters_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, clustered_data.active_clusters_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, clustered_data.light_buffer.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, clustered_data.light_indexes_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, clustered_data.light_grid_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, clustered_data.global_index_count_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, clustered_data.global_active_clusters_count_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, clustered_data.unique_active_clusters_data_ssbo);



    glBindTextureUnit(0, s_deferredShadingData.g_depth_buffer);
    glBindTextureUnit(1, s_deferredShadingData.g_normal);
    glBindTextureUnit(2, s_deferredShadingData.g_colorSpec);
    glBindTextureUnit(3, s_RenderData.ssaoBlurColorBuffer);
    glBindTextureUnit(4, s_shadowMapData.light_depth_maps);
    glBindTextureUnit(5, irradiance_data.irradianceCubemapTexture);
    glBindTextureUnit(6, irradiance_data.preFilteredCubemapTexture);
    glBindTextureUnit(7, irradiance_data.brdfLutTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D_ARRAY, s_shadowMapData.light_depth_maps);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   
    //Render bloom texture
    RenderBloomTexture(s_deferredShadingData.bloom_threshold_texture);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_deferredShadingData.g_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
    glBlitFramebuffer(0, 0, W_WIDTH, W_HEIGHT, 0, 0, W_WIDTH, W_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
   
    //_drawSkybox();

    glEnable(GL_DEPTH_TEST);
    //transparents pass
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
   // glBlendEquation(GL_MAX);
    glBindTextureUnit(0, tex_atlas.id);
    glBindTextureUnit(1, cubemap_texture.id);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, world_render_data->draw_cmds_buffer.buffer);
    glBindVertexArray(world_render_data->vao);
    glBindVertexBuffer(0, world_render_data->transparents_vertex_buffer.buffer, 0, sizeof(ChunkVertex));
    glBindBuffer(GL_ARRAY_BUFFER, world_render_data->transparents_vertex_buffer.buffer);
    glUseProgram(world_render_data->transparents_forward_pass_shader);
    Shader_SetVector3f_2(world_render_data->transparents_forward_pass_shader, "u_viewPos", s_RenderData.camera->data.position);
    Shader_SetVector3f(world_render_data->transparents_forward_pass_shader, "u_dirLight.direction", 0, 1, 1);
    Shader_SetVector3f(world_render_data->transparents_forward_pass_shader, "u_dirLight.color", 1, 1, 1);
    Shader_SetFloat(world_render_data->transparents_forward_pass_shader, "u_dirLight.ambient_intensity", 0.5);
    Shader_SetFloat(world_render_data->transparents_forward_pass_shader, "u_dirLight.specular_intensity", 0.5);
    glMultiDrawArraysIndirect(GL_TRIANGLES, (void*)16, chunk_amount, 32);

    //glDisable(GL_BLEND);
    glBindVertexArray(s_particleDrawData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_particleDrawData.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_particleDrawData.ebo);
    glUseProgram(s_particleDrawData.render_shader);
    glBindTexture(GL_TEXTURE_2D, particle_spritesheet_texture.id);
    Shader_SetVector3f_2(s_particleDrawData.render_shader, "u_viewPos", s_RenderData.camera->data.position);
   // Shader_SetMatrix4(s_particleDrawData.render_shader, "u_view", s_RenderData.camera->data.view_matrix);
    Shader_SetVector3f(s_particleDrawData.render_shader, "u_dirLight.direction", -1, 1, 0);
    Shader_SetVector3f(s_particleDrawData.render_shader, "u_dirLight.color", 0, 0, 1);

   // glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, 5);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    
    //WATER PASS
    glUseProgram(world_render_data->water_forward_pass_shader);
    glBindTextureUnit(0, tex_atlas.id);
    glBindTextureUnit(1, cubemap_texture.id);
    glBindTextureUnit(2, water_data.dudv_map.id);
    glBindTextureUnit(3, water_data.normal_map.id);
    Shader_SetVector3f_2(world_render_data->water_forward_pass_shader, "u_viewPos", s_RenderData.camera->data.position);
    Shader_SetFloat(world_render_data->water_forward_pass_shader, "u_moveFactor", time);
    Shader_SetVector3f(world_render_data->water_forward_pass_shader, "u_dirLight.direction", 0, 1, 1);
    Shader_SetVector3f(world_render_data->water_forward_pass_shader, "u_dirLight.color", 1, 1, 1);
    Shader_SetFloat(world_render_data->water_forward_pass_shader, "u_dirLight.ambient_intensity", 0.5);
    Shader_SetFloat(world_render_data->water_forward_pass_shader, "u_dirLight.specular_intensity", 0.5);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
   //glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ZERO);
    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);


    //BLUR THE MAIN SCREEN TEXTURE
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_RenderData.dof_fbo);
    glBlitFramebuffer(0, 0, W_WIDTH, W_HEIGHT, 0, 0, W_WIDTH, W_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.dof_fbo);

    glUseProgram(s_RenderData.box_blur_shader);
    glBindTextureUnit(0, s_RenderData.dof_unfocus_texture);
    Shader_SetInteger(s_RenderData.box_blur_shader, "u_size", 4);
    glBindVertexArray(s_deferredShadingData.screen_quad_vao);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //FXAA PASS
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glUseProgram(s_RenderData.FXAA_shader);
    Shader_SetInteger(s_RenderData.FXAA_shader, "screen_texture", 0);
    glBindTextureUnit(0, s_deferredShadingData.color_buffer_texture);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


    //DEPTH OF FIELD THE COLOR BUFFER
    glUseProgram(s_RenderData.dof_shader);
    glBindTextureUnit(0, s_deferredShadingData.g_depth_buffer);
    glBindTextureUnit(2, s_deferredShadingData.color_buffer_texture);
    glBindTextureUnit(3, s_RenderData.dof_unfocus_texture);
    Shader_SetMatrix4(s_RenderData.dof_shader, "u_InverseProj", inverse_proj);
    //Shader_SetMatrix4(s_RenderData.dof_shader, "u_InverseView", inverse_view);
    Shader_SetFloat(s_RenderData.dof_shader, "u_minDistance", 5);
    Shader_SetFloat(s_RenderData.dof_shader, "u_maxDistance", 34);
    Shader_SetVector2f(s_RenderData.dof_shader, "u_focusPoint", 0.5, 0.5);
    Shader_SetFloat(s_RenderData.dof_shader, "u_focusInfluence", 0.2);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
    glBindVertexArray(s_deferredShadingData.indexed_screen_quad_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


  

    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.g_fbo);
    //glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
   
    //HIZ CULLING DEBUG
    glDisable(GL_CULL_FACE); //NEVER ENABLE CULL FACE WHEN DEPTH TESTING
    glEnable(GL_DEPTH_TEST);
    //glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindVertexArray(s_deferredShadingData.cull_vao);
    glUseProgram(s_deferredShadingData.hiz_debug_shader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, world_render_data->chunk_data.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, world_render_data->draw_cmds_buffer.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, s_deferredShadingData.SSBO);
    glEnable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.g_depth_buffer);

   // Shader_SetMatrix4(s_deferredShadingData.hiz_debug_shader, "u_view", s_RenderData.camera->data.view_matrix);
   // Shader_SetMatrix4(s_deferredShadingData.hiz_debug_shader, "u_proj", s_RenderData.camera->data.proj_matrix);
    Shader_SetVector3f_2(s_deferredShadingData.hiz_debug_shader, "u_viewPos", s_RenderData.camera->data.position);
    glPolygonOffset(-1, -1);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glDrawArrays(GL_POINTS, 0, chunk_amount);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0, 0);
    glClearNamedBufferSubData(s_deferredShadingData.atomic_counter, GL_R32UI, 0, sizeof(unsigned), GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    





}

void r_renderFrameNew()
{

}

void r_endFrame()
{
   

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(s_deferredShadingData.post_process_shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.color_buffer_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloom_data.mip_textures[0]);

   // glBindTexture(GL_TEXTURE_2D, s_RenderData.reflectedUV_ColorBuffer);

    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.screen_quad_vbo);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   
}


unsigned r_registerLightSource(PointLight p_lightData)
{
    const float maxBrightness = fmaxf(fmaxf(p_lightData.color[0], p_lightData.color[1]), p_lightData.color[2]);
    p_lightData.radius = (-p_lightData.linear + sqrt(p_lightData.linear * p_lightData.linear - 4 * p_lightData.quadratic * (p_lightData.constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * p_lightData.quadratic);

    p_lightData.position[3] = 32.0;
    p_lightData.color[3] = 1.0;

    unsigned index = RSB_Request(&clustered_data.light_buffer);

    glNamedBufferSubData(clustered_data.light_buffer.buffer, index * sizeof(PointLight), sizeof(PointLight), &p_lightData);


    s_lightDataIndex++;
    glUseProgram(s_RenderData.deffered_lighting_shader);
    Shader_SetInteger(s_RenderData.deffered_lighting_shader, "u_pointLightCount", s_lightDataIndex);

    glUseProgram(s_RenderData.deffered_compute_shader);
    Shader_SetInteger(s_RenderData.deffered_compute_shader, "u_pointLightCount", s_lightDataIndex);

    glUseProgram(clustered_data.cluster_cull_shader);
    Shader_SetInteger(clustered_data.cluster_cull_shader, "u_pointLightCount", s_lightDataIndex);

    return index;
}

void r_removeLightSource(unsigned p_index)
{
    RSB_FreeItem(&clustered_data.light_buffer, p_index, true);
}


