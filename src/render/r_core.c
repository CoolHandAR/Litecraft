#include <GLFW/glfw3.h>
#include "r_core.h"

R_CMD_Buffers* cmdBuffers;
R_DrawData* drawData;
R_BackendData* backend_data;
R_Cvars* cvars;
R_Metrics metrics;

extern void r_processCommands1();


//update metrics values and check if we can render this frame
static void _updateMetrics()
{
	backend_data->skip_frame = false;
	backend_data->render_thread_active = false;

	metrics.frame_ticks_per_second++;
	float new_time = glfwGetTime();

	//update fps
	if (new_time - metrics.previous_fps_timed_time > 1.0)
	{
		metrics.fps = metrics.frame_ticks_per_second;
		metrics.frame_ticks_per_second = 0;;
		metrics.previous_fps_timed_time = new_time;
	}
	
	if (cvars->r_limitFPS->int_value == 1)
	{
		float max_fps = (cvars->r_maxFPS->int_value > 0) ? cvars->r_maxFPS->int_value : 144; //make sure we are not diving by zero
		float max_fps_norm = 1.0 / max_fps;

		if (new_time - metrics.previous_render_time >= max_fps_norm)
		{
			backend_data->skip_frame = false;
			metrics.previous_render_time = new_time;
		}
		else
		{
			backend_data->skip_frame = true;
		}
	}
}

static void _checkForModifiedCvars()
{
	if (cvars->r_useVsync->modified)
	{
		glfwSwapInterval(cvars->r_useVsync->int_value);
		cvars->r_useVsync->modified = false;
	}
}

static void _beginProcessingCommands()
{
	if (cvars->r_multithread->int_value == 1)
	{
		if (thrd_create(&backend_data->render_thread, r_processCommands1, NULL) != thrd_success)
		{
			return;
		}
		backend_data->render_thread_active = true;
	}
	else
	{
		r_processCommands1();
	}
}

void r_startFrame()
{
	_updateMetrics();
	_checkForModifiedCvars();

	if (backend_data->skip_frame)
	{
		return;
	}

	_beginProcessingCommands();
}

void r_endFrame()
{
	if (backend_data->skip_frame)
	{
		return;
	}

	//if we have thread active wait for it to finish its work
	if (backend_data->render_thread_active)
	{
		thrd_join(backend_data->render_thread, NULL);
		backend_data->render_thread_active = false;
	}


}