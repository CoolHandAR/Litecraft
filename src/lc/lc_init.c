/*
~~~~~~~~~~~~~~~~~~
INIT GAME RESOURCES, GAME WORLD, PHYSICS WORLD, ETC...
~~~~~~~~~~~~~~~~~~
*/

#include "input.h"
#include "sound/sound.h"
#include "core/resource_manager.h"
#include "lc_core.h"


LC_ResourceList lc_resources;
LC_SoundGroups lc_sound_groups;
LC_Emitters lc_emitters;

static void Init_Player()
{

}

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

void Init_Emitters()
{
	memset(&lc_emitters, 0, sizeof(lc_emitters));

	for (int i = 0; i < 5; i++)
	{
		lc_emitters.block_dig[i] = Particle_RegisterEmitter();

		lc_emitters.block_dig[i]->texture = Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE);

		lc_emitters.block_dig[i]->settings.particle_amount = 5;
		glm_mat4_identity(lc_emitters.block_dig[i]->settings.xform);
		lc_emitters.block_dig[i]->settings.state_flags |= EMITTER_STATE_FLAG__EMITTING;
		lc_emitters.block_dig[i]->settings.settings_flags |= EMITTER_SETTINGS_FLAG__ONE_SHOT | EMITTER_SETTINGS_FLAG__CAST_SHADOWS;
		lc_emitters.block_dig[i]->settings.direction[0] = -1;
		lc_emitters.block_dig[i]->settings.initial_velocity = 4.5;
		lc_emitters.block_dig[i]->settings.gravity = 10;
		lc_emitters.block_dig[i]->settings.speed_scale = 1.0;
		lc_emitters.block_dig[i]->settings.spread = 10;
		lc_emitters.block_dig[i]->settings.explosiveness = 1;
		lc_emitters.block_dig[i]->settings.randomness = 1.0;
		lc_emitters.block_dig[i]->settings.frame_count = 0;
		lc_emitters.block_dig[i]->settings.frame = 12;
		lc_emitters.block_dig[i]->settings.scale = 0.05;
		lc_emitters.block_dig[i]->settings.life_time = 1.0;
		lc_emitters.block_dig[i]->settings.anim_speed_scale = 0;
		lc_emitters.block_dig[i]->settings.h_frames = 25;
		lc_emitters.block_dig[i]->settings.v_frames = 25;
		lc_emitters.block_dig[i]->settings.friction = 1.0;
		lc_emitters.block_dig[i]->settings.ambient_intensity = 0.0;
		lc_emitters.block_dig[i]->settings.diffuse_intensity = 0.1;
		lc_emitters.block_dig[i]->settings.specular_intensity = 0.3;

		lc_emitters.block_dig[i]->settings.xform[3][0] += 14.5;
		lc_emitters.block_dig[i]->settings.xform[3][1] += 16.5;
		lc_emitters.block_dig[i]->settings.xform[3][2] += 8.5;
					
		lc_emitters.block_dig[i]->settings.emission_shape = EES__BOX;
		lc_emitters.block_dig[i]->settings.emission_size[0] = 0.3;
		lc_emitters.block_dig[i]->settings.emission_size[1] = 0.3;
		lc_emitters.block_dig[i]->settings.emission_size[2] = 0.3;
					
		lc_emitters.block_dig[i]->aabb[0][0] = -5;
		lc_emitters.block_dig[i]->aabb[0][1] = -5;
		lc_emitters.block_dig[i]->aabb[0][2] = -5;
		lc_emitters.block_dig[i]->aabb[1][0] = 5;
		lc_emitters.block_dig[i]->aabb[1][1] = 5;
		lc_emitters.block_dig[i]->aabb[1][2] = 5;

		glm_mat4_mulv3(lc_emitters.block_dig[i]->settings.xform, lc_emitters.block_dig[i]->aabb[0], 1.0, lc_emitters.block_dig[i]->settings.aabb[0]);
		glm_mat4_mulv3(lc_emitters.block_dig[i]->settings.xform, lc_emitters.block_dig[i]->aabb[1], 1.0, lc_emitters.block_dig[i]->settings.aabb[1]);

		Particle_MarkUpdate(lc_emitters.block_dig[i]);
	}
	
}

int LC_Init2()
{
	if (!Init_loadResources()) return false;
	Init_Emitters();

}