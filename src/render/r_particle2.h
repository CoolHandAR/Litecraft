#pragma once

#include <cglm/cglm.h>


//Singular particle
//must mirror the GLSL struct
typedef struct
{
	mat4 xform;
	vec3 velocity;
	unsigned emitter_index;
	unsigned local_index;
	vec4 color;
} Particle;

//Particle Emitter
//must mirror the GLSL struct
typedef struct
{

} InternalParticleEmitter;

//Client size particle Emitter
typedef struct
{

} ParticleEmitter;
