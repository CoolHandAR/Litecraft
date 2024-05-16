#pragma once

#include <cglm/cglm.h>
#include "miniaudio/miniaudio.h"

ma_sound* Sound_get(const char* p_soundName);
ma_sound* Sound_load(const char* p_soundName, const char* p_filePath, uint32_t p_flags);
void Sound_play(const char* p_soundName);
void Sound_stop(const char* p_soundName);