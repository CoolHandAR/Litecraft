#ifndef R_PUBLIC_H
#define R_PUBLIC_H
#pragma once

#include "utility/dynamic_array.h"
#include "utility/u_math.h"
#include "render/r_texture.h"

typedef int RenderInstanceID;
typedef unsigned LightID;
typedef unsigned ModelID;
typedef unsigned MeshID;

/*
~~~~~~~~~~~~~~~~~~~
LIGHTS
~~~~~~~~~~~~~~~~~~~
*/

typedef struct
{
    vec3 direction;

    vec3 color;
    float ambient_intensity;
    float specular_intensity;
} DirLight;

typedef struct
{
    vec4 position;

    vec4 color;
    float ambient_intensity;
    float specular_intensity;

    float radius;
    float attenuation;
} PointLight;
typedef struct
{
    vec4 position;
    vec4 direction;

    vec4 color;
    float ambient_intensity;
    float specular_intensity;

    float range;
    float attenuation;
    float angle;
    float angle_attenuation;
} SpotLight;


/*
~~~~~~~~~~~~~~~~~~~
Materials
~~~~~~~~~~~~~~~~~~~
*/
typedef enum
{
    RM__TRANSPARENT,
    RM__USE_ALPHA_DISCARD,

} RM_Flags;

typedef struct
{
    bool flags;
} RenderMaterial;

/*
~~~~~~~~~~~~~~~~~~~
DRAW COMMANDS
~~~~~~~~~~~~~~~~~~~
*/
void Draw_ScreenTexture(R_Texture* p_texture, M_Rect2Df* p_textureRegion, float p_x, float p_y, float p_xScale, float p_yScale, float p_rotation);
void Draw_ScreenTextureColored(R_Texture* p_texture, M_Rect2Df* p_textureRegion, float p_x, float p_y, float p_xScale, float p_yScale, float p_rotation, float p_r, float p_g, float p_b, float p_a);
void Draw_ScreenTexture2(R_Texture* p_texture, M_Rect2Df* p_textureRegion, vec2 p_position);
void Draw_CubeWires(vec3 box[2], vec4 p_color);
void Draw_TexturedCube(vec3 p_box[2], R_Texture* p_tex, M_Rect2Df* p_texRegion);
void Draw_TexturedCubeColored(vec3 p_box[2], R_Texture* p_tex, M_Rect2Df* p_texRegion, float p_r, float p_g, float p_b, float p_a);
void Draw_TexturedQuad(vec3 p_minMax[2], R_Texture* p_tex, M_Rect2Df* p_texRegion);
void Draw_Line(vec3 p_from, vec3 p_to, vec4 p_color);
void Draw_Line2(float x1, float y1, float z1, float x2, float y2, float z2);
void Draw_Triangle(vec3 p1, vec3 p2, vec3 p3, vec4 p_color);
//void Draw_Model(R_Model* const p_model, vec3 p_position);
//void Draw_ModelWires(R_Model* const p_model, vec3 p_position);
void Draw_LCWorld();

/*
~~~~~~~~~~~~~~~~~~~
SCENE
~~~~~~~~~~~~~~~~~~~
*/

typedef struct
{
    float depthFogDensity;
    float heightFogDensity;

    float depthFogCurve;
    float depthFogBegin;
    float depthFogEnd;

    float heightFogCurve;
    float heightFogMin;
    float heightFogMax;

    vec3 fog_color;

    bool depth_fog_enabled;
    bool height_fog_enabled;
} FogSettings;

void RScene_SetDirLight(DirLight p_dirLight);
void RScene_SetDirLightDirection(vec3 dir);
void RScene_SetFog(FogSettings p_fog_settings);
RenderInstanceID RScene_RegisterPointLight(PointLight p_pointLight, bool p_dynamic);
RenderInstanceID RScene_RegisterSpotLight(SpotLight p_spotLight, bool p_dynamic);

void* RScene_GetRenderInstanceData(RenderInstanceID p_id);
void RScene_DeleteRenderInstance(RenderInstanceID p_id);
void RScene_SetRenderInstancePosition(RenderInstanceID p_id, vec3 position);

void RScene_SetSkyboxTexturePanorama(R_Texture* p_tex);
void RScene_SetSkyboxTextureSingleImage(const char* p_path);
void RScene_SetSkyboxTextureCubemap(Cubemap_Faces_Paths p_cubemapPaths);
void RScene_SetAmbientLightInfluence(float p_ratio);
void RScene_SetSkyColor(vec3 color);
void RScene_SetSkyHorizonColor(vec3 color);
void RScene_SetGroundHorizonColor(vec3 color);
void RScene_SetGroundColor(vec3 color);
void RScene_ForceUpdateIBL();
void RScene_SetNightTexture(R_Texture* p_tex);


/*
~~~~~~~~~~~~~~~~~~~
PARTICLES
~~~~~~~~~~~~~~~~~~~
*/
typedef enum
{
    EMITTER_STATE_FLAG__NONE = 0,
    EMITTER_STATE_FLAG__SKIP_DRAW = 1 << 0,
    EMITTER_STATE_FLAG__EMITTING = 1 << 1,
    EMITTER_STATE_FLAG__CLEAR = 1 << 2,
    EMITTER_STATE_FLAG__FORCE_RESTART = 1 << 3,
    EMITTER_STATE_FLAG__ANIMATION_PLAYING = 1 << 4,
} EmitterStateFlags;

typedef enum
{
    EMITTER_SETTINGS_FLAG__NONE = 0,
    EMITTER_SETTINGS_FLAG__ONE_SHOT = 1 << 0,
    EMITTER_SETTINGS_FLAG__LOOP_ANIMATION = 1 << 1,
    EMITTER_SETTINGS_FLAG__CAST_SHADOWS = 1 << 2,
    EMITTER_SETTINGS_FLAG__ANIMATION = 1 << 3,
} EmitterSettingsFlags;

typedef enum
{
    EES__POINT,
    EES__BOX,
    EES__SPHERE,
    EES__MAX_EMSHAPE
} EmitterEmissionShape;

typedef struct
{
    mat4 xform;
    vec3 velocity;
    unsigned prev_visible;
    unsigned local_index;
    unsigned emitter_index;
    vec4 color;
} Particle;
typedef struct
{
    mat4 xform;
    vec3 velocity;
    vec4 color;

    float time;

    bool active;
} ParticleCpu;
typedef struct
{
    mat4 xform;

    vec4 aabb[2];
    vec4 direction;

    vec4 color;
    vec4 end_color;

    vec4 emission_size;

    unsigned state_flags;
    unsigned settings_flags;

    float delta;
    float time;
    float prev_time;
    float system_time;

    float explosiveness;
    float randomness;
    float life_time;
    float speed_scale;

    float initial_velocity;
    float anim_speed_scale;

    float anim_frame_progress;

    float spread;

    float gravity;

    float friction;

    float linear_accel;

    float scale;

    float ambient_intensity;
    float diffuse_intensity;
    float specular_intensity;

    int texture_index;

    EmitterEmissionShape emission_shape;

    unsigned particle_amount;
    unsigned cycle;

    unsigned frame;
    unsigned frame_offset;
    unsigned frame_count;

    unsigned h_frames;
    unsigned v_frames;
} ParticleEmitterGL;

typedef void (*ParticleCollision_fun)(ParticleCpu* const particle, struct ParticleEmitterSettings* const emitter, float local_delta);
typedef struct
{
    mat4 xform;

    vec3 aabb[2];
    vec3 direction;

    vec4 color;
    vec4 end_color;

    vec4 emission_size;

    float _delta;
    float _time;
    float _prev_time;
    float _system_time;
    float _anim_frame_progress;
    int _cycle;

    float explosiveness;
    float randomness;
    float life_time;
    float speed_scale;

    float initial_velocity;
    float anim_speed_scale;

    float spread;
    float flatness;

    float gravity;

    float friction;

    float linear_accel;

    float scale;

    unsigned emission_shape;

    int particle_amount;

    int frame;
    int frame_offset;
    int frame_count;

    int h_frames;
    int v_frames;

    bool one_shot;
    bool emitting;

    bool force_restart;
    bool animation_enabled;


    bool _queue_update;

    ParticleCollision_fun collision_function;

    R_Texture* texture;
    dynamic_array* particles;
} ParticleEmitterSettings;


ParticleEmitterSettings* Particle_RegisterEmitter();
void Particle_RemoveEmitter(ParticleEmitterSettings* p_emitter);
void Particle_MarkUpdate(ParticleEmitterSettings* p_emitter);
void Particle_Emit(ParticleEmitterSettings* p_emitter);
void Particle_EmitTransformed(ParticleEmitterSettings* p_emitter, vec3 direction, vec3 origin);
void Particle_Pause();
void Particle_Stop();


#endif // !R_PUBLIC_H
