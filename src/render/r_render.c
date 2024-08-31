/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Functions that issue draw calls to the gpu
    called from r_pass.c
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "r_core.h"

extern R_DrawData* drawData;
extern R_RendererResources resources;
extern R_RenderPassData* pass;
extern R_Cvars r_cvars;

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

    glBindVertexArray(data->vao);
    glDrawArrays(GL_LINES, 0, data->vertices_count);

   // data->vertices_count = 0;
}

static void Render_TriangleBatch()
{
    R_TriangleDrawData* data = &drawData->triangle;

    if (data->vertices_count == 0)
        return;

    glBindVertexArray(data->vao);
    glDrawArrays(GL_TRIANGLES, 0, data->vertices_count);

   // data->vertices_count = 0;
}

void Render_Quad()
{
    glBindVertexArray(resources.quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void Render_Cube()
{
    glBindVertexArray(resources.cubeVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}

void Render_Particles()
{

}

void Render_OpaqueWorldChunks(bool p_TextureDraw)
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data)
    {
        return;
    }
    size_t chunk_amount = LC_World_GetChunkAmount();

    if (p_TextureDraw)
    {
        glBindTextureUnit(0, drawData->lc_world.world_render_data->texture_atlas.id);
        glBindTextureUnit(1, drawData->lc_world.world_render_data->texture_atlas_normals.id);
        glBindTextureUnit(2, drawData->lc_world.world_render_data->texture_atlas_mer.id);
    }

    glBindVertexArray(drawData->lc_world.world_render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->lc_world.world_render_data->vertex_buffer.buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->draw_cmds_buffer.buffer);
    
    if (r_cvars.r_wireframe->int_value == 2)
    {
        glMultiDrawArraysIndirect(GL_LINES, 0, chunk_amount, sizeof(CombinedChunkDrawCmdData));
    }
    else
    {
        glMultiDrawArraysIndirect(GL_TRIANGLES, 0, chunk_amount, sizeof(CombinedChunkDrawCmdData));
    }
}

void Render_TransparentWorldChunks()
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data)
    {
        return;
    }
    size_t chunk_amount = LC_World_GetChunkAmount();

    glBindVertexArray(drawData->lc_world.world_render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->lc_world.world_render_data->transparents_vertex_buffer.buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->draw_cmds_buffer.buffer);

    glMultiDrawArraysIndirect(GL_TRIANGLES, (void*)16, chunk_amount, sizeof(CombinedChunkDrawCmdData));
}

void Render_DrawChunkOcclusionBoxes()
{
    glUseProgram(pass->lc.occlussion_boxes_shader);

    glBindVertexArray(drawData->lc_world.world_render_data->ocl_boxes_vao);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->ocl_boxes_draw_cmd_buffer);

    glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, 0, 1, 0);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, LC_World_GetChunkAmount() * 14);
}

void Render_DebugScene()
{   
    Render_LineBatch();
   // Render_TriangleBatch();
}

static void Compute_Particles()
{
    //glUseProgram()
    //glDispatchCompute();
}

static void Compute_WorldChunks()
{
    if (drawData->lc_world.draw == false)
    {
        return;
    }

    size_t chunk_amount = LC_World_GetChunkAmount();
    int num_x_groups = ceilf((float)chunk_amount / 16.0f);

    drawData->lc_world.world_render_data = LC_World_getRenderData();

    glUseProgram(pass->lc.chunk_process_shader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, drawData->lc_world.world_render_data->chunk_data.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, drawData->lc_world.world_render_data->draw_cmds_buffer.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, drawData->lc_world.world_render_data->ocl_boxes_vertices_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, drawData->lc_world.world_render_data->ocl_boxes_draw_cmd_buffer);
    Shader_SetUnsigned(pass->lc.chunk_process_shader, "u_TotalChunkAmount", chunk_amount);

    if (drawData->lc_world.world_render_data->opaque_update_move_by != 0)
    {
        Shader_SetUnsigned(pass->lc.chunk_process_shader, "u_OpaqueUpdateOffset", drawData->lc_world.world_render_data->opaque_update_offset);
        Shader_SetInteger(pass->lc.chunk_process_shader, "u_OpaqueUpdateMoveBy", drawData->lc_world.world_render_data->opaque_update_move_by);
        drawData->lc_world.offset_update_consumed = true;
    }
    if (drawData->lc_world.world_render_data->transparent_update_move_by != 0)
    {
        Shader_SetUnsigned(pass->lc.chunk_process_shader, "u_TransparentUpdateOffset", drawData->lc_world.world_render_data->opaque_update_offset);
        Shader_SetInteger(pass->lc.chunk_process_shader, "u_TransparentUpdateMoveBy", drawData->lc_world.world_render_data->opaque_update_move_by);
        drawData->lc_world.offset_update_consumed = true;
    }

    glDispatchCompute(num_x_groups, 1, 1);
}

static void Compute_Meshes()
{

}

void Compute_DispatchAll()
{
    Compute_Particles();
    Compute_WorldChunks();
}

void Compute_Sync()
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


    if (drawData->lc_world.draw)
    {
        //Sync processing world chunks
        glUseProgram(pass->lc.chunk_process_shader);
        if (drawData->lc_world.offset_update_consumed)
        {
            if (drawData->lc_world.world_render_data->opaque_update_move_by != 0)
            {
                Shader_SetUnsigned(pass->lc.chunk_process_shader, "u_OpaqueUpdateOffset", 0);
                Shader_SetInteger(pass->lc.chunk_process_shader, "u_OpaqueUpdateMoveBy", 0);
                drawData->lc_world.world_render_data->opaque_update_move_by = 0;
                drawData->lc_world.world_render_data->opaque_update_offset = 0;
            }
            if (drawData->lc_world.world_render_data->transparent_update_move_by != 0)
            {
                Shader_SetUnsigned(pass->lc.chunk_process_shader, "u_TransparentUpdateOffset", 0);
                Shader_SetInteger(pass->lc.chunk_process_shader, "u_TransparentUpdateMoveBy", 0);
                drawData->lc_world.world_render_data->transparent_update_move_by = 0;
                drawData->lc_world.world_render_data->transparent_update_offset = 0;
            }
            drawData->lc_world.offset_update_consumed = false;
        }
    }
}