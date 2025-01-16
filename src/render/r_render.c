/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Functions that issue draw, dispatch calls to the gpu
    called from r_pass.c
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "render/r_core.h"

#include "render/r_public.h"
#include "core/core_common.h"


extern RDraw_DrawData* drawData;
extern R_RendererResources resources;
extern RPass_PassData* pass;
extern R_Cvars r_cvars;
extern R_StorageBuffers storage;
extern R_Scene scene;

static void Render_ScreenQuadBatch()
{
    RDraw_ScreenQuadData* data = &drawData->screen_quad;

    if (data->vertices_count == 0)
    {
        return;
    }

    Shader_Use(&pass->scene.screen_shader);
   
    //bind texture units
    for (int i = 0; i < 32; i++)
    {
        if (data->tex_array[i] == 0)
        {
            break;
        }

        unsigned texture_index = data->tex_array[i];

        glBindTextureUnit(i, texture_index);
    }

    Shader_SetMat4(&pass->scene.screen_shader, SCREEN_SHADER_UNIFORM_PROJECTION, data->ortho);

    glBindVertexArray(data->vao);
    glDrawElements(GL_TRIANGLES, data->indices_count, GL_UNSIGNED_INT, 0);

    data->vertices_count = 0;
    data->indices_count = 0;
    data->tex_index = 0;
}

static void Render_LineBatch()
{
    RDraw_LineData* data = &drawData->lines;

    if (data->vertices_count == 0)
    {
        return;
    }

    Shader_ResetDefines(&pass->scene.shader_3d_forward);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_COLOR_ATTRIB, true);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_COLOR_8BIT, true);

    Shader_Use(&pass->scene.shader_3d_forward);

    glBindVertexArray(data->vao);
    glDrawArrays(GL_LINES, 0, data->vertices_count);
}

static void Render_TriangleBatch()
{
    RDraw_TriangleData* data = &drawData->triangle;

    if (data->vertices_count == 0)
        return;

    return;

   // glUseProgram(pass->general.triangle_3d_shader);

    glBindTextures(0, 32, data->texture_ids);

    glEnable(GL_BLEND);
    glBindVertexArray(data->vao);
    glDrawArrays(GL_TRIANGLES, 0, data->vertices_count);
}

static void Render_CubeInstances()
{
    RDraw_CubeData* data = &drawData->cube;

    if (data->vertices_buffer->elements_size == 0)
    {
        return;
    }

    Shader_ResetDefines(&pass->scene.shader_3d_forward);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_TEXCOORD_ATTRIB, true);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_MAT3, true);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_UV, true);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_COLOR, true);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_CUSTOM, true);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_USE_TEXTURE_ARR, true);
    Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_SEMI_TRANSPARENT_PASS, true);

    Shader_Use(&pass->scene.shader_3d_forward);

    glDisable(GL_CULL_FACE);
    glBindVertexArray(drawData->cube.vao);
    
    glBindTextures(0, data->texture_index, data->texture_ids);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, drawData->cube.instance_count);

    glEnable(GL_CULL_FACE);
}

static void Render_Particles()
{
    RDraw_ParticleData* data = &drawData->particles;

    //nothing to draw?
    if (data->instance_count <= 0)
    {
        return;
    }
    glBindTextures(0, drawData->particles.texture_index, drawData->particles.texture_ids);

    glBindVertexArray(data->vao);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, data->instance_count);
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


static void Render_OpaqueWorldChunks(bool p_TextureDraw, int mode)
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data)
    {
        return;
    }
    size_t max_chunk_render_amount = scene.cull_data.lc_world.opaque_in_frustrum;

    //hacky but, add little so that won't cause popups 
    if (max_chunk_render_amount > 0)
    {
        max_chunk_render_amount += 20;
    }

    if (p_TextureDraw)
    {
        glBindTextureUnit(0, drawData->lc_world.world_render_data->texture_atlas->id);
        glBindTextureUnit(1, drawData->lc_world.world_render_data->texture_atlas_normals->id);
        glBindTextureUnit(2, drawData->lc_world.world_render_data->texture_atlas_mer->id);
    }
    glBindVertexArray(drawData->lc_world.world_render_data->vao);
    glBindVertexBuffer(0, drawData->lc_world.world_render_data->opaque_buffer.buffer, 0, sizeof(ChunkVertex));
      
    //Normal rendering
    if (mode == 0)
    {
        GLenum render_mode = (r_cvars.r_wireframe->int_value == 2) ? GL_LINES : GL_TRIANGLES;

        int offset = 0;
        if (max_chunk_render_amount > 0)
        {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->draw_cmds_sorted_buffer);
            glBindBuffer(GL_PARAMETER_BUFFER, drawData->lc_world.world_render_data->atomic_counters[0]);

            glMultiDrawArraysIndirectCount(render_mode, offset, 0, max_chunk_render_amount, 0);

            //kinda hacky but works(only for debugging)
            if (r_cvars.r_wireframe->int_value == 1)
            {
                glDisable(GL_DEPTH_TEST);
                glMultiDrawArraysIndirectCount(GL_LINES, offset, 0, max_chunk_render_amount, 0);
                glEnable(GL_DEPTH_TEST);
            }
        }
       
    }
    //Shadow rendering
    else if (mode < 5)
    {
        int shadow_split = min(mode - 1, 3);

        int* first = NULL;
        int* count = NULL;
        int draw_count = 0;

        first = drawData->lc_world.shadow_firsts[shadow_split]->data;
        count = drawData->lc_world.shadow_counts[shadow_split]->data;
        draw_count = scene.cull_data.lc_world.shadow_cull_count[shadow_split];
        if (draw_count > 0)
        {
            glMultiDrawArrays(GL_TRIANGLES, first, count, draw_count);
        } 
    }
    //reflection pass rendering
    else if (mode == 5)
    {
        int* first = drawData->lc_world.reflection_pass_opaque_firsts->data;
        int* count = drawData->lc_world.reflection_pass_opaque_counts->data;
        int draw_count = scene.cull_data.lc_world.reflection_opaque_count;

        if (draw_count > 0)
        {
            glBindTextureUnit(2, pass->ibl.irradianceCubemapTexture);
            glBindTextureUnit(3, pass->ibl.envCubemapTexture);
            glBindTextureUnit(4, pass->ibl.brdfLutTexture);

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
    }

    size_t max_chunk_render_amount = scene.cull_data.lc_world.transparent_in_frustrum;

    //hacky but, add little so that won't cause popups 
    if (max_chunk_render_amount > 0)
    {
        max_chunk_render_amount += 20;
    }

    glBindVertexArray(drawData->lc_world.world_render_data->vao);
    glBindVertexBuffer(0, drawData->lc_world.world_render_data->semi_transparent_buffer.buffer, 0, sizeof(ChunkVertex));

    if (mode == 0)
    {   
        GLenum render_mode = (r_cvars.r_wireframe->int_value == 2) ? GL_LINES : GL_TRIANGLES;
        if (max_chunk_render_amount > 0)
        {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->draw_cmds_sorted_buffer);
            glBindBuffer(GL_PARAMETER_BUFFER, drawData->lc_world.world_render_data->atomic_counters[1]);
            glMultiDrawArraysIndirectCount(render_mode, sizeof(DrawArraysIndirectCommand) * LC_WORLD_MAX_CHUNK_LIMIT, 0, max_chunk_render_amount, 0);

            //kinda hacky but works(only for debugging)
            if (r_cvars.r_wireframe->int_value == 1)
            {
                glDisable(GL_DEPTH_TEST);
                glMultiDrawArraysIndirectCount(GL_LINES, sizeof(DrawArraysIndirectCommand) * LC_WORLD_MAX_CHUNK_LIMIT, 0, max_chunk_render_amount, 0);
                glEnable(GL_DEPTH_TEST);
            }
        }
    }
    //Shadow rendering
    else if (mode < 5)
    {
        int shadow_split = min(mode - 1, 3);

        int* first = NULL;
        int* count = NULL;
        int draw_count = 0;

        first = drawData->lc_world.shadow_firsts_transparent[shadow_split]->data;
        count = drawData->lc_world.shadow_counts_transparent[shadow_split]->data;
        draw_count = scene.cull_data.lc_world.shadow_cull_transparent_count[shadow_split];

        if (draw_count > 0)
        {
            glBindTextureUnit(0, drawData->lc_world.world_render_data->texture_atlas->id);
            glMultiDrawArrays(GL_TRIANGLES, first, count, draw_count);
        }
    }
    //reflection pass rendering
    else if (mode == 5)
    {
        int* first = drawData->lc_world.reflection_pass_transparent_firsts->data;
        int* count = drawData->lc_world.reflection_pass_transparent_counts->data;
        int draw_count = scene.cull_data.lc_world.reflection_transparent_count;

        if (draw_count > 0)
        {
            glBindTextureUnit(2, pass->ibl.irradianceCubemapTexture);
            glBindTextureUnit(3, pass->ibl.envCubemapTexture);
            glBindTextureUnit(4, pass->ibl.brdfLutTexture);

            glMultiDrawArrays(GL_TRIANGLES, first, count, draw_count);
        }
    }

}
void Render_WorldWaterChunks()
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data)
    {
        return;
    }

    size_t max_chunk_render_amount = scene.cull_data.lc_world.water_in_frustrum;

    if (max_chunk_render_amount > 0)
    {
        max_chunk_render_amount += 20;
    }

    Shader_Use(&pass->lc.water_shader);

    glBindTextureUnit(0, pass->ibl.envCubemapTexture);
    glBindTextureUnit(1, drawData->lc_world.world_render_data->water_displacement_texture->id);
    glBindTextureUnit(2, pass->water.reflection_back_texture);
    glBindTextureUnit(3, pass->water.refraction_texture);
    glBindTextureUnit(4, pass->deferred.depth_texture);
    glBindTextureUnit(5, drawData->lc_world.world_render_data->gradient_map->id);

    glBindVertexArray(drawData->lc_world.world_render_data->water_vao);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawData->lc_world.world_render_data->draw_cmds_sorted_buffer);
    glBindBuffer(GL_PARAMETER_BUFFER, drawData->lc_world.world_render_data->atomic_counters[2]);

    if (max_chunk_render_amount > 0)
    {
        glMultiDrawArraysIndirectCount(GL_TRIANGLES, sizeof(DrawArraysIndirectCommand) * LC_WORLD_MAX_CHUNK_LIMIT * 2, 0, max_chunk_render_amount, 0);
    }
}

void Render_DrawChunkOcclusionBoxes()
{
    if (drawData->lc_world.draw == false || !drawData->lc_world.world_render_data || scene.cull_data.lc_world.total_in_frustrum_count <= 0)
    {
        return;
    }

    glNamedBufferSubData(drawData->lc_world.world_render_data->visibles_sorted_buffer, 0, sizeof(int) * scene.cull_data.lc_world.total_in_frustrum_count, scene.cull_data.lc_world.frustrum_sorted_query_buffer);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    glBindVertexArray(resources.cubeVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 14, scene.cull_data.lc_world.total_in_frustrum_count);
}

void Render_SimpleScene()
{   
    Render_LineBatch();
    Render_TriangleBatch();
    Render_CubeInstances();
}

void Render_UI()
{
    Render_ScreenQuadBatch();
}

static void Compute_WorldChunks()
{
    if (drawData->lc_world.draw == false)
    {
        return;
    }

    size_t chunk_amount = scene.cull_data.lc_world.total_in_frustrum_count;
    int num_x_groups = ceilf((float)chunk_amount / 16.0f);

    drawData->lc_world.world_render_data = LC_World_getRenderData();

    Shader_Use(&pass->lc.process_chunks_shader);

    for (int i = 0; i < 3; i++)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10 + i, drawData->lc_world.world_render_data->atomic_counters[i]);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, drawData->lc_world.world_render_data->draw_cmds_buffer.buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, drawData->lc_world.world_render_data->prev_in_frustrum_bitset_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, drawData->lc_world.world_render_data->draw_cmds_sorted_buffer);

    glDispatchCompute(num_x_groups, 1, 1);
}

static void Compute_Meshes()
{

}

void Compute_DispatchAll()
{
    Compute_WorldChunks();
}

void Compute_Sync()
{
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_ATOMIC_COUNTER_BUFFER | GL_SHADER_STORAGE_BARRIER_BIT);
}

void Render_OpaqueScene(RenderPassState rpass_state)
{
    switch (rpass_state)
    {
    case RPass__DEPTH_PREPASS:
    {
        //Render opaque world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);

        Shader_Use(&pass->lc.world_shader);

        Render_OpaqueWorldChunks(false, 0);

        //Render other opaque stuff

        break;
    }
    case RPass__GBUFFER:
    {
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TBN_MATRIX, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_GBUFFER_PASS, true);

        Shader_Use(&pass->lc.world_shader);

        //Render opaque world chunks
        Render_OpaqueWorldChunks(true, 0);

        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT1:
    {
        //Render opaque world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);

        Shader_Use(&pass->lc.world_shader);

        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[0]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_offsets[0]);
        
        Render_OpaqueWorldChunks(false, 1);


        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT2:
    {
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);

        Shader_Use(&pass->lc.world_shader);

        //Render opaque world chunks
        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[1]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_offsets[1]);
        Render_OpaqueWorldChunks(false, 2);

        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT3:
    {
        //Render opaque world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);

        Shader_Use(&pass->lc.world_shader);

        //Render opaque world chunks
        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[2]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_offsets[2]);
        Render_OpaqueWorldChunks(false, 3);

        //Render other opaque stuff

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT4:
    {
        //Render opaque world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);

        Shader_Use(&pass->lc.world_shader);

        //Render opaque world chunks
        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[3]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_offsets[3]);
        Render_OpaqueWorldChunks(false, 4);

        //Render other opaque stuff

        break;
    }
    case RPass__REFLECTION_CLIP:
    {
        glNamedBufferSubData(drawData->lc_world.world_render_data->visibles_sorted_buffer, 0, sizeof(int) * drawData->lc_world.reflection_pass_chunk_indexes->elements_size,
            drawData->lc_world.reflection_pass_chunk_indexes->data);

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_CLIP_DISTANCE, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_FORWARD_PASS, true);
        
        Shader_Use(&pass->lc.world_shader);

        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, pass->water.reflection_projView_matrix);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, 0);
        Shader_SetFloaty(&pass->lc.world_shader, LC_WORLD_UNIFORM_CLIPDISTANCE, -LC_WORLD_WATER_HEIGHT);

        Render_OpaqueWorldChunks(true, 5);

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
    bool allow_particle_shadows = (r_cvars.r_allowParticleShadows->int_value == 1);
    unsigned shadow_data_offset = dA_size(drawData->lc_world.shadow_sorted_chunk_indexes);

    const int MAX_PARTICLE_SHADOW_SPLITS = 1; //adjust if needed
  
    switch (rpass_state)
    {
    case RPass__DEPTH_PREPASS:
    {
        //Render world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_SEMI_TRANSPARENT, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);

        Shader_Use(&pass->lc.world_shader);

        Render_SemiTransparentWorldChunks(true, 0);
     
        Shader_ResetDefines(&pass->scene.shader_3d_forward);
        Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_TEXCOORD_ATTRIB, true);
        Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_SEMI_TRANSPARENT_PASS, true);
        Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_MAT3, true);
        Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_UV, true);
        Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_CUSTOM, true);
        Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_USE_TEXTURE_ARR, true);
        Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_RENDER_DEPTH, true);

        Shader_Use(&pass->scene.shader_3d_forward);

        glDisable(GL_CULL_FACE);
        Render_Particles();
        glEnable(GL_CULL_FACE);

        break;
    }
    case RPass__GBUFFER:
    {
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_GBUFFER_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_SEMI_TRANSPARENT, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TBN_MATRIX, true);

        Shader_Use(&pass->lc.world_shader);
        Render_SemiTransparentWorldChunks(true, 0);
        
        Shader_ResetDefines(&pass->scene.shader_3d_deferred);
        Shader_SetDefine(&pass->scene.shader_3d_deferred, SCENE_3D_DEFINE_TEXCOORD_ATTRIB, true);
        Shader_SetDefine(&pass->scene.shader_3d_deferred, SCENE_3D_DEFINE_SEMI_TRANSPARENT_PASS, true);
        Shader_SetDefine(&pass->scene.shader_3d_deferred, SCENE_3D_DEFINE_INSTANCE_MAT3, true);
        Shader_SetDefine(&pass->scene.shader_3d_deferred, SCENE_3D_DEFINE_INSTANCE_UV, true);
        Shader_SetDefine(&pass->scene.shader_3d_deferred, SCENE_3D_DEFINE_INSTANCE_CUSTOM, true);
        Shader_SetDefine(&pass->scene.shader_3d_deferred, SCENE_3D_DEFINE_USE_TEXTURE_ARR, true);
        Shader_SetDefine(&pass->scene.shader_3d_deferred, SCENE_3D_DEFINE_NORMAL_ATTRIB, true);

        Shader_Use(&pass->scene.shader_3d_deferred);

        glDisable(GL_CULL_FACE);
        Render_Particles();
        glEnable(GL_CULL_FACE);

        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT1:
    {
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_SEMI_TRANSPARENT, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);

        Shader_Use(&pass->lc.world_shader);

        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[0]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_transparent_offsets[0] + shadow_data_offset);

        Render_SemiTransparentWorldChunks(false, 1);

        if (allow_particle_shadows && MAX_PARTICLE_SHADOW_SPLITS >= 1)
        {

            Shader_ResetDefines(&pass->scene.shader_3d_forward);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_TEXCOORD_ATTRIB, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_SEMI_TRANSPARENT_PASS, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_MAT3, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_UV, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_INSTANCE_CUSTOM, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_USE_TEXTURE_ARR, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_RENDER_DEPTH, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_BILLBOARD_SHADOWS, true);
            Shader_SetDefine(&pass->scene.shader_3d_forward, SCENE_3D_DEFINE_USE_UNIFORM_CAMERA_MATRIX, true);
            
            Shader_Use(&pass->scene.shader_3d_forward);

            Shader_SetMat4(&pass->scene.shader_3d_forward, SCENE_3D_UNIFORM_CAMERAMATRIX, scene.scene_data.shadow_matrixes[0]);
            Render_Particles();
        }
        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT2:
    {
        //Render transparent world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_SEMI_TRANSPARENT, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);

        Shader_Use(&pass->lc.world_shader);

        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[1]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_transparent_offsets[1] + shadow_data_offset);

        Render_SemiTransparentWorldChunks(false, 2);

      
        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT3:
    {
        //Render transparent world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_SEMI_TRANSPARENT, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);

        Shader_Use(&pass->lc.world_shader);

        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[2]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_transparent_offsets[2] + shadow_data_offset);

        Render_SemiTransparentWorldChunks(false, 3);

        //Render other transparent stuff
      
        break;
    }
    case RPass__SHADOW_MAPPING_SPLIT4:
    {
        //Render transparent world chunks
        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_DEPTH_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_SEMI_TRANSPARENT, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);

        Shader_Use(&pass->lc.world_shader);

        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, scene.scene_data.shadow_matrixes[3]);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.shadow_sorted_chunk_transparent_offsets[3] + shadow_data_offset);

        Render_SemiTransparentWorldChunks(false, 4);

        //Render other transparent stuff
        
        break;
    }
    case RPass__REFLECTION_CLIP:
    {
        glNamedBufferSubData(drawData->lc_world.world_render_data->visibles_sorted_buffer, sizeof(int) * drawData->lc_world.reflection_pass_chunk_indexes->elements_size, 
            sizeof(int) * drawData->lc_world.reflection_pass_transparent_chunk_indexes->elements_size,
            drawData->lc_world.reflection_pass_transparent_chunk_indexes->data);

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

        Shader_ResetDefines(&pass->lc.world_shader);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_CHUNK_INDEX_USE_OFFSET, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_UNIFORM_MATRIX, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_TEXCOORDS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_USE_CLIP_DISTANCE, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_FORWARD_PASS, true);
        Shader_SetDefine(&pass->lc.world_shader, LC_WORLD_DEFINE_SEMI_TRANSPARENT, true);

        Shader_Use(&pass->lc.world_shader);

        Shader_SetMat4(&pass->lc.world_shader, LC_WORLD_UNIFORM_MATRIX, pass->water.reflection_projView_matrix);
        Shader_SetInt(&pass->lc.world_shader, LC_WORLD_UNIFORM_CHUNKOFFSET, drawData->lc_world.reflection_pass_chunk_indexes->elements_size);
        Shader_SetFloaty(&pass->lc.world_shader, LC_WORLD_UNIFORM_CLIPDISTANCE, -LC_WORLD_WATER_HEIGHT);

        Render_SemiTransparentWorldChunks(true, 5);
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