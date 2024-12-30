#pragma once

#include <cglm/cglm.h>
#include "miniaudio/miniaudio.h"


bool Sound_load(const char* p_filePath, uint32_t p_flags, ma_sound* r_sound);
void Sound_play(const char* p_soundName);
void Sound_stop(const char* p_soundName);

bool Sound_createGroup(uint32_t p_flags, ma_sound_group* r_group);