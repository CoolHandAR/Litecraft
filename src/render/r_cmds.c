/*
	Process all renderer commands
*/

#include "r_core.h"

extern R_CMD_Buffer* cmdBuffer;
extern R_DrawData* drawData;

static void Process_ScreenSprite(R_Sprite* p_sprite)
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
    for (int i = 1; i < SCREEN_QUAD_MAX_TEXTURES_IN_ARRAY; i++)
    {
        if (data->tex_array[i] == 0)
            break;

        //exists already?
        if (p_sprite->texture->id == data->tex_array[i])
        {
            texture_slot_index = i;
            break;
        }
    }
    //not found? then add to the array
     if (texture_slot_index == -1)
     {
         data->tex_array[data->tex_index] = p_sprite->texture->id;
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

    ScreenVertex* vertices_buffer = data->vertices;

    for (int i = 0; i < 4; i++)
    {
        glm_mat4_mulv3(p_sprite->model_matrix, vertices[i], 1.0f, vertices[i]);

        vertices_buffer[data->vertices_count + i].position[0] = vertices[i][0];
        vertices_buffer[data->vertices_count + i].position[1] = vertices[i][1];
        vertices_buffer[data->vertices_count + i].position[2] = p_sprite->position[2]; //z_index aka draw order layer index

        vertices_buffer[data->vertices_count + i].tex_coords[0] = p_sprite->texture_coords[i][0];
        vertices_buffer[data->vertices_count + i].tex_coords[1] = p_sprite->texture_coords[i][1];

        vertices_buffer[data->vertices_count + i].tex_index = texture_slot_index;
    }
    data->vertices_count += 4;
    data->indices_count += 6;
};

static void Process_Line(vec3 from, vec3 to, vec4 color)
{
    R_LineDrawData* data = &drawData->lines;
    
    if (data->vertices_count + 2 >= LINE_VERTICES_BUFFER_SIZE)
        return;

    //start point
    glm_vec3_copy(from, data->vertices[data->vertices_count].position);
    glm_vec4_copy(color, data->vertices[data->vertices_count].color);

    data->vertices_count++;

    //end point
    glm_vec3_copy(to, data->vertices[data->vertices_count].position);
    glm_vec4_copy(color, data->vertices[data->vertices_count].color);
  
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

static void Process_Triangle(vec3 p1, vec3 p2, vec3 p3, vec4 color)
{
    R_TriangleDrawData* data = &drawData->triangle;

    if (data->vertices_count + 3 >= TRIANGLE_VERTICES_BUFFER_SIZE)
        return;

    //point 1
    glm_vec3_copy(p1, data->vertices[data->vertices_count].position);
    glm_vec4_copy(color, data->vertices[data->vertices_count].color);
  
    data->vertices_count++;

    //point 2
    glm_vec3_copy(p1, data->vertices[data->vertices_count].position);
    glm_vec4_copy(color, data->vertices[data->vertices_count].color);
    
    data->vertices_count++;

    //point 3
    glm_vec3_copy(p1, data->vertices[data->vertices_count].position);
    glm_vec4_copy(color, data->vertices[data->vertices_count].color);

    data->vertices_count++;
}

static void Process_Triangle2(float p1_x, float p1_y, float p1_z, float p2_x, float p2_y, float p2_z, float p3_x, float p3_y, float p3_z, vec4 color)
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

    Process_Triangle(p1, p2, p3, color);
}

static void Process_AABBWires(AABB box, vec4 color)
{
    //perform frustrum cull
    vec3 b[2];
    glm_vec3_copy(box.position, b[0]);
    b[1][0] = box.position[0] + box.width;
    b[1][1] = box.position[1] + box.height;
    b[1][2] = box.position[2] + box.length;

    if(!glm_aabb_frustum(b, Camera_getCurrent()->data.frustrum_planes))
        return;

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

static void Process_AABB(AABB box, vec4 color)
{
    //perform frustrum cull
    vec3 b[2];
    glm_vec3_copy(box.position, b[0]);
    b[1][0] = box.position[0] + box.width;
    b[1][1] = box.position[1] + box.height;
    b[1][2] = box.position[2] + box.length;

    if (!glm_aabb_frustum(b, Camera_getCurrent()->data.frustrum_planes))
        return;

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
    Math_Model(box.position, box.position, 0, model);

    for (int i = 0; i < 36; i += 3)
    {
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i], 1.0f, CUBE_POSITION_VERTICES[i]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 1], 1.0f, CUBE_POSITION_VERTICES[i + 1]);
        glm_mat4_mulv3(model, CUBE_POSITION_VERTICES[i + 2], 1.0f, CUBE_POSITION_VERTICES[i + 2]);

        Process_Triangle(CUBE_POSITION_VERTICES[i], CUBE_POSITION_VERTICES[i + 1], CUBE_POSITION_VERTICES[i + 2], color);
    }
}


static void _uploadDataToGpu()
{
    //Upload data to gpu using a persistent buffer map. The ptr is always persistent, so we can upload safely from render thread if needed
	if (drawData->screen_quad.vertices_count > 0)
	{
        memcpy(drawData->screen_quad.buffer_ptr, drawData->screen_quad.vertices, sizeof(ScreenVertex) * drawData->screen_quad.vertices_count);
	}
	if (drawData->lines.vertices_count > 0)
	{
        memcpy(drawData->lines.buffer_ptr, drawData->lines.vertices, sizeof(BasicVertex) * drawData->lines.vertices_count);
	}
    if (drawData->triangle.vertices_count > 0)
    {
        memcpy(drawData->triangle.buffer_ptr, drawData->triangle.vertices, sizeof(BasicVertex) * drawData->triangle.vertices_count);
    }

}


void r_processCommands1()
{
    void* itr_ptr = cmdBuffer->cmds_data;
    for (int i = 0; i < cmdBuffer->cmds_counter; i++)
    {
        switch (cmdBuffer->cmd_enums[i])
        {
        case R_CMD__SPRITE:
        {
            R_CMD_DrawSprite* cmd = itr_ptr;

            if (cmd->proj_type == R_CMD_PT__SCREEN)
            {
                Process_ScreenSprite(cmd->sprite_ptr);
            }
            (char*)itr_ptr += sizeof(R_CMD_DrawSprite);

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
                Process_Triangle(cmd->points[0], cmd->points[1], cmd->points[2], cmd->color);
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
        default:
            break;
        }
    }

    cmdBuffer->cmds_ptr = cmdBuffer->cmds_data;
    cmdBuffer->cmds_counter = 0;
    cmdBuffer->byte_count = 0;

	_uploadDataToGpu();
}