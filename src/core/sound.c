#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "core/sound.h"
#include "core/cvar.h"
#include "utility/u_utility.h"

ma_engine sound_engine;


bool Sound_load(const char* p_filePath, uint32_t p_flags, ma_sound* r_sound)
{
	ma_result result;

	result = ma_sound_init_from_file(&sound_engine, p_filePath, p_flags, NULL, NULL, r_sound);

	if (result != MA_SUCCESS)
	{
		printf("Failed to load sound file at path %s \n", p_filePath);
		return false;
	}

	return true;
}

void Sound_play(const char* p_soundName)
{
	ma_sound* ma_handle = NULL;

	if (!ma_handle)
	{
		printf("Sound not found by name %s \n", p_soundName);
		return;
	}
	
	ma_sound_start(ma_handle);
}

void Sound_stop(const char* p_soundName)
{
	ma_sound* ma_handle = NULL;

	if (!ma_handle)
	{
		printf("Sound not found by name %s \n", p_soundName);
		return;
	}

	ma_sound_stop(ma_handle);
}

bool Sound_createGroup(uint32_t p_flags, ma_sound_group* r_group)
{
	ma_result result;

	result = ma_sound_group_init(&sound_engine, p_flags, NULL, r_group);

	if (result != MA_SUCCESS)
	{
		printf("Failed to create sound group");
		return false;
	}

	return true;
}

void Sound_setMasterVolume(float volume)
{
	ma_engine_set_volume(&sound_engine, volume);
}

int Sound_Init()
{
	ma_result result;

	result = ma_engine_init(NULL, &sound_engine);

	if (result != MA_SUCCESS)
	{
		printf("Failed to init MA sound engine \n");
		return 0;
	}

	return 1;
}

void Sound_Cleanup()
{
	ma_engine_uninit(&sound_engine);
}

