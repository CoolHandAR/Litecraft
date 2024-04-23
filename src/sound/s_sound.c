#include "s_sound.h"


extern ma_engine sound_engine;


bool s_createSoundGroup(ma_sound_group* r_sound_group, uint32_t p_flags, float p_volume)
{
	ma_result result = ma_sound_group_init(&sound_engine, p_flags, NULL, r_sound_group);

	if (result != MA_SUCCESS)
	{
		printf("Failed to create a sound group\n");
		return false;
	}

	ma_sound_group_set_volume(r_sound_group, p_volume);

	return true;
}

bool s_LoadSound(ma_sound* r_sound, const char* p_soundPath, uint32_t p_flags, float p_volume, ma_sound_group* p_sound_group)
{
	uint32_t flags = p_flags;
	flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
//	flags |= MA_SOUND_FLAG_ASYNC;

	ma_result result;

	result = ma_sound_init_from_file(&sound_engine, p_soundPath, flags, p_sound_group, NULL, r_sound);

	if (result != MA_SUCCESS)
	{
		printf("Failed to load sound file at path %s \n", p_soundPath);
		return false;
	}

	ma_sound_set_volume(r_sound, p_volume);

	return true;
}

bool s_LoadSpatialSound(ma_sound* r_sound, const char* p_soundPath, uint32_t p_flags, float p_volume, vec3 p_position, vec3 p_direction, ma_sound_group* p_sound_group)
{
	uint32_t flags = p_flags;
	
	ma_result result;

	result = ma_sound_init_from_file(&sound_engine, p_soundPath, flags, p_sound_group, NULL, r_sound);

	if (result != MA_SUCCESS)
	{
		printf("Failed to load sound file at path %s \n", p_soundPath);
		return false;
	}

	ma_sound_set_volume(r_sound, p_volume);
	
	if (p_position)
	{
		ma_sound_set_position(r_sound, p_position[0], p_position[1], p_position[2]);
	}
	else
	{
		ma_sound_set_position(r_sound, 0, 0, 0);
	}
	
	if (p_direction)
	{
		ma_sound_set_direction(r_sound, p_direction[0], p_direction[1], p_direction[2]);
	}
	else
	{
		ma_sound_set_direction(r_sound, 0, 0, 0);
	}

	return true;
}

