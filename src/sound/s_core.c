#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "s_sound.h"

ma_engine sound_engine;

int s_InitSoundEngine()
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


void s_SoundEngineCleanup()
{
	ma_engine_uninit(&sound_engine);
}