/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Main core of the rendering pipeline. Holds all the rendering data.
Start and ends the frame
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "r_core.h"
#include "core/c_common.h"

R_CMD_Buffer* cmdBuffer;
R_DrawData* drawData;
R_RenderPassData* pass;
R_BackendData* backend_data;
R_Cvars* r_cvars;
R_Metrics metrics;
R_StorageBuffers storage_buffers;

extern void r_processCommands1();
extern void r_RenderAll();
extern GLFWwindow* glfw_window;
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Updates metrics and sets if we should skip this frame
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
static void _updateMetrics()
{
	backend_data->skip_frame = false;

	metrics.frame_ticks_per_second++;
	float new_time = glfwGetTime();

	//update fps
	if (new_time - metrics.previous_fps_timed_time > 1.0)
	{
		metrics.fps = metrics.frame_ticks_per_second;
		metrics.frame_ticks_per_second = 0;;
		metrics.previous_fps_timed_time = new_time;
	}
	
	if (r_cvars->r_limitFPS->int_value == 1)
	{
		float max_fps = (r_cvars->r_maxFPS->int_value > 0) ? r_cvars->r_maxFPS->int_value : 144; //make sure we are not diving by zero
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
static void _updateMSSAFBOTexture()
{

}
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Runs specific update/change functions 
only if certain cvars are modified
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
static void _checkForModifiedCvars()
{
	if (r_cvars->w_useVsync->modified)
	{
		//glfwSwapInterval(r_cvars->w_useVsync->int_value);
		r_cvars->w_useVsync->modified = false;
	}
	if (r_cvars->w_width->modified)
	{
		
	}
	if (r_cvars->w_height->modified)
	{

	}
}

static void r_waitForRenderThread()
{
	//wait till thread completes it's work
	WaitForSingleObject(backend_data->thread.event_completed, INFINITE);
}

static void r_wakeRenderThread()
{
	SetEvent(backend_data->thread.event_work_permssion);

	//wait till starts working
	//WaitForSingleObject(backend_data->thread.event_active, INFINITE);
}

static void r_renderThreadSleep()
{	
	ResetEvent(backend_data->thread.event_active);

	//signal that we have completed our work
	SetEvent(backend_data->thread.event_completed);

	//stall till we get permission to work from the main thread
	WaitForSingleObject(backend_data->thread.event_work_permssion, INFINITE);
	
	//reset state
	ResetEvent(backend_data->thread.event_completed);
	ResetEvent(backend_data->thread.event_work_permssion);

	SetEvent(backend_data->thread.event_active);
}

void r_threadLoop()
{
	while (true)
	{
		r_renderThreadSleep();

		backend_data->thread.boolean_active = true;

		r_processCommands1();

		backend_data->thread.boolean_active = false;
	}
}

void r_startFrame1()
{
	_updateMetrics();
	_checkForModifiedCvars();
	
	r_processCommands1();
	//r_wakeRenderThread();
}

void r_endFrame1()
{
	//r_waitForRenderThread();
	
	//int sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	//glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, INFINITE);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	r_RenderAll();


}