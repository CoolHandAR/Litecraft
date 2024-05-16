#include "r_core.h"

#include "r_particle.h"
#include <string.h>
#include <glad/glad.h>
#include <assert.h>

extern R_CMD_Buffer* cmdBuffer;
extern R_DrawData* drawData;
extern R_BackendData* backend_data;
extern R_RenderPassData* pass;
extern R_Cvars* r_cvars;
extern R_StorageBuffers storage_buffers;


extern void r_threadLoop();

static void _registerCvars()
{
    r_cvars->r_multithread = Cvar_Register("r_multithread", "1", "Use multiple threads for rendering", CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars->r_limitFPS = Cvar_Register("r_limitFPS", "1", "Limit FPS with r_maxFPS", CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars->r_maxFPS = Cvar_Register("r_maxFPS", "144", NULL, CVAR__SAVE_TO_FILE, 30, 1000);
    r_cvars->r_mssaLevel = Cvar_Register("r_mssaLevel", "0", NULL, CVAR__SAVE_TO_FILE, 0, 8);
    r_cvars->r_useDirShadowMapping = Cvar_Register("r_useDirShadowMapping", "1", NULL, CVAR__SAVE_TO_FILE, 0, 1);
    r_cvars->r_DirShadowMapResolution = Cvar_Register("r_DirShadowMapResolution", "1024", NULL, CVAR__SAVE_TO_FILE, 256, 4048);


    //WINDOW SPECIFIC
    r_cvars->w_width = Cvar_Register("w_width", "1024", "Window width", CVAR__SAVE_TO_FILE, 480, 4048);
    r_cvars->w_height = Cvar_Register("w_heigth", "720", "Window heigth", CVAR__SAVE_TO_FILE, 480, 4048);
    r_cvars->w_useVsync = Cvar_Register("w_useVsync", "1", "Use vsync", CVAR__SAVE_TO_FILE, 0, 1);
}

static void _initScreenQuadDrawData()
{
	drawData->screen_quad.shader = Shader_CompileFromFile("src/shaders/screen_shader.vs", "src/shaders/screen_shader.fs", NULL);

    drawData->screen_quad.tex_index = 1;

    glGenVertexArrays(1, &drawData->screen_quad.vao);
    glGenBuffers(1, &drawData->screen_quad.vbo);
    glGenBuffers(1, &drawData->screen_quad.ebo);

    glBindVertexArray(drawData->screen_quad.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->screen_quad.vbo);

    glBufferStorage(GL_ARRAY_BUFFER, sizeof(drawData->screen_quad.vertices), NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT |
    GL_DYNAMIC_STORAGE_BIT);

    drawData->screen_quad.buffer_ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(drawData->screen_quad.vertices), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

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
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices_count, indices, 0);

        free(indices);
    }


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, tex_coords)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, tex_index)));
    glEnableVertexAttribArray(2);

    //SETUP WHITE TEXTURE  
    //create texture
    glGenTextures(1, &drawData->screen_quad.white_texture);
    glBindTexture(GL_TEXTURE_2D, drawData->screen_quad.white_texture);

    //set texture wrap and filter modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    uint32_t color_White = 0xffffffff;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color_White);

    glUseProgram(drawData->screen_quad.shader);

    //set up samplers
    GLint texture_array_location = glGetUniformLocation(drawData->screen_quad.shader, "u_textures");
    int32_t samplers[32];

    for (int i = 0; i < 32; i++)
    {
        samplers[i] = i;
    }

    glUniform1iv(texture_array_location, 32, samplers);

    glBindTextureUnit(0, drawData->screen_quad.white_texture);

    drawData->screen_quad.tex_array[0] = drawData->screen_quad.white_texture;
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

    glBufferStorage(GL_ARRAY_BUFFER, sizeof(data->vertices), NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT |
        GL_DYNAMIC_STORAGE_BIT);

    data->buffer_ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(data->vertices), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

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
        // glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices_count, indices, 0);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices_count, indices, GL_STATIC_DRAW);

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

static void _initLineDrawData()
{
    drawData->lines.shader = Shader_CompileFromFile("src/shaders/line_shader.vs", "src/shaders/line_shader.fs", NULL);

    glGenVertexArrays(1, &drawData->lines.vao);
    glGenBuffers(1, &drawData->lines.vbo);

    glBindVertexArray(drawData->lines.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->lines.vbo);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(drawData->lines.vertices), NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT |
        GL_DYNAMIC_STORAGE_BIT);

    drawData->lines.buffer_ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(drawData->lines.vertices), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(offsetof(LineVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(offsetof(LineVertex, color)));
    glEnableVertexAttribArray(1);
}

static void _initTriangleDrawData()
{
    drawData->triangle.shader = Shader_CompileFromFile("src/shaders/triangle_shader.vs", "src/shaders/triangle_shader.fs", NULL);

    glGenVertexArrays(1, &drawData->triangle.vao);
    glGenBuffers(1, &drawData->triangle.vbo);

    glBindVertexArray(drawData->triangle.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->triangle.vbo);
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(drawData->triangle.vertices), NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT |
        GL_DYNAMIC_STORAGE_BIT);

    drawData->triangle.buffer_ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(drawData->triangle.vertices), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)(offsetof(TriangleVertex, color)));
    glEnableVertexAttribArray(1);
}

static void _initDrawData()
{
    _initScreenQuadDrawData();
    _initLineDrawData();
    _initTriangleDrawData();
}


static void _initPostProcessData()
{
    pass->post.shader = Shader_CompileFromFile("src/shaders/post_process.vs", "src/shaders/post_process.fs", NULL);

    glGenFramebuffers(1, &pass->post.FBO);
    glGenFramebuffers(1, &pass->post.msFBO);
    glGenRenderbuffers(1, &pass->post.msRBO);
    glGenTextures(1, &pass->post.ms_color_buffer_texture);
    glGenTextures(1, &pass->post.color_buffer_texture);
    glGenTextures(1, &pass->post.depth_texture);

    //MS DATA
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, pass->post.ms_color_buffer_texture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, 800, 600, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->post.msFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, pass->post.ms_color_buffer_texture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, pass->post.msRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, 800, 600);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pass->post.msRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }

    //INTERMIADATE DATA
    glBindFramebuffer(GL_FRAMEBUFFER, pass->post.FBO);
    //COLOR BUFFER TEXTURE
    glBindTexture(GL_TEXTURE_2D, pass->post.color_buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->post.color_buffer_texture, 0);
    //DEPTH BUFFER TEXTURE
    glBindTexture(GL_TEXTURE_2D, pass->post.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 800, 600, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->post.depth_texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void _initShadowMappingData()
{
    //SETUP SHADER
    pass->shadow.depth_shader = Shader_CompileFromFile("src/shaders/shadow_depth_shader.vs", "src/shaders/shadow_depth_shader.fs", "src/shaders/shadow_depth_shader.gs");
    assert(pass->shadow.depth_shader > 0);

    //SETUP FRAMEBUFFER
    glGenFramebuffers(1, &pass->shadow.FBO);
    glGenTextures(1, &pass->shadow.depth_maps);

    glBindTexture(GL_TEXTURE_2D_ARRAY, pass->shadow.depth_maps);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, 800, 800, SHADOW_CASCADE_LEVELS + 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, pass->shadow.FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pass->shadow.depth_maps, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(false);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //MATRICES UBO
    glGenBuffers(1, &pass->shadow.light_space_matrices_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, pass->shadow.light_space_matrices_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * LIGHT_MATRICES_COUNT, NULL, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, pass->shadow.light_space_matrices_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    float z_far = 250;

    pass->shadow.cascade_levels[0] = z_far / 50.0f;
    pass->shadow.cascade_levels[1] = z_far / 50.0f;
    pass->shadow.cascade_levels[2] = z_far / 50.0f;
    pass->shadow.cascade_levels[3] = z_far / 50.0f;
}

static void _initPassData()
{
    _initPostProcessData();
    _initShadowMappingData();
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
    memset(&storage_buffers, 0, sizeof(storage_buffers));

    storage_buffers.particles = RSB_Create(200, sizeof(R_Particle), false, true, false, false);
    storage_buffers.particle_emitters = RSB_Create(200, sizeof(R_ParticleEmitter), false, true, false, false);


}

int r_Init1()
{
    cmdBuffer = malloc(sizeof(R_CMD_Buffer));

    if (!cmdBuffer)
    {
        return false;
    }
    

    memset(cmdBuffer, 0, sizeof(R_CMD_Buffer));
    cmdBuffer->cmds_data = malloc(RENDER_BUFFER_COMMAND_ALLOC_SIZE);
    
    
    memset(cmdBuffer->cmds_data, 0, RENDER_BUFFER_COMMAND_ALLOC_SIZE);
    cmdBuffer->cmds_ptr = cmdBuffer->cmds_data;

    drawData = malloc(sizeof(R_DrawData));

    if (!drawData)
    {
        return false;
    }
    memset(drawData, 0, sizeof(R_DrawData));

    pass = malloc(sizeof(R_RenderPassData));

    if (!pass)
    {
        return false;
    }
    memset(pass, 0, sizeof(R_RenderPassData));

    backend_data = malloc(sizeof(R_BackendData));

    if (!backend_data)
    {
        return false;
    }
    memset(backend_data, 0, sizeof(R_BackendData));

    r_cvars = malloc(sizeof(R_Cvars));

    if (!r_cvars)
    {
        return false;
    }
    memset(r_cvars, 0, sizeof(R_Cvars));

    _registerCvars();
    _initDrawData();
    _initPassData();
    

   // _initRenderThread();
    Init_StorageBuffers();


    return true;
}