#include <stdbool.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "lc_core.h"
#include "utility/u_math.h"

#include "cvar.h"

#include "render/r_renderer.h"

#include <threads.h>

#include <windows.h>

#include "input.h"

#include "lc/lc_world.h"

extern GLFWwindow* glfw_window;

/*
~~~~~~~~~~~~~~~~~~~~~~
CORE ENGINE FUNCTIONS
~~~~~~~~~~~~~~~~~~~~~~
*/
#define NUKLEAR_MAX_VERTEX_BUFFER 512 * 1024
#define NUKLEAR_MAX_ELEMENT_BUFFER 128 * 1024

#define USE_NEW_RENDERER
#include "core/c_common.h"
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_IMPLEMENTATION
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>



#define MAX_PHYS_STEPS 6
#define MAX_PHYS_DELTA_TIME 1.0
#define PHYS_DESIRED_FPS 60.0f
#define PHYS_MS_PER_SECOND 1000.0f

extern void r_startFrame1();
extern void r_endFrame1();
extern void Window_EndFrame();
extern bool C_init();
extern void C_Exit();
extern void r_startFrame();
extern void r_endFrame();
extern void Input_processActions();
extern void Con_Update();
extern void ThreadCore_ShutdownInactiveThreads();
extern void RCore_Start();
extern void RCore_End();
extern void LC_Draw();
extern void LC_World_StartFrame();
extern void LC_World_EndFrame();

typedef struct C_EngineTiming
{
	size_t ticks;
	size_t phys_ticks;
	float delta_time;
	bool phys_in_frame;
} C_EngineTiming;

NK_Data nk;
static C_EngineTiming s_engineTiming;
static bool s_blockedInput = false;

size_t C_getTicks()
{
	return s_engineTiming.ticks;
}
size_t C_getPhysicsTicks()
{
	return s_engineTiming.phys_ticks;
}

float C_getDeltaTime()
{
	return s_engineTiming.delta_time;
}

float C_getPhysDeltaTime()
{
	return 0.0f;
}

bool C_CheckForBlockedInputState()
{
	if (s_blockedInput == true)
	{
		return true;
	}

	return false;
}

void C_BlockInputThisFrame()
{
	s_blockedInput = true;
}

void C_Loop()
{
	//TIMING CONSTANTS. 
	//It is better to to put them as variables instead of macros since i am afraid of macro casts
	const float MS_PER_SECOND = PHYS_MS_PER_SECOND;
	const float DESIRED_FPS = PHYS_DESIRED_FPS;
	const float DESIRED_FRAME_TIME = MS_PER_SECOND / DESIRED_FPS;
	const float MAX_DELTA_TIME = 1.0f;
	const int MAX_PHYSICS_STEPS = 6;

	float previous_frame = glfwGetTime();
	float previous_render_frame = 0;

	/*
	* ~~~~~~~~~~~~~~~~~~
	*	MAIN LOOP
	* ~~~~~~~~~~~~~~~~~~
	*/
	while (!glfwWindowShouldClose(glfw_window))
	{
		if (glfwGetKey(glfw_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(glfw_window, true);
		
		
		s_blockedInput = false;

		nk_glfw3_new_frame(&nk.glfw);

		/*
		* ~~~~~~~~~~~~~~~~~~
		*	DELTA TIMINGS UPDATE
		* ~~~~~~~~~~~~~~~~~~
		*/

		float new_time = glfwGetTime();
		float delta_time = new_time - previous_frame;
		previous_frame = new_time;
		float total_delta_time = delta_time / DESIRED_FRAME_TIME;
		s_engineTiming.delta_time = delta_time;
	
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
		LC_Loop(delta_time);
		LC_World_StartFrame();
	
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
		int physics_steps = 0;
		while (total_delta_time > 0.0f && physics_steps < MAX_PHYSICS_STEPS)
		{
			s_engineTiming.phys_in_frame = true;
			
			float physics_delta_time = min(total_delta_time, MAX_DELTA_TIME);
			
			LC_PhysLoop(physics_delta_time);

			total_delta_time -= physics_delta_time;
			s_engineTiming.phys_ticks++;
			physics_steps++;
			s_engineTiming.phys_in_frame = false;
		}
		
		/*
		* ~~~~~~~~~~~~~~~~~~
		* RENDER
		* ~~~~~~~~~~~~~~~~~~
		*/

		//RENDER
		RCore_End();

		nk_glfw3_render(&nk.glfw, NK_ANTI_ALIASING_ON, NUKLEAR_MAX_VERTEX_BUFFER, NUKLEAR_MAX_ELEMENT_BUFFER);
		glfwSwapBuffers(glfw_window);

		LC_World_EndFrame();
		ThreadCore_ShutdownInactiveThreads();
		s_engineTiming.ticks++;
		glfwPollEvents();

		Window_EndFrame();
	}
}


int C_entry()
{
	if (!C_init()) return -1;

	LC_Init();
	
	memset(&s_engineTiming, 0, sizeof(C_EngineTiming));
	

	//START CORE LOOP
	C_Loop();


	//CLEAN UP
	C_Exit();
	LC_Cleanup();

	return 0;
}