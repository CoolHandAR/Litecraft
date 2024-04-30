#include <stdbool.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "lc_core.h"
#include "utility/u_math.h"

#include "c_console.h"
#include "c_cvars.h"

#include "render/r_renderer.h"

#include <threads.h>

#include <windows.h>

#define MAX_PHYS_STEPS 6
#define MAX_PHYS_DELTA_TIME 1.0
#define PHYS_DESIRED_FPS 60.0f
#define PHYS_MS_PER_SECOND 1000.0f

 GLFWwindow* s_Window;
 ivec2 s_windowSize;

extern bool C_init();
extern void r_onWindowResize(ivec2 window_size);
extern void s_SoundEngineCleanup();
extern void r_startFrame();
extern void r_endFrame();

typedef struct C_EngineTiming
{
	size_t ticks;
	size_t phys_ticks;
	bool phys_in_frame;
} C_EngineTiming;

static C_EngineTiming s_engineTiming;

void C_setWindowPtr(GLFWwindow* ptr)
{
	s_Window = ptr;
}

void C_resizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	s_windowSize[0] = width;
	s_windowSize[1] = height;

	r_onWindowResize(s_windowSize);
}

void C_mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	LC_MouseUpdate(xpos, ypos);
}
void C_kbCallback(GLFWwindow* Window, int key, int scancode, int action, int mods)
{
	//backspace for console
	if ((key == 259 || key == 257)&& action == GLFW_PRESS)
	{
		C_KB_Input(key);
	}
}
void C_charCallback(GLFWwindow* window, unsigned int codepoint)
{
		C_KB_Input(codepoint);
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
	while (!glfwWindowShouldClose(s_Window))
	{
		if (glfwGetKey(s_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(s_Window, true);
		

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
		*	CORE GAME LOOP
		* ~~~~~~~~~~~~~~~~~~
		*/
		LC_Loop(delta_time);

		/*
		* ~~~~~~~~~~~~~~~~~~
		* EXTRAS
		* ~~~~~~~~~~~~~~~~~~
		*/
		C_DrawConsole();


		/*
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		* PROCESS RENDER COMMANDS ON THE RENDER THREAD
		* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		*/
		thrd_t render_thread;
		thrd_create(&render_thread, r_processCommands, NULL);
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
		//WAIT FOR THE RENDER THREAD TO COMPLETE
		thrd_join(render_thread, NULL);
		//RENDER
		++frames_per_second;
		if (use_fps_limit)
		{
			if (new_time - previous_render_frame >= MAX_FPS)
			{
				r_startFrame();
				r_endFrame();

				glfwSwapBuffers(s_Window);
				previous_render_frame = new_time;
			}
		}
		else
		{
			r_startFrame();
			r_endFrame();

			glfwSwapBuffers(s_Window);


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

void C_Cleanup()
{
	glfwTerminate();
	s_SoundEngineCleanup();
	C_CvarCoreCleanup();
}

int C_entry()
{
	if (!C_init()) return -1;

	LC_Init();
	
	memset(&s_engineTiming, 0, sizeof(C_EngineTiming));
	

	

	
	s_windowSize[0] = 800;
	s_windowSize[1] = 600;

	//START CORE LOOP
	C_Loop();


	//CLEAN UP
	LC_Cleanup();
	C_Cleanup();

	return 0;
}