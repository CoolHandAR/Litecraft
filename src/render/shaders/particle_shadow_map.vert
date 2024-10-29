#version 460 core 

#include "scene_incl.incl"
#include "particles_common.incl"

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_texCoords;

layout(std430, binding = 5) readonly restrict buffer ParticleEmittersBuffer
{
    ParticleEmitter data[];
} emitters;
layout(std430, binding = 7) restrict buffer TransformsBuffer
{
    vec4 data[];
} instances;

out VS_OUT
{
    vec2 TexCoords;
    flat int TexIndex;
} vs_out;

uniform mat4 u_matrix;

void main()
{
    vec4 custom;

    uint read_index = gl_InstanceID * (3 + 1 + 1); //xform + color + custom

    //custom 
    custom = instances.data[read_index + 4];

    uint emitter_index = uint(custom.x);
    uint particle_flags = uint(custom.y);

#define EMITTER emitters.data[emitter_index]
    
    //Not casting shadows or emitting??
    //Do vertex level culling and return
    if(!bool(EMITTER.settings_flags & EMITTER_SETTINGS_FLAG_CAST_SHADOWS) || !bool(particle_flags & PARTICLE_STATE_FLAG_EMITTING))
    {   
        vs_out.TexCoords = vec2(0, 0);
        gl_Position = vec4(-1.0 / 0.0, -1.0 / 0.0, -1.0 / 0.0, -1.0);
        return;
    }
        
    uint frame = uint(EMITTER.frame) + EMITTER.h_frames;

    vec2 frameOffset = vec2(frame % EMITTER.h_frames, frame / EMITTER.h_frames);

    vec2 coords = a_texCoords;

    coords.x += frameOffset.x;
    coords.y += -frameOffset.y;

    coords.x /= EMITTER.h_frames;
    coords.y /= EMITTER.v_frames;

    vs_out.TexCoords = vec2((coords.x), ((coords.y)));
    vs_out.TexIndex = EMITTER.texture_index;

    mat4 xform;

    //read matrix data from the buffer
    xform[0] = instances.data[read_index + 0];
    xform[1] = instances.data[read_index + 1];
    xform[2] = instances.data[read_index + 2];
    xform[3] = vec4(0.0, 0.0, 0.0, 1.0);

    xform = transpose(xform);

    
    //Extract scale and position from model matrix
    vec3 scale = vec3(length(xform[0]), length(xform[1]), length(xform[2]));
    vec3 position = xform[3].xyz;

     
     //Rebuild the matrix to ignore rotations
     xform = mat4(1.0);
     xform[0].xyz *= scale.x;
     xform[1].xyz *= scale.y;
     xform[2].xyz *= scale.y;
     xform[3].xyz = position;
   
     vec3 up = vec3(0.0, 1.0, 0.0);
     vec3 dirLightDirection = scene.dirLightDirection.xyz;
     mat3 local = mat3(normalize(cross(up, dirLightDirection)), up, dirLightDirection);
     //Rotate the matrix so that it will always face the dir light direction
     //this allows our shadows to always look flat and ignore stuff like billboarding
     local = local * mat3(xform);
     xform[0].xyz = local[0];
     xform[1].xyz = local[1];
     xform[2].xyz = local[2]; 

    vec4 worldPos = xform * vec4(a_Pos, 0.0, 1.0);


    gl_Position = u_matrix * worldPos;
}