#include "r_renderer.h"
#include "r_shader.h"


#include <glad/glad.h>


#include "r_renderDefs.h"
#include "r_mesh.h"
#include "utility/u_gl.h"

#include "lc/lc_player.h"

#include "render/r_font.h"

#define W_WIDTH 800
#define W_HEIGHT 600

#define SHADOW_MAP_RESOLUTION 2048

#define LIGHT_MATRICES_COUNT 5
#define SHADOW_CASCADE_LEVELS 4

#define BITMAP_TEXT_VERTICES_BUFFER_SIZE 1024
#define LINE_VERTICES_BUFFER_SIZE 16384
#define TRIANGLE_VERTICES_BUFFER_SIZE 8192
#define SCREEN_QUAD_VERTICES_BUFFER_SIZE 8192
#define SCREEN_QUAD_INDICES_BUFFER_SIZE 12288
#define SCREEN_QUAD_MAX_TEXTURES 32
const vec4 DEFAULT_COLOR = { 1, 1, 1, 1 };

extern GLFWwindow* s_Window;


typedef struct DrawArraysIndirectCommand
{
    unsigned  count;
    unsigned  instanceCount;
    unsigned  first;
    unsigned  baseInstance;
} DrawElementsIndirectCommand;

typedef struct R_LineDrawData
{
    R_Shader shader;
    LineVertex line_vertices[LINE_VERTICES_BUFFER_SIZE];
    size_t vertices_count;
    unsigned vao, vbo;
} R_LineDrawData;

typedef struct R_TriangleDrawData
{
    R_Shader shader;
    TriangleVertex triangle_vertices[TRIANGLE_VERTICES_BUFFER_SIZE];
    size_t vertices_count;
    unsigned vao, vbo;
} R_TriangleDrawData;

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

typedef struct R_BillboardData
{
    R_Shader shader;
    unsigned vao, vbo;
    R_Texture texture;

} R_BillboardData;


typedef struct R_DeferredShadingData
{
    R_Shader g_shader;
    unsigned g_buffer;
    unsigned g_position, g_normal, g_colorSpec;
    unsigned depth_buffer;

    R_Shader lighting_shader;

    unsigned screen_quad_vao, screen_quad_vbo;

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

    unsigned cull_test_vao;

    R_Shader frustrum_select_shader;

    unsigned camera_info_ubo;

    unsigned draw_buffer_disoccluded;

    unsigned atomic_counter_disoccluded;

    unsigned instanced_positions_vbo;

    unsigned light_depth_maps;

    GL_UniformBuffer light_space_matrixes_ubo;

    R_Shader skybox_shader;

    unsigned chunk_block_info_ubo;

    R_Shader model_shader;

} R_DeferredShadingData;


typedef struct R_GeneralRenderData
{
    mat4 orthoProj;
    GL_UniformBuffer camera_buffer;
    R_Camera* camera;
    vec4 camera_frustrum[6];
    int window_width;
    int window_height;

    unsigned msFBO, FBO;
    unsigned msRBO;
    unsigned color_buffer_multisampled_texture;
    unsigned color_buffer_texture;
    unsigned depth_texture;

} R_GeneralRenderData;

R_RenderSettings render_settings;
static R_FontData s_fontData;
static R_BitmapTextDrawData s_bitmapTextData;
static R_BillboardData s_billboardData;
static R_ShadowMapData s_shadowMapData;
static R_LineDrawData s_lineDrawData;
static R_TriangleDrawData s_triangleDrawData;
static R_ScreenQuadDrawData s_screenQuadDrawData;
static R_DeferredShadingData s_deferredShadingData;
static R_GeneralRenderData s_RenderData;
static R_Texture tex_atlas;
static R_Texture cubemap_texture;
static R_Texture bitmap_texture;
static R_Model model;

typedef struct SimpleVertex
{
    vec3 pos;
} SimpleVertex;

void r_onWindowResize(ivec2 window_size)
{
    s_RenderData.window_width = window_size[0];
    s_RenderData.window_height = window_size[1];

    //update the ortho proj matrix
    glm_ortho(0.0f, s_RenderData.window_width, s_RenderData.window_height, 0.0f, -1.0f, 1.0f, s_RenderData.orthoProj);
    
    //Update framebuffer textures
    if (render_settings.mssa_setting != R_MSSA_NONE)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, s_RenderData.color_buffer_multisampled_texture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, render_settings.mssa_setting, GL_RGB, s_RenderData.window_width, s_RenderData.window_height, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, s_RenderData.msRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, render_settings.mssa_setting, GL_DEPTH24_STENCIL8, s_RenderData.window_width, s_RenderData.window_height);
    }
   
    glBindTexture(GL_TEXTURE_2D, s_RenderData.color_buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, s_RenderData.window_width, s_RenderData.window_height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, s_RenderData.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, s_RenderData.window_width, s_RenderData.window_height, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
}



static void _initLineRenderData()
{
    memset(&s_lineDrawData, 0, sizeof(R_LineDrawData));

    s_lineDrawData.shader = Shader_CompileFromFile("src/shaders/line_shader.vs", "src/shaders/line_shader.fs", NULL);

    if (s_lineDrawData.shader == 0)
        return false;

    glGenVertexArrays(1, &s_lineDrawData.vao);
    glGenBuffers(1, &s_lineDrawData.vbo);

    glBindVertexArray(s_lineDrawData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_lineDrawData.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(s_lineDrawData.line_vertices), NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(offsetof(LineVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(offsetof(LineVertex, color)));
    glEnableVertexAttribArray(1);
}

static void _initTriangleRenderData()
{
    memset(&s_triangleDrawData, 0, sizeof(R_TriangleDrawData));

    s_triangleDrawData.shader = Shader_CompileFromFile("src/shaders/triangle_shader.vs", "src/shaders/triangle_shader.fs", NULL);

    if (s_triangleDrawData.shader == 0)
        return;

    glGenVertexArrays(1, &s_triangleDrawData.vao);
    glGenBuffers(1, &s_triangleDrawData.vbo);

    glBindVertexArray(s_triangleDrawData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_triangleDrawData.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(s_triangleDrawData.triangle_vertices), NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, color)));
    glEnableVertexAttribArray(1);
}

static void _initScreenQuadRenderData()
{
    memset(&s_screenQuadDrawData, 0, sizeof(R_ScreenQuadDrawData));

    s_screenQuadDrawData.tex_index = 1;

    s_screenQuadDrawData.shader = Shader_CompileFromFile("src/shaders/screen_shader.vs", "src/shaders/screen_shader.fs", NULL);
    assert(s_screenQuadDrawData.shader > 0);

    glGenVertexArrays(1, &s_screenQuadDrawData.vao);
    glGenBuffers(1, &s_screenQuadDrawData.vbo);
    glGenBuffers(1, &s_screenQuadDrawData.ebo);

    glBindVertexArray(s_screenQuadDrawData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_screenQuadDrawData.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(s_screenQuadDrawData.screen_quad_vertices), NULL, GL_STREAM_DRAW);
    

    unsigned* indices = malloc(sizeof(unsigned) * SCREEN_QUAD_INDICES_BUFFER_SIZE);
    
    unsigned offset = 0;
    if (indices)
    {
        for (int i = 0; i < SCREEN_QUAD_INDICES_BUFFER_SIZE / sizeof(unsigned); i+= 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_screenQuadDrawData.ebo);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * SCREEN_QUAD_INDICES_BUFFER_SIZE, indices, GL_STATIC_DRAW);

        free(indices);
    }


    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, tex_coords)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, tex_index)));
    glEnableVertexAttribArray(2);

    //SETUP WHITE TEXTURE  
    //create texture
     glGenTextures(1, &s_screenQuadDrawData.white_texture);
    glBindTexture(GL_TEXTURE_2D, s_screenQuadDrawData.white_texture);

    //set texture wrap and filter modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    uint32_t color_White = 0xffffffff;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color_White);

    glUseProgram(s_screenQuadDrawData.shader);

    //set up samplers
    GLint texture_array_location = glGetUniformLocation(s_screenQuadDrawData.shader, "u_textures");
    int32_t samplers[32];

    for (int i = 0; i < 32; i++)
    {
       samplers[i] = i;
    }

    glUniform1iv(texture_array_location, 32, samplers);
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
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, render_settings.shadow_map_resolution, render_settings.shadow_map_resolution, SHADOW_CASCADE_LEVELS + 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
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
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, s_shadowMapData.light_space_matrices_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
   
    float z_far = 500;

    s_shadowMapData.shadow_cascade_levels[0] = z_far / 50.0f;
    s_shadowMapData.shadow_cascade_levels[1] = z_far / 25.0f;
    s_shadowMapData.shadow_cascade_levels[2] = z_far / 10.0f;
    s_shadowMapData.shadow_cascade_levels[3] = z_far / 2.0f;
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

static void _initBillboardData()
{
    memset(&s_billboardData, 0, sizeof(R_BillboardData));

    s_billboardData.shader = Shader_CompileFromFile("src/shaders/billboard.vs", "src/shaders/billboard.fs", "src/shaders/billboard.gs");
    assert(s_billboardData.shader > 0);

    s_billboardData.texture = Texture_Load("assets/cube_textures/sand.png", NULL);

    vec3 positions;
    positions[0] = -20;
    positions[1] = 0;
    positions[2] = 0;

    glGenVertexArrays(1, &s_billboardData.vao);
    glGenBuffers(1, &s_billboardData.vbo);
    glBindVertexArray(s_billboardData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_billboardData.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3), positions, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


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
    memset(&s_RenderData, 0, sizeof(R_GeneralRenderData));

    //CAMERA UBO
    glGenBuffers(1, &s_RenderData.camera_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, s_RenderData.camera_buffer);
    glBufferStorage(GL_UNIFORM_BUFFER, sizeof(mat4), NULL, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, s_RenderData.camera_buffer);

    //SETUP FRAMEBUFFERS AND RENDERBUFFERS
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, W_WIDTH, W_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_RenderData.depth_texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}   

static void _initDeferredShadingData()
{

    glGenFramebuffers(1, &s_deferredShadingData.depth_mapFBO);
    glGenTextures(1, &s_deferredShadingData.light_depth_maps);
    glBindTexture(GL_TEXTURE_2D_ARRAY, s_deferredShadingData.light_depth_maps);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, SHADOW_CASCADE_LEVELS + 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

    s_deferredShadingData.hi_z_shader = Shader_CompileFromFile("src/shaders/hi-z.vs", "src/shaders/hi-z.fs", NULL);
    assert(s_deferredShadingData.hi_z_shader > 0);

    s_deferredShadingData.cull_shader = Shader_CompileFromFile("src/shaders/cull2.vs", "src/shaders/cull.fs", NULL);
    assert(s_deferredShadingData.cull_shader > 0);

    s_deferredShadingData.post_process_shader = Shader_CompileFromFile("src/shaders/post_process.vs", "src/shaders/post_process.fs", NULL);
    assert(s_deferredShadingData.post_process_shader > 0);

    s_deferredShadingData.cull_compute_shader = ComputeShader_CompileFromFile("src/shaders/cull_compute.comp");
    assert(s_deferredShadingData.cull_compute_shader > 0);

    s_deferredShadingData.bounding_box_cull_shader = Shader_CompileFromFile("src/shaders/cull_bound_box.vs", "src/shaders/cull_bound_box.fs", "src/shaders/cull_bound_box.gs");
    assert(s_deferredShadingData.bounding_box_cull_shader > 0);

    s_deferredShadingData.frustrum_select_shader = ComputeShader_CompileFromFile("src/shaders/frustrum_select.comp");
    assert(s_deferredShadingData.frustrum_select_shader > 0);

    s_deferredShadingData.skybox_shader = Shader_CompileFromFile("src/shaders/skybox.vs", "src/shaders/skybox.fs", NULL);
    assert(s_deferredShadingData.skybox_shader > 0);

    s_deferredShadingData.model_shader = Shader_CompileFromFile("src/shaders/model_shader.vs", "src/shaders/model_shader.fs", NULL);
    assert(s_deferredShadingData.model_shader > 0);

    //SETUP SHADER
    glUseProgram(s_deferredShadingData.lighting_shader);
    Shader_SetInteger(s_deferredShadingData.lighting_shader, "u_position", 0);
    Shader_SetInteger(s_deferredShadingData.lighting_shader, "u_normal", 1);
    Shader_SetInteger(s_deferredShadingData.lighting_shader, "u_colorSpec", 2);

    glUseProgram(s_deferredShadingData.simple_lighting_shader);
    Shader_SetInteger(s_deferredShadingData.simple_lighting_shader, "atlas_texture", 0);
    Shader_SetInteger(s_deferredShadingData.simple_lighting_shader, "shadowMap", 1);
    Shader_SetInteger(s_deferredShadingData.simple_lighting_shader, "skybox_texture", 2);
    Shader_SetVector4f(s_deferredShadingData.simple_lighting_shader, "u_Fog.color", 0.6, 0.8, 1.0, 1.0);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_Fog.density", 0.00009);


    glUseProgram(s_deferredShadingData.skybox_shader);
    Shader_SetInteger(s_deferredShadingData.skybox_shader, "skybox_texture", 0);

    glUseProgram(s_deferredShadingData.model_shader);
    Shader_SetInteger(s_deferredShadingData.model_shader, "texture_diffuse1", 0);
    Shader_SetInteger(s_deferredShadingData.model_shader, "skybox_texture", 1);

    glUseProgram(s_deferredShadingData.post_process_shader);
    Shader_SetInteger(s_deferredShadingData.post_process_shader, "ColorBuffer", 0);

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W_WIDTH, W_HEIGHT, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //depth texture for occlussion culling
    glGenTextures(1, &s_deferredShadingData.depth_map_texture);
    glBindTexture(GL_TEXTURE_2D, s_deferredShadingData.depth_map_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, W_WIDTH, W_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);

    //framebuffer for occullsion culling
    glGenFramebuffers(1, &s_deferredShadingData.frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, s_deferredShadingData.frame_buffer);
    // setup color and depth buffer texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_deferredShadingData.color_buffer_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_deferredShadingData.depth_map_texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer creation failure \n");
        assert(false);
    }
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
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
   // glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STREAM_DRAW);
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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, s_deferredShadingData.bounds_ssbo);


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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, s_deferredShadingData.chunks_info_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, s_deferredShadingData.draw_buffer);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, s_deferredShadingData.atomic_counter);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, s_deferredShadingData.cull_vbo);
}

bool r_Init()
{
    render_settings.flags = RF__ENABLE_SHADOW_MAPPING;
    render_settings.mssa_setting = R_MSSA_NONE;
    render_settings.shadow_map_resolution = R_SM__1024;

    _initGLStates();
    _initBillboardData();
    _initGeneralRenderData();
    _initLineRenderData();
    _initTriangleRenderData();
    _initScreenQuadRenderData();
    _initBitmapTextDrawData();
    _initDeferredShadingData();
    _initShadowMapData();
   
        
    ivec2 window_size;
    window_size[0] = 800;
    window_size[1] = 600;
    r_onWindowResize(window_size);

   
   

    bitmap_texture = Texture_Load("assets/ui/bigchars.tga", NULL);

    tex_atlas = Texture_Load("assets/cube_textures/block_atlas.png", NULL);
   
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    s_fontData = R_loadFontData("assets/ui/myFont4.json", "assets/ui/myFont4.bmp");
    

    float z_far = 500;

   
  
    glUseProgram(s_deferredShadingData.simple_lighting_shader);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[0]", s_shadowMapData.shadow_cascade_levels[0]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[1]", s_shadowMapData.shadow_cascade_levels[1]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[2]", s_shadowMapData.shadow_cascade_levels[2]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_cascadePlaneDistances[3]", s_shadowMapData.shadow_cascade_levels[3]);

    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_camerafarPlane", z_far);

    Cubemap_Faces_Paths cubemap_paths;
    cubemap_paths.right_face = "assets/cubemaps/skybox/right.jpg";
    cubemap_paths.left_face = "assets/cubemaps/skybox/left.jpg";
    cubemap_paths.bottom_face = "assets/cubemaps/skybox/bottom.jpg";
    cubemap_paths.top_face = "assets/cubemaps/skybox/top.jpg";
    cubemap_paths.back_face = "assets/cubemaps/skybox/back.jpg";
    cubemap_paths.front_face = "assets/cubemaps/skybox/front.jpg";
    cubemap_texture = CubemapTexture_Load(cubemap_paths);

	return true;
}

static void _drawTextBatch()
{
    glUseProgram(s_bitmapTextData.shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_fontData.texture.id);

    Shader_SetMatrix4(s_bitmapTextData.shader, "u_orthoProjection", s_RenderData.orthoProj);

    glBindVertexArray(s_bitmapTextData.vao);

    glBindBuffer(GL_ARRAY_BUFFER, s_bitmapTextData.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(BitmapTextVertex) * s_bitmapTextData.vertices_count, s_bitmapTextData.vertices);

    glDrawElements(GL_TRIANGLES, s_bitmapTextData.indices_count, GL_UNSIGNED_INT, 0);

    memset(s_bitmapTextData.vertices, 0, sizeof(s_bitmapTextData.vertices));
    s_bitmapTextData.vertices_count = 0;
    s_bitmapTextData.indices_count = 0;
}

void r_DrawScreenText(const char* p_string, vec2 p_position, vec2 p_size, vec4 p_color, float h_spacing, float y_spacing)
{
    const int str_len = strlen(p_string);

    //flush the batch??
    if (s_bitmapTextData.vertices_count + (4 * str_len) >= BITMAP_TEXT_VERTICES_BUFFER_SIZE - 1)
    {
        _drawTextBatch();
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

void r_DrawScreenText2(const char* p_string, float p_x, float p_y, float p_width, float p_height, float p_r, float p_g, float p_b, float p_a, float h_spacing, float y_spacing)
{
    vec2 position;
    position[0] = p_x;
    position[1] = p_y;

    vec2 size;
    size[0] = p_width;
    size[1] = p_height;

    vec4 color;
    color[0] = p_r;
    color[1] = p_g;
    color[2] = p_b;
    color[3] = p_a;

    r_DrawScreenText(p_string, position, size, color, h_spacing, y_spacing);
}

void RenderbillboardTexture()
{
    glUseProgram(s_billboardData.shader);
    
    Shader_SetVector3f(s_billboardData.shader, "u_viewPos", s_RenderData.camera->data.position[0], s_RenderData.camera->data.position[1], s_RenderData.camera->data.position[2]);
    Shader_SetFloat(s_billboardData.shader, "u_billboardSize", 1);
    Shader_SetInteger(s_billboardData.shader, "texture_sampler", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_billboardData.texture.id);

    glDrawArrays(GL_POINTS, 0, 1);

}

void _drawSkybox()
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(s_deferredShadingData.skybox_shader);
    mat4 view_no_translation;
    glm_mat4_zero(view_no_translation);
    
    glm_mat4_identity(view_no_translation);
    glm_mat3_make(s_RenderData.camera->data.view_matrix, view_no_translation);

    view_no_translation[2][0] = s_RenderData.camera->data.view_matrix[2][0];
    view_no_translation[2][1] = s_RenderData.camera->data.view_matrix[2][1];
    view_no_translation[2][2] = s_RenderData.camera->data.view_matrix[2][2];
    view_no_translation[2][3] = s_RenderData.camera->data.view_matrix[2][3];
    
    Shader_SetMatrix4(s_deferredShadingData.skybox_shader, "u_view", view_no_translation);
    Shader_SetMatrix4(s_deferredShadingData.skybox_shader, "u_projection", s_RenderData.camera->data.proj_matrix);

    glBindVertexArray(s_deferredShadingData.light_box_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.light_box_vbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture.id);
        
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);
}

static void _drawLineBatch()
{
    glUseProgram(s_lineDrawData.shader);

    glBindVertexArray(s_lineDrawData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_lineDrawData.vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LineVertex) * s_lineDrawData.vertices_count, s_lineDrawData.line_vertices);

    glLineWidth(2);

    glDrawArrays(GL_LINES, 0, s_lineDrawData.vertices_count);

    memset(s_lineDrawData.line_vertices, 0, sizeof(s_lineDrawData.line_vertices));
    s_lineDrawData.vertices_count = 0;
}
static void _drawTriangleBatch()
{
    glUseProgram(s_triangleDrawData.shader);

    glBindVertexArray(s_triangleDrawData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_triangleDrawData.vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TriangleVertex) * s_triangleDrawData.vertices_count, s_triangleDrawData.triangle_vertices);

    glDrawArrays(GL_TRIANGLES, 0, s_triangleDrawData.vertices_count);

    memset(s_triangleDrawData.triangle_vertices, 0, sizeof(s_triangleDrawData.triangle_vertices));
    s_triangleDrawData.vertices_count = 0;
}

static void _drawScreenQuadBatch()
{   
    //bind texture units
    for (int i = 0; i < (s_screenQuadDrawData.tex_index + 1); i++)
    {
        glBindTextureUnit(i, s_screenQuadDrawData.tex_array[i]);
    }

    glUseProgram(s_screenQuadDrawData.shader);

    Shader_SetMatrix4(s_screenQuadDrawData.shader, "u_projection", s_RenderData.orthoProj);
    
    glBindVertexArray(s_screenQuadDrawData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_screenQuadDrawData.vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ScreenVertex) * s_screenQuadDrawData.vertices_count, s_screenQuadDrawData.screen_quad_vertices);
    glDrawElements(GL_TRIANGLES, s_screenQuadDrawData.indices_count, GL_UNSIGNED_INT, 0);

    memset(s_screenQuadDrawData.screen_quad_vertices, 0, sizeof(s_screenQuadDrawData.screen_quad_vertices));
    s_screenQuadDrawData.vertices_count = 0;
    s_screenQuadDrawData.indices_count = 0;
    s_screenQuadDrawData.indices_offset = 0;
    s_screenQuadDrawData.tex_index = 1;
}

void r_drawLine(vec3 from, vec3 to, vec4 color)
{
    //draw the current batch??
    if (s_lineDrawData.vertices_count + 2 >= LINE_VERTICES_BUFFER_SIZE - 1)
    {
        _drawLineBatch();
    }

    //start point
    glm_vec3_copy(from, s_lineDrawData.line_vertices[s_lineDrawData.vertices_count].position);
    if (color != NULL)
    {
        glm_vec4_copy(color, s_lineDrawData.line_vertices[s_lineDrawData.vertices_count].color);
    }
    else
    {
        glm_vec4_copy(DEFAULT_COLOR, s_lineDrawData.line_vertices[s_lineDrawData.vertices_count].color);
    }

    s_lineDrawData.vertices_count++;

    //end point
    glm_vec3_copy(to, s_lineDrawData.line_vertices[s_lineDrawData.vertices_count].position);
    if (color != NULL)
    {
        glm_vec4_copy(color, s_lineDrawData.line_vertices[s_lineDrawData.vertices_count].color);
    }
    else
    {
        glm_vec4_copy(DEFAULT_COLOR, s_lineDrawData.line_vertices[s_lineDrawData.vertices_count].color);
    }

    s_lineDrawData.vertices_count++;
}

void r_drawLine2(float f_x, float f_y, float f_z, float t_x, float t_y, float t_z, vec4 color)
{
    vec3 from;
    from[0] = f_x;
    from[1] = f_y;
    from[2] = f_z;

    vec3 to;
    to[0] = t_x;
    to[1] = t_y;
    to[2] = t_z;

    r_drawLine(from, to, color);
}

void r_drawTriangle(vec3 p1, vec3 p2, vec3 p3, vec4 color)
{
    //draw the current batch??
    if (s_triangleDrawData.vertices_count + 3 >= TRIANGLE_VERTICES_BUFFER_SIZE - 1)
    {
        _drawTriangleBatch();
    }

    //point 1
    glm_vec3_copy(p1, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].position);
    if (color != NULL)
    {
        glm_vec4_copy(color, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].color);
    }
    else
    {
        glm_vec4_copy(DEFAULT_COLOR, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].color);
    }
    s_triangleDrawData.vertices_count++;

    //point 2
    glm_vec3_copy(p2, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].position);
    if (color != NULL)
    {
        glm_vec4_copy(color, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].color);
    }
    else
    {
        glm_vec4_copy(DEFAULT_COLOR, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].color);
    }
    s_triangleDrawData.vertices_count++;

    //point 3
    glm_vec3_copy(p3, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].position);
    if (color != NULL)
    {
        glm_vec4_copy(color, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].color);
    }
    else
    {
        glm_vec4_copy(DEFAULT_COLOR, s_triangleDrawData.triangle_vertices[s_triangleDrawData.vertices_count].color);
    }
    s_triangleDrawData.vertices_count++;

}

void r_drawTriangle2(float p1_x, float p1_y, float p1_z, float p2_x, float p2_y, float p2_z, float p3_x, float p3_y, float p3_z, vec4 color)
{
    vec3 p1;
    p1[0] = p1_x;
    p1[1] = p1_y;
    p1[2] = p1_z;

    vec3 p2;
    p2[0] = p2_x;
    p2[1] = p2_y;
    p2[2] = p2_z;

    vec3 p3;
    p3[0] = p3_x;
    p3[1] = p3_y;
    p3[2] = p3_z;

    r_drawTriangle(p1, p2, p3, color);
}

void r_drawAABBWires(AABB box, vec4 color)
{
    //adding a small amount fixes z_fighting issues
    for (int i = 0; i < 3; i++)
    {
        box.position[i] += 0.001;
    }
    r_drawLine2(box.position[0], box.position[1], box.position[2], box.position[0] + box.width, box.position[1], box.position[2], color);
    r_drawLine2(box.position[0], box.position[1] + box.height, box.position[2], box.position[0] + box.width, box.position[1] + box.height, box.position[2], color);
    r_drawLine2(box.position[0], box.position[1] + box.height, box.position[2], box.position[0], box.position[1], box.position[2], color);
    r_drawLine2(box.position[0] + box.width, box.position[1], box.position[2], box.position[0] + box.width, box.position[1] + box.height, box.position[2], color);
    r_drawLine2(box.position[0], box.position[1], box.position[2], box.position[0], box.position[1], box.position[2] + box.length, color);
    r_drawLine2(box.position[0], box.position[1], box.position[2] + box.length, box.position[0] + box.width, box.position[1], box.position[2] + box.length, color);
    r_drawLine2(box.position[0] + box.width, box.position[1], box.position[2] + box.length, box.position[0] + box.width, box.position[1], box.position[2], color);
    r_drawLine2(box.position[0], box.position[1], box.position[2] + box.length, box.position[0], box.position[1] + box.height, box.position[2] + box.length, color);
    r_drawLine2(box.position[0], box.position[1] + box.height, box.position[2] + box.length, box.position[0] + box.width, box.position[1] + box.height, box.position[2] + box.length, color);
    r_drawLine2(box.position[0], box.position[1] + box.height, box.position[2], box.position[0], box.position[1] + box.height, box.position[2] + box.length, color);
    r_drawLine2(box.position[0] + box.width, box.position[1] + box.height, box.position[2], box.position[0] + box.width, box.position[1] + box.height, box.position[2] + box.length, color);
    r_drawLine2(box.position[0] + box.width, box.position[1], box.position[2] + box.length, box.position[0] + box.width, box.position[1] + box.height, box.position[2] + box.length, color);
}

void r_drawAABBWires2(vec3 box[2], vec4 color)
{
    AABB aabb;
    aabb.position[0] = box[0][0];
    aabb.position[1] = box[0][1];
    aabb.position[2] = box[0][2];

    aabb.width = (box[1][0] - box[0][0]) - 0.5;
    aabb.height = (box[1][1] - box[0][1]) - 0.5;
    aabb.length = (box[1][2] - box[0][2]) - 0.5;

    r_drawAABBWires(aabb, color);
}

void r_drawCube(vec3 size, vec3 pos, vec4 color)
{
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

    mat4 model;
    Math_Model(pos, size, 0, model);

    for (int i = 0; i < 36; i+= 3)
    {
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i], 1.0f, CUBE_POSITION_VERTICES[i]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 1], 1.0f, CUBE_POSITION_VERTICES[i + 1]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 2], 1.0f, CUBE_POSITION_VERTICES[i + 2]);

        r_drawTriangle(CUBE_POSITION_VERTICES[i], CUBE_POSITION_VERTICES[i + 1], CUBE_POSITION_VERTICES[i + 2], color);
    }

}

static void _drawAll()
{
    if (s_lineDrawData.vertices_count > 0)
    {
        _drawLineBatch();
    }
    if (s_triangleDrawData.vertices_count > 0)
    {
        _drawTriangleBatch();
    }
}

static void DrawMesh(const R_Mesh* p_mesh, vec3 pos)
{
    unsigned diffuse_nr = 1;
    unsigned specular_nr = 1;
    unsigned normal_nr = 1;
    unsigned height_nr = 1;
    glUseProgram(s_deferredShadingData.model_shader);

    for (unsigned i = 0; i < p_mesh->textures->elements_size; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);

        TextureData* array = p_mesh->textures->data;

        char number[256];
        memset(number, 0, 256);

        if (strcmp(array[i].type, "texture_diffuse") == 0)
        {
            sprintf(number, "%i", diffuse_nr++);
        }
        else if (strcmp(array[i].type, "texture_specular") == 0)
        {
            sprintf(number, "%i", specular_nr++);
        }
        else if (strcmp(array[i].type, "texture_normal") == 0)
        {
            sprintf(number, "%i", normal_nr++);
        }
        else if (strcmp(array[i].type, "texture_height") == 0)
        {
            sprintf(number, "%i", height_nr++);
        }

        char name_and_number[512];
        memset(name_and_number, 0, 512);

        strcpy(name_and_number, array[i].type);
        strcat(name_and_number, number);

        Shader_SetInteger(s_deferredShadingData.model_shader, name_and_number, i);
        glBindTexture(GL_TEXTURE_2D, array[i].texture.id);

  
        
    }


    mat4 m;

    vec3 size;
    memset(size, 16, sizeof(vec3));
    size[0] = 1;
    size[1] = 1;
    size[2] = 1;

    Math_Model(pos, size, 0, m);


    Shader_SetMatrix4(s_deferredShadingData.model_shader, "u_model", m);
    Shader_SetVector3f(s_deferredShadingData.model_shader, "u_viewPos", s_RenderData.camera->data.position[0], s_RenderData.camera->data.position[1], s_RenderData.camera->data.position[2]);
    glBindVertexArray(p_mesh->vao);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture.id);

    glDrawElements(GL_TRIANGLES, p_mesh->indices->elements_size, GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
}

static void DrawModel(R_Model* const p_model, vec3 pos)
{
    for (int i = 0; i < p_model->meshes->elements_size; i++)
    {
        R_Mesh* mesh_array = p_model->meshes->data;
        
        DrawMesh(&mesh_array[i], pos);

    }


}


void r_Draw(R_Model* const p_model, vec3 pos)
{
   
    DrawModel(p_model, pos);
}


void r_DrawWorldChunks2(LC_World* const world)
{
    if (s_RenderData.camera == NULL)
        return;

    //PERFORM FRUSTRUM CULLING ON CHUNKS AND ISSUE DRAW COMMANDS
    size_t chunk_count = world->chunks->pool->elements_size;

    glUseProgram(s_deferredShadingData.frustrum_select_shader);
    glDispatchCompute(chunk_count, 1, 1);

    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_ATOMIC_COUNTER_BUFFER);
    

    glBindVertexArray(world->vao);
    glBindBuffer(GL_ARRAY_BUFFER, world->vbo);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_deferredShadingData.draw_buffer);
    glBindBuffer(GL_PARAMETER_BUFFER, s_deferredShadingData.atomic_counter);

    if (render_settings.flags & RF__ENABLE_SHADOW_MAPPING)
    {
        //SET UP_LIGHT MATRICES
        mat4 light_matrixes[5];
        Math_getLightSpacesMatrixesForFrustrum(s_RenderData.camera, s_RenderData.window_width, s_RenderData.window_height, 5, s_shadowMapData.shadow_cascade_levels, world->sun.direction_to_center, light_matrixes);
        glBindBuffer(GL_UNIFORM_BUFFER, s_shadowMapData.light_space_matrices_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(light_matrixes), light_matrixes);

        glUseProgram(s_shadowMapData.shadow_depth_shader);
        glBindFramebuffer(GL_FRAMEBUFFER, s_shadowMapData.frame_buffer);
        glViewport(0, 0, render_settings.shadow_map_resolution, render_settings.shadow_map_resolution);
        glClear(GL_DEPTH_BUFFER_BIT);

        //disable cull face for peter panning
         glDisable(GL_CULL_FACE);

        //RENDER FROM LIGHTS PERSPECTIVE
        glMultiDrawArraysIndirectCount(GL_TRIANGLES, 0, 0, chunk_count, sizeof(DrawElementsIndirectCommand));

        
        if (render_settings.mssa_setting != R_MSSA_NONE)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.msFBO);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.FBO);
        }
        glEnable(GL_CULL_FACE);
        glViewport(0, 0, s_RenderData.window_width, s_RenderData.window_height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

   

    glUseProgram(s_deferredShadingData.simple_lighting_shader);

    Shader_SetMatrix4(s_deferredShadingData.simple_lighting_shader, "u_view", s_RenderData.camera->data.view_matrix);
    Shader_SetVector3f(s_deferredShadingData.simple_lighting_shader, "u_viewPos", s_RenderData.camera->data.position[0], s_RenderData.camera->data.position[1], s_RenderData.camera->data.position[2]);

    Shader_SetVector3f(s_deferredShadingData.simple_lighting_shader, "u_dirLight.color", world->sun.sun_color[0], world->sun.sun_color[1], world->sun.sun_color[2]);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_dirLight.ambient_intensity", 0.5f);
    Shader_SetFloat(s_deferredShadingData.simple_lighting_shader, "u_dirLight.diffuse_intensity", 0.8f);
    Shader_SetVector3f(s_deferredShadingData.simple_lighting_shader, "u_dirLight.direction", world->sun.direction_to_center[0], world->sun.direction_to_center[1], world->sun.direction_to_center[2]);

    //NORMAL RENDER PASS
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_atlas.id);
    if (render_settings.flags & RF__ENABLE_SHADOW_MAPPING)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, s_shadowMapData.light_depth_maps);
    }
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture.id);

    glMultiDrawArraysIndirectCount(GL_TRIANGLES, 0, 0, chunk_count, sizeof(DrawElementsIndirectCommand));

    //Clear draw and atomic buffers
    glClearBufferSubData(GL_DRAW_INDIRECT_BUFFER, GL_R32UI, 0, sizeof(DrawElementsIndirectCommand) * chunk_count, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(unsigned), GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    
}


void r_Update(R_Camera* const p_cam, ivec2 window_size)
{
    mat4 view_proj;
    glm_mat4_mul(p_cam->data.proj_matrix, p_cam->data.view_matrix, view_proj);

    glBindBuffer(GL_UNIFORM_BUFFER, s_RenderData.camera_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), view_proj);

    s_RenderData.camera = p_cam;

    glm_frustum_planes(view_proj, s_RenderData.camera_frustrum);

    glNamedBufferSubData(s_deferredShadingData.camera_info_ubo, 0, sizeof(vec4) * 6, s_RenderData.camera_frustrum);

    if (glfwGetKey(s_Window, GLFW_KEY_C) == GLFW_PRESS)
    {
        s_deferredShadingData.use_hi_z_cull = !s_deferredShadingData.use_hi_z_cull;
    }

    if (glfwGetKey(s_Window, GLFW_KEY_L) == GLFW_PRESS)
    {
        render_settings.mssa_setting = R_MSSA_8;
        printf("mssa on");
    }
    if (glfwGetKey(s_Window, GLFW_KEY_K) == GLFW_PRESS)
    {
        render_settings.mssa_setting = R_MSSA_NONE;
        printf("mssa off");
    }

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

void r_DrawScreenSprite(vec2 pos, R_Sprite* sprite)
{
      //draw the current batch??
    if (s_screenQuadDrawData.vertices_count + 4 >= SCREEN_QUAD_VERTICES_BUFFER_SIZE || s_screenQuadDrawData.tex_index >= SCREEN_QUAD_MAX_TEXTURES - 1)
    {
        _drawScreenQuadBatch();
    }
    
    int texture_slot_index = -1;

    //find or assign the texture
    for (int i = 0; i < SCREEN_QUAD_MAX_TEXTURES; i++)
    {
        //exists already?
        if (sprite->texture->id == s_screenQuadDrawData.tex_array[i])
        {
            texture_slot_index = i;
            break;
        }
    }
    //not found? then add to the array
    if (texture_slot_index == -1)
    {
        s_screenQuadDrawData.tex_array[s_screenQuadDrawData.tex_index] = sprite->texture->id;
        texture_slot_index = s_screenQuadDrawData.tex_index;

        s_screenQuadDrawData.tex_index++;
    }

    vec2 sprite_size;
    Sprite_getSize(sprite, sprite_size);

    //compute the model matrix
    mat4 model;
    Math_Model2D(pos, sprite_size, sprite->rotation, model);
    
    //set up the vertices
    vec3 vertices[] =
    {
          0.5f,  0.5f, 0.0f,// top right
          0.5f, -0.5f, 0.0f, // bottom right
         -0.5f, -0.5f, 0.0f,// bottom left
         -0.5f,  0.5f, 0.0f // top left 
    };

    for (int i = 0; i < 4; i++)
    {
        glm_mat4_mulv3(model, vertices[i], 1.0f, vertices[i]);
        
        s_screenQuadDrawData.screen_quad_vertices[s_screenQuadDrawData.vertices_count + i].position[0] = vertices[i][0];
        s_screenQuadDrawData.screen_quad_vertices[s_screenQuadDrawData.vertices_count + i].position[1] = vertices[i][1];

        s_screenQuadDrawData.screen_quad_vertices[s_screenQuadDrawData.vertices_count + i].tex_coords[0] = sprite->texture_coords[i][0];
        s_screenQuadDrawData.screen_quad_vertices[s_screenQuadDrawData.vertices_count + i].tex_coords[1] = sprite->texture_coords[i][1];

        s_screenQuadDrawData.screen_quad_vertices[s_screenQuadDrawData.vertices_count + i].tex_index = texture_slot_index;
    }
    s_screenQuadDrawData.vertices_count += 4;
    s_screenQuadDrawData.indices_count += 6;
}


void r_startFrame(R_Camera* const p_cam, ivec2 window_size, LC_World* const world)
{
    //update camera stuff
    r_Update(p_cam, window_size);

    if (render_settings.mssa_setting != R_MSSA_NONE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.msFBO);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, s_RenderData.FBO);
    }
   
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    
    //Chunks pass. TODO include other draw calls like models, monsters and so on 
    //that are not part of chunk drawing in between the shadow map pass
  //  r_DrawWorldChunks2(world);

    //Lines, triangles etc... pass
   // _drawAll();

    //Skybox pass
   // _drawSkybox();

   // RenderbillboardTexture();

  
    //text pass
    if (s_bitmapTextData.vertices_count > 0)
    {
        _drawTextBatch();
    }

    //Screen quads pass
    if (s_screenQuadDrawData.vertices_count > 0)
    {
        _drawScreenQuadBatch();
    }

}

void r_endFrame()
{
    if (render_settings.mssa_setting != R_MSSA_NONE)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_RenderData.msFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_RenderData.FBO);
        glBlitFramebuffer(0, 0, s_RenderData.window_width, s_RenderData.window_height, 0, 0, s_RenderData.window_width, s_RenderData.window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(s_deferredShadingData.post_process_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_RenderData.color_buffer_texture);
    glBindVertexArray(s_deferredShadingData.screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_deferredShadingData.screen_quad_vbo);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   
}
