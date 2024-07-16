#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "sound.h"
#include "utility/u_utility.h"

#define MAX_SOUNDS 32
#define SOUND_HASH_SIZE 1024
#define MAX_SOUND_GROUPS 10

typedef struct
{
	ma_sound ma_handle;
	const char* name;

	struct SoundInternal* next;
	struct SoundInternal* hash_next;
} SoundInternal;

typedef struct
{
	SoundInternal sounds[MAX_SOUNDS];
	int index_count;

	SoundInternal* hash_table[SOUND_HASH_SIZE];
	SoundInternal* next;
	ma_sound_group sound_groups[32];
	
	char* empty_string;

} SoundCore;

ma_engine sound_engine;
static SoundCore sound_core;



static long _genHashValueForSound(const char* p_string)
{
	int i = 0;
	long hash = 0;
	char letter = 0;

	while (p_string[i] != '\0')
	{
		letter = tolower(p_string[i]);
		hash += (long)(letter) * (i + 119);
		i++;
	}

	hash &= (SOUND_HASH_SIZE - 1);
	return hash;
}


ma_sound* Sound_get(const char* p_soundName)
{
	SoundInternal* sound;
	long hash = 0;

	hash = _genHashValueForSound(p_soundName);

	for (sound = sound_core.hash_table[hash]; sound; sound = sound->hash_next)
	{
		if (!_strcmpi(p_soundName, sound->name))
		{
			return &sound->ma_handle;
		}
	}

	return NULL;
}

ma_sound* Sound_load(const char* p_soundName, const char* p_filePath, uint32_t p_flags)
{
	if (sound_core.index_count + 1 >= MAX_SOUNDS)
	{
		return NULL;
	}
	if (!p_filePath)
	{
		return NULL;
	}
	
	//if the sound exists return it
	SoundInternal* find_cvar = Sound_get(p_soundName);
	if (find_cvar)
	{
		return find_cvar;
	}

	ma_sound ma_sound;
	ma_result result;

	result = ma_sound_init_from_file(&sound_engine, p_filePath, p_flags, NULL, NULL, &ma_sound);

	if (result != MA_SUCCESS)
	{
		printf("Failed to load sound file at path %s \n", p_filePath);
		return false;
	}

	SoundInternal* sound = NULL;

	sound = &sound_core.sounds[sound_core.index_count];
	sound_core.index_count++;

	sound->name = String_safeCopy(p_soundName);
	sound->next = sound_core.next;
	sound_core.next = sound;

	long hash = _genHashValueForSound(sound->name);
	sound->hash_next = sound_core.hash_table[hash];
	sound_core.hash_table[hash] = sound;

	sound->ma_handle = ma_sound;

	return &sound->ma_handle;
}

void Sound_play(const char* p_soundName)
{
	ma_sound* ma_handle = Sound_get(p_soundName);

	if (!ma_handle)
	{
		printf("Sound not found by name %s \n", p_soundName);
		return;
	}
	
	ma_sound_start(ma_handle);
}

void Sound_stop(const char* p_soundName)
{
	ma_sound* ma_handle = Sound_get(p_soundName);

	if (!ma_handle)
	{
		printf("Sound not found by name %s \n", p_soundName);
		return;
	}

	ma_sound_stop(ma_handle);
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

