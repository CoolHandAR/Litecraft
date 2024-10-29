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

extern R_Scene scene;
extern R_StorageBuffers storage;
extern R_CMD_Buffer* cmdBuffer;
extern R_DrawData* drawData;
extern R_RenderPassData* pass;
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

static void Process_ScreenTexture(R_Texture* p_texture, M_Rect2Df p_textureRegion, vec2 p_position, vec2 p_scale, float p_rotation, vec4 p_color)
{
    R_ScreenQuadDrawData* data = &drawData->screen_quad;

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
    R_LineDrawData* data = &drawData->lines;
    
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
    R_TriangleDrawData* data = &drawData->triangle;

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

static void Process_AABBWires(AABB box, vec4 color)
{
    vec3 b[2];
    glm_vec3_copy(box.position, b[0]);
    b[1][0] = box.position[0] + box.width;
    b[1][1] = box.position[1] + box.height;
    b[1][2] = box.position[2] + box.length;

    //perform frustrum cull
    if (!glm_aabb_frustum(b, scene.camera.frustrum_planes))
    {
        return;
    }

    //adding a small amount fixes z_fighting issues
    for (int i = 0; i < 3; i++)
    {
        box.position[i] += 0.001;
    }
    Process_Line2(box.position[0], box.position[1], box.position[2], box.position[0] + box.width, box.position[1], box.position[2], color);
    Process_Line2(box.position[0], box.position[1] + box.height, box.position[2], box.position[0] + box.width, box.position[1] + box.height, box.position[2], color);
    Process_Line2(box.position[0], box.position[1] + box.height, box.position[2], box.position[0], box.position[1], box.position[2], color);
    Process_Line2(box.position[0] + box.width, box.position[1], box.position[2], box.position[0] + box.width, box.position[1] + box.height, box.position[2], color);
    Process_Line2(box.position[0], box.position[1], box.position[2], box.position[0], box.position[1], box.position[2] + box.length, color);
    Process_Line2(box.position[0], box.position[1], box.position[2] + box.length, box.position[0] + box.width, box.position[1], box.position[2] + box.length, color);
    Process_Line2(box.position[0] + box.width, box.position[1], box.position[2] + box.length, box.position[0] + box.width, box.position[1], box.position[2], color);
    Process_Line2(box.position[0], box.position[1], box.position[2] + box.length, box.position[0], box.position[1] + box.height, box.position[2] + box.length, color);
    Process_Line2(box.position[0], box.position[1] + box.height, box.position[2] + box.length, box.position[0] + box.width, box.position[1] + box.height, box.position[2] + box.length, color);
    Process_Line2(box.position[0], box.position[1] + box.height, box.position[2], box.position[0], box.position[1] + box.height, box.position[2] + box.length, color);
    Process_Line2(box.position[0] + box.width, box.position[1] + box.height, box.position[2], box.position[0] + box.width, box.position[1] + box.height, box.position[2] + box.length, color);
    Process_Line2(box.position[0] + box.width, box.position[1], box.position[2] + box.length, box.position[0] + box.width, box.position[1] + box.height, box.position[2] + box.length, color);
}

static void Process_Cube(vec3 box[2], vec4 color, R_Texture* p_tex, M_Rect2Df p_textureRegion)
{
    vec3 CUBE_POSITION_VERTICES[] =
    {
         -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f, 
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f, 

        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 

         0.5f,  0.5f,  0.5f, 
         0.5f,  0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 

        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
    };


    vec2 CUBE_TEX_COORDS[] =
    {
          0.0f,  0.0f,
          1.0f,  0.0f,
          1.0f,  1.0f,
          1.0f,  1.0f,
          0.0f,  1.0f,
          0.0f,  0.0f,

          0.0f,  0.0f,
          1.0f,  0.0f,
          1.0f,  1.0f,
          1.0f,  1.0f,
          0.0f,  1.0f,
          0.0f,  0.0f,

          0.0f,  0.0f,
          1.0f,  0.0f,
          1.0f,  1.0f,
          1.0f,  1.0f,
          0.0f,  1.0f,
          0.0f,  0.0f,

          1.0f,  0.0f,
          1.0f,  1.0f,
          0.0f,  1.0f,
          0.0f,  1.0f,
          0.0f,  0.0f,
          1.0f,  0.0f,

          0.0f,  1.0f,
          1.0f,  1.0f,
          1.0f,  0.0f,
          1.0f,  0.0f,
          0.0f,  0.0f,
          0.0f,  1.0f,

          0.0f,  1.0f,
          1.0f,  1.0f,
          1.0f,  0.0f,
          1.0f,  0.0f,
          0.0f,  0.0f,
          0.0f,  1.0f
    };


    mat4 model;
    vec3 size;
    size[0] = box[1][0] - box[0][0];
    size[1] = box[1][1] - box[0][1];
    size[2] = box[1][2] - box[0][2];
    Math_Model(box[0], size, 0, model);


    const float left = p_textureRegion.x;
    const float right = p_textureRegion.x + p_textureRegion.width;
    const float top = p_textureRegion.y;
    const float bottom = p_textureRegion.y + p_textureRegion.height;

    const int texture_width = p_tex->width;
    const int texture_height = p_tex->height;

    vec2 coords1, coords2, coords3, coords4;

    coords1[0] = right / texture_width;
    coords1[1] = bottom / texture_height;

    coords2[0] = right / texture_width;
    coords2[1] = top / texture_height;

    coords3[0] = left / texture_width;
    coords3[1] = top / texture_height;

    coords4[0] = left / texture_width;
    coords4[1] = bottom / texture_height;

    bool prev_itr = false;
    for (int i = 0; i < 36; i += 3)
    {
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i], 1.0f, CUBE_POSITION_VERTICES[i]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 1], 1.0f, CUBE_POSITION_VERTICES[i + 1]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 2], 1.0f, CUBE_POSITION_VERTICES[i + 2]);
        

        if (prev_itr)
        {
            Process_Triangle(CUBE_POSITION_VERTICES[i], CUBE_POSITION_VERTICES[i + 1], CUBE_POSITION_VERTICES[i + 2], coords3, coords2,
                coords1, color, p_tex);
            prev_itr = false;
        }
        else
        {
            Process_Triangle(CUBE_POSITION_VERTICES[i], CUBE_POSITION_VERTICES[i + 1], CUBE_POSITION_VERTICES[i + 2], coords1, coords2,
                coords3, color, p_tex);
            prev_itr = true;
        }
        
        

        //Process_Triangle(CUBE_POSITION_VERTICES[i], CUBE_POSITION_VERTICES[i + 1], CUBE_POSITION_VERTICES[i + 2], coords1, coords2,
         //   coords3, color, p_tex);

      
    }
}

static void Process_AABB(AABB box, vec4 color)
{
    vec3 b[2];
    b[0][0] = box.position[0];
    b[0][1] = box.position[1];
    b[0][2] = box.position[2];

    b[1][0] = box.position[0] + box.width;
    b[1][1] = box.position[1] + box.height;
    b[1][2] = box.position[2] + box.length;

    //perform frustrum cull
    //if (!glm_aabb_frustum(b, Camera_getCurrent()->data.frustrum_planes))
     //   return;

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
    vec3 size;
    size[0] = box.width;
    size[1] = box.height;
    size[2] = box.length;
    Math_Model(b[0], size, 0, model);

    for (int i = 0; i < 36; i += 3)
    {
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i], 1.0f, CUBE_POSITION_VERTICES[i]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 1], 1.0f, CUBE_POSITION_VERTICES[i + 1]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 2], 1.0f, CUBE_POSITION_VERTICES[i + 2]);

        Process_Triangle(CUBE_POSITION_VERTICES[i], CUBE_POSITION_VERTICES[i + 1], CUBE_POSITION_VERTICES[i + 2], NULL, NULL, NULL, color, NULL);
    }
}
//#define USE_BINDLESS_TEXTURES
static void Process_ParticleUpdates()
{
    for (int i = 0; i < storage.particle_update_index; i++)
    {
        ParticleEmitterSettings* emitter = storage.particle_update_queue[i];
         
        //Particle update
        if (emitter->settings.particle_amount > 0)
        {
            DRB_Item particle_item = DRB_GetItem(&storage.particles, emitter->_particle_drb_index);

            int num_allocated_particles = particle_item.count / sizeof(Particle);

            //Only realloc if particles need to be updated
            if (num_allocated_particles != emitter->settings.particle_amount)
            {
                Particle* particles_buf = malloc(sizeof(Particle) * emitter->settings.particle_amount);

                if (particles_buf)
                {
                    memset(particles_buf, 0, sizeof(Particle) * emitter->settings.particle_amount);

                    for (int j = 0; j < emitter->settings.particle_amount; j++)
                    {
                        Particle* p = &particles_buf[j];

                        p->local_index = j;
                        p->emitter_index = emitter->_gl_emitter_index;
                    }

                    DRB_ChangeData(&storage.particles, sizeof(Particle) * emitter->settings.particle_amount, particles_buf, emitter->_particle_drb_index);

                    free(particles_buf);
                }
            }
        }

        if (emitter->texture)
        {
#ifdef USE_BINDLESS_TEXTURES
            uint64_t texture_handle = glGetTextureHandleARB(emitter->texture->id);

            if (!glIsTextureHandleResidentARB(texture_handle))
            {
                glMakeTextureHandleResidentARB(texture_handle);
            }
#else
            int texture_index = 0;
            for (int k = 0; k < 32; k++)
            {
                if (pass->particles.texture_ids[k] == emitter->texture->id)
                {
                    texture_index = k;
                    break;
                }
                else if (pass->particles.texture_ids[k] == 0)
                {
                    texture_index = k;
                    break;
                }
            }

            pass->particles.texture_ids[texture_index] = emitter->texture->id;
            emitter->settings.texture_index = texture_index;
#endif   
        }

        glNamedBufferSubData(storage.particle_emitters.buffer, emitter->_gl_emitter_index * sizeof(ParticleEmitterGL), sizeof(ParticleEmitterGL), &emitter->settings);
    }
    
    if (storage.particle_update_index > 0)
    {
        FL_Head* h = storage.particle_emitter_clients;
        FL_Node* node = h->next;

        pass->particles.total_particle_amount = 0;
        pass->particles.total_emitter_amount = 0;

        while (node)
        {
            ParticleEmitterSettings* pe = node->value;

            pass->particles.total_particle_amount += pe->settings.particle_amount;
            pass->particles.total_emitter_amount++;

            node = node->next;
        }

        //5: PARTICLE EMITTERS
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, storage.particle_emitters.buffer);

        //6: PARTICLES
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, storage.particles.buffer);

        //7: INSTANCE DATA
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, storage.instances.buffer);

        //8: COLLISION DATA (USED FOR PARTICLE COLLISIONS)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, storage.collider_boxes.buffer);
    }
   

    if (storage.instances.reserve_size <= pass->particles.total_particle_amount * (3 + 1 + 1))
    {
        RSB_IncreaseSize(&storage.instances, pass->particles.total_particle_amount * (3 + 1 + 1) - storage.instances.reserve_size);
    }

    storage.particle_update_index = 0;
 
}

static void Process_LCWorld(R_LCWorldDrawData* const p_lcWorldData)
{
    //Just a simple boolean to make sure we want to draw
    drawData->lc_world.draw = true;

    drawData->lc_world.world_render_data = LC_World_getRenderData();
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

        //zNEAR, zFat
        scene.camera.z_near = cam->config.zNear;
        scene.camera.z_far = cam->config.zFar;

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
}

static void Process_CalcShadowSplits(vec4 dest, float p_minZ, float p_maxZ, float p_splitLambda)
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
    for (int i = 0; i < 4; i++)
    {
        float p = (i + 1) / 4.0;
        float log = (minZ * powf(ratio, p));
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearZ) / clipRange;
        dest[i] = (nearZ + cascadeSplits[i] * clipRange);
    }
}

static void Process_CalcShadowMatrixes()
{
    R_Camera* cam = Camera_getCurrent();

    if (!cam || !r_cvars.r_useDirShadowMapping->int_value)
    {
        return;
    }

    vec4 splits2;
    Process_CalcShadowSplits(splits2, scene.camera.z_near, 150, 0.80);

  //  scene.scene_data.shadow_splits[0] = pass->shadow.cascade_levels[0];
  //  scene.scene_data.shadow_splits[1] = pass->shadow.cascade_levels[1];
  //  scene.scene_data.shadow_splits[2] = pass->shadow.cascade_levels[2];
  //  scene.scene_data.shadow_splits[3] = pass->shadow.cascade_levels[3];

    scene.scene_data.shadow_splits[0] = splits2[0];
    scene.scene_data.shadow_splits[1] = splits2[1];
    scene.scene_data.shadow_splits[2] = splits2[2];
    scene.scene_data.shadow_splits[3] = splits2[3];
    

    float split_distances[5];
    split_distances[0] = cam->config.zNear;
    split_distances[1] = scene.scene_data.shadow_splits[0];
    split_distances[2] = scene.scene_data.shadow_splits[1];
    split_distances[3] = scene.scene_data.shadow_splits[2];
    split_distances[4] = scene.scene_data.shadow_splits[3];

    vec3 negative_look_at;
    negative_look_at[0] = -scene.scene_data.dirLightDirection[0];
    negative_look_at[1] = -scene.scene_data.dirLightDirection[1];
    negative_look_at[2] = -scene.scene_data.dirLightDirection[2];

    vec3 up;
    glm_vec3_zero(up);
    up[1] = 1;

    vec3 zero;
    glm_vec3_zero(zero);

    mat4 base_look_at;
    glm_lookat(zero, negative_look_at, up, base_look_at);

    float shadow_texture_size = SHADOW_MAP_SIZE;
    int splits = 4;

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
        frustrum_center[0] = floorf(frustrum_center[0]);
        frustrum_center[1] = floorf(frustrum_center[1]);
        glm_mat4_mulv3(base_look_at_inv, frustrum_center, 1.0, frustrum_center);


        //calculate the light view matrix
        vec3 eye;
        eye[0] = frustrum_center[0] + (scene.scene_data.dirLightDirection[0] * radius);
        eye[1] = frustrum_center[1] + (scene.scene_data.dirLightDirection[1] * radius);
        eye[2] = frustrum_center[2] + (scene.scene_data.dirLightDirection[2] * radius);
        
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
    }

}

//#define CULL_SHADOWS_PLANES
static void Process_CullScene()
{
    //Cull lc world chunks
    if (drawData->lc_world.draw && drawData->lc_world.world_render_data)
    {
        memset(scene.cull_data.lc_world_frustrum_query_buffer, 0, sizeof(scene.cull_data.lc_world_frustrum_query_buffer));

        for (int j = 0; j < 1; j++)
        {
            scene.cull_data.lc_world_in_frustrum_count = AABB_Tree_IntersectsFrustrumPlanes(&drawData->lc_world.world_render_data->aabb_tree, scene.camera.frustrum_planes,
                MAX_CULL_QUERY_BUFFER_ITEMS, scene.cull_data.lc_world_frustrum_query_buffer, scene.cull_data.lc_world_frustrum_sorted_query_buffer);

            drawData->lc_world.world_render_data->chunks_in_frustrum_count = scene.cull_data.lc_world_in_frustrum_count;
        }
    
        if (r_cvars.r_useDirShadowMapping->int_value)
        {
            bool allow_transparent_shadows = (r_cvars.r_allowTransparentShadows->int_value == 1);

            dA_clear(drawData->lc_world.shadow_sorted_chunk_indexes);

            if (allow_transparent_shadows)
            {
                dA_clear(drawData->lc_world.shadow_sorted_chunk_transparent_indexes);
            }

            mat4 ident;
            glm_mat4_identity(ident);

            for (int i = 0; i < 4; i++)
            {
                memset(scene.cull_data.lc_world_frustrum_shadow_query_buffer[i], 0, sizeof(scene.cull_data.lc_world_frustrum_shadow_query_buffer[i]));

                int shadow_count = 0;
#ifdef CULL_SHADOWS_PLANES
                vec4 planes[6];
                // *Exracted planes order : [left, right, bottom, top, near, far]
                glm_frustum_planes(scene.scene_data.shadow_matrixes[i], planes);

                shadow_count = AABB_Tree_IntersectsFrustrumPlanes(&drawData->lc_world.world_render_data->aabb_tree, planes,
                    5000, scene.cull_data.lc_world_frustrum_shadow_query_buffer[i], NULL);
#else
                //Box culling is faster
                mat4 invMat;
                glm_mat4_inv(scene.scene_data.shadow_matrixes[i], invMat);

                vec4 frustrum_corners[8];
                glm_frustum_corners(invMat, frustrum_corners);

                vec3 box[2];
                glm_frustum_box(frustrum_corners, ident, box);

                shadow_count = AABB_Tree_IntersectsFrustrumBox(&drawData->lc_world.world_render_data->aabb_tree, box,
                    7000, scene.cull_data.lc_world_frustrum_shadow_query_buffer[i], NULL, scene.cull_data.lc_world_frustrum_shadow_sorted_query_buffer[i]);
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
        case R_CMD__AABB:
        {
            R_CMD_DrawAABB* cmd = itr_ptr;

            if (cmd->polygon_mode == R_CMD_PM__WIRES)
            {
                Process_AABBWires(cmd->aabb, cmd->color);
            }
            else if (cmd->polygon_mode == R_CMD_PM__FULL)
            {
                Process_AABB(cmd->aabb, cmd->color);
            }
            (char*)itr_ptr += sizeof(R_CMD_DrawAABB);

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
    Process_ParticleUpdates();
    Process_CalcShadowMatrixes();
    Process_CameraUpdate();
    Process_CullScene();
   
}