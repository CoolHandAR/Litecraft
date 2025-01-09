/*
~~~~~~~~~~~~~~~~~~
INIT GAME RESOURCES, GAME WORLD, PHYSICS WORLD, ETC...
~~~~~~~~~~~~~~~~~~
*/

#include "core/input.h"
#include "core/sound.h"
#include "core/resource_manager.h"
#include "lc_core.h"
#include "render/r_public.h"


LC_ResourceList lc_resources;
LC_SoundGroups lc_sound_groups;
LC_Emitters lc_emitters;
LC_CoreData lc_core_data;

static int Init_loadResources()
{
	memset(&lc_resources, 0, sizeof(lc_resources));

	//Textures



	//Sounds
	lc_resources.fall_sound = Resource_get("assets/sounds/player/fall/fallsmall.wav", RESOURCE__SOUND);
	lc_resources.big_fall_sound = Resource_get("assets/sounds/player/fall/fallbig.wav", RESOURCE__SOUND);

	lc_resources.grass_step_sounds[0] = Resource_get("assets/sounds/player/step/grass1.wav", RESOURCE__SOUND);
	lc_resources.grass_step_sounds[1] = Resource_get("assets/sounds/player/step/grass2.wav", RESOURCE__SOUND);
	lc_resources.grass_step_sounds[2] = Resource_get("assets/sounds/player/step/grass3.wav", RESOURCE__SOUND);
	lc_resources.grass_step_sounds[3] = Resource_get("assets/sounds/player/step/grass4.wav", RESOURCE__SOUND);
	lc_resources.grass_step_sounds[4] = Resource_get("assets/sounds/player/step/grass5.wav", RESOURCE__SOUND);
	lc_resources.grass_step_sounds[5] = Resource_get("assets/sounds/player/step/grass6.wav", RESOURCE__SOUND);

	lc_resources.sand_step_sounds[0] = Resource_get("assets/sounds/player/step/sand2.wav", RESOURCE__SOUND);
	lc_resources.sand_step_sounds[1] = Resource_get("assets/sounds/player/step/sand3.wav", RESOURCE__SOUND);
	lc_resources.sand_step_sounds[2] = Resource_get("assets/sounds/player/step/sand4.wav", RESOURCE__SOUND);
	lc_resources.sand_step_sounds[3] = Resource_get("assets/sounds/player/step/sand5.wav", RESOURCE__SOUND);
	lc_resources.sand_step_sounds[4] = Resource_get("assets/sounds/player/step/sand5.wav", RESOURCE__SOUND);

	lc_resources.stone_step_sounds[0] = Resource_get("assets/sounds/player/step/stone1.wav", RESOURCE__SOUND);
	lc_resources.stone_step_sounds[1] = Resource_get("assets/sounds/player/step/stone2.wav", RESOURCE__SOUND);
	lc_resources.stone_step_sounds[2] = Resource_get("assets/sounds/player/step/stone3.wav", RESOURCE__SOUND);
	lc_resources.stone_step_sounds[3] = Resource_get("assets/sounds/player/step/stone4.wav", RESOURCE__SOUND);
	lc_resources.stone_step_sounds[4] = Resource_get("assets/sounds/player/step/stone5.wav", RESOURCE__SOUND);
	lc_resources.stone_step_sounds[5] = Resource_get("assets/sounds/player/step/stone6.wav", RESOURCE__SOUND);

	lc_resources.wood_step_sounds[0] = Resource_get("assets/sounds/player/step/wood1.wav", RESOURCE__SOUND);
	lc_resources.wood_step_sounds[1] = Resource_get("assets/sounds/player/step/wood2.wav", RESOURCE__SOUND);
	lc_resources.wood_step_sounds[2] = Resource_get("assets/sounds/player/step/wood3.wav", RESOURCE__SOUND);
	lc_resources.wood_step_sounds[3] = Resource_get("assets/sounds/player/step/wood4.wav", RESOURCE__SOUND);
	lc_resources.wood_step_sounds[4] = Resource_get("assets/sounds/player/step/wood5.wav", RESOURCE__SOUND);
	lc_resources.wood_step_sounds[5] = Resource_get("assets/sounds/player/step/wood6.wav", RESOURCE__SOUND);

	lc_resources.gravel_step_sounds[0] = Resource_get("assets/sounds/player/step/gravel1.wav", RESOURCE__SOUND);
	lc_resources.gravel_step_sounds[1] = Resource_get("assets/sounds/player/step/gravel2.wav", RESOURCE__SOUND);
	lc_resources.gravel_step_sounds[2] = Resource_get("assets/sounds/player/step/gravel3.wav", RESOURCE__SOUND);
	lc_resources.gravel_step_sounds[3] = Resource_get("assets/sounds/player/step/gravel4.wav", RESOURCE__SOUND);

	lc_resources.grass_dig_sounds[0] = Resource_get("assets/sounds/player/dig/grass1.wav", RESOURCE__SOUND);
	lc_resources.grass_dig_sounds[1] = Resource_get("assets/sounds/player/dig/grass2.wav", RESOURCE__SOUND);
	lc_resources.grass_dig_sounds[2] = Resource_get("assets/sounds/player/dig/grass3.wav", RESOURCE__SOUND);
	lc_resources.grass_dig_sounds[3] = Resource_get("assets/sounds/player/dig/grass4.wav", RESOURCE__SOUND);

	lc_resources.sand_dig_sounds[0] = Resource_get("assets/sounds/player/dig/sand1.wav", RESOURCE__SOUND);
	lc_resources.sand_dig_sounds[1] = Resource_get("assets/sounds/player/dig/sand2.wav", RESOURCE__SOUND);
	lc_resources.sand_dig_sounds[2] = Resource_get("assets/sounds/player/dig/sand3.wav", RESOURCE__SOUND);
	lc_resources.sand_dig_sounds[3] = Resource_get("assets/sounds/player/dig/sand4.wav", RESOURCE__SOUND);

	lc_resources.stone_dig_sounds[0] = Resource_get("assets/sounds/player/dig/stone1.wav", RESOURCE__SOUND);
	lc_resources.stone_dig_sounds[1] = Resource_get("assets/sounds/player/dig/stone2.wav", RESOURCE__SOUND);
	lc_resources.stone_dig_sounds[2] = Resource_get("assets/sounds/player/dig/stone3.wav", RESOURCE__SOUND);
	lc_resources.stone_dig_sounds[3] = Resource_get("assets/sounds/player/dig/stone4.wav", RESOURCE__SOUND);

	lc_resources.wood_dig_sounds[0] = Resource_get("assets/sounds/player/dig/wood1.wav", RESOURCE__SOUND);
	lc_resources.wood_dig_sounds[1] = Resource_get("assets/sounds/player/dig/wood2.wav", RESOURCE__SOUND);
	lc_resources.wood_dig_sounds[2] = Resource_get("assets/sounds/player/dig/wood3.wav", RESOURCE__SOUND);
	lc_resources.wood_dig_sounds[3] = Resource_get("assets/sounds/player/dig/wood4.wav", RESOURCE__SOUND);
}

void Init_SoundGroups()
{
	memset(&lc_sound_groups, 0, sizeof(lc_sound_groups));

	Sound_createGroup(0, &lc_sound_groups.step);
}

void Process_ParticleCollideWithWorld(ParticleCpu* const particle, ParticleEmitterSettings* const emitter, float local_delta)
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

void Init_Emitters()
{
	//hardcoded particle emitters, fine for now(not many different emitters), might make a serialization format later
	memset(&lc_emitters, 0, sizeof(lc_emitters));
	for (int i = 0; i < 5; i++)
	{
		lc_emitters.block_dig[i] = Particle_RegisterEmitter();

		lc_emitters.block_dig[i]->texture = Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE);

		lc_emitters.block_dig[i]->particle_amount = (i == 4) ? 5 : 5;
		glm_mat4_identity(lc_emitters.block_dig[i]->xform);
		lc_emitters.block_dig[i]->initial_velocity = 8.0;
		lc_emitters.block_dig[i]->linear_accel = 0.0;
		lc_emitters.block_dig[i]->gravity = -24;
		lc_emitters.block_dig[i]->speed_scale = 1.0;
		lc_emitters.block_dig[i]->spread = 45;
		lc_emitters.block_dig[i]->explosiveness = 1;
		lc_emitters.block_dig[i]->randomness = 1.0;
		lc_emitters.block_dig[i]->scale = 0.1;
		lc_emitters.block_dig[i]->life_time = 2.4;
		lc_emitters.block_dig[i]->h_frames = 25;
		lc_emitters.block_dig[i]->v_frames = -25;
		lc_emitters.block_dig[i]->friction = 3.0;
					
		lc_emitters.block_dig[i]->emission_shape = EES__BOX;
		lc_emitters.block_dig[i]->emission_size[0] = 0.2;
		lc_emitters.block_dig[i]->emission_size[1] = 0.2;
		lc_emitters.block_dig[i]->emission_size[2] = 0.2;
		lc_emitters.block_dig[i]->aabb[0][0] = -5;
		lc_emitters.block_dig[i]->aabb[0][1] = -5;
		lc_emitters.block_dig[i]->aabb[0][2] = -5;
		lc_emitters.block_dig[i]->aabb[1][0] = 5;
		lc_emitters.block_dig[i]->aabb[1][1] = 5;
		lc_emitters.block_dig[i]->aabb[1][2] = 5;

		lc_emitters.block_dig[i]->color[0] = 1;
		lc_emitters.block_dig[i]->color[1] = 1;
		lc_emitters.block_dig[i]->color[2] = 1;
		lc_emitters.block_dig[i]->color[3] = 1;

		lc_emitters.block_dig[i]->end_color[0] = 1;
		lc_emitters.block_dig[i]->end_color[1] = 1;
		lc_emitters.block_dig[i]->end_color[2] = 1;
		lc_emitters.block_dig[i]->end_color[3] =1;

		lc_emitters.block_dig[i]->one_shot = true;

		lc_emitters.block_dig[i]->collision_function = Process_ParticleCollideWithWorld;
	}
	
}
extern void PL_initPlayer(vec3 p_spawnPos);
void LC_Init()
{
	if (!Init_loadResources()) return false;
	Init_Emitters();

	memset(&lc_core_data, 0, sizeof(lc_core_data));

	lc_core_data.cam = Camera_Init();
	Camera_setCurrent(&lc_core_data.cam);

	LC_World_Create(16, 7, 16);

	vec3 player_pos;
	player_pos[0] = -5;
	player_pos[1] = 16;
	player_pos[2] = 5;
	PL_initPlayer(player_pos);

	
}
