#ifndef R_PUBLIC_H
#define R_PUBLIC_H
#pragma once

#include "r_sprite.h"
#include "r_model.h"

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
void Draw_Triangle(vec3 p1, vec3 p2, vec3 p3, vec4 p_color);
void Draw_Model(R_Model* const p_model, vec3 p_position);
void Draw_ModelWires(R_Model* const p_model, vec3 p_position);

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

/*
~~~~~~~~~~~~~~~~~~~
LIGHTS
~~~~~~~~~~~~~~~~~~~
*/
void Light_RegisterPointLight();
void Light_RegisterSpotLight();


#endif // !R_PUBLIC_H
