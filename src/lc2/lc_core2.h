#pragma once


#include "utility/dynamic_array.h"
#include "render/r_camera.h"
#include "core/cvar.h"

/*
* ~~~~~~~~~~~~~~~~~~~~
	CVARS
* ~~~~~~~~~~~~~~~~~~~
*/
typedef struct
{
	Cvar* lc_timeofday;
} LC_Cvars;

/*
* ~~~~~~~~~~~~~~~~~~~~
	CORE
* ~~~~~~~~~~~~~~~~~~~
*/
void LC_Draw();
void LC_Loop(float delta);
void LC_PhysLoop(float delta);
void LC_Cleanup();