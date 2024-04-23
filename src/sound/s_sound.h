#pragma once

#include <cglm/cglm.h>
#include "miniaudio/miniaudio.h"


bool s_createSoundGroup(ma_sound_group* r_sound_group, uint32_t p_flags, float p_volume);

bool s_LoadSound(ma_sound* r_sound, const char* p_soundPath, uint32_t p_flags, float p_volume, ma_sound_group* p_sound_group);
bool s_LoadSpatialSound(ma_sound* r_sound, const char* p_soundPath, uint32_t p_flags, float p_volume, vec3 p_position, vec3 p_direction, ma_sound_group* p_sound_group);