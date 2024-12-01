/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Processes various renderer releated commands
    async or sync depending on the settings
    No gl calls here
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "r_core.h"
#include "r_public.h"
#include "utility/u_math.h"
#include "core/core_common.h"

extern R_Scene scene;
extern R_StorageBuffers storage;
extern R_CMD_Buffer* cmdBuffer;
extern RDraw_DrawData* drawData;
extern RPass_PassData* pass;
extern R_BackendData* backend_data;
extern R_Cvars r_cvars;

static void DecimalColorTo8Bit(vec4 src, uint8_t dest[4])
{
    dest[0] = 255.0 * src[0];
    dest[1] = 255.0 * src[1];
    dest[2] = 255.0 * src[2];
    dest[3] = 255.0 * src[3];
}

static void GetCoordsFromTextureRegion(float p_textureWidth, float p_textureHeight, M_Rect2Df p_textureRegion, mat4x2 dest)
{
    int flip_x = (p_textureRegion.x > 0) ? 1 : -1;
    int flip_y = (p_textureRegion.y > 0) ? 1 : -1;

    float rect_width = p_textureRegion.width;
    float rect_height = p_textureRegion.height;

    float rect_x = fabsf(p_textureRegion.x);
    float rect_y = fabsf(p_textureRegion.y);

    const float left = rect_x;
    const float right = rect_x + rect_width;
    const float top = rect_y;
    const float bottom = rect_y + rect_height;

    const int texture_width = p_textureWidth;
    const int texture_height = p_textureHeight;

    dest[0][0] = flip_x * (right / texture_width);
    dest[0][1] = flip_y * (bottom / texture_height);

    dest[1][0] = flip_x * (right / texture_width);
    dest[1][1] = flip_y * (top / texture_height);

    dest[2][0] = flip_x * (left / texture_width);
    dest[2][1] = flip_y * (top / texture_height);

    dest[3][0] = flip_x * (left / texture_width);
    dest[3][1] = flip_y * (bottom / texture_height);
}

static int AssignTextureToTextureArr(unsigned p_textureID, int* r_index, unsigned texture_ids[32])
{
    int texture_slot_index = -1;
    //find or assign the texture
    for (int i = 0; i < 32; i++)
    {
        if (texture_ids[i] == 0)
            break;

        //exists already?
        if (p_textureID == texture_ids[i])
        {
            texture_slot_index = i;
            break;
        }
    }
    //not found? then add to the array
    if (texture_slot_index == -1)
    {
        texture_ids[*r_index] = p_textureID;
        texture_slot_index = *r_index;

        *r_index = *r_index + 1;
    }

    return max(texture_slot_index, 0);
}

static void Process_ScreenTexture(R_Texture* p_texture, M_Rect2Df p_textureRegion, vec2 p_position, vec2 p_scale, float p_rotation, vec4 p_color)
{
    RDraw_ScreenQuadData* data = &drawData->screen_quad;

    //draw the current batch??
    if (data->vertices_count + 4 >= SCREEN_QUAD_VERTICES_BUFFER_SIZE || data->tex_index >= SCREEN_QUAD_MAX_TEXTURES_IN_ARRAY - 1)
    {
        //_drawScreenQuadBatch();
        return;
    }
    int texture_slot_index = -1;

    //find or assign the texture
    for (int i = 0; i < SCREEN_QUAD_MAX_TEXTURES_IN_ARRAY; i++)
    {
        if (data->tex_array[i] == 0)
            break;

        //exists already?
        if (p_texture->id == data->tex_array[i])
        {
            texture_slot_index = i;
            break;
        }
    }
    //not found? then add to the array
    if (texture_slot_index == -1)
    {
        data->tex_array[data->tex_index] = p_texture->id;
        texture_slot_index = data->tex_index;

        data->tex_index++;
    }

    //set up the vertices
    vec3 vertices[] =
    {
          0.5f,  0.5f, 0.0f,// top right
          0.5f, -0.5f, 0.0f, // bottom right
         -0.5f, -0.5f, 0.0f,// bottom left
         -0.5f,  0.5f, 0.0f // top left 
    };

    vec2 scale;
    scale[0] = p_textureRegion.width * p_scale[0];
    scale[1] = p_textureRegion.height * p_scale[1];

    mat4 model;
    Math_Model2D(p_position, scale, p_rotation, model);

    mat4x2 tex_coords;
    GetCoordsFromTextureRegion(p_texture->width, p_texture->height, p_textureRegion, tex_coords);

    ScreenVertex* vertices_buffer = data->vertices;
    for (int i = 0; i < 4; i++)
    {
        glm_mat4_mulv3(model, vertices[i], 1.0f, vertices[i]);

        vertices_buffer[data->vertices_count + i].position[0] = vertices[i][0];
        vertices_buffer[data->vertices_count + i].position[1] = vertices[i][1];
        vertices_buffer[data->vertices_count + i].position[2] = 1; //z_index aka draw order layer index

        vertices_buffer[data->vertices_count + i].tex_coords[0] = tex_coords[i][0];
        vertices_buffer[data->vertices_count + i].tex_coords[1] = tex_coords[i][1];

        vertices_buffer[data->vertices_count + i].packed_texIndex_Flags = texture_slot_index;

        DecimalColorTo8Bit(p_color, vertices_buffer[data->vertices_count + i].color);
    }
    data->vertices_count += 4;
    data->indices_count += 6;
}

static void Process_Line(vec3 from, vec3 to, vec4 color)
{
    RDraw_LineData* data = &drawData->lines;
    
    if (data->vertices_count + 2 >= LINE_VERTICES_BUFFER_SIZE)
        return;

    //start point
    glm_vec3_copy(from, data->vertices[data->vertices_count].position);
    DecimalColorTo8Bit(color, data->vertices[data->vertices_count].color);

    data->vertices_count++;

    //end point
    glm_vec3_copy(to, data->vertices[data->vertices_count].position);
    DecimalColorTo8Bit(color, data->vertices[data->vertices_count].color);
  
    data->vertices_count++;
}

static void Process_Line2(float f_x, float f_y, float f_z, float t_x, float t_y, float t_z, vec4 color)
{
    vec3 from, to;

    from[0] = f_x;
    from[1] = f_y;
    from[2] = f_z;

    to[0] = t_x;
    to[1] = t_y;
    to[2] = t_z;

    Process_Line(from, to, color);
}

static void Process_Triangle(vec3 p1, vec3 p2, vec3 p3, vec2 p1Coords, vec2 p2Coords, vec2 p3Coords, vec4 color, R_Texture* p_tex)
{
    RDraw_TriangleData* data = &drawData->triangle;

    if (data->vertices_count + 3 >= TRIANGLE_VERTICES_BUFFER_SIZE)
        return;

    int texture_slot_index = -1;

    if (p_tex)
    {
        //find or assign the texture
        for (int i = 1; i < SCREEN_QUAD_MAX_TEXTURES_IN_ARRAY; i++)
        {
            if (data->texture_ids[i] == 0)
                break;

            //exists already?
            if (p_tex->id == data->texture_ids[i])
            {
                texture_slot_index = i;
                break;
            }
        }
        //not found? then add to the array
        if (texture_slot_index == -1)
        {
            data->texture_ids[data->texture_index] = p_tex->id;
            texture_slot_index = data->texture_index;

            data->texture_index++;
        }
    }
    
    //point 1
    glm_vec3_copy(p1, data->vertices[data->vertices_count].position);
    DecimalColorTo8Bit(color, data->vertices[data->vertices_count].color);
    if (p1Coords)
    {
        glm_vec2_copy(p1Coords, data->vertices[data->vertices_count].texOffset);
    }
    data->vertices[data->vertices_count].texture_index = texture_slot_index;
  
    data->vertices_count++;

    //point 2
    glm_vec3_copy(p2, data->vertices[data->vertices_count].position);
    DecimalColorTo8Bit(color, data->vertices[data->vertices_count].color);
    if (p2Coords)
    {
        glm_vec2_copy(p2Coords, data->vertices[data->vertices_count].texOffset);
    }
    data->vertices[data->vertices_count].texture_index = texture_slot_index;
    
    data->vertices_count++;

    //point 3
    glm_vec3_copy(p3, data->vertices[data->vertices_count].position);
    DecimalColorTo8Bit(color, data->vertices[data->vertices_count].color);
    if (p3Coords)
    {
        glm_vec2_copy(p3Coords, data->vertices[data->vertices_count].texOffset);
    }
    data->vertices[data->vertices_count].texture_index = texture_slot_index;

    data->vertices_count++;
}

static void Process_Triangle2(float p1_x, float p1_y, float p1_z, float p2_x, float p2_y, float p2_z, float p3_x, float p3_y, float p3_z, vec4 color, R_Texture* p_tex)
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

    Process_Triangle(p1, p2, p3, NULL, NULL, NULL, color, p_tex);
}

static void Process_CubeWires(vec3 box[2], vec4 color)
{
    //perform frustrum cull
    if (!glm_aabb_frustum(box, scene.camera.frustrum_planes))
    {
        return;
    }

    //adding a small amount fixes z_fighting issues
    for (int i = 0; i < 3; i++)
    {
        box[0][i] -= 0.001;
        box[1][i] += 0.001;
    }

    vec3 box_size;
    glm_vec3_sub(box[1], box[1], box_size);

    Process_Line2(box[0][0], box[0][1], box[0][2], box[1][0], box[0][1], box[0][2], color);
    Process_Line2(box[0][0], box[1][1], box[0][2], box[1][0], box[1][1], box[0][2], color);
    Process_Line2(box[0][0], box[1][1], box[0][2], box[0][0], box[0][1], box[0][2], color);
    Process_Line2(box[1][0], box[0][1], box[0][2], box[1][0], box[1][1], box[0][2], color);
    Process_Line2(box[0][0], box[0][1], box[0][2], box[0][0], box[0][1], box[1][2], color);
    Process_Line2(box[0][0], box[0][1], box[1][2], box[1][0], box[0][1], box[1][2], color);
    Process_Line2(box[1][0], box[0][1], box[1][2], box[1][0], box[0][1], box[0][2], color);
    Process_Line2(box[0][0], box[0][1], box[1][2], box[0][0], box[1][1], box[1][2], color);
    Process_Line2(box[0][0], box[1][1], box[1][2], box[1][0], box[1][1], box[1][2], color);
    Process_Line2(box[0][0], box[1][1], box[0][2], box[0][0], box[1][1], box[1][2], color);
    Process_Line2(box[1][0], box[1][1], box[0][2], box[1][0], box[1][1], box[1][2], color);
    Process_Line2(box[1][0], box[0][1], box[1][2], box[1][0], box[1][1], box[1][2], color);
}

static void Process_Cube(vec3 box[2], vec4 color, R_Texture* p_tex, M_Rect2Df p_textureRegion)
{
    //perform frustrum cull
    if (!glm_aabb_frustum(box, scene.camera.frustrum_planes))
    {
        return;
    }

    RDraw_CubeData* data = &drawData->cube;

    mat4 model_matrix;
    glm_mat4_identity(model_matrix);

    vec3 box_size, box_position;
    glm_vec3_sub(box[1], box[0], box_size);

    box_position[0] = box[0][0] + (box_size[0] * 0.5);
    box_position[1] = box[0][1] + (box_size[1] * 0.5);
    box_position[2] = box[0][2] + (box_size[2] * 0.5);

    glm_translate(model_matrix, box_position);
    glm_scale(model_matrix, box_size);

    glm_mat4_transpose(model_matrix);

    int texture_slot_index = -1;
    if (p_tex)
    {
        if (p_tex)
        {
            //find or assign the texture
            for (int i = 0; i < SCREEN_QUAD_MAX_TEXTURES_IN_ARRAY; i++)
            {
                if (data->texture_ids[i] == 0)
                    break;

                //exists already?
                if (p_tex->id == data->texture_ids[i])
                {
                    texture_slot_index = i;
                    break;
                }
            }
            //not found? then add to the array
            if (texture_slot_index == -1)
            {
                data->texture_ids[data->texture_index] = p_tex->id;
                texture_slot_index = data->texture_index;

                data->texture_index++;
            }
        }
    }

    float instance_uv[4];
    instance_uv[0] = p_textureRegion.x;
    instance_uv[1] = p_textureRegion.y;
    instance_uv[2] = p_textureRegion.width;
    instance_uv[3] = p_textureRegion.height;

    float custom[4];
    custom[0] = max(texture_slot_index, 0);
    custom[1] = 1;
    custom[2] = 2;
    custom[3] = 3;

    dA_emplaceBackMultipleData(drawData->cube.vertices_buffer, 4 * 3, model_matrix);
    dA_emplaceBackMultipleData(drawData->cube.vertices_buffer, 4, instance_uv);
    dA_emplaceBackMultipleData(drawData->cube.vertices_buffer, 4, color);
    dA_emplaceBackMultipleData(drawData->cube.vertices_buffer, 4, custom);

    drawData->cube.instance_count++;
}

static void Process_AddParticleToQuadInstances(ParticleCpu* const particle, mat4 matrix, M_Rect2Df textureRegion, int textureIndex)
{
    mat4 transposed_matrix;
    glm_mat4_transpose_to(matrix, transposed_matrix);

    float instance_uv[4];
    instance_uv[0] = textureRegion.x;
    instance_uv[1] = textureRegion.y;
    instance_uv[2] = textureRegion.width;
    instance_uv[3] = textureRegion.height;

    float custom[4];
    custom[0] = textureIndex;
    custom[1] = 1;
    custom[2] = 2;
    custom[3] = 3;

    dA_emplaceBackMultipleData(drawData->particles.instance_buffer, 4 * 3, transposed_matrix);
    dA_emplaceBackMultipleData(drawData->particles.instance_buffer, 4, instance_uv);
    dA_emplaceBackMultipleData(drawData->particles.instance_buffer, 4, particle->color);
    dA_emplaceBackMultipleData(drawData->particles.instance_buffer, 4, custom);

    drawData->particles.instance_count++;
}

static void Process_ParticleCollideWithWorld(ParticleCpu* const particle, ParticleEmitterSettings* emitter)
{
    vec3 normal_vel;
    glm_vec3_normalize_to(particle->velocity, normal_vel);

    LC_Chunk* chunk;
    ivec3 relative_block_pos;

    LC_Block* block = LC_World_GetBlock(particle->xform[3][0] + (normal_vel[0] * emitter->scale), particle->xform[3][1] + (normal_vel[1] * emitter->scale),
        particle->xform[3][2] + (normal_vel[2] * emitter->scale), relative_block_pos, &chunk);

    if (block && LC_isBlockCollidable(block->type) && particle->time > 0.2)
    {
        ivec3 relative_particle_position;
        relative_particle_position[0] = roundf(particle->xform[3][0]) - chunk->global_position[0];
        relative_particle_position[1] = roundf(particle->xform[3][1]) - chunk->global_position[1];
        relative_particle_position[2] = roundf(particle->xform[3][2]) - chunk->global_position[2];

        vec3 normal;
        glm_vec3_zero(normal);

        if (relative_particle_position[0] > relative_block_pos[0])
        {
            normal[0] = 1;
        }
        else if (relative_particle_position[0] < relative_block_pos[0])
        {
            normal[0] = -1;
        }
        else if (relative_particle_position[1] > relative_block_pos[1])
        {
            normal[1] = 1;
        }
        else if (relative_particle_position[1] < relative_block_pos[1])
        {
            normal[1] = -1;
        }
        else if (relative_particle_position[2] > relative_block_pos[2])
        {
            normal[2] = 1;
        }
        else if (relative_particle_position[2] < relative_block_pos[2])
        {
            normal[2] = -1;
        }

        vec3 reflect;
        float normal_dot = glm_dot(normal, particle->velocity);
        reflect[0] = particle->velocity[0] - 2.0 * normal_dot * normal[0];
        reflect[1] = particle->velocity[1] - 2.0 * normal_dot * normal[1];
        reflect[2] = particle->velocity[2] - 2.0 * normal_dot * normal[2];

        particle->velocity[0] = reflect[0] * 0.5;
        particle->velocity[1] = reflect[1] * 0.5;
        particle->velocity[2] = reflect[2] * 0.5;
    }
}

static void Process_ParticleSystemUpdate()
{
    //mainly inspired by godot's particle system https://github.com/godotengine/godot/blob/4.3/scene/3d/cpu_particles_3d.cpp#L657
    dA_clear(drawData->particles.instance_buffer);

    const vec3 up = { 0.0, 1.0, 0.0 };

    const double delta = Core_getDeltaTime();

    FL_Node* emitter_node = storage.particle_emitter_clients->next;

    while (emitter_node)
    {
        ParticleEmitterSettings* emitter = emitter_node->value;
        assert(emitter);

        M_Rect2Df texture_region;
        memset(&texture_region, 0, sizeof(M_Rect2Df));

        if (!emitter->emitting)
        {
            emitter_node = emitter_node->next;
            continue;
        }

        if (emitter->force_restart)
        {
            emitter->_cycle = 0;
            emitter->_time = 0;
            emitter->force_restart = false;
        }

        int NUM_PARTICLES = dA_size(emitter->particles);

        //increase particle amount if needed
        if (NUM_PARTICLES < emitter->particle_amount)
        {
            dA_clear(emitter->particles);
            dA_emplaceBackMultiple(emitter->particles, emitter->particle_amount);

            NUM_PARTICLES = dA_size(emitter->particles);
        }

        double e_delta = delta * emitter->speed_scale;
        double prev_time = emitter->_time;

        emitter->_time += e_delta;

        if (emitter->_time > emitter->life_time)
        {
            emitter->_time = fmodf(emitter->_time, emitter->life_time);
            emitter->_cycle++;

            if (emitter->one_shot && emitter->_cycle > 0)
            {
                emitter->emitting = false;
                emitter_node = emitter_node->next;
                continue;
            }
        }

        bool draw = false;

        vec3 transformed_box[2];
        glm_mat4_mulv3(emitter->xform, emitter->aabb[0], 1.0, transformed_box[0]);
        glm_mat4_mulv3(emitter->xform, emitter->aabb[1], 1.0, transformed_box[1]);
        //perform frustrum check 
        if (glm_aabb_frustum(transformed_box, scene.camera.frustrum_planes))
        {
            draw = true;
        }
        int texture_index = 0;
        if (draw)
        {
            if (emitter->texture)
            {
                texture_index = AssignTextureToTextureArr(emitter->texture->id, &drawData->particles.texture_index, drawData->particles.texture_ids);
            }

            int h_frames = max(emitter->h_frames, 1);
            int v_frames = max(emitter->v_frames, 1);

            vec2 frameOffset;
            frameOffset[0] = emitter->frame % h_frames;
            frameOffset[1] = emitter->frame / h_frames;

            texture_region.width = emitter->h_frames;
            texture_region.height = emitter->v_frames;
            texture_region.x = frameOffset[0];
            texture_region.y = frameOffset[1];
        }
        //draw = true;
        double system_time = emitter->_time / max(emitter->life_time, 0.0001);

        //process particles
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            ParticleCpu* particle = dA_at(emitter->particles, i);

            double local_delta = e_delta;

            double restart_phase = (double)i / (double)NUM_PARTICLES;

            if (emitter->randomness > 0)
            {
                uint32_t seed = emitter->_cycle;
                if (restart_phase >= system_time)
                {
                    seed -= 1;
                }
                seed *= NUM_PARTICLES;
                seed += i;

                uint32_t hash = Hash_id(seed);
                double random = (hash % 65536) / 65536.0;
                restart_phase += emitter->randomness * random * 1.0 / (double)NUM_PARTICLES;
            }

            restart_phase *= (1.0 - emitter->explosiveness);

            bool restart = false;

            if (emitter->_time > prev_time)
            {
                if (restart_phase >= prev_time && restart_phase < emitter->_time)
                {
                    restart = true;
                    local_delta = (emitter->_time - restart_phase) * emitter->life_time;
                }
            }
            else if (e_delta > 0.0)
            {
                if (restart_phase >= prev_time)
                {
                    restart = true;
                    local_delta = (1.0 - restart_phase + emitter->_time) * emitter->life_time;
                }
                else if (restart_phase < emitter->_time)
                {
                    restart = true;
                    local_delta = (emitter->_time - restart_phase) * emitter->life_time;
                }
            }
            
            if (particle->time * (1.0 - emitter->explosiveness) > emitter->life_time)
            {
                restart = true;
            }

            if (restart)
            {
                vec3 v;
                glm_vec3_copy(emitter->direction, v);

                particle->active = true;

                if (emitter->spread > 0)
                {
                    vec3   v1, v2;
                    float  c, s;

                    float angle1 = glm_rad((Math_randf() * 2.0 - 1.0) * emitter->spread);
                    float angle2 = glm_rad((Math_randf() * 2.0 - 1.0) * ((1.0 - emitter->flatness) * emitter->spread));

                    c = cos(angle1) * cos(angle2);
                    s = sin(angle1) * sin(angle2);

                    vec3 k = { 0, 1, 0 };

                    glm_vec3_normalize(k);
                    /* Right Hand, Rodrigues' rotation formula:
                        v = v*cos(t) + (kxv)sin(t) + k*(k.v)(1 - cos(t))
                    */
                    glm_vec3_scale(v, c, v1);

                    glm_vec3_cross(k, v, v2);
                    glm_vec3_scale(v2, s, v2);

                    glm_vec3_add(v1, v2, v1);

                    glm_vec3_scale(k, glm_vec3_dot(k, v) * (1.0f - c), v2);
                    glm_vec3_add(v1, v2, v);
                }
               
                float initial_velocity = emitter->initial_velocity;

                glm_mat4_copy(emitter->xform, particle->xform);
                glm_vec3_scale(v, initial_velocity, particle->velocity);
                glm_vec4_copy(emitter->color, particle->color);
                particle->time = 0;

                if (emitter->scale != 1.0)
                {
                   glm_vec3_scale(particle->xform[0], emitter->scale, particle->xform[0]);
                   glm_vec3_scale(particle->xform[1], emitter->scale, particle->xform[1]);
                   glm_vec3_scale(particle->xform[2], emitter->scale, particle->xform[2]);
                }

                switch (emitter->emission_shape)
                {
                case EES__POINT:
                {
                    break;
                }
                case EES__BOX:
                {
                    float random = Math_randf();
                    vec3 e = { Math_randf() * 2.0 - 1.0, Math_randf() * 2.0 - 1.0, Math_randf() * 2.0 - 1.0 };
                    glm_vec3_mul(e, emitter->emission_size, e);
                    glm_vec3_add(particle->xform[3], e, emitter->xform[3]);
                    break;
                }
                default:
                    break;
                }
            }
            else if (!particle->active)
            {
                continue;
            }
            else if (particle->time > emitter->life_time)
            {
                particle->active = false;
            }

            if (particle->active)
            {
                particle->time += local_delta;

                vec3 force = {0, emitter->gravity, 0};

                //linear accel
                if (glm_vec3_norm(particle->velocity) > 0)
                {
                    vec3 normalized_vel;
                    glm_vec3_normalize_to(particle->velocity, normalized_vel);
                    
                    float linear_accel = emitter->linear_accel;
                    glm_vec3_scale(normalized_vel, linear_accel, normalized_vel);

                    glm_vec3_add(normalized_vel, force, force);
                }
                glm_vec3_scale(force, local_delta, force);
                glm_vec3_add(force, particle->velocity, particle->velocity);

                //call collision function if provided
                if (emitter->collision_function)
                {
                    (*emitter->collision_function)(particle, emitter, local_delta);
                }
    
                //friction
                if (emitter->friction > 0.0)
                {
                    float friction = emitter->friction;
                    float l = glm_vec3_norm(particle->velocity);
                    l -= friction * local_delta;

                    if (l < 0)
                    {
                        glm_vec3_zero(particle->velocity);
                    }
                    else
                    {
                        vec3 normalized_vel;
                        glm_vec3_normalize_to(particle->velocity, normalized_vel);

                        glm_vec3_scale(normalized_vel, l, particle->velocity);
                    }
                }

                vec3 delta_vel;
                glm_vec3_scale(particle->velocity, local_delta, delta_vel);
                glm_vec3_add(particle->xform[3], delta_vel, particle->xform[3]);

                glm_vec4_lerp(particle->color, emitter->end_color, local_delta, particle->color);

           
                if (draw)
                {
                    mat4 local_matrix;
                    glm_mat4_copy(particle->xform, local_matrix);

                    //billboard to cam 
                    vec3 to_camera;
                    to_camera[0] = scene.camera.position[0] - local_matrix[3][0];
                    to_camera[1] = scene.camera.position[1] - local_matrix[3][1];
                    to_camera[2] = scene.camera.position[2] - local_matrix[3][2];
                    glm_vec3_normalize(to_camera);

                    int mode = 1;
                    switch (mode)
                    {
                    case 0:
                    {
                        break;
                    }
                    case 1:
                    {
                        vec3 crossed;
                        glm_vec3_crossn(up, to_camera, crossed);

                        mat3 local;
                        glm_vec3_copy(crossed, local[0]);
                        glm_vec3_copy(up, local[1]);
                        glm_vec3_copy(to_camera, local[2]);
                        //do this way since cglm copies incorrecly
                        mat3 matrix_3;
                        matrix_3[0][0] = local_matrix[0][0];
                        matrix_3[0][1] = local_matrix[0][1];
                        matrix_3[0][2] = local_matrix[0][2];

                        matrix_3[1][0] = local_matrix[1][0];
                        matrix_3[1][1] = local_matrix[1][1];
                        matrix_3[1][2] = local_matrix[1][2];

                        matrix_3[2][0] = local_matrix[2][0];
                        matrix_3[2][1] = local_matrix[2][1];
                        matrix_3[2][2] = local_matrix[2][2];

                        glm_mat3_mul(local, matrix_3, local);
                        glm_vec3_copy(local[0], local_matrix[0]);
                        glm_vec3_copy(local[1], local_matrix[1]);
                        glm_vec3_copy(local[2], local_matrix[2]);
                        break;
                    }
                    case 2:
                    {
                        vec3 screen_velocity;
                        glm_vec3_sub(particle->velocity, to_camera, screen_velocity);
                        glm_vec3_scale(screen_velocity, glm_vec3_dot(to_camera, particle->velocity), screen_velocity);

                        if (glm_vec3_norm(screen_velocity) == 0)
                        {
                            glm_vec3_copy(up, screen_velocity);
                        }

                        glm_vec3_normalize(screen_velocity);

                        glm_vec3_crossn(screen_velocity, to_camera, local_matrix[0]);
                        glm_vec3_scale(local_matrix[0], glm_vec3_norm(particle->xform[0]), local_matrix[0]);

                        glm_vec3_scale(screen_velocity, glm_vec3_norm(particle->xform[1]), local_matrix[1]);

                        glm_vec3_scale(to_camera, glm_vec3_norm(particle->xform[2]), local_matrix[2]);
                        break;
                    }
                    default:
                        break;
                    }

                    Process_AddParticleToQuadInstances(particle, local_matrix, texture_region, texture_index);
                }
            }
        }
        emitter->force_restart = false;

        emitter_node = emitter_node->next;
    }
}

static void Process_LCWorld(RDraw_LCWorldData* const p_lcWorldData)
{
    //Just a simple boolean to make sure we want to draw
    drawData->lc_world.draw = true;

    drawData->lc_world.world_render_data = LC_World_getRenderData();
}


static void GetLineIntersectionToZPlane(vec3 A, vec3 B, float zDist, vec3 dest)
{
    vec3 normal = { 0, 0, -1 };

    vec3 ab;
    glm_vec3_sub(B, A, ab);

    float t = (zDist - glm_vec3_dot(normal, A)) / glm_vec3_dot(normal, ab);

    dest[0] = A[0] + t * ab[0];
    dest[1] = A[1] + t * ab[1];
    dest[2] = A[2] + t * ab[2];
}
static void Process_BuildClusters()
{
    vec3 eyePos;
    glm_vec3_zero(eyePos);
    
    vec3 minMax[2];
    glm_vec3_broadcast(FLT_MAX, minMax[0]);
    glm_vec3_broadcast(-FLT_MIN, minMax[1]);

    float tileSizePx = backend_data->screenSize[0] / (float)CLUSTERS_X;
    float tileSizePy = backend_data->screenSize[1] / (float)CLUSTERS_Y;
    float scale = 24.0 / log2(500.0 / 0.1);
    float bias = 24.0 * log2(0.1) / log2(500.0 / 0.1);
    for (int x = 0; x < CLUSTERS_X; x++)
    {
        for (int y = 0; y < CLUSTERS_Y; y++)
        {
            for (int z = 0; z < CLUSTERS_Z; z++)
            {
                int flat_index = x + (y * CLUSTERS_X) +
                    (z * CLUSTERS_X * CLUSTERS_Y);

                vec3 box_screen_space[2];
                box_screen_space[0][0] = (float)x * tileSizePx;
                box_screen_space[0][1] = (float)y * tileSizePy;
                box_screen_space[0][2] = -1.0;

                box_screen_space[1][0] = (float)(x + 1) * tileSizePx;
                box_screen_space[1][1] = (float)(y + 1) * tileSizePy;
                box_screen_space[1][2] = -1.0;


                vec4 box_view_space[2];
                box_view_space[0][0] = (box_screen_space[0][0] / backend_data->screenSize[0]) * 2.0 - 1.0;
                box_view_space[0][1] = (box_screen_space[0][1] / backend_data->screenSize[1]) * 2.0 - 1.0;
                box_view_space[0][2] = -1;
                box_view_space[0][3] = 1;

                box_view_space[1][0] = (box_screen_space[1][0] / backend_data->screenSize[0]) * 2.0 - 1.0;
                box_view_space[1][1] = (box_screen_space[1][1] / backend_data->screenSize[1]) * 2.0 - 1.0;
                box_view_space[1][2] = -1;
                box_view_space[1][3] = 1;

                glm_mat4_mulv(scene.camera.invProj, box_view_space[0], box_view_space[0]);
                glm_mat4_mulv(scene.camera.invProj, box_view_space[1], box_view_space[1]);

                for (int j = 0; j < 3; j++)
                {
                    box_view_space[0][j] /= box_view_space[0][3];
                    box_view_space[1][j] /= box_view_space[1][3];
                }

                float clusterNear = scene.camera.z_near * pow(500 / max(scene.camera.z_near, 0.00001), (float)z / (float)CLUSTERS_Z);
                float clusterFar = scene.camera.z_near * pow(500 / max(scene.camera.z_near, 0.00001), (float)(z + 1) / (float)CLUSTERS_Z);

                vec3 minPointNear, minPointFar;
                vec3 maxPointNear, maxPointFar;

                GetLineIntersectionToZPlane(eyePos, box_view_space[0], clusterNear, minPointNear);
                GetLineIntersectionToZPlane(eyePos, box_view_space[0], clusterFar, minPointFar);

                GetLineIntersectionToZPlane(eyePos, box_view_space[1], clusterNear, maxPointNear);
                GetLineIntersectionToZPlane(eyePos, box_view_space[1], clusterFar, maxPointFar);

                vec3 box[2];
                for (int j = 0; j < 3; j++)
                {
                    box[0][j] = min(min(minPointNear[j], minPointFar[j]), min(maxPointNear[j], maxPointFar[j]));
                    box[1][j] = max(max(minPointNear[j], minPointFar[j]), max(maxPointNear[j], maxPointFar[j]));

                    minMax[0][j] = min(minMax[0][j], box[0][j]);
                    minMax[1][j] = max(minMax[1][j], box[1][j]);
                }

                glm_vec3_copy(box[0], scene.clusters_data.cluster_bounding_boxes[flat_index]);
                glm_vec3_copy(box[1], scene.clusters_data.cluster_bounding_boxes[flat_index]);
            
            }
        }
    }

    glm_vec3_copy(minMax[0], scene.clusters_data.min_max[0]);
    glm_vec3_copy(minMax[1], scene.clusters_data.min_max[1]);
}
static void Process_CameraUpdate()
{
    R_Camera* cam = Camera_getCurrent();

    if (!cam)
    {
        return;
    }

    //UPDATE ONLY IF DIRTY
    if (scene.dirty_cam)
    {
        //PROJ
        glm_perspective(glm_rad(cam->config.fov), backend_data->screenSize[0] / backend_data->screenSize[1], cam->config.zNear, cam->config.zFar, cam->data.proj_matrix);

        glm_mat4_copy(cam->data.proj_matrix, scene.camera.proj);

        //INV PROJ
        glm_mat4_inv(cam->data.proj_matrix, scene.camera.invProj);

        //zNEAR, zFar
        scene.camera.z_near = cam->config.zNear;
        scene.camera.z_far = cam->config.zFar;

        //Build clusters
        Process_BuildClusters();

        scene.dirty_cam = false;
    }
    //VIEW MATRIX
    vec3 center;
    glm_vec3_add(cam->data.position, cam->data.camera_front, center);

    glm_lookat(cam->data.position, center, cam->data.camera_up, cam->data.view_matrix);
    glm_mat4_copy(cam->data.view_matrix, scene.camera.view);

    //INV VIEW MATRIX
    glm_mat4_inv(cam->data.view_matrix, scene.camera.invView);

    //VIEW PROJ
    glm_mat4_mul(cam->data.proj_matrix, cam->data.view_matrix, scene.camera.viewProjectionMatrix);

    //POSITION
    glm_vec3_copy(cam->data.position, scene.camera.position);

    //FRUSTRUM PLANES
    glm_frustum_planes(scene.camera.viewProjectionMatrix, scene.camera.frustrum_planes);

    scene.camera.screen_size[0] = backend_data->screenSize[0];
    scene.camera.screen_size[1] = backend_data->screenSize[1];


    bool process_reflection_matrix = true;
    if (process_reflection_matrix)
    {
        R_Camera reflection_camera = *cam;
        reflection_camera.data.position[1] -= 2 * (reflection_camera.data.position[1] - 12);
        reflection_camera.data.pitch *= -1;
        reflection_camera.data.camera_up[1] = -1;
        Camera_updateFront(&reflection_camera);

        static const vec3 up = { 0, -1, 0 };

        vec3 center;
        glm_vec3_add(reflection_camera.data.position, reflection_camera.data.camera_front, center);
        glm_lookat(reflection_camera.data.position, center, up, pass->water.reflection_view_matrix);
        glm_mat4_mul(scene.camera.proj, pass->water.reflection_view_matrix, pass->water.reflection_projView_matrix);
    }
}

static void Process_CalcShadowSplits(vec4 dest, float p_minZ, float p_maxZ, float p_splitLambda, int splits)
{
    float cascadeSplitLambda = p_splitLambda;

    vec4 cascadeSplits;

    float nearZ = p_minZ;
    float farZ = p_maxZ;
    float clipRange = farZ - nearZ;

    float minZ = nearZ;
    float maxZ = nearZ + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;
    
    //CALCULATE SPLIT PLANES
    for (int i = 0; i < splits; i++)
    {
        float p = (i + 1) / (float)splits;
        float log = (minZ * powf(ratio, p));
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearZ) / clipRange;
        dest[i] = (nearZ + cascadeSplits[i] * clipRange);
    }
}
static double snapped(double p_value, float p_step)
{
    if (p_step != 0)
    {
        p_value = floor(p_value / p_step + 0.5) * p_step;
    }

    return p_value;
}

static void Process_CalcShadowMatrixes()
{
    R_Camera* cam = Camera_getCurrent();

    if (!cam || !r_cvars.r_useDirShadowMapping->int_value)
    {
        return;
    }
    int splits = r_cvars.r_shadowSplits->int_value;

    vec4 splits2;
    Process_CalcShadowSplits(splits2, scene.camera.z_near, 150, 0.80, splits);

    scene.scene_data.shadow_splits[0] = splits2[0];
    scene.scene_data.shadow_splits[1] = splits2[1];
    scene.scene_data.shadow_splits[2] = splits2[2];
    scene.scene_data.shadow_splits[3] = splits2[3];

    scene.scene_data.shadow_blur_factors[0] = 1.0;
    scene.scene_data.shadow_blur_factors[1] = splits2[0] / max(splits2[1], 0.0001);
    scene.scene_data.shadow_blur_factors[2] = splits2[0] / max(splits2[2], 0.0001);
    scene.scene_data.shadow_blur_factors[3] = splits2[0] / max(splits2[3], 0.0001);

    scene.scene_data.shadow_split_count = splits;

    float split_distances[5];
    split_distances[0] = cam->config.zNear;
    split_distances[1] = scene.scene_data.shadow_splits[0];
    split_distances[2] = scene.scene_data.shadow_splits[1];
    split_distances[3] = scene.scene_data.shadow_splits[2];
    split_distances[4] = scene.scene_data.shadow_splits[3];

    vec3 light_dir;
    light_dir[0] = scene.scene_data.dirLightDirection[0];
    light_dir[1] = scene.scene_data.dirLightDirection[1];
    light_dir[2] = scene.scene_data.dirLightDirection[2];
    glm_vec3_normalize(light_dir);

    vec3 negative_look_at;
    negative_look_at[0] = -light_dir[0];
    negative_look_at[1] = -light_dir[1];
    negative_look_at[2] = -light_dir[2];

    vec3 up;
    glm_vec3_zero(up);
    up[1] = 1;

    vec3 zero;
    glm_vec3_zero(zero);

    mat4 base_look_at;
    glm_lookat(zero, negative_look_at, up, base_look_at);

    float shadow_texture_size = pass->shadow.shadow_map_size;
    float aspect = backend_data->screenSize[0] / backend_data->screenSize[1];
    float max_distance = min(cam->config.zFar, scene.scene_data.shadow_splits[3]);
    float min_distance = scene.scene_data.shadow_splits[0];
    float range = max_distance - min_distance;

    float first_radius = 0.0;

    for (int i = 0; i < splits; i++)
    {
        vec4 frustrum_center;
        vec4 frustrum_corners[8];
        mat4 lightProj;

        float nearPlane = split_distances[i];
        float farPlane = split_distances[i + 1];

        //Compute projection matrix, Only update if dirty
        if (scene.dirty_cam)
        {
            glm_perspective(glm_rad(cam->config.fov), aspect, nearPlane, farPlane, pass->shadow.cascade_projections[i]);
        }
       
        //Muliply proj with cam's view matrix and then invert it
        glm_mat4_mul(pass->shadow.cascade_projections[i], cam->data.view_matrix, lightProj);
        glm_mat4_inv(lightProj, lightProj);

        //Get frustrum corners
        glm_frustum_corners(lightProj, frustrum_corners);

        //Get Frustrum center
        glm_frustum_center(frustrum_corners, frustrum_center);
        
        //calculale radius
        float radius = 0.0;

        for (int j = 0; j < 8; j++)
        {
            float dist = glm_vec4_distance(frustrum_corners[j], frustrum_center);

            radius = max(radius, dist);
        }

        radius *= shadow_texture_size / (shadow_texture_size - 2.0);

        float bias_scale = 0.0;
             
        if (i == 0)
        {
            first_radius = radius;
        }
        else
        {
            bias_scale = radius / max(first_radius, 0.0001);
        }
        
        pass->shadow.split_bias_scales[i] = bias_scale;

        float texels_per_unit = shadow_texture_size / (radius * 2.0);
        
        //Stabilaze the light center, so that the shadows won't jitter when camera is being moved
        mat4 base_look_at_copy;
        mat4 base_look_at_inv;

        //kinda slow, there are better methods, but this works well enough
        glm_mat4_copy(base_look_at, base_look_at_copy);
        glm_mat4_scale(base_look_at_copy, texels_per_unit);
        glm_mat4_inv(base_look_at_copy, base_look_at_inv);

        glm_mat4_mulv3(base_look_at_copy, frustrum_center, 1.0, frustrum_center);
        frustrum_center[0] = snapped(frustrum_center[0], texels_per_unit);
        frustrum_center[1] = snapped(frustrum_center[1], texels_per_unit);
        glm_mat4_mulv3(base_look_at_inv, frustrum_center, 1.0, frustrum_center);

        //calculate the light view matrix
        vec3 eye;
        eye[0] = frustrum_center[0] + (light_dir[0] * radius);
        eye[1] = frustrum_center[1] + (light_dir[1] * radius);
        eye[2] = frustrum_center[2] + (light_dir[2] * radius);
        
        mat4 lightView;
        glm_lookat(eye, frustrum_center, up, lightView);

        //get minZ and maxZ
        float minZ = FLT_MAX;
        float maxZ = FLT_MIN;

        for (int i = 0; i < 8; i++)
        {
            vec4 v;
            glm_mat4_mulv(lightView, frustrum_corners[i], v);

            minZ = min(minZ, v[2]);
            maxZ = min(maxZ, v[2]);
        }
      

        float zMult = 2.0f;
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

        //calculate ortho proj
        mat4 orthoProj;
        glm_ortho(-radius, radius, -radius, radius, 0.0, maxZ - minZ, orthoProj);

        //multiply light view matrix with proj
        glm_mat4_mul(orthoProj, lightView, scene.scene_data.shadow_matrixes[i]);

        scene.scene_data.shadow_bias[i] = r_cvars.r_shadowBias->float_value / 100.0 * bias_scale;
        scene.scene_data.shadow_normal_bias[i] = r_cvars.r_shadowNormalBias->float_value * (radius * 2.0 / shadow_texture_size);
    }

}

static void Process_CullRegisterHit(const void* _data, BVH_ID _index)
{
    scene.cull_data.static_cull_instance_result[scene.cull_data.static_cull_instances_count] = _data;

    scene.cull_data.static_cull_instances_count++;
}

//#define CULL_SHADOWS_PLANES
static void Process_CullScene()
{
    scene.cull_data.static_cull_instances_count = 0;
    scene.scene_data.numPointLights = 0;
    scene.scene_data.numSpotLights = 0;

    dA_clear(storage.point_lights_backbuffer);
    dA_clear(storage.spot_lights_backbuffer);

    //Cull scene
    int static_cull_count = BVH_Tree_Cull_Planes(&scene.cull_data.static_partition_tree, scene.camera.frustrum_planes, 6, 1000, Process_CullRegisterHit);

    //printf("%i \n", static_cull_count);

   // static_cull_count = 0;


    for (int i = 0; i < static_cull_count; i++)
    {
        int instance_index = scene.cull_data.static_cull_instance_result[i];

        RenderInstance* instance = dA_at(scene.render_instances_pool->pool, instance_index);

        if (instance->type == INST__POINT_LIGHT)
        {
            PointLight2* point_light = dA_at(storage.point_lights_pool->pool, instance->data_index);

            scene.scene_data.numPointLights++;

            dA_emplaceBackData(storage.point_lights_backbuffer, point_light);
        }
        else if (instance->type == INST__SPOT_LIGHT)
        {
            SpotLight* spot_light = dA_at(storage.spot_lights_pool->pool, instance->data_index);

            scene.scene_data.numSpotLights++;

            dA_emplaceBackData(storage.spot_lights_backbuffer, spot_light);
        }
    }


    //Cull lc world chunks
    if (drawData->lc_world.draw && drawData->lc_world.world_render_data)
    {
        memset(scene.cull_data.lc_world_frustrum_query_buffer, 0, sizeof(scene.cull_data.lc_world_frustrum_query_buffer));

        for (int j = 0; j < 1; j++)
        {
            scene.cull_data.lc_world_in_frustrum_count = AABB_Tree_IntersectsFrustrumPlanes(&drawData->lc_world.world_render_data->aabb_tree, scene.camera.frustrum_planes,
                MAX_CULL_QUERY_BUFFER_ITEMS, scene.cull_data.lc_world_frustrum_query_buffer, scene.cull_data.lc_world_frustrum_sorted_query_buffer, NULL);

            drawData->lc_world.world_render_data->chunks_in_frustrum_count = scene.cull_data.lc_world_in_frustrum_count;

           // scene.cull_data.lc_world_in_frustrum_count = 1000;
            //drawData->lc_world.world_render_data->chunks_in_frustrum_count = 1000;
        }
        
        //cull chunks in shadow
        if (r_cvars.r_useDirShadowMapping->int_value)
        {
            int splits = r_cvars.r_shadowSplits->int_value;

            const float SHADOW_MARGIN_MULTIPLIER = 2.0;

            bool allow_transparent_shadows = (r_cvars.r_allowTransparentShadows->int_value == 1);

            dA_clear(drawData->lc_world.shadow_sorted_chunk_indexes);

            if (allow_transparent_shadows)
            {
                dA_clear(drawData->lc_world.shadow_sorted_chunk_transparent_indexes);
            }

            mat4 ident;
            glm_mat4_identity(ident);

            for (int i = 0; i < splits; i++)
            {
                int shadow_count = 0;
#ifdef CULL_SHADOWS_PLANES
                vec4 planes[6];
                // *Exracted planes order : [left, right, bottom, top, near, far]
                glm_frustum_planes(scene.scene_data.shadow_matrixes[i], planes);

                shadow_count = AABB_Tree_IntersectsFrustrumPlanes(&drawData->lc_world.world_render_data->aabb_tree, planes,
                    5000, scene.cull_data.lc_world_frustrum_shadow_query_buffer[i], NULL, scene.cull_data.lc_world_frustrum_shadow_sorted_query_buffer[i]);
#else
                //Box culling is faster
                mat4 invMat;
                glm_mat4_inv(scene.scene_data.shadow_matrixes[i], invMat);

                vec4 frustrum_corners[8];
                glm_frustum_corners(invMat, frustrum_corners);

                vec3 box[2];
                glm_frustum_box(frustrum_corners, ident, box);
                
                if (i == 0)
                {
                    //Fatten the box a little, since shadows get cut off incorrectly in high peaks and valleys
                    box[0][0] -= LC_CHUNK_WIDTH * SHADOW_MARGIN_MULTIPLIER;
                    box[0][1] -= LC_CHUNK_HEIGHT * SHADOW_MARGIN_MULTIPLIER;
                    box[0][2] -= LC_CHUNK_LENGTH * SHADOW_MARGIN_MULTIPLIER;

                    box[1][0] += LC_CHUNK_WIDTH * SHADOW_MARGIN_MULTIPLIER;
                    box[1][1] += LC_CHUNK_HEIGHT * SHADOW_MARGIN_MULTIPLIER;
                    box[1][2] += LC_CHUNK_LENGTH * SHADOW_MARGIN_MULTIPLIER;
                }

                shadow_count = AABB_Tree_IntersectsFrustrumBox(&drawData->lc_world.world_render_data->aabb_tree, box,
                    MAX_CULL_LC_SHADOW_QUERY_BUFFER_ITEMS, NULL, NULL, scene.cull_data.lc_world_frustrum_shadow_sorted_query_buffer[i]);
#endif

                dA_clear(drawData->lc_world.shadow_firsts[i]);
                dA_clear(drawData->lc_world.shadow_counts[i]);
                drawData->lc_world.shadow_sorted_chunk_offsets[i] = dA_size(drawData->lc_world.shadow_sorted_chunk_indexes);

                if (allow_transparent_shadows)
                {
                    dA_clear(drawData->lc_world.shadow_firsts_transparent[i]);
                    dA_clear(drawData->lc_world.shadow_counts_transparent[i]);

                    drawData->lc_world.shadow_sorted_chunk_transparent_offsets[i] = dA_size(drawData->lc_world.shadow_sorted_chunk_transparent_indexes);
                }
                int total_opaque_added = 0;
                int total_transparent_added = 0;
                for (int j = 0; j < shadow_count; j++)
                {
                    LCTreeData* tree_data = scene.cull_data.lc_world_frustrum_shadow_sorted_query_buffer[i][j].data;

                    if (tree_data->opaque_index >= 0)
                    {
                        DRB_Item opaque_item = DRB_GetItem(&drawData->lc_world.world_render_data->vertex_buffer, tree_data->opaque_index);

                        int* first = dA_emplaceBack(drawData->lc_world.shadow_firsts[i]);
                        int* count = dA_emplaceBack(drawData->lc_world.shadow_counts[i]);

                        *first = opaque_item.offset / sizeof(ChunkVertex);
                        *count = opaque_item.count / sizeof(ChunkVertex);

                        int* chunk_index = dA_emplaceBack(drawData->lc_world.shadow_sorted_chunk_indexes);

                        *chunk_index = tree_data->chunk_data_index;
                        
                        total_opaque_added++;
                    }
                    if (allow_transparent_shadows)
                    {
                        if (tree_data->transparent_index >= 0)
                        {
                            DRB_Item transparent_item = DRB_GetItem(&drawData->lc_world.world_render_data->transparents_vertex_buffer, tree_data->transparent_index);

                            int* first = dA_emplaceBack(drawData->lc_world.shadow_firsts_transparent[i]);
                            int* count = dA_emplaceBack(drawData->lc_world.shadow_counts_transparent[i]);

                            *first = transparent_item.offset / sizeof(ChunkVertex);
                            *count = transparent_item.count / sizeof(ChunkVertex);

                            int* chunk_index = dA_emplaceBack(drawData->lc_world.shadow_sorted_chunk_transparent_indexes);

                            *chunk_index = tree_data->chunk_data_index;

                            total_transparent_added++;
                        }
                    }
                }

                scene.cull_data.lc_world_shadow_cull_count[i] = total_opaque_added;
                scene.cull_data.lc_world_shadow_cull_transparent_count[i] = total_transparent_added;
            }
        }
    }
  
}

#include <GLFW/glfw3.h>
static dynamic_array* test_buf;
static void Process_CullLightsForEachCluster()
{
    static bool init = false;
    if (!init)
    {
        test_buf = dA_INIT(PointLight2, 0);
        init = true;
    }
   // double start_Time = glfwGetTime();
    dA_clear(test_buf);
    const int NUM_LIGHTS = storage.point_lights_pool->pool->elements_size;
    float scale = 24.0 / log2(500.0 / 0.1);
    float tileSizePx = backend_data->screenSize[0] / (float)CLUSTERS_X;
    float tileSizePy = backend_data->screenSize[1] / (float)CLUSTERS_Y;
    vec3 scaleVec;
    scaleVec[0] = scene.cluster_items_ssbo;
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        PointLight2* light = dA_at(storage.point_lights_pool->pool, i);

        if (light->radius <= 0)
        {
           // continue;
        }
        vec4 light_vs;
        light_vs[0] = light->position[0];
        light_vs[1] = light->position[1];
        light_vs[2] = light->position[2];
        light_vs[3] = 1;

        glm_mat4_mulv(scene.camera.view, light_vs, light_vs);

        PointLight2* new_light = dA_emplaceBack(test_buf);

        memcpy(new_light, light, sizeof(PointLight2));

        new_light->position[0] = light_vs[0];
        new_light->position[1] = light_vs[1];
        new_light->position[2] = light_vs[2];
        new_light->position[3] = light->radius;
        
       // new_light->position[0] = glm_clamp(new_light->position[0], -15, 15);
       // new_light->position[1] = glm_clamp(new_light->position[1], -8, 8);
       // new_light->position[2] = glm_clamp(new_light->position[2], -31, 31);

       // glm_clamp(new_light->position[2], -24, 24);
    }
    
   

    
    memset(scene.clusters_data.lights, 0, sizeof(scene.clusters_data.lights));
    memset(scene.clusters_data.light_grid, 0, sizeof(scene.clusters_data.light_grid));
    memset(scene.clusters_data.light_indexes, 0, sizeof(scene.clusters_data.light_indexes));
    scene.clusters_data.lights_in_clusters = 0;

    int num_intersections = 0;
    int offset = 0;
    for (int x = 0; x < CLUSTERS_X; x++)
    {
        for (int y = 0; y < CLUSTERS_Y; y++)
        {
            for (int z = 0; z < CLUSTERS_Z; z++)
            {
                int flat_index = x + (y * CLUSTERS_X) +
                    (z * CLUSTERS_X * CLUSTERS_Y);

                vec3 cluster_box[2];
                glm_vec3_copy(scene.clusters_data.cluster_bounding_boxes[flat_index], cluster_box[0]);
                glm_vec3_copy(scene.clusters_data.cluster_bounding_boxes[flat_index], cluster_box[1]);

                const int NUM_LIGHTS = test_buf->elements_size;
     
                scene.clusters_data.light_grid[flat_index].offset = offset;
                assert(flat_index < CLUSTERS_X * CLUSTERS_Y * CLUSTERS_Z);

                for (int i = 0; i < NUM_LIGHTS; i++)
                {
                    PointLight2* light = dA_at(test_buf, i);

                    
                

                    if (glm_aabb_sphere(cluster_box, light->position))
                    {
                        scene.clusters_data.lights[x][y][z]++;
                                              
                       
                        scene.clusters_data.light_grid[flat_index].light_indices[scene.clusters_data.light_grid[flat_index].count] = i;

                            scene.clusters_data.light_grid[flat_index].count++;
                        scene.clusters_data.light_indexes[offset] = i;
                        offset++;
                       

                        scene.clusters_data.lights_in_clusters++;
                    }

                   
                }
            }
        }
    }
   // double end_time = glfwGetTime();

   // printf("%f \n", end_time - start_Time);

    printf("%i \n", scene.clusters_data.lights_in_clusters);

    float x = 0;
}

static void Process_CullLightsBVH()
{
    memset(scene.clusters_data.frustrum_culled_lights, 0, sizeof(scene.clusters_data.frustrum_culled_lights));
    int culled_count = AABB_Tree_IntersectsFrustrumPlanesLightsTest(&scene.clusters_data.light_tree, scene.camera.frustrum_planes, 1000, 
        scene.clusters_data.frustrum_culled_lights);

    if (culled_count > 0)
    {
        scene.clusters_data.frustrum_culled_lights[culled_count].next_index = -3;
    }
    else
    {
        scene.clusters_data.frustrum_culled_lights[culled_count].next_index = -3;
    }

    scene.clusters_data.culled_light_count = culled_count + 1;
}

void RCmds_processCommands()
{
    drawData->lc_world.draw = false;

    void* itr_ptr = cmdBuffer->cmds_data;
    for (int i = 0; i < cmdBuffer->cmds_counter; i++)
    {
        switch (cmdBuffer->cmd_enums[i])
        {
        case R_CMD__TEXTURE:
        {
            R_CMD_DrawTexture* cmd = itr_ptr;

            if (cmd->proj_type == R_CMD_PT__SCREEN)
            {
                Process_ScreenTexture(cmd->texture, cmd->texture_region, cmd->position, cmd->scale, cmd->rotation, cmd->color);
            }
            (char*)itr_ptr += sizeof(R_CMD_DrawTexture);

            break;
        }
        case R_CMD__LINE:
        {
            R_CMD_DrawLine* cmd = itr_ptr;

            if (cmd->proj_type == R_CMD_PT__3D)
            {
                Process_Line(cmd->from, cmd->to, cmd->color);
            }
            (char*)itr_ptr += sizeof(R_CMD_DrawLine);
            break;
        }
        case R_CMD__TRIANGLE:
        {
            R_CMD_DrawTriangle* cmd = itr_ptr;

            if (cmd->proj_type == R_CMD_PT__3D)
            {
                Process_Triangle(cmd->points[0], cmd->points[1], cmd->points[2], NULL, NULL, NULL, cmd->color, NULL);
            }
            (char*)itr_ptr += sizeof(R_CMD_DrawTriangle);
            break;
        }
        case R_CMD__CUBE:
        {
            R_CMD_DrawCube* cmd = itr_ptr;

            if (cmd->polygon_mode == R_CMD_PM__WIRES)
            {
                Process_CubeWires(cmd->box, cmd->color);
            }
            else if (cmd->polygon_mode == R_CMD_PM__FULL)
            {
               // Process_AABB(cmd->aabb, cmd->color);
            }
            (char*)itr_ptr += sizeof(R_CMD_DrawCube);

            break;
        }
        case R_CMD__TEXTURED_CUBE:
        {
            R_CMD_DrawTextureCube* cmd = itr_ptr;

            Process_Cube(cmd->box, cmd->color, cmd->texture, cmd->texture_region);

            (char*)itr_ptr += sizeof(R_CMD_DrawTextureCube);

            break;
        }
        case R_CMD__LCWorld:
        {
            R_CMD_DrawLCWorld* cmd = itr_ptr;

            Process_LCWorld(cmd);
            
            (char*)itr_ptr += sizeof(R_CMD_DrawLCWorld);
            break;
        }
        default:
            break;
        }
    }

    cmdBuffer->cmds_ptr = cmdBuffer->cmds_data;
    cmdBuffer->cmds_counter = 0;
    cmdBuffer->byte_count = 0;

    //Also other commands to process
    Process_ParticleSystemUpdate();
    Process_CalcShadowMatrixes();
    Process_CameraUpdate();
    Process_CullScene();
    //Process_CullLights();
   // Process_CullLightsForEachCluster();
    //Process_CullLightsBVH();
}