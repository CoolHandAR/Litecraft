#ifndef R_PUBLIC_H
#define R_PUBLIC_H
#pragma once

#include "r_sprite.h"
#include "r_model.h"

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

    float linear;
    float quadratic;
    float radius;
    float constant;
} PointLight2;
typedef struct
{
    vec3 position;
    vec3 direction;

    vec3 color;
    float ambient_intensity;
    float specular_intensity;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
} SpotLight;


/*
~~~~~~~~~~~~~~~~~~~
DRAW COMMANDS
~~~~~~~~~~~~~~~~~~~
*/
void Draw_ScreenSprite(R_Sprite* const p_sprite);
void Draw_3DSprite(R_Sprite* const p_sprite);
void Draw_BillboardSprite(R_Sprite* const p_sprite);
void Draw_AABBWires1(AABB p_aabb, vec4 p_color);
void Draw_AABB(AABB p_aabb, vec4 p_fillColor);
void Draw_Line(vec3 p_from, vec3 p_to, vec4 p_color);
void Draw_Line2(float x1, float y1, float z1, float x2, float y2, float z2);
void Draw_Triangle(vec3 p1, vec3 p2, vec3 p3, vec4 p_color);
void Draw_Model(R_Model* const p_model, vec3 p_position);
void Draw_ModelWires(R_Model* const p_model, vec3 p_position);
void Draw_LCWorld();

/*
~~~~~~~~~~~~~~~~~~~
SCENE
~~~~~~~~~~~~~~~~~~~
*/
void RScene_SetDirLight(DirLight p_dirLight);
LightID RScene_RegisterPointLight(PointLight2 p_pointLight);
LightID RScene_RegisterSpotLight(SpotLight p_spotLight);
void RSCene_DeletePointLight(LightID p_lightID);
void RSCene_DeleteSpotLight(LightID p_lightID);
void RScene_SetSkyboxTexturePanorama(const char* p_path);
void RScene_SetSkyboxTextureCubemap(Cubemap_Faces_Paths p_cubemapPaths);
ModelID RScene_RegisterModel(R_Model* p_model);

/*
~~~~~~~~~~~~~~~~~~~
PARTICLES
~~~~~~~~~~~~~~~~~~~
*/
void Particle_Register();
void Particle_Remove();
void Particle_Emit();
void Particle_Pause();
void Particle_Stop();


#endif // !R_PUBLIC_H
