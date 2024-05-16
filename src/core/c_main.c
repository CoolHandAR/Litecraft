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

extern GLFWwindow* glfw_window;

/*
~~~~~~~~~~~~~~~~~~~~~~
CORE ENGINE FUNCTIONS
~~~~~~~~~~~~~~~~~~~~~~
*/
#define NUKLEAR_MAX_VERTEX_BUFFER 512 * 1024
#define NUKLEAR_MAX_ELEMENT_BUFFER 128 * 1024


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
extern bool C_init();
extern void C_Exit();
extern void r_startFrame();
extern void r_endFrame();
extern void Input_processActions();
extern void Con_Update();

typedef struct C_EngineTiming
{
	size_t ticks;
	size_t phys_ticks;
	bool phys_in_frame;
} C_EngineTiming;

NK_Data nk;
static C_EngineTiming s_engineTiming;

size_t C_getTicks()
{
	return s_engineTiming.ticks;
}
size_t C_getPhysicsTicks()
{
	return s_engineTiming.phys_ticks;
}

void C_Loop()
{
	bool use_fps_limit = false;

	const double MAX_FPS = 1.0 / 40;

	int fps = 0;
	float previous_fps_timed_frame = 0.0f;
	float frames_per_second = 0.0f;

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


		//we need to issue draw calls before the core game loop
		//start rendering
		//r_startFrame1();

		/*
		* ~~~~~~~~~~~~~~~~~~
		* EXTRAS
		* ~~~~~~~~~~~~~~~~~~
		*/
		Con_Update();
		/*
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		* PROCESS RENDER COMMANDS ON THE RENDER THREAD
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		*/
		thrd_t render_thread;
		//thrd_create(&render_thread, r_processCommands, NULL);
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
		//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		//r_endFrame1();
		
		//WAIT FOR THE RENDER THREAD TO COMPLETE
		//thrd_join(render_thread, NULL);
		//RENDER
		++frames_per_second;
		if (use_fps_limit)
		{
			if (new_time - previous_render_frame >= MAX_FPS)
			{
				//r_startFrame();
				//r_endFrame();

				//nk_glfw3_render(&nk.glfw, NK_ANTI_ALIASING_ON, NUKLEAR_MAX_VERTEX_BUFFER, NUKLEAR_MAX_ELEMENT_BUFFER);
				//glfwSwapBuffers(glfw_window);
				previous_render_frame = new_time;
			}
		}
		else
		{
			r_startFrame();
			r_endFrame();

			nk_glfw3_render(&nk.glfw, NK_ANTI_ALIASING_ON, NUKLEAR_MAX_VERTEX_BUFFER, NUKLEAR_MAX_ELEMENT_BUFFER);
			glfwSwapBuffers(glfw_window);


			if (new_time - previous_fps_timed_frame > 1.0)
			{
				fps = frames_per_second;
				frames_per_second = 0;
				previous_fps_timed_frame = new_time;
			}
			
		}
		
		s_engineTiming.ticks++;
		glfwPollEvents();
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