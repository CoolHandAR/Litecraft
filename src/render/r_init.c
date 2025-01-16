/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Inits the renderer. Registers cvars, loads shaders and setups FBOs,
textures and etc...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "render/r_core.h"

#include <stb_perlin/stb_perlin.h>
#include <string.h>
#include <assert.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "core/input.h"
#include "render/r_public.h"


extern R_CMD_Buffer* cmdBuffer;
extern RDraw_DrawData* drawData;
extern R_BackendData* backend_data;
extern RPass_PassData* pass;
extern R_Cvars r_cvars;
extern R_StorageBuffers storage;
extern R_Scene scene;
extern R_RendererResources resources;

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
   // r_cvars.r_multithread = Cvar_Register("r_multithread", "1", "Use multiple threads for rendering", CVAR__SAVE_TO_FILE, 0, 1);
   // r_cvars.r_limitFPS = Cvar_Register("r_limitFPS", "1", "Limit FPS with r_maxFPS", CVAR__SAVE_TO_FILE, 0, 1);
   // r_cvars.r_maxFPS = Cvar_Register("r_maxFPS", "144", NULL, CVAR__SAVE_TO_FILE, 30, 1000);
    r_cvars.r_useDirShadowMapping = Cvar_Register("r_useDirShadowMapping", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_drawSky = Cvar_Register("r_drawSky", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);

    //EFFECTS
    r_cvars.r_useDepthOfField = Cvar_Register("r_useDepthOfField", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_useFxaa = Cvar_Register("r_useFxaa", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_useBloom = Cvar_Register("r_useBloom", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_useSsao = Cvar_Register("r_useSsao", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_bloomStrength = Cvar_Register("r_bloomStrength", "0.1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_bloomThreshold = Cvar_Register("r_bloomThreshold", "4.0", NULL, CVAR__SAVE_TO_FILE, 0, 12);
    r_cvars.r_bloomSoftThreshold = Cvar_Register("r_bloomSoftThreshold", "1.0", NULL, CVAR__SAVE_TO_FILE, 0, 12);
    r_cvars.r_ssaoBias = Cvar_Register("r_ssaoBias", "0.025", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_ssaoRadius = Cvar_Register("r_ssaoRadius", "0.4", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_ssaoStrength = Cvar_Register("r_ssaoStrength", "3", NULL, CVAR__SAVE_TO_FILE, 0, 8);
    r_cvars.r_ssaoHalfSize = Cvar_Register("r_ssaoHalfSize", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_Exposure = Cvar_Register("r_Exposure", "1.2", NULL, CVAR__SAVE_TO_FILE, 0, 16);
    r_cvars.r_Gamma = Cvar_Register("r_Gamma", "2.0", NULL, CVAR__SAVE_TO_FILE, 0, 3);
    r_cvars.r_Brightness = Cvar_Register("r_Brightness", "1.1", NULL, CVAR__SAVE_TO_FILE, 0.01, 8);
    r_cvars.r_Contrast = Cvar_Register("r_Contrast", "1.05", NULL, CVAR__SAVE_TO_FILE, 0.01, 8);
    r_cvars.r_Saturation = Cvar_Register("r_Saturation", "1.05", NULL, CVAR__SAVE_TO_FILE, 0.01, 8);
    r_cvars.r_TonemapMode = Cvar_Register("r_TonemapMode", "1", NULL, CVAR__SAVE_TO_FILE, 0, 2);
    r_cvars.r_enableGodrays = Cvar_Register("r_enableGodrays", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
  
    //WINDOW SPECIFIC
  //  r_cvars.w_width = Cvar_Register("w_width", "1024", "Window width", CVAR__SAVE_TO_FILE, 480, 4048);
   // r_cvars.w_height = Cvar_Register("w_heigth", "720", "Window heigth", CVAR__SAVE_TO_FILE, 480, 4048);
    r_cvars.w_useVsync = Cvar_Register("w_useVsync", "1", "Use vsync", CVAR__SAVE_TO_FILE, 0, 1);

    //CAMERA SPECIFIC
    r_cvars.cam_fov = Cvar_Register("cam_fov", "90", NULL, CVAR__SAVE_TO_FILE, 1, 130);
    r_cvars.cam_mouseSensitivity = Cvar_Register("cam_mouseSensitivity", "1", NULL, CVAR__SAVE_TO_FILE, 0, 20);
    r_cvars.cam_zNear = Cvar_Register("cam_zNear", "0.1", NULL, CVAR__SAVE_TO_FILE, 0.01, 10.0);
    r_cvars.cam_zFar = Cvar_Register("cam_zFar", "1500", NULL, CVAR__SAVE_TO_FILE, 100, 4000);

    //SHADOW SPECIFIC
    r_cvars.r_allowParticleShadows = Cvar_Register("r_allowParticleShadws", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_allowTransparentShadows = Cvar_Register("r_allowTransparentShadows", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_shadowBias = Cvar_Register("r_shadowBias", "0.1", NULL, CVAR__SAVE_TO_FILE, -10, 10);
    r_cvars.r_shadowNormalBias = Cvar_Register("r_shadowNormalBias", "2.0", NULL, CVAR__SAVE_TO_FILE, 0, 10);
    r_cvars.r_shadowQualityLevel = Cvar_Register("r_shadowQualityLevel", "7", NULL, CVAR__SAVE_TO_FILE, 0, 24);
    r_cvars.r_shadowBlurLevel = Cvar_Register("r_shadowBlurLevel", "1", NULL, CVAR__SAVE_TO_FILE, 0, 4);
    r_cvars.r_shadowSplits = Cvar_Register("r_shadowSplits", "4", NULL, CVAR__SAVE_TO_FILE, 1, 4);
    
    //DOF SPECIFIC
    r_cvars.r_DepthOfFieldMode = Cvar_Register("r_DepthOfFieldMode", "0", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars.r_DepthOfFieldQuality = Cvar_Register("r_DepthOfFieldQuality", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);

    //WATER 
    r_cvars.r_waterReflectionQuality = Cvar_Register("r_waterReflectionQuality", "1", NULL, CVAR__SAVE_TO_FILE, 0, 2);

    //DEBUG
    r_cvars.r_drawDebugTexture = Cvar_Register("r_drawDebugTexture", "-1", NULL, 0, -1, 5);
    r_cvars.r_wireframe = Cvar_Register("r_wireframe", "0", NULL, 0, 0, 2);
    r_cvars.r_drawPanel = Cvar_Register("r_drawPanel", "0", NULL, 0, 0, 1);
}

static void Init_ScreenQuadDrawData()
{
    glm_ortho(0.0f, 1280, 720, 0.0f, -1.0f, 1.0f, drawData->screen_quad.ortho);

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



}

static void Init_TextDrawData()
{
    RDraw_TextData* data = &drawData->text;

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

    glVertexAttribIPointer(2, 4, GL_UNSIGNED_BYTE, sizeof(BasicVertex), (void*)(offsetof(BasicVertex, color)));
    glEnableVertexAttribArray(2);

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

static void Init_CubeDrawData()
{
   const float cube_vertices[] = {
        // positions          // texture coords  // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,   0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,   0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,   0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,   0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,   0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,   0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,   0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,   0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,   0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,   0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,   1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  1.0f,   1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  1.0f,   1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,   1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,   1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,   0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  1.0f,   0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,   0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,   0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,   0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,   0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,   0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,   0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f
    };

    glGenVertexArrays(1, &drawData->cube.vao);
    glGenBuffers(1, &drawData->cube.vbo);
    glGenBuffers(1, &drawData->cube.instance_vbo);

    glBindVertexArray(drawData->cube.vao);

    //BASE CUBE BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, drawData->cube.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    
    //POSITION
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
    glEnableVertexAttribArray(0);
    //UV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    //NORMALS
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 5));
    glEnableVertexAttribArray(3);

    //INSTANCES
    glBindBuffer(GL_ARRAY_BUFFER, drawData->cube.instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, 1, NULL, GL_STREAM_DRAW);
    drawData->cube.allocated_size = 1;

    const int STRIDE = sizeof(float) * (4 + 4 + 4 + 4 + 4 + 4);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 4));
    glEnableVertexAttribArray(8);
    glVertexAttribDivisor(8, 1);

    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 8));
    glEnableVertexAttribArray(9);
    glVertexAttribDivisor(9, 1);

    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 12));
    glEnableVertexAttribArray(10);
    glVertexAttribDivisor(10, 1);

    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 16));
    glEnableVertexAttribArray(11);
    glVertexAttribDivisor(11, 1);

    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 20));
    glEnableVertexAttribArray(12);
    glVertexAttribDivisor(12, 1);

    drawData->cube.vertices_buffer = dA_INIT(float, 1);
    
}

static void Init_ParticleDrawData()
{
    drawData->particles.instance_buffer = dA_INIT(float, 0);
    
    const float quad_vertices[] =
    {       -1, 1, 0,   0, 0,    0, 0, 1,  
            -1, -1, 0,   0, 1,    0, 0, 1,
             1, -1, 0,   1, 1,    0, 0, 1,
             1, 1, 0,   1, 0,    0, 0, 1,
    };

    //setup vao and vbo
    glGenVertexArrays(1, &drawData->particles.vao);
    glGenBuffers(1, &drawData->particles.vbo);
    glGenBuffers(1, &drawData->particles.instance_vbo);

    //BASE QUAD SETUP
    glBindVertexArray(drawData->particles.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->particles.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    //POSITION
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
    glEnableVertexAttribArray(0);
    //UV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    //NORMAL
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 5));
    glEnableVertexAttribArray(3);

   
    //INSTANCE BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, drawData->particles.instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, 32, NULL, GL_STREAM_DRAW);

    drawData->particles.allocated_size = 32;

    const int STRIDE = sizeof(float) * (4 + 4 + 4 + 4 + 4 + 4);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 4));
    glEnableVertexAttribArray(8);
    glVertexAttribDivisor(8, 1);

    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 8));
    glEnableVertexAttribArray(9);
    glVertexAttribDivisor(9, 1);

    glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 12));
    glEnableVertexAttribArray(10);
    glVertexAttribDivisor(10, 1);

    glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 16));
    glEnableVertexAttribArray(11);
    glVertexAttribDivisor(11, 1);

    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, STRIDE, (void*)(sizeof(float) * 20));
    glEnableVertexAttribArray(12);
    glVertexAttribDivisor(12, 1);

}

static void Init_DrawData()
{
    Init_ScreenQuadDrawData();
    Init_LineDrawData();
    Init_TriangleDrawData();
    Init_CubeDrawData();
    Init_ParticleDrawData();
}


static bool Init_GeneralPassData()
{
    bool result;
    
    pass->general.blur_shader = Shader_ComputeCreate("assets/shaders/screen/box_blur.comp", 0, BOX_BLUR_UNIFORM_MAX, 1, NULL, BOX_BLUR_UNIFORMS_STR, BOX_BLUR_TEXTURES_STR, &result);

    glGenFramebuffers(1, &pass->general.halfsize_fbo);
    glGenTextures(1, &pass->general.depth_halfsize_texture);
    glGenTextures(1, &pass->general.normal_halfsize_texture);
   // glGenTextures(1, &pass->general.perlin_noise_texture);

    glBindTexture(GL_TEXTURE_2D, pass->general.depth_halfsize_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, INIT_WIDTH / 2, INIT_HEIGHT / 2, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, pass->general.normal_halfsize_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, INIT_WIDTH / 2, INIT_HEIGHT / 2, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->general.halfsize_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->general.normal_halfsize_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->general.depth_halfsize_texture, 0);

    CheckBoundFrameBufferStatus("General");

    R_Texture noise = Texture_Load("assets/perlin_noise.png", NULL);

    pass->general.perlin_noise_texture = noise.id;
    
    return result;
}

static bool Init_PostProcessData()
{
    bool result;
    pass->post.post_process_shader = Shader_PixelCreate("assets/shaders/screen/base_screen.vert", "assets/shaders/screen/post_process.frag", POST_PROCESS_DEFINE_MAX, POST_PROCESS_UNIFORM_MAX
        , 4, POST_PROCESS_DEFINES_STR, POST_PROCESS_UNIFORMS_STR, POST_PROCESS_TEXTURES_STR, &result);

    bool result2;
    pass->post.debug_shader = Shader_PixelCreate("assets/shaders/screen/base_screen.vert", "assets/shaders/screen/debug_screen.frag", 0, DEBUG_SCREEN_UNIFORM_MAX, 5, NULL, DEBUG_SCREEN_UNIFORMS_STR, DEBUG_SCREEN_TEXTURES_STR, &result2);

    return result && result2;
}

static bool Init_LCSpecificData()
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

    drawData->lc_world.reflection_pass_opaque_counts = dA_INIT(int, 0);
    drawData->lc_world.reflection_pass_opaque_firsts = dA_INIT(int, 0);

    drawData->lc_world.reflection_pass_transparent_counts = dA_INIT(int, 0);
    drawData->lc_world.reflection_pass_transparent_firsts = dA_INIT(int, 0);

    drawData->lc_world.reflection_pass_chunk_indexes = dA_INIT(int, 0);
    drawData->lc_world.reflection_pass_transparent_chunk_indexes = dA_INIT(int, 0);


    bool result = false;
    pass->lc.world_shader = Shader_PixelCreate("assets/shaders/lc_world/lc_world.vert", "assets/shaders/lc_world/lc_world.frag", LC_WORLD_DEFINE_MAX, LC_WORLD_UNIFORM_MAX, 6, LC_WORLD_DEFINES_STR, LC_WORLD_UNIFORMS_STR, LC_WORLD_TEXTURES_STR, &result);
    
    bool result2 = false;
    pass->lc.occlusion_box_shader = Shader_PixelCreate("assets/shaders/lc_world/lc_occlusion_boxes.vert", "assets/shaders/lc_world/lc_occlusion_boxes.frag", 0, 0, 0, NULL, NULL, NULL, &result2);

    bool result3 = false;
    pass->lc.water_shader = Shader_PixelCreate("assets/shaders/lc_world/lc_water.vert", "assets/shaders/lc_world/lc_water.frag", 0, 0, 6, NULL, NULL, LC_WATER_TEXTURES_STR, &result3);

    bool result4 = false;
    pass->lc.process_chunks_shader = Shader_ComputeCreate("assets/shaders/lc_world/process_chunks.comp", 0, 0, 0, NULL, NULL, NULL, &result4);

    return result && result2 && result3 && result4;
}

static bool Init_DeferredData()
{
    bool result;
    pass->deferred.shading_shader = Shader_PixelCreate("assets/shaders/screen/base_screen.vert", "assets/shaders/screen/deferred_scene.frag", DEFERRED_SCENE_DEFINE_MAX, DEFERRED_SCENE_UNIFORM_MAX, 9, 
        DEFERRED_SCENE_DEFINES_STR, DEFERRED_SCENE_UNIFORMS_STR, DEFERRED_SCENE_TEXTURES_STR, &result);

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

    return result;
}

static bool Init_MainSceneData()
{
    assert(pass->deferred.depth_texture > 0 && "Init deferred first");
    
    bool result;
    pass->scene.shader_3d_forward = Shader_PixelCreate("assets/shaders/scene_3d.vert", "assets/shaders/scene_3d_forward.frag", SCENE_3D_DEFINE_MAX, SCENE_3D_UNIFORM_MAX, 2, SCENE_3D_DEFINES_STR, SCENE_3D_UNIFORMS_STR, SCENE_3D_TEXTURES_STR, &result);

    bool result2;
    pass->scene.shader_3d_deferred = Shader_PixelCreate("assets/shaders/scene_3d.vert", "assets/shaders/scene_3d_deferred.frag", SCENE_3D_DEFINE_MAX, SCENE_3D_UNIFORM_MAX, 2, SCENE_3D_DEFINES_STR, SCENE_3D_UNIFORMS_STR, SCENE_3D_TEXTURES_STR, &result2);

    bool result3;
    pass->scene.screen_shader = Shader_PixelCreate("assets/shaders/screen/screen_shader.vert", "assets/shaders/screen/screen_shader.frag", 0, SCREEN_SHADER_UNIFORM_MAX, 1, NULL, SCREEN_SHADER_UNIFORMS_STR, SCREEN_SHADER_TEXTURES_STR, &result3);

    glGenFramebuffers(1, &pass->scene.FBO);

    //Setup scene textures
    glGenTextures(1, &pass->scene.MainSceneColorBuffer);

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
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->deferred.depth_texture, 0);

    CheckBoundFrameBufferStatus("Main scene");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return result && result2 && result3;
}

static bool Init_AoData()
{
    //SETUP SHADER
    bool result;
    pass->ao.shader = Shader_ComputeCreate("assets/shaders/screen/ssao.comp", SSAO_DEFINE_MAX, SSAO_UNIFORM_MAX, 7, SSAO_DEFINES_STR, SSAO_UNIFORMS_STR, SSAO_TEXTURES_STR, &result);

    //SETUP TEXTURES 
    glGenTextures(1, &pass->ao.ao_texture);
    glGenTextures(1, &pass->ao.back_ao_texture);
    glGenTextures(1, &pass->ao.noise_texture);
   
    glBindTexture(GL_TEXTURE_2D, pass->ao.ao_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, INIT_WIDTH, INIT_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, pass->ao.back_ao_texture);
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

    glGenSamplers(1, &pass->ao.linear_sampler);
    glSamplerParameteri(pass->ao.linear_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(pass->ao.linear_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(pass->ao.linear_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(pass->ao.linear_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return result;
}

static bool Init_BloomData()
{
    bool result;
    pass->bloom.shader = Shader_PixelCreate("assets/shaders/screen/base_screen.vert", "assets/shaders/screen/bloom.frag", BLOOM_DEFINE_MAX, BLOOM_UNIFORM_MAX, 1, BLOOM_DEFINES_STR, BLOOM_UNIFORMS_STR, BLOOM_TEXTURES_STR, &result);

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, (int)mipWidth, (int)mipHeight, 0, GL_RGB, GL_FLOAT, NULL);

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

    return result;
}

static bool Init_DofData()
{
    bool result;
    pass->dof.shader = Shader_ComputeCreate("assets/shaders/screen/dof.comp", DOF_DEFINE_MAX, DOF_UNIFORM_MAX, 3, DOF_DEFINES_STR, DOF_UNIFORMS_STR, DOF_TEXTURES_STR, &result);

    glGenTextures(1, &pass->dof.dof_texture);

    glBindTexture(GL_TEXTURE_2D, pass->dof.dof_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, INIT_WIDTH / 2, INIT_HEIGHT / 2, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return result;
}

static bool Init_IBLData()
{
    //SETUP SHADERS
    bool result;
    pass->ibl.brdf_shader = Shader_PixelCreate("assets/shaders/screen/base_screen.vert", "assets/shaders/screen/brdf.frag", 0, BRDF_UNIFORM_MAX, 0, NULL, BRDF_UNIFORMS_STR, NULL, &result);

    bool result2;
    pass->ibl.cubemap_shader = Shader_PixelCreate("assets/shaders/cubemap/cubemap.vert", "assets/shaders/cubemap/cubemap.frag", CUBEMAP_DEFINE_MAX, CUBEMAP_UNIFORM_MAX, 4, CUBEMAP_DEFINES_STR, CUBEMAP_UNIFORMS_STR, CUBEMAP_TEXTURES_STR, &result2);
    
    pass->ibl.env_size = 512;
    pass->ibl.irr_size = 32;
    pass->ibl.filter_size = 128;

    glGenFramebuffers(1, &pass->ibl.FBO);
    glGenFramebuffers(1, &pass->ibl.env_FBO);
    glGenFramebuffers(1, &pass->ibl.irr_FBO);
    glGenFramebuffers(1, &pass->ibl.filter_FBO);
    glGenRenderbuffers(1, &pass->ibl.irr_RBO);
    glGenRenderbuffers(1, &pass->ibl.env_RBO);
    glGenRenderbuffers(1, &pass->ibl.RBO);
    glGenTextures(1, &pass->ibl.envCubemapTexture);
    glGenTextures(1, &pass->ibl.prefilteredCubemapTexture);
    glGenTextures(1, &pass->ibl.irradianceCubemapTexture);
    glGenTextures(1, &pass->ibl.brdfLutTexture);
    glGenTextures(1, &pass->ibl.nightTexture);
    
    for (int i = 0; i < 5; i++)
    {
        unsigned int mipWidth = (unsigned)(pass->ibl.filter_size * pow(0.5, i));
        unsigned int mipHeight = (unsigned)(pass->ibl.filter_size * pow(0.5, i));

        glGenTextures(1, &pass->ibl.filter_depth_mipmaps[i]);
        glBindTexture(GL_TEXTURE_2D, pass->ibl.filter_depth_mipmaps[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, mipWidth, mipHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, pass->ibl.env_size, pass->ibl.env_size, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.prefilteredCubemapTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, pass->ibl.filter_size, pass->ibl.filter_size, 0, GL_RGB, GL_FLOAT, NULL);
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
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, pass->ibl.irr_size, pass->ibl.irr_size, 0, GL_RGB, GL_FLOAT, NULL);
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

    glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.nightTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, pass->ibl.env_size, pass->ibl.env_size, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.env_FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.env_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, pass->ibl.env_size, pass->ibl.env_size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pass->ibl.env_RBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, pass->ibl.envCubemapTexture, 0);
    CheckBoundFrameBufferStatus("ENV-IBL");

    glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.irr_FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.irr_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, pass->ibl.irr_size, pass->ibl.irr_size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pass->ibl.irr_RBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, pass->ibl.irradianceCubemapTexture, 0);
    CheckBoundFrameBufferStatus("IRR-IBL");

    glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.filter_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, pass->ibl.prefilteredCubemapTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->ibl.filter_depth_mipmaps[0], 0);
    CheckBoundFrameBufferStatus("FILTER-IBL");

    glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.RBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->ibl.brdfLutTexture, 0);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pass->ibl.RBO);

    CheckBoundFrameBufferStatus("BRDF_IBL");
   
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

    return result && result2;
}

static void Init_ShadowMappingData()
{
    //SETUP FRAMEBUFFER
    glGenFramebuffers(1, &pass->shadow.FBO);
    glGenTextures(1, &pass->shadow.depth_maps);

    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };

    glBindTexture(GL_TEXTURE_2D_ARRAY, pass->shadow.depth_maps);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT16, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, SHADOW_CASCADE_LEVELS, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT , NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, pass->shadow.FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pass->shadow.depth_maps, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    CheckBoundFrameBufferStatus("Shadow");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float z_far = 1500;

    pass->shadow.cascade_levels[0] = z_far / 350.0f;
    pass->shadow.cascade_levels[1] = z_far / 180.0f;
    pass->shadow.cascade_levels[2] = z_far / 70.0f;
    pass->shadow.cascade_levels[3] = z_far / 10.0f;

    pass->shadow.shadow_map_size = SHADOW_MAP_SIZE;
}

static void Init_WaterData()
{
    glGenFramebuffers(1, &pass->water.reflection_fbo);
    glGenRenderbuffers(1, &pass->water.reflection_rbo);

    glGenTextures(1, &pass->water.reflection_texture);
    glGenTextures(1, &pass->water.reflection_back_texture);
    glGenTextures(1, &pass->water.refraction_texture);

    glBindTexture(GL_TEXTURE_2D, pass->water.reflection_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, INIT_WIDTH, INIT_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, pass->water.reflection_back_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, INIT_WIDTH, INIT_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, pass->water.refraction_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, INIT_WIDTH, INIT_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, pass->water.reflection_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, INIT_WIDTH, INIT_HEIGHT);
    
    glBindFramebuffer(GL_FRAMEBUFFER, pass->water.reflection_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->water.reflection_texture, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pass->water.reflection_rbo);

    CheckBoundFrameBufferStatus("Reflection");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

static bool Init_GodrayData()
{
    bool result;
    pass->godray.shader = Shader_ComputeCreate("assets/shaders/screen/godray.comp", GODRAY_DEFINE_MAX, GODRAY_UNIFORM_MAX, 4, GODRAY_DEFINES_STR, GODRAY_UNIFORMS_STR, GODRAY_TEXTURES_STR, &result);

    glGenTextures(1, &pass->godray.godray_fog_texture);
    glGenTextures(1, &pass->godray.back_texture);
    glGenTextures(1, &pass->godray.noise_texture);

    glBindTexture(GL_TEXTURE_2D, pass->godray.godray_fog_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, INIT_WIDTH / 2, INIT_HEIGHT / 2, 0, GL_RED, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, pass->godray.back_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, INIT_WIDTH / 2, INIT_HEIGHT / 2, 0, GL_RED, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    return result;
}


static bool Init_PassData()
{
    if(!Init_PostProcessData()) return false;
    if(!Init_GeneralPassData()) return false;
    if(!Init_LCSpecificData()) return false;
    if(!Init_DeferredData()) return false;
    if(!Init_MainSceneData()) return false;
    if(!Init_AoData()) return false;
    if(!Init_BloomData()) return false;
    if(!Init_DofData()) return false;
    if(!Init_IBLData()) return false;
    Init_ShadowMappingData();
    Init_WaterData();
    if(!Init_GodrayData()) return false;

    return true;
}

static void Init_SceneData()
{
    glGenBuffers(1, &scene.camera_ubo);
    glGenBuffers(1, &scene.scene_ubo);

    glBindBuffer(GL_UNIFORM_BUFFER, scene.camera_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(RScene_CameraData), NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, scene.scene_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(RScene_UBOData), NULL, GL_DYNAMIC_DRAW);


   // scene.cull_data.dynamic_partition_tree = BVH_Tree_Create(0.5);
    scene.cull_data.static_partition_tree = BVH_Tree_Create(0.0);

    scene.render_instances_pool = Object_Pool_INIT(RenderInstance, 0);

    //Init scene defaults
    scene.scene_data.dirLightDirection[0] = 1;
    scene.scene_data.dirLightDirection[1] = 1;
    scene.scene_data.dirLightDirection[2] = 1;

    scene.scene_data.dirLightColor[0] = 1;
    scene.scene_data.dirLightColor[1] = 1;
    scene.scene_data.dirLightColor[2] = 1;

    scene.scene_data.dirLightAmbientIntensity = 8;
    scene.scene_data.dirLightSpecularIntensity = 24;

    scene.scene_data.ambientLightInfluence = 1.0;

    scene.environment.sky_color[0] = 0.0;
    scene.environment.sky_color[1] = 0.6;
    scene.environment.sky_color[2] = 1.0;

    scene.environment.sky_horizon_color[0] = 0.6;
    scene.environment.sky_horizon_color[1] = 0.6;
    scene.environment.sky_horizon_color[2] = 0.8;

    scene.environment.ground_bottom_color[0] = 0.8;
    scene.environment.ground_bottom_color[1] = 0.8;
    scene.environment.ground_bottom_color[2] = 0.8;

    scene.environment.ground_horizon_color[0] = 0.4;
    scene.environment.ground_horizon_color[1] = 0.4;
    scene.environment.ground_horizon_color[2] = 0.6;

    scene.environment.godrayFogAmount = 1.4;
    scene.environment.godrayScatteringAmount = 0.3;

    scene.environment.depthOfFieldNearBegin = 0;
    scene.environment.depthOfFieldFarBegin = 12;
    scene.environment.depthOfFieldNearEnd = 2;
    scene.environment.depthOfFieldFarEnd = 24;
    scene.environment.depthOfFieldBlurScale = 0.5;
}

static bool _initRenderThread()
{
    /*
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
    */

    return true;
}

static void Init_StorageBuffers()
{
    memset(&storage, 0, sizeof(storage));

    storage.particle_emitter_clients = FL_INIT(ParticleEmitterSettings);

    storage.point_lights_pool = Object_Pool_INIT(PointLight, 0);
    storage.point_lights_backbuffer = dA_INIT(PointLight, 0);
    storage.spot_lights_pool = Object_Pool_INIT(SpotLight, 0);
    storage.spot_lights_backbuffer = dA_INIT(SpotLight, 0);
   
    storage.spot_lights = RSB_Create(10000, sizeof(SpotLight), RSB_FLAG__RESIZABLE | RSB_FLAG__WRITABLE);
    storage.point_lights = RSB_Create(10000, sizeof(PointLight), RSB_FLAG__RESIZABLE | RSB_FLAG__WRITABLE);
    storage.texture_handles = RSB_Create(1, sizeof(GLuint64), RSB_FLAG__RESIZABLE | RSB_FLAG__WRITABLE | RSB_FLAG__POOLABLE);
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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, storage.spot_lights.buffer);

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

    drawData = malloc(sizeof(RDraw_DrawData));
    if (!drawData)
    {
        free(cmdBuffer->cmds_data);
        free(cmdBuffer);
        return false;
    }
    memset(drawData, 0, sizeof(RDraw_DrawData));

    pass = malloc(sizeof(RPass_PassData));

    if (!pass)
    {
        free(cmdBuffer->cmds_data);
        free(cmdBuffer);
        free(drawData);
        return false;
    }
    memset(pass, 0, sizeof(RPass_PassData));

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


    //ALLOCATE AND MEMSET DATA
    if (!Init_Mem())
    {
        return false;
    }
    //INIT GL DATA
    Init_DrawData();
    if(!Init_PassData()) return false;
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

    RSB_Destruct(&storage.spot_lights);
    RSB_Destruct(&storage.point_lights);
    RSB_Destruct(&storage.texture_handles);
    
    dA_Destruct(storage.point_lights_backbuffer);
    dA_Destruct(storage.spot_lights_backbuffer);
    dA_Destruct(drawData->cube.vertices_buffer);
    dA_Destruct(drawData->particles.instance_buffer);

    Object_Pool_Destruct(scene.render_instances_pool);
    Object_Pool_Destruct(storage.point_lights_pool);
    Object_Pool_Destruct(storage.spot_lights_pool);

    for (int i = 0; i < 4; i++)
    {
        dA_Destruct(drawData->lc_world.shadow_counts[i]);
        dA_Destruct(drawData->lc_world.shadow_firsts[i]);

        dA_Destruct(drawData->lc_world.shadow_counts_transparent[i]);
        dA_Destruct(drawData->lc_world.shadow_firsts_transparent[i]);
    }
    dA_Destruct(drawData->lc_world.shadow_sorted_chunk_indexes);
    dA_Destruct(drawData->lc_world.shadow_sorted_chunk_transparent_indexes);
    dA_Destruct(drawData->lc_world.reflection_pass_opaque_firsts);
    dA_Destruct(drawData->lc_world.reflection_pass_opaque_counts);
    dA_Destruct(drawData->lc_world.reflection_pass_transparent_firsts);
    dA_Destruct(drawData->lc_world.reflection_pass_transparent_counts);
    dA_Destruct(drawData->lc_world.reflection_pass_chunk_indexes);
    dA_Destruct(drawData->lc_world.reflection_pass_transparent_chunk_indexes);

    BVH_Tree_Destruct(&scene.cull_data.static_partition_tree);

    //destroy shaders
    Shader_Destruct(&pass->dof.shader);
    Shader_Destruct(&pass->post.post_process_shader);
    Shader_Destruct(&pass->post.debug_shader);
    Shader_Destruct(&pass->godray.shader);
    Shader_Destruct(&pass->general.blur_shader);
    Shader_Destruct(&pass->ao.shader);
    Shader_Destruct(&pass->scene.shader_3d_forward);
    Shader_Destruct(&pass->scene.shader_3d_deferred);
    Shader_Destruct(&pass->scene.screen_shader);
    Shader_Destruct(&pass->bloom.shader);
    Shader_Destruct(&pass->lc.world_shader);
    Shader_Destruct(&pass->lc.water_shader);
    Shader_Destruct(&pass->lc.occlusion_box_shader);
    Shader_Destruct(&pass->lc.process_chunks_shader);
    Shader_Destruct(&pass->ibl.cubemap_shader);
    Shader_Destruct(&pass->deferred.shading_shader);

    //Mem clean up
    free(cmdBuffer->cmds_data);
    free(cmdBuffer);
    free(drawData);
    free(pass);
    free(backend_data);
   
}
