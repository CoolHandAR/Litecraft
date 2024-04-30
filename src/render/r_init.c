#include "r_core.h"

#include <string.h>

extern R_CMD_Buffers* cmdBuffers;
extern R_DrawData* drawData;
extern R_BackendData* backend_data;
extern R_Cvars* cvars;

static void _registerCvars()
{
    cvars->r_multithread = C_cvarRegister("r_multithread", "1", "Use multiple threads for rendering", CVAR__SAVE_TO_FILE, 0, 1);
    cvars->r_limitFPS = C_cvarRegister("r_limitFPS", "1", "Limit FPS with r_maxFPS", CVAR__SAVE_TO_FILE, 0, 1);
    cvars->r_maxFPS = C_cvarRegister("r_maxFPS", "144", NULL, CVAR__SAVE_TO_FILE, 30, 1000);
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

    glBufferData(GL_ARRAY_BUFFER, sizeof(drawData->screen_quad.vertices), NULL, GL_STREAM_DRAW);

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


    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), (void*)(offsetof(ScreenVertex, position)));
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
}



bool r_Init1()
{
    cmdBuffers = malloc(sizeof(R_CMD_Buffers));

    if (!cmdBuffers)
    {
        return false;
    }
    memset(cmdBuffers, 0, sizeof(R_CMD_Buffers));

    drawData = malloc(sizeof(R_DrawData));

    if (!drawData)
    {
        return false;
    }
    memset(drawData, 0, sizeof(R_DrawData));

    backend_data = malloc(sizeof(R_BackendData));

    if (!backend_data)
    {
        return false;
    }
    memset(backend_data, 0, sizeof(R_BackendData));

    cvars = malloc(sizeof(R_Cvars));

    if (!cvars)
    {
        return false;
    }
    memset(cvars, 0, sizeof(R_Cvars));

    _registerCvars();
    _initScreenQuadDrawData();
    
    
}