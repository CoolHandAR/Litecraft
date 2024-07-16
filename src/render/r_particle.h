#pragma once

#include <cglm/cglm.h>

#include "r_texture.h"

typedef struct R_Particle
{
	mat4 xform;
	vec3 velocity;
	unsigned emitter_index;
	unsigned local_index;
	vec4 color;
} R_Particle;

typedef enum R_ParticleEmissionShape
{
	PES__POINT,
	PES__BOX,
	PES__SPHERE,
	PES__MAX_EMSHAPE
} R_ParticleEmissionShape;

typedef enum
{
	PTA__SKIP,
	PTA__BILLBOARD,
	PTA__BILLBOARD_VELOCITY,
	PTA__BILLBOARD_VELOCITY_Y,
	PTA__MAX
} R_ParticleTransformAlign;

typedef struct R_ParticleEmitter
{
	vec4 color;
	vec4 end_color;
	vec3 velocity;
	mat4 xform;
	unsigned flags;

	float delta;
	float time;
	float prev_time;

	float explosivness;
	float randomness;
	float life_time;

	unsigned emission_shape;
	vec3 emission_size;

	unsigned particle_amount;
	unsigned cycle;

	unsigned anim_frame;
	unsigned anim_offset;

	unsigned transform_align;
} R_ParticleEmitter;

typedef struct ParticleEmitterClient
{
	R_ParticleEmitter emitter_gl;
	unsigned storage_index;
	bool playing;
	bool one_shot;
	float gravity;
	float life_time;
	float speed_scale;
	float time;
	int cycles;
	int amount;
	vec3 aabb[2];
	float initial_velocity;
	vec3 direction;

	float anim_speed_scale;
	float frame_progress;
	int frame_count;

} ParticleEmitterClient;

