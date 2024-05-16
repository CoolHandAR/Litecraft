/*
    Functions that issue draw calls to the gpu
    after processing has been done in r_cmds.c.
    Usually this is the last step in the rendering pipeline
*/

#include "r_core.h"

extern R_DrawData* drawData;

static void Render_ScreenQuadBatch()
{
    R_ScreenQuadDrawData* data = &drawData->screen_quad;

    if (data->vertices_count == 0)
    {
        return;
    }

	glUseProgram(data->shader);

    //bind texture units
    for (int i = 1; i < data->tex_index; i++)
    {
        glBindTextureUnit(i, data->tex_array[i]);
    }

    mat4 ortho;
    glm_ortho(0.0f, 1024, 720, 0.0f, -1.0f, 1.0f, ortho);

    Shader_SetVector2f(data->shader, "u_windowScale", 1, 1);
    Shader_SetMatrix4(data->shader, "u_projection", ortho);

    glBindVertexArray(data->vao);
    glDrawElements(GL_TRIANGLES, data->indices_count, GL_UNSIGNED_INT, 0);

    data->vertices_count = 0;
    data->indices_count = 0;
    data->tex_index = 1;
}

static void Render_LineBatch()
{
    R_LineDrawData* data = &drawData->lines;

    if (data->vertices_count == 0)
    {
        return;
    }

    glUseProgram(data->shader);

    glBindVertexArray(data->vao);
    glDrawArrays(GL_LINES, 0, data->vertices_count);

    data->vertices_count = 0;
}

static void Render_TriangleBatch()
{
    R_TriangleDrawData* data = &drawData->triangle;

    if (data->vertices_count == 0)
        return;

    glUseProgram(data->shader);

    glBindVertexArray(data->vao);
    glDrawArrays(GL_TRIANGLES, 0, data->vertices_count);

    data->vertices_count = 0;
}



void r_RenderAll()
{
   // glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    Render_ScreenQuadBatch();
    Render_LineBatch();
    Render_TriangleBatch();
   
}