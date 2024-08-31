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
R_Cvars r_cvars;
R_Metrics metrics;
R_StorageBuffers storage;
R_Scene scene;
R_RendererResources resources;

extern void RCmds_processCommands();
extern void Pass_Main();
extern void Compute_DispatchAll();
extern void Compute_Sync();
extern void	RPanel_Main();
extern GLFWwindow* glfw_window;
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Updates metrics and sets if we should skip this frame (if fps limit is on)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
static void RCore_updateMetrics()
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
	
	if (r_cvars.r_limitFPS->int_value == 1)
	{
		float max_fps = (r_cvars.r_maxFPS->int_value > 0) ? r_cvars.r_maxFPS->int_value : 144; //make sure we are not diving by zero
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
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Called on any window resize event. Updates textures sizes, etc...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void RCore_onWindowResize(int width, int height)
{
	//DEFERRED PASS
	glBindFramebuffer(GL_FRAMEBUFFER, pass->deferred.FBO);
	//Update textures
	glBindTexture(GL_TEXTURE_2D, pass->deferred.depth_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->deferred.depth_texture, 0);
	//NORMAL
	glBindTexture(GL_TEXTURE_2D, pass->deferred.gNormalMetal_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->deferred.gNormalMetal_texture, 0);
	//COLOR
	glBindTexture(GL_TEXTURE_2D, pass->deferred.gColorRough_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, pass->deferred.gColorRough_texture, 0);
	//EMISSIVE
	glBindTexture(GL_TEXTURE_2D, pass->deferred.gEmissive_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, pass->deferred.gEmissive_texture, 0);

	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	//SCENE
	glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);
	//main scene
	glBindTexture(GL_TEXTURE_2D, pass->scene.MainSceneColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->scene.MainSceneColorBuffer, 0);

	//AO
	glBindFramebuffer(GL_FRAMEBUFFER, pass->ao.FBO);
	glBindTexture(GL_TEXTURE_2D, pass->ao.ao_texture);
	if (r_cvars.r_useSsao->int_value)
	{
		if (r_cvars.r_ssaoHalfSize->int_value)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width / 2, height / 2, 0, GL_RED, GL_FLOAT, NULL);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, NULL);
		}
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->ao.ao_texture, 0);
	}
	else
	{
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, 1, 1, 0, GL_RED, GL_FLOAT, NULL);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->ao.ao_texture, 0);
	}

	glClearColor(1, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, pass->ao.BLURRED_FBO);
	glBindTexture(GL_TEXTURE_2D, pass->ao.blurred_ao_texture);
	if (r_cvars.r_useSsao->int_value)
	{
		if (r_cvars.r_ssaoHalfSize->int_value)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width / 2, height / 2, 0, GL_RED, GL_FLOAT, NULL);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, NULL);
		}
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->ao.blurred_ao_texture, 0);
	}
	else
	{
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, 1, 1, 0, GL_RED, GL_FLOAT, NULL);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->ao.ao_texture, 0);
	}

	glClearColor(1, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);


	//Bloom
	glBindFramebuffer(GL_FRAMEBUFFER, pass->bloom.FBO);
	float mipWidth = width;
	float mipHeight = height;
	for (int i = 0; i < BLOOM_MIP_COUNT; i++)
	{
		mipWidth *= 0.5;
		mipHeight *= 0.5;

		glBindTexture(GL_TEXTURE_2D, pass->bloom.mip_textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mipWidth, mipHeight, 0, GL_RGB, GL_FLOAT, NULL);

		pass->bloom.mip_sizes[i][0] = mipWidth;
		pass->bloom.mip_sizes[i][1] = mipHeight;
	}

	scene.camera.screen_size[0] = width;
	scene.camera.screen_size[1] = height;

	//Update viewport size vectors
	backend_data->screenSize[0] = width;
	backend_data->screenSize[1] = height;

	backend_data->halfScreenSize[0] = (float)width / 2.0;
	backend_data->halfScreenSize[1] = (float)height / 2.0;

	//Used so that proj matrix only updates when changed
	scene.dirty_cam = true;
}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Runs specific update/change functions 
only if certain cvars are modified
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
static void RCore_checkForModifiedCvars()
{
	if (r_cvars.w_useVsync->modified)
	{
		int enabled = r_cvars.w_useVsync->int_value;
	
		glfwSwapInterval(enabled);
		r_cvars.w_useVsync->modified = false;
	}
	if (r_cvars.w_width->modified)
	{
		
	}
	if (r_cvars.w_height->modified)
	{

	}
	if (r_cvars.r_useSsao->modified || r_cvars.r_ssaoHalfSize->modified)
	{
		//Must clear the ao texture if we want to enable or disable ssao
		RCore_onWindowResize(backend_data->screenSize[0], backend_data->screenSize[1]);

		r_cvars.r_useSsao->modified = false;
	}
	if (r_cvars.r_ssaoBias->modified || r_cvars.r_ssaoRadius->modified || r_cvars.r_ssaoStrength->modified)
	{
		glUseProgram(pass->ao.shader);
		Shader_SetFloat(pass->ao.shader, "u_bias", r_cvars.r_ssaoBias->float_value);
		Shader_SetFloat(pass->ao.shader, "u_radius", r_cvars.r_ssaoRadius->float_value);
		Shader_SetFloat(pass->ao.shader, "u_strength", r_cvars.r_ssaoStrength->float_value);

		r_cvars.r_ssaoBias->modified = false;
		r_cvars.r_ssaoRadius->modified = false;
		r_cvars.r_ssaoStrength->modified = false;
	}
	if (r_cvars.r_Exposure->modified || r_cvars.r_Gamma->modified || r_cvars.r_Brightness->modified || r_cvars.r_Contrast->modified ||
		r_cvars.r_Saturation->modified ||r_cvars.r_bloomStrength->modified)
	{
		glUseProgram(pass->post.post_process_shader);
		Shader_SetFloat(pass->post.post_process_shader, "u_Exposure", r_cvars.r_Exposure->float_value);
		Shader_SetFloat(pass->post.post_process_shader, "u_Gamma", r_cvars.r_Gamma->float_value);
		Shader_SetFloat(pass->post.post_process_shader, "u_Brightness", r_cvars.r_Brightness->float_value);
		Shader_SetFloat(pass->post.post_process_shader, "u_Contrast", r_cvars.r_Contrast->float_value);
		Shader_SetFloat(pass->post.post_process_shader, "u_Saturation", r_cvars.r_Saturation->float_value);
		Shader_SetFloat(pass->post.post_process_shader, "u_bloomStrength", r_cvars.r_bloomStrength->float_value);

		r_cvars.r_Exposure->modified = false;
		r_cvars.r_Gamma->modified = false;
		r_cvars.r_Brightness->modified = false;
		r_cvars.r_Contrast->modified = false;
		r_cvars.r_Saturation->modified = false;
		r_cvars.r_bloomStrength->modified = false;
	}
	if (r_cvars.cam_fov->modified || r_cvars.cam_zFar->modified || r_cvars.cam_zNear->modified)
	{
		R_Camera* cam = Camera_getCurrent();

		if (cam)
		{
			cam->config.fov = r_cvars.cam_fov->float_value;
			cam->config.zNear = r_cvars.cam_zNear->float_value;
			cam->config.zFar = r_cvars.cam_zFar->float_value;

			scene.dirty_cam = true;

			r_cvars.cam_fov->modified = false;
			r_cvars.cam_zNear->modified = false;
			r_cvars.cam_zFar->modified = false;
		}
	}
}
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Uploads various data to the gpu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
static void RCore_UploadGpuData()
{
	glBindVertexArray(0);
	//Batch data
	if (drawData->screen_quad.vertices_count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, drawData->screen_quad.vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ScreenVertex) * drawData->screen_quad.vertices_count, drawData->screen_quad.vertices);
	}
	if (drawData->lines.vertices_count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, drawData->lines.vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(BasicVertex) * drawData->lines.vertices_count, drawData->lines.vertices);
	}
	if (drawData->triangle.vertices_count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, drawData->triangle.vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(BasicVertex) * drawData->triangle.vertices_count, drawData->triangle.vertices);
	}
	if (drawData->text.vertices_count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, drawData->text.vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TextVertex) * drawData->text.vertices_count, drawData->text.vertices);
	}

	//Camera update
	glBindBuffer(GL_UNIFORM_BUFFER, scene.camera_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(R_SceneCameraData), &scene.camera);

	//Scene update
	glBindBuffer(GL_UNIFORM_BUFFER, scene.scene_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(R_SceneData), &scene.scene_data);
}


static void RCore_EndFrameCleanup()
{
	//Reset the vertex count of the batches
	drawData->lines.vertices_count = 0;
	drawData->triangle.vertices_count = 0;
	drawData->screen_quad.vertices_count = 0;
	drawData->screen_quad.indices_count = 0;
	drawData->text.vertices_count = 0;
	drawData->text.indices_count = 0;

	//Reset occlussion draw cmds buffer
	DrawArraysIndirectCommand cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.instanceCount = 1;
	glNamedBufferSubData(drawData->lc_world.world_render_data->ocl_boxes_draw_cmd_buffer, 0, sizeof(DrawArraysIndirectCommand), &cmd);
}

static void RCore_DrawUI()
{
	RPanel_Main();
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

		RCmds_processCommands();

		backend_data->thread.boolean_active = false;
	}
}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Called in c_main.c. Called by the main thread
Dispatches various computes, adds jobs to render thread, updates metrics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void RCore_Start()
{
	//Render panel, metrics ui
	RCore_DrawUI();

	//Dispatch computes
	Compute_Sync();

	//Process various renderer tasks
	RCmds_processCommands();

	//Update backend stuff
	RCore_updateMetrics();
	RCore_checkForModifiedCvars();
}
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Called in c_main.c. Called by the main thread at the end of the main loop
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void RCore_End()
{
	//Sync the render process thread and dispatched gl computes
	//and upload data to gpu
	//Compute_Sync();
	Compute_DispatchAll();
	//CPU SYNC

	//Upload data to gpu
	RCore_UploadGpuData();

	//Perform main rendering pass
	metrics.total_render_frame_count++;
	Pass_Main();

	//Perform cleanup at the end of the main rendering passes
	RCore_EndFrameCleanup();
	
}