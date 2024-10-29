/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Inits the renderer. Registers cvars, loads shaders and setups FBOs,
textures and etc...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


#include "r_core.h"
#include "input.h"
#include "r_public.h"
#include <string.h>
#include <glad/glad.h>
#include <assert.h>
#include <GLFW/glfw3.h>


extern R_CMD_Buffer* cmdBuffer;
extern R_DrawData* drawData;
extern R_BackendData* backend_data;
extern R_RenderPassData* pass;
extern R_Cvars r_cvars;
extern R_StorageBuffers storage;
extern R_Scene scene;
extern R_RendererResources resources;

extern void r_threadLoop();

static float INIT_WIDTH = 1280;
static float INIT_HEIGHT = 720;


static void CheckBoundFrameBufferStatus(const char* p_fboName)
{
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false && "Failed to create framebuffer");
    }
}

static void Init_registerCvars()
{
    r_cvars.r_multithread = Cvar_Register("r_multithread", "1", "Use multiple threads for rendering", CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_limitFPS = Cvar_Register("r_limitFPS", "1", "Limit FPS with r_maxFPS", CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_maxFPS = Cvar_Register("r_maxFPS", "144", NULL, CVAR__SAVE_TO_FILE, 30, 1000);
    r_cvars.r_useDirShadowMapping = Cvar_Register("r_useDirShadowMapping", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_DirShadowMapResolution = Cvar_Register("r_DirShadowMapResolution", "1024", NULL, CVAR__SAVE_TO_FILE, 256, 4048);
    r_cvars.r_drawSky = Cvar_Register("r_drawSky", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);

    //EFFECTS
    r_cvars.r_useDepthOfField = Cvar_Register("r_useDepthOfField", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_useFxaa = Cvar_Register("r_useFxaa", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_useBloom = Cvar_Register("r_useBloom", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_useSsr = Cvar_Register("r_useSsr", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_useSsao = Cvar_Register("r_useSsao", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_bloomStrength = Cvar_Register("r_bloomStrength", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_ssaoBias = Cvar_Register("r_ssaoBias", "0.025", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_ssaoRadius = Cvar_Register("r_ssaoRadius", "0.6", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_ssaoStrength = Cvar_Register("r_ssaoStrength", "1", NULL, CVAR__SAVE_TO_FILE, 0, 8);
    r_cvars.r_ssaoHalfSize = Cvar_Register("r_ssaoHalfSize", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_Exposure = Cvar_Register("r_Exposure", "1", NULL, CVAR__SAVE_TO_FILE, 0, 16);
    r_cvars.r_Gamma = Cvar_Register("r_Gamma", "2.2", NULL, CVAR__SAVE_TO_FILE, 0, 3);
    r_cvars.r_Brightness = Cvar_Register("r_Brightness", "1", NULL, CVAR__SAVE_TO_FILE, 0.01, 8);
    r_cvars.r_Contrast = Cvar_Register("r_Contrast", "1", NULL, CVAR__SAVE_TO_FILE, 0.01, 8);
    r_cvars.r_Saturation = Cvar_Register("r_Saturation", "1", NULL, CVAR__SAVE_TO_FILE, 0.01, 8);
  
    //WINDOW SPECIFIC
    r_cvars.w_width = Cvar_Register("w_width", "1024", "Window width", CVAR__SAVE_TO_FILE, 480, 4048);
    r_cvars.w_height = Cvar_Register("w_heigth", "720", "Window heigth", CVAR__SAVE_TO_FILE, 480, 4048);
    r_cvars.w_useVsync = Cvar_Register("w_useVsync", "1", "Use vsync", CVAR__SAVE_TO_FILE, 0, 1);

    //CAMERA SPECIFIC
    r_cvars.cam_fov = Cvar_Register("cam_fov", "90", NULL, CVAR__SAVE_TO_FILE, 1, 130);
    r_cvars.cam_mouseSensitivity = Cvar_Register("cam_mouseSensitivity", "1", NULL, CVAR__SAVE_TO_FILE, 0, 20);
    r_cvars.cam_zNear = Cvar_Register("cam_zNear", "0.1", NULL, CVAR__SAVE_TO_FILE, 0.01, 10.0);
    r_cvars.cam_zFar = Cvar_Register("cam_zFar", "1500", NULL, CVAR__SAVE_TO_FILE, 100, 4000);

    //SHADOW SPECIFIC
    r_cvars.r_allowParticleShadows = Cvar_Register("r_allowParticleShadws", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_allowTransparentShadows = Cvar_Register("r_allowTransparentShadows", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_shadowBias = Cvar_Register("r_shadowBias", "-0.07", NULL, CVAR__SAVE_TO_FILE, -10, 10);
    r_cvars.r_shadowNormalBias = Cvar_Register("r_shadowNormalBias", "0.0", NULL, CVAR__SAVE_TO_FILE, 0, 10);
    r_cvars.r_shadowVarianceMin = Cvar_Register("r_shadowVarianceMin", "0.0001", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_shadowLightBleedReduction = Cvar_Register("r_shadowLightBleedReduction", "1.0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    
    //DEBUG
    r_cvars.r_drawDebugTexture = Cvar_Register("r_drawDebugTexture", "-1", NULL, CVAR__SAVE_TO_FILE, -1, 6);
    r_cvars.r_wireframe = Cvar_Register("r_wireframe", "0", NULL, CVAR__SAVE_TO_FILE, 0, 2);
    r_cvars.r_drawPanel = Cvar_Register("r_drawPanel", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
}

static void Init_ScreenQuadDrawData()
{
	drawData->screen_quad.shader = Shader_CompileFromFile("src/render/shaders/screen/screen_shader.vert", "src/render/shaders/screen/screen_shader.frag", NULL);

    drawData->screen_quad.tex_index = 0;

    glGenVertexArrays(1, &drawData->screen_quad.vao);
    glGenBuffers(1, &drawData->screen_quad.vbo);
    glGenBuffers(1, &drawData->screen_quad.ebo);

    glBindVertexArray(drawData->screen_quad.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->screen_quad.vbo);

    glBufferStorage(GL_ARRAY_BUFFER, sizeof(drawData->screen_quad.vertices), NULL, GL_DYNAMIC_STORAGE_BIT);

    size_t indices_count = SCREEN_QUAD_VERTICES_BUFFER_SIZE * 6;

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

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawData->screen_quad.ebo);
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices_count, indices, GL_DYNAMIC_STORAGE_BIT);

        free(indices);
    }


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, tex_coords)));
    glEnableVertexAttribArray(1);

    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, packed_texIndex_Flags)));
    glEnableVertexAttribArray(2);

    glVertexAttribIPointer(3, 4, GL_UNSIGNED_BYTE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, color)));
    glEnableVertexAttribArray(3);

    //SETUP WHITE TEXTURE  
    //create texture
  //  glGenTextures(1, &drawData->screen_quad.white_texture);
   // glBindTexture(GL_TEXTURE_2D, drawData->screen_quad.white_texture);

    //set texture wrap and filter modes
   // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // uint32_t color_White = 0xffffffff;

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color_White);

    glUseProgram(drawData->screen_quad.shader);

    //set up samplers
    for (int i = 0; i < 32; i++)
    {
        char buf[256];
        memset(buf, 0, sizeof(buf));

        sprintf(buf, "u_textures[%i]", i);

        Shader_SetInteger(drawData->screen_quad.shader, buf, i);
    }

    //glBindTextureUnit(0, drawData->screen_quad.white_texture);

  //  drawData->screen_quad.tex_array[0] = drawData->screen_quad.white_texture;
}

static void Init_TextDrawData()
{
    R_TextDrawData* data = &drawData->text;

    data->shader = Shader_CompileFromFile("src/shaders/screen_shader.vs", "src/shaders/screen_shader.fs", NULL);

    glGenVertexArrays(1, &data->vao);
    glGenBuffers(1, &data->vbo);
    glGenBuffers(1, &data->ebo);

    glBindVertexArray(data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, data->vbo);

    glBufferStorage(GL_ARRAY_BUFFER, sizeof(data->vertices), NULL, GL_DYNAMIC_STORAGE_BIT);

    size_t indices_count = TEXT_VERTICES_BUFFER_SIZE * 6;

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

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawData->screen_quad.ebo);
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices_count, indices, GL_DYNAMIC_STORAGE_BIT);

        free(indices);
    }


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)(offsetof(TextVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)(offsetof(TextVertex, tex_coords)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)(offsetof(TextVertex, color)));
    glEnableVertexAttribArray(2);

    glVertexAttribIPointer(3, 1, GL_INT, GL_FALSE, sizeof(TextVertex), (void*)(offsetof(TextVertex, effect_flags)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)(offsetof(TextVertex, tex_index)));
    glEnableVertexAttribArray(4);

   
    glUseProgram(data->shader);

    //set up samplers
    GLint texture_array_location = glGetUniformLocation(data->shader, "u_textures");
    int32_t samplers[32];

    for (int i = 0; i < 32; i++)
    {
        samplers[i] = i;
    }

    glUniform1iv(texture_array_location, 32, samplers);
}

static void Init_LineDrawData()
{
    glGenVertexArrays(1, &drawData->lines.vao);
    glGenBuffers(1, &drawData->lines.vbo);

    glBindVertexArray(drawData->lines.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->lines.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(drawData->lines.vertices), NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (void*)(offsetof(BasicVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(BasicVertex), (void*)(offsetof(BasicVertex, color)));
    glEnableVertexAttribArray(1);
}

static void Init_TriangleDrawData()
{
    glGenVertexArrays(1, &drawData->triangle.vao);
    glGenBuffers(1, &drawData->triangle.vbo);

    glBindVertexArray(drawData->triangle.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->triangle.vbo);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(drawData->triangle.vertices), NULL, GL_DYNAMIC_STORAGE_BIT);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, color)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, texOffset)));
    glEnableVertexAttribArray(2);

    glVertexAttribIPointer(3, 1, GL_INT, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, texture_index)));
    glEnableVertexAttribArray(3);
}

static void Init_DrawData()
{
    Init_ScreenQuadDrawData();
    Init_LineDrawData();
    Init_TriangleDrawData();
}


static void Init_GeneralPassData()
{
    pass->general.basic_3d_shader = Shader_CompileFromFile("src/render/shaders/basic_3d.vert", "src/render/shaders/basic_3d.frag", NULL);
    pass->general.triangle_3d_shader = Shader_CompileFromFile("src/render/shaders/triangle_3d.vert", "src/render/shaders/triangle_3d.frag", NULL);
    pass->general.box_blur_shader = ComputeShader_CompileFromFile("src/render/shaders/screen/box_blur.comp");
    pass->general.downsample_shader = Shader_CompileFromFile("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/downsample.frag", NULL);
    pass->general.upsample_shader = Shader_CompileFromFile("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/upsample.frag", NULL);
    pass->general.copy_shader = ComputeShader_CompileFromFile("src/render/shaders/screen/copy.comp");

    glUseProgram(pass->general.triangle_3d_shader);
#ifndef USE_BINDLESS_TEXTURES
    GLint texture_array_location = glGetUniformLocation(pass->general.triangle_3d_shader, "texture_arr");
    int samplers[32];

    for (int i = 0; i < 32; i++)
    {
        samplers[i] = i;
    }

    glUniform1iv(texture_array_location, 32, samplers);
#endif

    glGenFramebuffers(1, &pass->general.halfsize_fbo);
    glGenTextures(1, &pass->general.depth_halfsize_texture);
    glGenTextures(1, &pass->general.normal_halfsize_texture);

    glBindTexture(GL_TEXTURE_2D, pass->general.depth_halfsize_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, INIT_WIDTH / 2, INIT_HEIGHT / 2, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, pass->general.normal_halfsize_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, INIT_WIDTH / 2, INIT_HEIGHT / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->general.halfsize_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->general.normal_halfsize_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->general.depth_halfsize_texture, 0);

    CheckBoundFrameBufferStatus("General");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void Init_PostProcessData()
{
    RInternal_UpdatePostProcessShader(true);
    pass->post.debug_shader = Shader_CompileFromFile("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/debug_screen.frag", NULL);

    glUseProgram(pass->post.debug_shader);
    Shader_SetInteger(pass->post.debug_shader, "NormalMetalTexture", 0);
    Shader_SetInteger(pass->post.debug_shader, "AlbedoRoughTexture", 1);
    Shader_SetInteger(pass->post.debug_shader, "DepthTexture", 2);
    Shader_SetInteger(pass->post.debug_shader, "AOTexture", 3);
    Shader_SetInteger(pass->post.debug_shader, "BloomTexture", 4);
}

static void Init_LCSpecificData()
{
    for (int i = 0; i < 4; i++)
    {
        drawData->lc_world.shadow_counts[i] = dA_INIT(int, 0);
        drawData->lc_world.shadow_firsts[i] = dA_INIT(int, 0);

        drawData->lc_world.shadow_counts_transparent[i] = dA_INIT(int, 0);
        drawData->lc_world.shadow_firsts_transparent[i] = dA_INIT(int, 0);
    }
    drawData->lc_world.shadow_sorted_chunk_indexes = dA_INIT(int, 0);
    drawData->lc_world.shadow_sorted_chunk_transparent_indexes = dA_INIT(int, 0);

    pass->lc.depthPrepass_shader = Shader_CompileFromFile("src/render/shaders/lc_world/lc_depth.vert", "src/render/shaders/depth.frag", NULL);
    pass->lc.gBuffer_shader = Shader_CompileFromFile("src/render/shaders/lc_world/lc_g_buffer.vert", "src/render/shaders/lc_world/lc_g_buffer.frag", NULL);
    pass->lc.shadow_map_shader = Shader_CompileFromFile("src/render/shaders/lc_world/lc_shadow_map.vert", "src/render/shaders/shadow_map.frag", NULL);
    pass->lc.transparents_forward_shader = Shader_CompileFromFile("src/render/shaders/lc_world/lc_transparent_forward.vert", "src/render/shaders/lc_world/lc_transparent_forward.frag", NULL);
    pass->lc.chunk_process_shader = ComputeShader_CompileFromFile("src/render/shaders/lc_world/chunk_process.comp");
    pass->lc.occlussion_boxes_shader = Shader_CompileFromFile("src/render/shaders/lc_world/lc_occlusion_boxes.vert", "src/render/shaders/lc_world/lc_occlusion_boxes.frag", NULL);
    const char* define[1] = { "TRANSPARENT_SHADOWS" };
    pass->lc.transparents_shadow_map_shader = Shader_CompileFromFileDefine("src/render/shaders/lc_world/lc_shadow_map.vert", "src/render/shaders/lc_world/lc_shadow_map.frag", NULL, define, 1);
    const char* define2[1] = { "SEMI_TRANSPARENT" };
    pass->lc.transparents_gBuffer_shader = Shader_CompileFromFileDefine("src/render/shaders/lc_world/lc_g_buffer.vert", "src/render/shaders/lc_world/lc_g_buffer.frag", NULL, define2, 1);
    //SETUP SHADER UNIFORMS
    glUseProgram(pass->lc.gBuffer_shader);
    Shader_SetInteger(pass->lc.gBuffer_shader, "texture_atlas", 0);
    Shader_SetInteger(pass->lc.gBuffer_shader, "texture_atlas_normal", 1);
    Shader_SetInteger(pass->lc.gBuffer_shader, "texture_atlas_mer", 2);

    glUseProgram(pass->lc.transparents_gBuffer_shader);
    Shader_SetInteger(pass->lc.transparents_gBuffer_shader, "texture_atlas", 0);
    Shader_SetInteger(pass->lc.transparents_gBuffer_shader, "texture_atlas_normal", 1);
    Shader_SetInteger(pass->lc.transparents_gBuffer_shader, "texture_atlas_mer", 2);


    glUseProgram(pass->lc.transparents_forward_shader);
    Shader_SetInteger(pass->lc.transparents_forward_shader, "texture_atlas", 0);
    Shader_SetInteger(pass->lc.transparents_forward_shader, "texture_atlas_normal", 1);
    Shader_SetInteger(pass->lc.transparents_forward_shader, "texture_atlas_mer", 2);

    Shader_SetInteger(pass->lc.transparents_forward_shader, "brdfLUT", 3);
    Shader_SetInteger(pass->lc.transparents_forward_shader, "irradianceMap", 4);
    Shader_SetInteger(pass->lc.transparents_forward_shader, "preFilterMap", 5);

    Shader_SetInteger(pass->lc.transparents_forward_shader, "momentMaps", 6);
    Shader_SetInteger(pass->lc.transparents_forward_shader, "shadowMapsDepth", 7);

    glUseProgram(pass->lc.transparents_shadow_map_shader);
    Shader_SetInteger(pass->lc.transparents_shadow_map_shader, "texture_atlas", 0);
}

static void Init_DeferredData()
{
    //SETUP SHADERS
    pass->deferred.depthPrepass_shader = 0;
    //assert(pass->deferred.depthPrepass_shader > 0);

    pass->deferred.gBuffer_shader = 0;
   // assert(pass->deferred.gBuffer_shader > 0);
    
    RInternal_UpdateDeferredShadingShader(true);

    //SETUP FRAMEBUFFER AND TEXTURES
    glGenFramebuffers(1, &pass->deferred.FBO);
    glGenTextures(1, &pass->deferred.depth_texture);
    glGenTextures(1, &pass->deferred.gNormalMetal_texture);
    glGenTextures(1, &pass->deferred.gColorRough_texture);
    glGenTextures(1, &pass->deferred.gEmissive_texture);
    
    glBindFramebuffer(GL_FRAMEBUFFER, pass->deferred.FBO);

    //DEPTH
    glBindTexture(GL_TEXTURE_2D, pass->deferred.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, INIT_WIDTH, INIT_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->deferred.depth_texture, 0);
    //NORMAL
    glBindTexture(GL_TEXTURE_2D, pass->deferred.gNormalMetal_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, INIT_WIDTH, INIT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->deferred.gNormalMetal_texture, 0);
    //COLOR
    glBindTexture(GL_TEXTURE_2D, pass->deferred.gColorRough_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, INIT_WIDTH, INIT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, pass->deferred.gColorRough_texture, 0);
    //EMMISIVE
    glBindTexture(GL_TEXTURE_2D, pass->deferred.gEmissive_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, INIT_WIDTH, INIT_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, pass->deferred.gEmissive_texture, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    CheckBoundFrameBufferStatus("Deferred");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void Init_MainSceneData()
{
    //Shaders
    pass->scene.skybox_shader = Shader_CompileFromFile("src/render/shaders/skybox.vert", "src/render/shaders/skybox.frag", NULL);
    pass->scene.oit_composite_shader = Shader_CompileFromFile("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/oit_composite.frag", NULL);

    glUseProgram(pass->scene.skybox_shader);
    Shader_SetInteger(pass->scene.skybox_shader, "skybox_texture", 0);

    glGenFramebuffers(1, &pass->scene.FBO);
    glGenFramebuffers(1, &pass->scene.transparent_FBO);

    //Setup scene textures
    glGenTextures(1, &pass->scene.MainSceneColorBuffer);
    glGenTextures(1, &pass->scene.transparent_accum_texture);
    glGenTextures(1, &pass->scene.transparent_reveal_texture);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);

    //Main scene hdr buffer
    glBindTexture(GL_TEXTURE_2D, pass->scene.MainSceneColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, INIT_WIDTH, INIT_HEIGHT, 0,
        GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->scene.MainSceneColorBuffer, 0);
    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->deferred.depth_texture, 0);

    CheckBoundFrameBufferStatus("Main scene");

    assert(pass->deferred.depth_texture > 0 && "Init deferred first");

    //Transparent
    glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.transparent_FBO);

    //Accum texture
    glBindTexture(GL_TEXTURE_2D, pass->scene.transparent_accum_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, INIT_WIDTH, INIT_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //Reveal texture
    glBindTexture(GL_TEXTURE_2D, pass->scene.transparent_reveal_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, INIT_WIDTH, INIT_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->scene.transparent_accum_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, pass->scene.transparent_reveal_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->deferred.depth_texture, 0);
    unsigned int attachments2[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments2);

    CheckBoundFrameBufferStatus("Transparents FBO");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void Init_AoData()
{
    //SETUP SHADER
    //pass->ao.shader = Shader_CompileFromFile("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/ssao.frag", NULL);
    pass->ao.shader = ComputeShader_CompileFromFile("src/render/shaders/screen/ssao.comp");
    assert(pass->ao.shader > 0);

    //SETUP SHADER UNIFORMS
    glUseProgram(pass->ao.shader);
    Shader_SetInteger(pass->ao.shader, "depth_texture", 0);
    Shader_SetInteger(pass->ao.shader, "noise_texture", 1);
    Shader_SetInteger(pass->ao.shader, "gNormalMetal", 2);

    //SETUP TEXTURES 
    glGenTextures(1, &pass->ao.ao_texture);
    glGenTextures(1, &pass->ao.blurred_ao_texture);
    glGenTextures(1, &pass->ao.noise_texture);
   
    glBindTexture(GL_TEXTURE_2D, pass->ao.ao_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, INIT_WIDTH, INIT_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, pass->ao.blurred_ao_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, INIT_WIDTH, INIT_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    vec2 ssao_noise[32];
    srand(time(NULL));
    for (int i = 0; i < 16; i++)
    {
        ssao_noise[i][0] = ((float)rand() / (float)RAND_MAX) * 2.0 - 1.0;
        ssao_noise[i][1] = ((float)rand() / (float)RAND_MAX) * 2.0 - 1.0;
    }

    glBindTexture(GL_TEXTURE_2D, pass->ao.noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 4, 4, 0, GL_RG, GL_FLOAT, ssao_noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

}

static void Init_BloomData()
{
    glGenFramebuffers(1, &pass->bloom.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->bloom.FBO);

    //Generate mip textures
    float mipWidth = INIT_WIDTH;
    float mipHeight = INIT_HEIGHT;

    for (int i = 0; i < BLOOM_MIP_COUNT; i++)
    {
        mipWidth *= 0.5;
        mipHeight *= 0.5;

        glGenTextures(1, &pass->bloom.mip_textures[i]);
        glBindTexture(GL_TEXTURE_2D, pass->bloom.mip_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mipWidth, mipHeight, 0, GL_RGB, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        pass->bloom.mip_sizes[i][0] = mipWidth;
        pass->bloom.mip_sizes[i][1] = mipHeight;
    }
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, pass->bloom.mip_textures[0], 0);

    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    CheckBoundFrameBufferStatus("Bloom");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void Init_DofData()
{
    //SETUP SHADER
    pass->dof.bohek_blur_shader = Shader_CompileFromFile("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/dof_blur.frag", NULL);

    glUseProgram(pass->dof.bohek_blur_shader);
    Shader_SetInteger(pass->dof.bohek_blur_shader, "source_texture", 0);
    Shader_SetInteger(pass->dof.bohek_blur_shader, "depth_texture", 1);

    glGenFramebuffers(1, &pass->dof.FBO);
    glGenTextures(1, &pass->dof.dof_texture);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->dof.FBO);
    glBindTexture(GL_TEXTURE_2D, pass->dof.dof_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, INIT_WIDTH, INIT_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->dof.dof_texture, 0);

    CheckBoundFrameBufferStatus("DOF");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void Init_IBLData()
{
    //SETUP SHADERS
    pass->ibl.equirectangular_to_cubemap_shader = Shader_CompileFromFile("src/render/shaders/cubemap/cubemap.vert", "src/render/shaders/cubemap/equirectangular_to_cubemap.frag", NULL);
    pass->ibl.irradiance_convulution_shader = Shader_CompileFromFile("src/render/shaders/cubemap/cubemap.vert", "src/render/shaders/cubemap/irradiance_convolution.frag", NULL);
    pass->ibl.prefilter_cubemap_shader = Shader_CompileFromFile("src/render/shaders/cubemap/cubemap.vert", "src/render/shaders/cubemap/prefilter_cubemap.frag", NULL);
    pass->ibl.brdf_shader = Shader_CompileFromFile("src/render/shaders/screen/base_screen.vert", "src/render/shaders/screen/brdf.frag", NULL);

    glGenFramebuffers(1, &pass->ibl.FBO);
    glGenRenderbuffers(1, &pass->ibl.RBO);
    glGenTextures(1, &pass->ibl.envCubemapTexture);
    glGenTextures(1, &pass->ibl.prefilteredCubemapTexture);
    glGenTextures(1, &pass->ibl.irradianceCubemapTexture);
    glGenTextures(1, &pass->ibl.brdfLutTexture);

    glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.prefilteredCubemapTexture);
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

    glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.irradianceCubemapTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, pass->ibl.brdfLutTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pass->ibl.RBO);

    CheckBoundFrameBufferStatus("IBL");

    //COMPUTE VIEW MATRIXES FOR CUBE
    vec3 eye, center, up;
    glm_vec3_zero(eye);
    glm_vec3_zero(center);
    glm_vec3_zero(up);
    {
        center[0] = 1.0;
        up[1] = -1.0;
        glm_lookat(eye, center, up, pass->ibl.cube_view_matrixes[0]);

        center[0] = -1.0;
        glm_lookat(eye, center, up, pass->ibl.cube_view_matrixes[1]);
    }
    glm_vec3_zero(center);
    glm_vec3_zero(up);
    {
        center[1] = 1.0;
        up[2] = 1.0;
        glm_lookat(eye, center, up, pass->ibl.cube_view_matrixes[2]);

        center[1] = -1.0;
        up[2] = -1.0;
        glm_lookat(eye, center, up, pass->ibl.cube_view_matrixes[3]);
    }
    glm_vec3_zero(center);
    glm_vec3_zero(up);
    {
        center[2] = 1.0;
        up[1] = -1.0;
        glm_lookat(eye, center, up, pass->ibl.cube_view_matrixes[4]);

        center[2] = -1.0;
        glm_lookat(eye, center, up, pass->ibl.cube_view_matrixes[5]);
    }

    //AND PROJ
    glm_perspective(glm_rad(90.0), 1.0, 0.1, 10.0, pass->ibl.cube_proj);
}

static void Init_ShadowMappingData()
{
    //SETUP SHADER
    pass->shadow.depth_shader = Shader_CompileFromFile("src/shaders/shadow_depth_shader.vs", "src/shaders/shadow_depth_shader.fs", "src/shaders/shadow_depth_shader.gs");
    assert(pass->shadow.depth_shader > 0);

    //SETUP FRAMEBUFFER
    glGenFramebuffers(1, &pass->shadow.FBO);
    glGenTextures(1, &pass->shadow.moment_maps);
    glGenTextures(1, &pass->shadow.depth_maps);

    glBindTexture(GL_TEXTURE_2D_ARRAY, pass->shadow.moment_maps);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R16, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, SHADOW_CASCADE_LEVELS, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindTexture(GL_TEXTURE_2D_ARRAY, pass->shadow.depth_maps);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT16, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, SHADOW_CASCADE_LEVELS, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT , NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->shadow.FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pass->shadow.moment_maps, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pass->shadow.depth_maps, 0);
    //glDrawBuffer(GL_NONE);
    //glReadBuffer(GL_NONE);

    CheckBoundFrameBufferStatus("Shadow");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float z_far = 1500;

    pass->shadow.cascade_levels[0] = z_far / 350.0f;
    pass->shadow.cascade_levels[1] = z_far / 180.0f;
    pass->shadow.cascade_levels[2] = z_far / 70.0f;
    pass->shadow.cascade_levels[3] = z_far / 10.0f;
}

static void Init_ParticlePassData()
{
    pass->particles.particle_render_shader = Shader_CompileFromFile("src/render/shaders/particle_render.vert", "src/render/shaders/particle_render.frag", NULL);
    pass->particles.particle_shadow_map_shader = Shader_CompileFromFile("src/render/shaders/particle_shadow_map.vert", "src/render/shaders/particle_shadow_map.frag", NULL);
    pass->particles.particle_process_shader = ComputeShader_CompileFromFile("src/render/shaders/compute/particle_process.comp");
    pass->particles.emitter_process_shader = ComputeShader_CompileFromFile("src/render/shaders/compute/emitter_process.comp");
  
    glUseProgram(pass->particles.particle_render_shader);
    Shader_SetInteger(pass->particles.particle_render_shader, "texture_atlas", 0);
    Shader_SetInteger(pass->particles.particle_render_shader, "shadowMaps", 1);

#ifndef USE_BINDLESS_TEXTURES
    GLint texture_array_location = glGetUniformLocation(pass->particles.particle_render_shader, "textures_arr");
    int samplers[32];

    for (int i = 0; i < 32; i++)
    {
        samplers[i] = i;
    }

    glUniform1iv(texture_array_location, 32, samplers);
#endif

    glUseProgram(pass->particles.particle_process_shader);
    Shader_SetInteger(pass->particles.particle_process_shader, "depth_texture", 0);
    Shader_SetInteger(pass->particles.particle_process_shader, "gNormal_texture", 1);
}

static void Init_PassData()
{
    Init_PostProcessData();
    Init_GeneralPassData();
    Init_LCSpecificData();
    Init_DeferredData();
    Init_MainSceneData();
    Init_AoData();
    Init_BloomData();
    Init_DofData();
    Init_IBLData();
    Init_ShadowMappingData();
    Init_ParticlePassData();
}

static void Init_SceneData()
{
    glGenBuffers(1, &scene.camera_ubo);
    glGenBuffers(1, &scene.scene_ubo);

    glBindBuffer(GL_UNIFORM_BUFFER, scene.camera_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(R_SceneCameraData), NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, scene.scene_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(R_SceneData), NULL, GL_DYNAMIC_DRAW);

    //Init scene defaults
    scene.scene_data.heightFogMin = 30;
    scene.scene_data.heightFogMax = 11;
    scene.scene_data.heightFogCurve = 1;
    scene.scene_data.depthFogBegin = 10;
    scene.scene_data.depthFogEnd = 100;
    scene.scene_data.depthFogCurve = 0;
    scene.scene_data.fogDensity = 1;

    scene.scene_data.dirLightDirection[0] = 1;
    scene.scene_data.dirLightDirection[1] = 1;
    scene.scene_data.dirLightDirection[2] = 1;

    scene.scene_data.dirLightColor[0] = 1;
    scene.scene_data.dirLightColor[1] = 1;
    scene.scene_data.dirLightColor[2] = 1;

    scene.scene_data.dirLightAmbientIntensity = 8;
    scene.scene_data.dirLightSpecularIntensity = 24;
}

static bool _initRenderThread()
{
    backend_data->thread.handle = CreateThread
    (
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)r_threadLoop,
        0,
        0,
        &backend_data->thread.id
    );
    
    if (!backend_data->thread.handle)
    {
        return false;
    }
    
    //create events
    backend_data->thread.event_active = CreateEvent(NULL, TRUE, FALSE, NULL);
    backend_data->thread.event_completed = CreateEvent(NULL, TRUE, FALSE, NULL);
    backend_data->thread.event_work_permssion = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!backend_data->thread.event_active || !backend_data->thread.event_completed || !backend_data->thread.event_work_permssion)
        return false;


    ResetEvent(backend_data->thread.event_active);
    ResetEvent(backend_data->thread.event_completed);
    ResetEvent(backend_data->thread.event_work_permssion);

    return true;
}

static void Init_StorageBuffers()
{
    memset(&storage, 0, sizeof(storage));

    storage.particle_emitter_clients = FL_INIT(ParticleEmitterSettings);

    storage.instances = RSB_Create(100, sizeof(vec4), RSB_FLAG__RESIZABLE);
    storage.point_lights = RSB_Create(100, sizeof(PointLight2), RSB_FLAG__RESIZABLE | RSB_FLAG__WRITABLE | RSB_FLAG__POOLABLE);
    storage.particles = DRB_Create(100000, 12, DRB_FLAG__POOLABLE | DRB_FLAG__RESIZABLE);
    storage.particle_emitters = RSB_Create(200, sizeof(ParticleEmitterGL), RSB_FLAG__RESIZABLE | RSB_FLAG__WRITABLE | RSB_FLAG__POOLABLE);
    storage.texture_handles = RSB_Create(100, sizeof(GLuint64), RSB_FLAG__RESIZABLE | RSB_FLAG__WRITABLE | RSB_FLAG__POOLABLE);
    storage.collider_boxes = DRB_Create(1000, 30, DRB_FLAG__POOLABLE | DRB_FLAG__RESIZABLE);
}

static void Init_RendererResources()
{
    //QUAD DATA
    glGenVertexArrays(1, &resources.quadVAO);
    glGenBuffers(1, &resources.quadVBO);

    const float quad_vertices[16] = 
    {       -1, -1, 0, 0,
            -1,  1, 0, 1,
            1,  1, 1, 1,
            1, -1, 1, 0,
    };
    glBindVertexArray(resources.quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    //CUBE DATA
    glGenVertexArrays(1, &resources.cubeVAO);
    glGenBuffers(1, &resources.quadVBO);

    const float cube_vertices[] =
    {
        -1, 1, 1, 0, 1, 0,
         1,  1, 1, 1, 1, 0,
         -1, -1, 1, 1, 0, 0,
         1, -1, 1, 0, 0, 0,

         1, -1, -1, 0, 1, 0,
         1, 1, 1, 1, 1, 0,
         1, 1, -1, 1, 0, 0,
        -1, 1, 1, 0, 0, 0,

         -1, 1, -1, 0, 1, 0,
         -1, -1, 1, 1, 1, 0,
         -1, -1, -1, 1, 0, 0,
          1, -1, -1, 0, 0, 0,

         -1,  1, -1, 1, 0, 0,
          1,  1, -1, 0, 0, 0
    };

    glBindVertexArray(resources.cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, resources.quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3) * 2, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3) * 2, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
}

void Init_SetupGLBindingPoints()
{
    //BINDING POINTS ARE NEVER CHANGED

    //UBOS:
    //0: SCENE
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, scene.scene_ubo);
    //1: CAMERA
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, scene.camera_ubo);

    //SSBOS
    //0: POINT LIGHTS
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, storage.point_lights.buffer);
    //1: SPOT LIGHTS
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, storage.point_lights.buffer);
    //2: POINT LIGHTS
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, storage.point_lights.buffer);
    

    //5: PARTICLE EMITTERS
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, storage.particle_emitters.buffer);

    //6: PARTICLES
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, storage.particles.buffer);

    //7: INSTANCE DATA
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, storage.instances.buffer);

    //8: COLLISION DATA (USED FOR PARTICLE COLLISIONS)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, storage.collider_boxes.buffer);
}

static void Init_SetupGLState()
{
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
}

static void Init_Inputs()
{
    Input_registerAction("Open-panel");

    Input_setActionBinding("Open-panel", IT__KEYBOARD, GLFW_KEY_P, 0);
}

static int Init_Mem()
{
    cmdBuffer = malloc(sizeof(R_CMD_Buffer));

    if (!cmdBuffer)
    {
        return false;
    }
    memset(cmdBuffer, 0, sizeof(R_CMD_Buffer));

    cmdBuffer->cmds_data = malloc(RENDER_BUFFER_COMMAND_ALLOC_SIZE);
    if (!cmdBuffer->cmds_data)
    {
        free(cmdBuffer);
        return false;
    }
    memset(cmdBuffer->cmds_data, 0, RENDER_BUFFER_COMMAND_ALLOC_SIZE);
    cmdBuffer->cmds_ptr = cmdBuffer->cmds_data;

    drawData = malloc(sizeof(R_DrawData));
    if (!drawData)
    {
        free(cmdBuffer->cmds_data);
        free(cmdBuffer);
        return false;
    }
    memset(drawData, 0, sizeof(R_DrawData));

    pass = malloc(sizeof(R_RenderPassData));

    if (!pass)
    {
        free(cmdBuffer->cmds_data);
        free(cmdBuffer);
        free(drawData);
        return false;
    }
    memset(pass, 0, sizeof(R_RenderPassData));

    backend_data = malloc(sizeof(R_BackendData));

    if (!backend_data)
    {
        free(cmdBuffer->cmds_data);
        free(cmdBuffer);
        free(drawData);
        free(pass);
        return false;
    }
    memset(backend_data, 0, sizeof(R_BackendData));

    //STATIC MEM
    memset(&storage, 0, sizeof(storage));
    memset(&scene, 0, sizeof(scene));
    memset(&resources, 0, sizeof(resources));

    return true;
}

int Renderer_Init(int p_initWidth, int p_initHeight)
{
    //REGISTER CVARS
    memset(&r_cvars, 0, sizeof(r_cvars));
    Init_registerCvars();

    //TRY TO LOAD A CVAR CONFIG FILE, (if we have one)


    //ALLOCATE AND MEMSET DATA
    if (!Init_Mem())
    {
        return false;
    }
    //INIT GL DATA
    Init_DrawData();
    Init_PassData();
    Init_SceneData();

    Init_RendererResources();
    Init_StorageBuffers();

    Init_SetupGLBindingPoints();

    Init_Inputs();

    backend_data->screenSize[0] = INIT_WIDTH;
    backend_data->screenSize[1] = INIT_HEIGHT;

    backend_data->halfScreenSize[0] = (float)INIT_WIDTH / 2.0;
    backend_data->halfScreenSize[1] = (float)INIT_HEIGHT / 2.0;

    scene.dirty_cam = true;
    scene.camera.screen_size[0] = backend_data->screenSize[0];
    scene.camera.screen_size[1] = backend_data->screenSize[1];

    return true;
}

void Renderer_Exit()
{
    FL_Destruct(storage.particle_emitter_clients);

    RSB_Destruct(&storage.instances);
    RSB_Destruct(&storage.point_lights);
    RSB_Destruct(&storage.particle_emitters);
    RSB_Destruct(&storage.texture_handles);
    
    DRB_Destruct(&storage.particles);
    DRB_Destruct(&storage.collider_boxes);
    

    for (int i = 0; i < 4; i++)
    {
        dA_Destruct(drawData->lc_world.shadow_counts[i]);
        dA_Destruct(drawData->lc_world.shadow_firsts[i]);

        dA_Destruct(drawData->lc_world.shadow_counts_transparent[i]);
        dA_Destruct(drawData->lc_world.shadow_firsts_transparent[i]);
    }
    dA_Destruct(drawData->lc_world.shadow_sorted_chunk_indexes);
    dA_Destruct(drawData->lc_world.shadow_sorted_chunk_transparent_indexes);

    //Mem clean up
    free(cmdBuffer->cmds_data);
    free(cmdBuffer);
    free(drawData);
    free(pass);
    free(backend_data);
   
}
