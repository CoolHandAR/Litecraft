#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "lc/lc_core.h"
#include "lc/lc_world.h"
#include "utility/u_math.h"
#include "core/cvar.h"
#include "core/input.h"
#include "core/core_common.h"
#include <Windows.h>

/*
~~~~~~~~~~~~~~~~~~~~~~
CORE ENGINE FUNCTIONS
~~~~~~~~~~~~~~~~~~~~~~
*/
#define NUKLEAR_MAX_VERTEX_BUFFER 512 * 1024
#define NUKLEAR_MAX_ELEMENT_BUFFER 128 * 1024

#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>

#define CONTROL_STEPS 12

extern void Window_EndFrame();
extern bool Core_init();
extern void Core_Exit();
extern void Input_processActions();
extern void Con_Update();
extern void ThreadCore_ShutdownInactiveThreads();
extern void RCore_Start();
extern void RCore_End();
extern void LC_Draw();
extern void LC_World_StartFrame();
extern void LC_World_EndFrame();

typedef struct
{
	size_t ticks;
	size_t phys_ticks;
	size_t frames_drawn;

	double delta_time;
	double phys_delta_time;

	double prev_time;
	double time_accum;
	double time_deficit;

	double lerp_fraction;

	int frame_physics_steps;

	int typical_physics_steps[CONTROL_STEPS];
	int accumulated_physics_steps[CONTROL_STEPS];

	bool phys_in_frame;
} Core_EngineTiming;

typedef struct
{
	Cvar* master_volume;
} Core_Cvars;

NK_Data nk;
static Core_EngineTiming s_engineTiming;
static Core_Cvars s_cvars;
static bool s_blockedInput = false;

static void Core_UpdateCvars()
{
	if (s_cvars.master_volume->modified)
	{
		Sound_setMasterVolume(s_cvars.master_volume->float_value);
		s_cvars.master_volume->modified = false;
	}
}

static void Core_CalcMainTimer()
{
	//inspired/adapted by godot's main tymer sync system https://github.com/godotengine/godot/blob/master/main/main_timer_sync.cpp

	double new_time = glfwGetTime();

	double delta_time = new_time - s_engineTiming.prev_time;
	s_engineTiming.prev_time = new_time;

	const int physics_fps = 60;
	const double physics_delta = 1.0 / (double)physics_fps;
	const double physics_jitter_fix = 0.5;

	float min_output_step = delta_time / 8;
	min_output_step = max(min_output_step, 1E-6);

	delta_time += s_engineTiming.time_deficit;

	s_engineTiming.time_accum += delta_time;
	
	double delta_time_2 = delta_time;

	int physics_steps = floor(s_engineTiming.time_accum * physics_fps);

	int min_typical_steps = s_engineTiming.typical_physics_steps[0];
	int max_typical_steps = min_typical_steps + 1;

	bool update_typical = false;

	for (int i = 0; i < CONTROL_STEPS - 1; i++)
	{
		int steps_left_to_match_typical = s_engineTiming.typical_physics_steps[i + 1] - s_engineTiming.accumulated_physics_steps[i];

		if (steps_left_to_match_typical > max_typical_steps ||
			steps_left_to_match_typical + 1 < min_typical_steps)
		{
			update_typical = true;
			break;
		}

		if (steps_left_to_match_typical > min_typical_steps) {
			min_typical_steps = steps_left_to_match_typical;
		}
		if (steps_left_to_match_typical + 1 < max_typical_steps) {
			max_typical_steps = steps_left_to_match_typical + 1;
		}
	}

	if (physics_steps < min_typical_steps)
	{
		const int max_possible_steps = floor((s_engineTiming.time_accum) * physics_fps + physics_jitter_fix);
		if (max_possible_steps < min_typical_steps)
		{
			physics_steps = max_possible_steps;
			update_typical = true;
		}
		else
		{
			physics_steps = min_typical_steps;
		}
	}
	else if (physics_steps > max_typical_steps)
	{
		const int min_possible_steps = floor((s_engineTiming.time_accum) * physics_fps - physics_jitter_fix);
		if (min_possible_steps > max_typical_steps)
		{
			physics_steps = min_possible_steps;
			update_typical = true;
		}
		else
		{
			physics_steps = max_typical_steps;
		}
	}

	physics_steps = max(physics_steps, 0);

	s_engineTiming.time_accum -= physics_steps * physics_delta;


	for (int i = CONTROL_STEPS - 2; i >= 0; --i)
	{
		s_engineTiming.accumulated_physics_steps[i + 1] = s_engineTiming.accumulated_physics_steps[i] + physics_steps;
	}

	s_engineTiming.accumulated_physics_steps[0] = physics_steps;

	if (update_typical)
	{
		for (int i = CONTROL_STEPS - 1; i >= 0; i--)
		{
			if (s_engineTiming.typical_physics_steps[i] > s_engineTiming.accumulated_physics_steps[i])
			{
				s_engineTiming.typical_physics_steps[i] = s_engineTiming.accumulated_physics_steps[i];
			}
			else if (s_engineTiming.typical_physics_steps[i] < s_engineTiming.accumulated_physics_steps[i] - 1)
			{
				s_engineTiming.typical_physics_steps[i] = s_engineTiming.accumulated_physics_steps[i] - 1;
			}
		}
	}

	const double idle_minus_accum = delta_time - s_engineTiming.time_accum;

	double min_average_physics_steps = 0;
	double max_average_physics_steps = 0;

	int consistent_steps = 0;

	min_average_physics_steps = s_engineTiming.typical_physics_steps[0];
	max_average_physics_steps = min_average_physics_steps + 1;
	for (int i = 1; i < CONTROL_STEPS; ++i)
	{
		const float typical_lower = s_engineTiming.typical_physics_steps[i];
		const float current_min = typical_lower / (i + 1);
		if (current_min > max_average_physics_steps) 
		{
			consistent_steps = i;
			break;
		}
		else if (current_min > min_average_physics_steps) 
		{
			min_average_physics_steps = current_min;
		}
		const float current_max = (typical_lower + 1) / (i + 1);
		if (current_max < min_average_physics_steps) 
		{
			consistent_steps = i;
			break;
		}
		else if (current_max < max_average_physics_steps) 
		{
			max_average_physics_steps = current_max;
		}
	}

	if (consistent_steps == 0 || consistent_steps > 3)
	{
		delta_time = Math_Clampd(delta_time, min_average_physics_steps * physics_delta, max_average_physics_steps * physics_delta);
	}
	float max_clock_deviation = physics_jitter_fix * physics_delta;
	delta_time = Math_Clampd(delta_time, delta_time_2 - max_clock_deviation, delta_time_2 + max_clock_deviation);

	delta_time = Math_Clampd(delta_time, idle_minus_accum, idle_minus_accum + physics_delta);

	if (delta_time < min_output_step)
	{
		delta_time = min_output_step;
	}

	s_engineTiming.time_accum = delta_time - idle_minus_accum;

	if (s_engineTiming.time_accum > physics_delta)
	{
		const int extra_physics_steps = floor(s_engineTiming.time_accum * physics_fps);
		s_engineTiming.time_accum -= extra_physics_steps * physics_delta;
		physics_steps += extra_physics_steps;
	}

	s_engineTiming.time_deficit = delta_time_2 - delta_time;


	static const int max_physics_steps = 8;
	if (physics_steps > max_physics_steps) 
	{
		delta_time -= (physics_steps - max_physics_steps) * physics_delta;
		physics_steps = max_physics_steps;
	}

	s_engineTiming.lerp_fraction = s_engineTiming.time_accum / physics_delta;
	s_engineTiming.delta_time = delta_time;
	s_engineTiming.phys_delta_time = physics_delta;
	s_engineTiming.frame_physics_steps = physics_steps;
}

size_t Core_getTicks()
{
	return s_engineTiming.ticks;
}
size_t Core_getPhysicsTicks()
{
	return s_engineTiming.phys_ticks;
}

double Core_getDeltaTime()
{
	return s_engineTiming.delta_time;
}

double Core_getPhysDeltaTime()
{
	return s_engineTiming.phys_delta_time;
}

double Core_getLerpFraction()
{
	return s_engineTiming.lerp_fraction;
}

bool Core_CheckForBlockedInputState()
{
	if (s_blockedInput == true)
	{
		return true;
	}

	return false;
}

void Core_BlockInputThisFrame()
{
	s_blockedInput = true;
}

static void Core_Loop()
{
	/*
	* ~~~~~~~~~~~~~~~~~~
	*	MAIN LOOP
	* ~~~~~~~~~~~~~~~~~~
	*/

	GLFWwindow* window = Window_getPtr();

	while (!glfwWindowShouldClose(window))
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
		
		
		s_blockedInput = false;

		if(nk.enabled) nk_glfw3_new_frame(&nk.glfw);

		/*
		* ~~~~~~~~~~~~~~~~~~
		*	CORE UPDATE
		* ~~~~~~~~~~~~~~~~~~
		*/
		Core_UpdateCvars();
		Core_CalcMainTimer();

		/*
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~
		*	RENDER RELATED REQUESTS
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~
		*/
		LC_Draw();
		/*

		/*
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		* RENDERING WORK START
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		*/
		RCore_Start();


		/*
		* ~~~~~~~~~~~~~~~~~~
		*	INPUT UPDATE
		* ~~~~~~~~~~~~~~~~~~
		*/
		Input_processActions();
	
		/*
		* ~~~~~~~~~~~~~~~~~~
		*	CORE GAME LOOP
		* ~~~~~~~~~~~~~~~~~~
		*/
		LC_StartFrame();
	
		/*
		* ~~~~~~~~~~~~~~~~~~
		* EXTRAS
		* ~~~~~~~~~~~~~~~~~~
		*/
		Con_Update();
		
		/*
		* ~~~~~~~~~~~~~~~~~~
		* PHYSICS LOOP
		* ~~~~~~~~~~~~~~~~~~
		*/
		double delta = s_engineTiming.phys_delta_time;	
		for (int physics_steps = 0; physics_steps < s_engineTiming.frame_physics_steps; physics_steps++)
		{
			s_engineTiming.phys_in_frame = true;
			
			LC_PhysUpdate(delta);

			s_engineTiming.phys_ticks++;

			s_engineTiming.phys_in_frame = false;
		}
		
		/*
		* ~~~~~~~~~~~~~~~~~~
		* RENDER
		* ~~~~~~~~~~~~~~~~~~
		*/

		//RENDER
		RCore_End();

		if (nk.enabled) nk_glfw3_render(&nk.glfw, NK_ANTI_ALIASING_ON, NUKLEAR_MAX_VERTEX_BUFFER, NUKLEAR_MAX_ELEMENT_BUFFER);
		glfwSwapBuffers(window);

		/*
		* ~~~~~~~~~~~~~~~~~~
		* END TICK
		* ~~~~~~~~~~~~~~~~~~
		*/
		LC_EndFrame();
		//ThreadCore_ShutdownInactiveThreads();
		s_engineTiming.ticks++;
		s_engineTiming.frames_drawn++;
		glfwPollEvents();

		Window_EndFrame();
	}
}

int Core_entry()
{
	if (!Core_init()) return -1;
	if (!LC_Init())
	{
		MessageBox(NULL, (LPCWSTR)L"Failed to load assets.\nMake sure there is asset folder", (LPCWSTR)L"Failure to load!", MB_ICONWARNING);

		return -1;
	}

	memset(&s_engineTiming, 0, sizeof(Core_EngineTiming));
	memset(&s_cvars, 0, sizeof(s_cvars));

	s_cvars.master_volume = Cvar_Register("master_volume", "1", NULL, CVAR__SAVE_TO_FILE, 0, 24);

	nk.enabled = true;

	for (int i = CONTROL_STEPS - 1; i >= 0; --i) 
	{
		s_engineTiming.typical_physics_steps[i] = i;
		s_engineTiming.accumulated_physics_steps[i] = i;
	}
	
	//Load cvar file if it exists
	Cvar_LoadAllFromFile("config.cfg");

	//START CORE LOOP
	Core_Loop();

	//Save cvar
	Cvar_PrintAllToFile("config.cfg");

	//CLEAN UP
	Core_Exit();
	LC_Exit();

	return 0;
}