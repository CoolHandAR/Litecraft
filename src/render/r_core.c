/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Main core of the rendering pipeline. Holds all the rendering data.
Start and ends the frame
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "render/r_core.h"

#include "core/input.h"
#include "render/r_public.h"

#include "core/core_common.h"


R_CMD_Buffer* cmdBuffer;
RDraw_DrawData* drawData;
RPass_PassData* pass;
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
extern void RPanel_Metrics();

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Updates metrics and sets if we should skip this frame (if fps limit is on)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/*
static void RCore_updateMetrics()
{
	backend_data->skip_frame = false;

	metrics.frame_ticks_per_second++;
	float new_time = glfwGetTime();
	
	metrics.frame_time = new_time - metrics.previous_render_time;
	metrics.previous_render_time = new_time;

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
*/
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->deferred.depth_texture, 0);
	//NORMAL
	glBindTexture(GL_TEXTURE_2D, pass->deferred.gNormalMetal_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->scene.MainSceneColorBuffer, 0);

	//AO
	glBindTexture(GL_TEXTURE_2D, pass->ao.ao_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glClearTexImage(pass->ao.ao_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, pass->ao.back_ao_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glClearTexImage(pass->ao.back_ao_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

	unsigned char* red_buffer = malloc(width * height);
	if (red_buffer)
	{
		memset(red_buffer, 1, width * height);
		glClearTexImage(pass->ao.ao_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, red_buffer);
		glClearTexImage(pass->ao.back_ao_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, red_buffer);

		free(red_buffer);
	}


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

	//DOF
	glBindTexture(GL_TEXTURE_2D, pass->dof.dof_texture);
	if (r_cvars.r_DepthOfFieldMode->int_value == 1)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / 2, height / 2, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
	}

	//GODRAYS
	glBindTexture(GL_TEXTURE_2D, pass->godray.godray_fog_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width / 2, height / 2, 0, GL_RED, GL_HALF_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, pass->godray.back_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width / 2, height / 2, 0, GL_RED, GL_HALF_FLOAT, NULL);

	//WATER REFLECTION
	int reflection_width = width;
	int reflection_height = height;

	if (r_cvars.r_waterReflectionQuality->int_value < 2)
	{
		if (r_cvars.r_waterReflectionQuality->int_value == 1)
		{
			//half size
			reflection_width /= 2;
			reflection_height /= 2;
		}
		else
		{
			//quarter size
			reflection_width /= 4;
			reflection_height /= 4;
		}
	}
	glBindTexture(GL_TEXTURE_2D, pass->water.reflection_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, reflection_width, reflection_height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, pass->water.reflection_back_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, reflection_width, reflection_height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glBindRenderbuffer(GL_RENDERBUFFER, pass->water.reflection_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, reflection_width, reflection_height);

	pass->water.reflection_size[0] = reflection_width;
	pass->water.reflection_size[1] = reflection_height;

	glBindTexture(GL_TEXTURE_2D, pass->water.refraction_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	//General
	glBindTexture(GL_TEXTURE_2D, pass->general.depth_halfsize_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width/ 2, height / 2, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	
	glBindTexture(GL_TEXTURE_2D, pass->general.normal_halfsize_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width / 2, height / 2, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, pass->general.halfsize_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->general.normal_halfsize_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass->general.depth_halfsize_texture, 0);

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
	if (r_cvars.r_useDepthOfField->modified || r_cvars.r_DepthOfFieldMode->modified)
	{
		RCore_onWindowResize(backend_data->screenSize[0], backend_data->screenSize[1]);
		r_cvars.r_useDepthOfField->modified = false;
		r_cvars.r_DepthOfFieldMode->modified = false;
	}
	if (r_cvars.r_useSsao->modified || r_cvars.r_ssaoHalfSize->modified)
	{
		RCore_onWindowResize(backend_data->screenSize[0], backend_data->screenSize[1]);

		r_cvars.r_useSsao->modified = false;
		r_cvars.r_ssaoHalfSize->modified = false;
	}
	if (r_cvars.r_useBloom->modified)
	{
		if (r_cvars.r_useBloom->modified)
		{
			RCore_onWindowResize(backend_data->screenSize[0], backend_data->screenSize[1]);
		}

		r_cvars.r_useBloom->modified = false;
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
	if (r_cvars.r_shadowQualityLevel->modified)
	{	
		float shadow_size = 256;
		RInternal_GetShadowQualityData(r_cvars.r_shadowQualityLevel->int_value, r_cvars.r_shadowBlurLevel->int_value, &shadow_size, NULL, NULL, NULL);
		
		glBindTexture(GL_TEXTURE_2D_ARRAY, pass->shadow.depth_maps);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT16, shadow_size, shadow_size, SHADOW_CASCADE_LEVELS, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

		glBindFramebuffer(GL_FRAMEBUFFER, pass->shadow.FBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pass->shadow.depth_maps, 0);

		pass->shadow.shadow_map_size = shadow_size;
		r_cvars.r_shadowQualityLevel->modified = false;
	}
	if (r_cvars.r_shadowBlurLevel->modified || r_cvars.r_useSsao->modified || r_cvars.r_useDirShadowMapping->modified)
	{
		RInternal_GetShadowQualityData(r_cvars.r_shadowQualityLevel->int_value, r_cvars.r_shadowBlurLevel->int_value, NULL, pass->shadow.shadow_sample_kernels,
			&pass->shadow.num_shadow_sample_kernels, &pass->shadow.quality_radius_scale);

		Shader_SetDefine(&pass->deferred.shading_shader, DEFERRED_SCENE_DEFINE_USE_DIR_SHADOWS, r_cvars.r_useDirShadowMapping->int_value == 1);
		Shader_SetDefine(&pass->deferred.shading_shader, DEFERRED_SCENE_DEFINE_USE_SSAO, r_cvars.r_useSsao->int_value == 1);

		Shader_Use(&pass->deferred.shading_shader);

		int loc = Shader_GetUniformLocation(&pass->deferred.shading_shader, DEFERRED_SCENE_UNIFORM_SHADOWSAMPLEKERNELS);

		if (loc > -1)
		{
			glUniform2fv(loc, pass->shadow.num_shadow_sample_kernels, pass->shadow.shadow_sample_kernels);
		}

		Shader_SetFloaty(&pass->deferred.shading_shader, DEFERRED_SCENE_UNIFORM_SHADOWQUALITYRADIUS, pass->shadow.quality_radius_scale);
		Shader_SetInt(&pass->deferred.shading_shader, DEFERRED_SCENE_UNIFORM_SHADOWSAMPLEAMOUNT, pass->shadow.num_shadow_sample_kernels);

		r_cvars.r_shadowBlurLevel->modified = false;
	}
	if (r_cvars.r_waterReflectionQuality->modified)
	{
		RCore_onWindowResize(backend_data->screenSize[0], backend_data->screenSize[1]);
		r_cvars.r_waterReflectionQuality->modified = false;
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
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TriangleVertex) * drawData->triangle.vertices_count, drawData->triangle.vertices);
	}
	if (drawData->text.vertices_count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, drawData->text.vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TextVertex) * drawData->text.vertices_count, drawData->text.vertices);
	}
	if (drawData->cube.instance_count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, drawData->cube.instance_vbo);
		size_t size = sizeof(float) * (dA_size(drawData->cube.vertices_buffer));
		if (size > drawData->cube.allocated_size)
		{
			glBufferData(GL_ARRAY_BUFFER, size, dA_getFront(drawData->cube.vertices_buffer), GL_STREAM_DRAW);
			drawData->cube.allocated_size = size;
		}
		else
		{
			glBufferSubData(GL_ARRAY_BUFFER, 0, size, dA_getFront(drawData->cube.vertices_buffer));
		}
	}
	if (drawData->particles.instance_count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, drawData->particles.instance_vbo);
		size_t size = sizeof(float) * (dA_size(drawData->particles.instance_buffer));
		if (size > drawData->particles.allocated_size)
		{
			glBufferData(GL_ARRAY_BUFFER, size, dA_getFront(drawData->particles.instance_buffer), GL_STREAM_DRAW);
			drawData->particles.allocated_size = size;
		}
		else
		{
			glBufferSubData(GL_ARRAY_BUFFER, 0, size, dA_getFront(drawData->particles.instance_buffer));
		}
	}

	//Scene data
	if (drawData->lc_world.draw)
	{
		if (scene.cull_data.lc_world.total_in_frustrum_count > 0)
		{
			//glNamedBufferSubData(drawData->lc_world.world_render_data->visibles_sorted_ssbo, 0, sizeof(int) * scene.cull_data.lc_world.total_in_frustrum_count, scene.cull_data.lc_world.frustrum_sorted_query_buffer);
		}
		//glNamedBufferSubData(drawData->lc_world.world_render_data->shadow_chunk_indexes_ssbo, 0, sizeof(int) * drawData->lc_world.shadow_sorted_chunk_indexes->elements_size,
		//	drawData->lc_world.shadow_sorted_chunk_indexes->data);

		//glNamedBufferSubData(drawData->lc_world.world_render_data->shadow_chunk_indexes_ssbo, sizeof(int) * drawData->lc_world.shadow_sorted_chunk_indexes->elements_size, sizeof(int) * drawData->lc_world.shadow_sorted_chunk_transparent_indexes->elements_size,
			//drawData->lc_world.shadow_sorted_chunk_transparent_indexes->data);


		//glNamedBufferSubData(drawData->lc_world.world_render_data->visibles_buffer, 0, sizeof(int) * 500, scene.cull_data.lc_world.frustrum_query_buffer);
	}

	//Camera update
	glBindBuffer(GL_UNIFORM_BUFFER, scene.camera_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RScene_CameraData), &scene.camera);

	//Scene update
	scene.scene_data.time += Core_getDeltaTime();

	glBindBuffer(GL_UNIFORM_BUFFER, scene.scene_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RScene_UBOData), &scene.scene_data);

	if (scene.scene_data.numPointLights > 0)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, storage.point_lights.buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(PointLight) * scene.scene_data.numPointLights, storage.point_lights_backbuffer->data);
	}
	if (scene.scene_data.numSpotLights > 0)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, storage.spot_lights.buffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(SpotLight) * scene.scene_data.numSpotLights, storage.spot_lights_backbuffer->data);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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

	drawData->triangle.texture_index = 0;

	dA_clear(drawData->cube.vertices_buffer);
	drawData->cube.instance_count = 0;
	drawData->particles.instance_count = 0;
}

static void RCore_DrawUI()
{
	if (r_cvars.r_drawPanel->int_value)
	{
		RPanel_Main();
	}
	//RPanel_Metrics();
}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Called in c_main.c. Called by the main thread
Dispatches various computes, adds jobs to render thread, updates metrics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void RCore_Start()
{
	//BVH_Tree_DrawNodes(&scene.cull_data.static_partition_tree, NULL, NULL, false);

	//Render panel, metrics ui
	RCore_DrawUI();

	if (drawData->lc_world.world_render_data)
	{
		glNamedBufferSubData(drawData->lc_world.world_render_data->prev_in_frustrum_bitset_buffer, 0, sizeof(int) * 500, scene.cull_data.lc_world.frustrum_query_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, drawData->lc_world.world_render_data->chunk_data_buffer.buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, drawData->lc_world.world_render_data->visibles_sorted_buffer);
	}
	

	//Process various renderer tasks
	RCmds_processCommands();

	if (drawData->lc_world.world_render_data)
	{
		glNamedBufferSubData(drawData->lc_world.world_render_data->visibles_sorted_buffer, 0, sizeof(int) * scene.cull_data.lc_world.total_in_frustrum_count, scene.cull_data.lc_world.frustrum_sorted_query_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	}
	

	////Dispatch computes
	Compute_DispatchAll();

	//Update backend stuff
	//RCore_updateMetrics();
	RCore_checkForModifiedCvars();
}
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Called in c_main.c. Called by the main thread at the end of the main loop
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void RCore_End()
{
	if (Input_IsActionJustPressed("Open-panel") && !Con_isOpened())
	{
		Cvar_setValueDirectInt(r_cvars.r_drawPanel, !r_cvars.r_drawPanel->int_value);
	}
	//Sync the render process thread and dispatched gl computes
	//and upload data to gpu
	Compute_Sync();
	//CPU SYNC

	//Upload data to gpu
	RCore_UploadGpuData();

	//Perform main rendering pass
	metrics.total_render_frame_count++;
	Pass_Main();

	//Perform cleanup at the end of the main rendering passes
	RCore_EndFrameCleanup();
	
}
