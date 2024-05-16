#pragma once

#include <cglm/cglm.h>

#include "r_texture.h"

typedef struct R_Particle
{
	mat4 xform;
	vec3 velocity;
	vec4 color;
	float life_time;
	unsigned flags;
	unsigned emitter_index;
} R_Particle;

typedef enum R_ParticleEmissionShape
{
	PES__POINT,
	PES__BOX,
	PES__CIRCLE,
	PES__DISK,
	PES__MAX_EMSHAPE
} R_ParticleEmissionShape;

typedef struct R_ParticleEmitter
{
	R_Texture texture; 
	
	vec3 direction; //direction the particles will go after they are emitted
	mat4 xform;

	bool one_shot; //if one shot the particles will not repeat after their lifetime is over

	int amount; //the amount of particles per emission

	float spread; 
	float speed_scale;
	
	float gap_between_particles;
	
	vec4 color;
} R_ParticleEmitter;