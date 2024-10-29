/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Functions that issue draw, dispatch calls to the gpu
    called from r_pass.c
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "r_core.h"
#include "r_public.h"

extern R_DrawData* drawData;
extern R_RendererResources resources;
extern R_RenderPassData* pass;
extern R_Cvars r_cvars;
extern R_StorageBuffers storage;
extern R_Scene scene;

static void Render_ScreenQuadBatch()
{
    R_ScreenQuadDrawData* data = &drawData->screen_quad;

    if (data->vertices_count == 0)
    {
        return;
    }

	glUseProgram(data->shader);

    //bind texture units
    for (int i = 0; i < 32; i++)
    {
        if (data->tex_array[i] == 0)
        {
           // break;
        }

        unsigned texture_index = data->tex_array[i];

        glBindTextureUnit(i, texture_index);
    }

   // glBindTexture(GL_TEXTURE_2D, data->tex_array[0]);

    mat4 ortho;
    glm_ortho(0.0f, 1280, 720, 0.0f, -1.0f, 1.0f, ortho);

   // Shader_SetVector2f(data->shader, "u_windowScale", 1, 1);
    Shader_SetMatrix4(data->shader, "u_projection", ortho);

    glBindVertexArray(data->vao);
    glDrawElements(GL_TRIANGLES, data->indices_count, GL_UNSIGNED_INT, 0);

    data->vertices_count = 0;
    data->indices_count = 0;
    data->tex_index = 0;
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
}

static void Render_TriangleBatch()
{
    R_TriangleDrawData* data = &drawData->triangle;

    if (data->vertices_count == 0)
        return;

    glUseProgram(pass->general.triangle_3d_shader);

    for (int i = 0; i < data->texture_index; i++)
    {
        if (data->texture_ids[i] == 0)
        {
            break;
        }
        glBindTextureUnit(i, data->texture_ids[i]);
    }

    glEnable(GL_BLEND);
    glBindVertexArray(data->vao);
    glDrawArrays(GL_TRIANGLES, 0, data->vertices_count);
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
    for (int i = 0; i < 32; i++)
    {
        if (pass->particles.texture_ids[i] == 0)
        {
            break;
        }
        glBindTextureUnit(i, pass->particles.texture_ids[i]);
    }
   
    glBindVertexArray(resources.quadVAO);
    unsigned amount = pass->particles.total_particle_amount;
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, amount);
}
static void Render_OpaqueWorldChunks(bool p_TextureDraw, int mode)
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data)
    {
        return;
    }
    size_t chunk_amount = LC_World_GetDrawCmdAmount();

    if (p_TextureDraw)
    {
        glBindTextureUnit(0, drawData->lc_world.world_render_data->texture_atlas->id);
        glBindTextureUnit(1, drawData->lc_world.world_render_data->texture_atlas_normals->id);
        glBindTextureUnit(2, drawData->lc_world.world_render_data->texture_atlas_mer->id);
    }
    glBindVertexArray(drawData->lc_world.world_render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->lc_world.world_render_data->vertex_buffer.buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->draw_cmds_buffer.buffer);
    glBindVertexBuffer(0, drawData->lc_world.world_render_data->vertex_buffer.buffer, 0, sizeof(ChunkVertex));
     
    GLenum render_mode = (r_cvars.r_wireframe->int_value == 2) ? GL_LINES : GL_TRIANGLES;
      
    //Normal rendering
    if (mode == 0)
    {
        int offset = 0;

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_ATOMIC_COUNTER_BUFFER);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->sorted_draw_cmds_buffer.buffer);
        glBindBuffer(GL_PARAMETER_BUFFER, drawData->lc_world.world_render_data->draw_cmds_counters_buffers[0]);

        glMultiDrawArraysIndirectCount(render_mode, offset, 0, chunk_amount, 0);
    }
    //Shadow rendering
    else if (mode > 0)
    {
        int shadow_split = min(mode - 1, 3);

        int* first = NULL;
        int* count = NULL;
        int draw_count = 0;

        first = drawData->lc_world.shadow_firsts[shadow_split]->data;
        count = drawData->lc_world.shadow_counts[shadow_split]->data;
        draw_count = scene.cull_data.lc_world_shadow_cull_count[shadow_split];
        if (draw_count > 0)
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 35, drawData->lc_world.world_render_data->shadow_chunk_indexes_ssbo);
            glMultiDrawArrays(GL_TRIANGLES, first, count, draw_count);
        } 
    }

   
}

static void Render_SemiTransparentWorldChunks(bool p_TextureDraw, int mode)
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data)
    {
        return;
    }
    if (p_TextureDraw)
    {
        glBindTextureUnit(0, drawData->lc_world.world_render_data->texture_atlas->id);
        glBindTextureUnit(1, drawData->lc_world.world_render_data->texture_atlas_normals->id);
        glBindTextureUnit(2, drawData->lc_world.world_render_data->texture_atlas_mer->id);

        //IBL Textures
        glBindTextureUnit(3, pass->ibl.brdfLutTexture);
        glBindTextureUnit(4, pass->ibl.irradianceCubemapTexture);
        glBindTextureUnit(5, pass->ibl.prefilteredCubemapTexture);

        //Shadow textures
        glBindTextureUnit(6, pass->shadow.moment_maps);
        glBindTextureUnit(7, pass->shadow.depth_maps);

    }

    size_t chunk_amount = LC_World_GetDrawCmdAmount();

    glBindVertexArray(drawData->lc_world.world_render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawData->lc_world.world_render_data->transparents_vertex_buffer.buffer);
    glBindVertexBuffer(0, drawData->lc_world.world_render_data->transparents_vertex_buffer.buffer, 0, sizeof(ChunkVertex));

    GLenum render_mode = (r_cvars.r_wireframe->int_value == 2) ? GL_LINES : GL_TRIANGLES;

    if (mode == 0)
    {
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_ATOMIC_COUNTER_BUFFER | GL_SHADER_STORAGE_BARRIER_BIT);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->sorted_draw_cmds_buffer.buffer);
        glBindBuffer(GL_PARAMETER_BUFFER, drawData->lc_world.world_render_data->draw_cmds_counters_buffers[1]);
        glMultiDrawArraysIndirectCount(render_mode, sizeof(DrawArraysIndirectCommand) * 2000, 0, chunk_amount, 0);
    }
    //Shadow rendering
    else if (mode > 0)
    {
        int shadow_split = min(mode - 1, 3);

        int* first = NULL;
        int* count = NULL;
        int draw_count = 0;

        first = drawData->lc_world.shadow_firsts_transparent[shadow_split]->data;
        count = drawData->lc_world.shadow_counts_transparent[shadow_split]->data;
        draw_count = scene.cull_data.lc_world_shadow_cull_transparent_count[shadow_split];

        if (draw_count > 0)
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 35, drawData->lc_world.world_render_data->shadow_chunk_indexes_ssbo);
            glBindTextureUnit(0, drawData->lc_world.world_render_data->texture_atlas->id);
            glMultiDrawArrays(GL_TRIANGLES, first, count, draw_count);
        }
    }

}

void Render_DrawChunkOcclusionBoxes()
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data)
    {
        return;
    }

    glUseProgram(pass->lc.occlussion_boxes_shader);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, drawData->lc_world.world_render_data->chunk_data.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, drawData->lc_world.world_render_data->visibles_sorted_ssbo);

    glBindVertexArray(resources.cubeVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 14, scene.cull_data.lc_world_in_frustrum_count);
}

void Render_DebugScene()
{   
    Render_LineBatch();
    Render_TriangleBatch();
}

void Render_UI()
{
    Render_ScreenQuadBatch();
}

extern float C_getDeltaTime();;
static void Compute_Particles()
{
    int num_emitters = pass->particles.total_emitter_amount;
    if (num_emitters > 0)
    {
        int num_x_groups = max(1, (int)ceilf(num_emitters / 8));
        //Dispatch emitter processing
        glUseProgram(pass->particles.emitter_process_shader);
        Shader_SetInteger(pass->particles.emitter_process_shader, "u_totalEmitterCount", num_emitters);
        Shader_SetFloat(pass->particles.emitter_process_shader, "u_cpuDelta", C_getDeltaTime());
        glDispatchCompute(1, 1, 1);
    }

    if (storage.particles.used_bytes > 0)
    {
        int num_particles = sizeof(Particle) / storage.particles.used_bytes;
        //Dispatch particle update
        glUseProgram(pass->particles.particle_process_shader);

        Shader_SetInteger(pass->particles.particle_process_shader, "u_totalParticleAmount", pass->particles.total_particle_amount);

        float num_x_groups_real = ceilf((float)pass->particles.total_particle_amount / 64.0f);

        unsigned num_x_groups = max(1, num_x_groups_real);
        glBindTextureUnit(0, pass->deferred.depth_texture);
        glBindTextureUnit(1, pass->deferred.gNormalMetal_texture);
        glDispatchCompute(num_x_groups, 1, 1);
    }
}

static void Compute_WorldChunks()
{
    if (drawData->lc_world.draw == false)
    {
        return;
    }

    size_t chunk_amount = LC_World_GetDrawCmdAmount();
    int num_x_groups = ceilf((float)chunk_amount / 16.0f);

    drawData->lc_world.world_render_data = LC_World_getRenderData();

    glUseProgram(pass->lc.chunk_process_shader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, drawData->lc_world.world_render_data->chunk_data.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, drawData->lc_world.world_render_data->draw_cmds_buffer.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, drawData->lc_world.world_render_data->visibles_sorted_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 19, drawData->lc_world.world_render_data->visibles_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, drawData->lc_world.world_render_data->sorted_draw_cmds_buffer.buffer);

    for (int i = 0; i < 2; i++)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21 + i, drawData->lc_world.world_render_data->draw_cmds_counters_buffers[i]);
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


}

void Render_OpaqueScene(RenderPassState rpass_state)
{
    switch (rpass_state)
    {
    case RPass__DEPTH_PREPASS:
    {
        //Render opaque world chunks
        glUseProgram(pass->lc.depthPrepass_shader);
        Render_OpaqueWorldChunks(false, 0);

        //Render other opaque stuff

        break;
    }
    case RPass__GBUFFER:
    {
        //Render opaque world chunks
        glUseProgram(pass->lc.gBuffer_shader);
        Render_OpaqueWorldChunks(true, 0);

        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT1:
    {
        //Render opaque world chunks
        glUseProgram(pass->lc.shadow_map_shader);
        Shader_SetMatrix4(pass->lc.shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[0]);
        Shader_SetInteger(pass->lc.shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_offsets[0]);
        Render_OpaqueWorldChunks(false, 1);


        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT2:
    {

        //Render opaque world chunks
        glUseProgram(pass->lc.shadow_map_shader);
        Shader_SetMatrix4(pass->lc.shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[1]);
        Shader_SetInteger(pass->lc.shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_offsets[1]);
        Render_OpaqueWorldChunks(false, 2);

        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT3:
    {
        //Render opaque world chunks
        glUseProgram(pass->lc.shadow_map_shader);
        Shader_SetMatrix4(pass->lc.shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[2]);
        Shader_SetInteger(pass->lc.shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_offsets[2]);
        Render_OpaqueWorldChunks(false, 3);

        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT4:
    {

        //Render opaque world chunks
        glUseProgram(pass->lc.shadow_map_shader);
        Shader_SetMatrix4(pass->lc.shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[3]);
        Shader_SetInteger(pass->lc.shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_offsets[3]);
        Render_OpaqueWorldChunks(false, 4);

        //Render other opaque stuff

        break;
    }
    case RPass__DEBUG_PASS:
    {
        break;
    }
    default:
        break;
    }
}
void Render_SemiOpaqueScene(RenderPassState rpass_state)
{
    unsigned shadow_data_offset = dA_size(drawData->lc_world.shadow_sorted_chunk_indexes);

    switch (rpass_state)
    {
    case RPass__GBUFFER:
    {
        glUseProgram(pass->lc.transparents_gBuffer_shader);
        Render_SemiTransparentWorldChunks(true, 0);
        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT1:
    {
        //Render transparent world chunks
        glUseProgram(pass->lc.transparents_shadow_map_shader);
        Shader_SetMatrix4(pass->lc.transparents_shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[0]);
        Shader_SetInteger(pass->lc.transparents_shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_transparent_offsets[0] + shadow_data_offset);
        Render_SemiTransparentWorldChunks(false, 1);

        //Render other transparent stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT2:
    {
        //Render transparent world chunks
        glUseProgram(pass->lc.transparents_shadow_map_shader);
        Shader_SetMatrix4(pass->lc.transparents_shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[1]);
        Shader_SetInteger(pass->lc.transparents_shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_transparent_offsets[1] + shadow_data_offset);
        Render_SemiTransparentWorldChunks(false, 2);

        //Render other transparent stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT3:
    {
        //Render transparent world chunks
        glUseProgram(pass->lc.transparents_shadow_map_shader);
        Shader_SetMatrix4(pass->lc.transparents_shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[2]);
        Shader_SetInteger(pass->lc.transparents_shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_transparent_offsets[2] + shadow_data_offset);
        Render_SemiTransparentWorldChunks(false, 3);

        //Render other transparent stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT4:
    {
        //Render transparent world chunks
        glUseProgram(pass->lc.transparents_shadow_map_shader);
        Shader_SetMatrix4(pass->lc.transparents_shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[3]);
        Shader_SetInteger(pass->lc.transparents_shadow_map_shader, "u_dataOffset", drawData->lc_world.shadow_sorted_chunk_transparent_offsets[3] + shadow_data_offset);
        Render_SemiTransparentWorldChunks(false, 4);

        //Render other transparent stuff

        break;
    }
    case RPass__DEBUG_PASS:
    {
        break;
    }
    default:
        break;
    }
}
void Render_TransparentScene(RenderPassState rpass_state)
{

}