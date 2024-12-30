#include "r_core.h"

#include "core/core_common.h"
#include "render/shaders/shader_info.h"

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Performs all the passes that make up the scene
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
extern RDraw_DrawData* drawData;
extern RPass_PassData* pass;
extern R_BackendData* backend_data;
extern R_Cvars r_cvars;
extern R_Scene scene;

extern void Render_OpaqueScene(RenderPassState rpass_state);
extern void Render_SemiOpaqueScene(RenderPassState rpass_state);
extern void Render_Quad();
extern void Render_Cube();
extern void Render_SimpleScene();
extern void Render_DrawChunkOcclusionBoxes();
extern void Render_UI();
extern void Render_WorldWaterChunks();
extern void Init_SetupGLBindingPoints();


static void Pass_GetRoundedDispatchGroupsForScreen(float p_x, float p_y, float p_xLocal, float p_yLocal, unsigned* r_x, unsigned* r_y)
{
	*r_x = ceilf(p_x / p_xLocal);
	*r_y = ceilf(p_y / p_yLocal);
}

static void Pass_DispatchScreenCompute(float p_width, float p_height, float p_xLocal, float p_yLocal)
{
	GLuint x = ceilf(p_width / p_xLocal);
	GLuint y = ceilf(p_height / p_yLocal);

	glDispatchCompute(x, y, 1);
}


static void Pass_shadowMap()
{
	if (!r_cvars.r_useDirShadowMapping->int_value)
		return;

	bool allow_transparent_shadows = (r_cvars.r_allowTransparentShadows->int_value == 1);
	bool allow_particle_shadows = (r_cvars.r_allowParticleShadows->int_value == 1);

	glBindFramebuffer(GL_FRAMEBUFFER, pass->shadow.FBO);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, pass->shadow.shadow_map_size, pass->shadow.shadow_map_size);
	glDisable(GL_BLEND);
	glCullFace(GL_FRONT);
	glEnable(GL_POLYGON_OFFSET_FILL);

	int splits = r_cvars.r_shadowSplits->int_value;

	for (int i = 0; i < splits; i++)
	{
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pass->shadow.depth_maps, 0, i);
		glClear(GL_DEPTH_BUFFER_BIT);

		Render_OpaqueScene(RPass__SHADOW_MAPPING_SPLIT1 + i);

		if (allow_transparent_shadows)
		{
			Render_SemiOpaqueScene(RPass__SHADOW_MAPPING_SPLIT1 + i);
		}
	}
	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_DEPTH_CLAMP);

	glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);

}

static void Pass_DepthPrepass()
{
	glDepthFunc(GL_LESS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	//draw opaque scene
	Render_OpaqueScene(RPass__DEPTH_PREPASS);

	//draw semi opaque scene
	Render_SemiOpaqueScene(RPass__DEPTH_PREPASS);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

static void Pass_gBuffer()
{
	//make sure we disable blend, even if we disabled it before
	glDisable(GL_BLEND);
	
	//We did a depth pre pass so set to equal
	glDepthFunc(GL_EQUAL);

	//draw opaque scene
	Render_OpaqueScene(RPass__GBUFFER);

	//draw semi opaque scene
	Render_SemiOpaqueScene(RPass__GBUFFER);

	glDepthFunc(GL_LESS);
}

static void Pass_downsampleDepthNormal()
{	
	if (r_cvars.r_ssaoHalfSize->int_value || r_cvars.r_enableGodrays->int_value);
	{
		//Downsample depth and normal
		glNamedFramebufferReadBuffer(pass->deferred.FBO, GL_COLOR_ATTACHMENT0);
		glNamedFramebufferDrawBuffer(pass->general.halfsize_fbo, GL_COLOR_ATTACHMENT0);

		glBlitNamedFramebuffer(pass->deferred.FBO, pass->general.halfsize_fbo, 0, 0, backend_data->screenSize[0], backend_data->screenSize[1], 0, 0,
			backend_data->halfScreenSize[0], backend_data->halfScreenSize[1], GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
}



static void Pass_SSAO()
{
	if (!r_cvars.r_useSsao->int_value)
	{
		return;
	}
		
	int viewport_width = backend_data->screenSize[0];
	int viewport_height = backend_data->screenSize[1];

	unsigned normal_tex = pass->deferred.gNormalMetal_texture;
	unsigned depth_tex = pass->deferred.depth_texture;
	if (r_cvars.r_ssaoHalfSize->int_value)
	{
		viewport_width >>= 1;
		viewport_height >>= 1;

		normal_tex = pass->general.normal_halfsize_texture;
		depth_tex = pass->general.depth_halfsize_texture;
	}

	//ACCUMULATION PASS
	Shader_ResetDefines(&pass->ao.shader);
	Shader_SetDefine(&pass->ao.shader, SSAO_DEFINE_PASS_ACCUM, true);

	Shader_Use(&pass->ao.shader);

	Shader_SetFloat2(&pass->ao.shader, SSAO_UNIFORM_VIEWPORTSIZE, viewport_width, viewport_height);
	Shader_SetFloaty(&pass->ao.shader, SSAO_UNIFORM_RADIUS, r_cvars.r_ssaoRadius->float_value);
	Shader_SetFloaty(&pass->ao.shader, SSAO_UNIFORM_BIAS, r_cvars.r_ssaoBias->float_value);
	Shader_SetFloaty(&pass->ao.shader, SSAO_UNIFORM_STRENGTH, r_cvars.r_ssaoStrength->float_value);

	glBindTextureUnit(0, depth_tex);
	glBindTextureUnit(1, normal_tex);
	glBindTextureUnit(2, pass->ao.noise_texture);
	
	glBindImageTexture(0, pass->ao.ao_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

	Pass_DispatchScreenCompute(viewport_width, viewport_height, 8, 8);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//FIRST BLUR PASS
	Shader_ResetDefines(&pass->ao.shader);
	Shader_SetDefine(&pass->ao.shader, SSAO_DEFINE_PASS_BLUR, true);

	Shader_Use(&pass->ao.shader);

	Shader_SetFloat2(&pass->ao.shader, SSAO_UNIFORM_VIEWPORTSIZE, backend_data->screenSize[0], backend_data->screenSize[1]);
	Shader_SetInt(&pass->ao.shader, SSAO_UNIFORM_SECONDPASS, false);

	glBindTextureUnit(3, pass->ao.ao_texture);

	glBindImageTexture(0, pass->ao.back_ao_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

	Pass_DispatchScreenCompute(viewport_width, viewport_height, 8, 8);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//SECOND BLUR PASS
	Shader_SetInt(&pass->ao.shader, SSAO_UNIFORM_SECONDPASS, true);

	glBindTextureUnit(3, pass->ao.back_ao_texture);

	glBindImageTexture(0, pass->ao.ao_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

	Pass_DispatchScreenCompute(viewport_width, viewport_height, 8, 8);

	//UPSAMPLE PASS (if needed)
	if (r_cvars.r_ssaoHalfSize->int_value)
	{
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		Shader_ResetDefines(&pass->ao.shader);
		Shader_SetDefine(&pass->ao.shader, SSAO_DEFINE_PASS_UPSAMPLE, true);
		
		Shader_Use(&pass->ao.shader);

		Shader_SetFloat2(&pass->ao.shader, SSAO_UNIFORM_VIEWPORTSIZE, backend_data->screenSize[0], backend_data->screenSize[1]);

		glBindTextureUnit(4, pass->ao.ao_texture);
		glBindTextureUnit(5, pass->ao.ao_texture);
		glBindTextureUnit(6, pass->deferred.depth_texture);

		glBindImageTexture(0, pass->ao.back_ao_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

		glBindSampler(5, pass->ao.linear_sampler);

		Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);

		glBindSampler(5, 0);
	}
}

static void Pass_SSIL()
{
	glUseProgram(pass->ssil.shader);
	
	Shader_SetInteger(pass->ssil.shader, "depth_texture", 0);
	Shader_SetInteger(pass->ssil.shader, "light_buffer", 1);
	Shader_SetVector2f(pass->ssil.shader, "u_viewportSize", backend_data->screenSize[0], backend_data->screenSize[1]);
	
	glBindTextureUnit(0, pass->deferred.depth_texture);
	glBindTextureUnit(1, pass->water.refraction_texture);

	glBindImageTexture(0, pass->ssil.output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);
}


static void Pass_BloomTextureSample()
{
	if (!r_cvars.r_useBloom->int_value)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, pass->bloom.FBO);

	glClear(GL_COLOR_BUFFER_BIT);
	//Downsample
	glUseProgram(pass->general.downsample_shader);

	Shader_SetVector2f(pass->general.downsample_shader, "u_srcResolution", backend_data->screenSize[0], backend_data->screenSize[1]);
	Shader_SetInteger(pass->general.downsample_shader, "u_mipLevel", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pass->scene.MainSceneColorBuffer);
	glDisable(GL_BLEND);

	for (int i = 0; i < BLOOM_MIP_COUNT; i++)
	{
		glViewport(0, 0, pass->bloom.mip_sizes[i][0], pass->bloom.mip_sizes[i][1]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, pass->bloom.mip_textures[i], 0);

		Render_Quad();

		Shader_SetVector2f(pass->general.downsample_shader, "u_srcResolution", pass->bloom.mip_sizes[i][0], pass->bloom.mip_sizes[i][1]);

		glBindTexture(GL_TEXTURE_2D, pass->bloom.mip_textures[i]);

		if (i == 0) Shader_SetInteger(pass->general.downsample_shader, "u_mipLevel", 1);
	}

	//Upsample
	glUseProgram(pass->general.upsample_shader);
	Shader_SetFloat(pass->general.upsample_shader, "u_filterRadius", 0.01);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	for (int i = BLOOM_MIP_COUNT - 1; i > 0; i--)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pass->bloom.mip_textures[i]);

		glViewport(0, 0, pass->bloom.mip_sizes[i - 1][0], pass->bloom.mip_sizes[i - 1][1]);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, pass->bloom.mip_textures[i - 1], 0);

		Render_Quad();
	}

	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
}

static void Pass_DepthOfField()
{
	if (!r_cvars.r_useDepthOfField->int_value)
		return;
	
	//CIRCLE OF CONFUSION PASS. Store near and far depths in scene's alpha channel 
	Shader_ResetDefines(&pass->dof.shader);

	Shader_SetDefine(&pass->dof.shader, DOF_DEFINE_COC_PASS, true);
	Shader_Use(&pass->dof.shader);

	Shader_SetFloat2(&pass->dof.shader, DOF_UNIFORM_VIEWPORTSIZE, backend_data->screenSize[0], backend_data->screenSize[1]);
	Shader_SetFloaty(&pass->dof.shader, DOF_UNIFORM_BLUR_SIZE, 8.0);

	glBindTextureUnit(0, pass->deferred.depth_texture);
	glBindImageTexture(0, pass->scene.MainSceneColorBuffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	Shader_ResetDefines(&pass->dof.shader);
	
	int quality = (r_cvars.r_DepthOfFieldQuality->int_value);
	bool use_circle_blur = (r_cvars.r_DepthOfFieldMode->int_value == 1);
	bool composite_pass = false;

	glBindTextureUnit(1, pass->scene.MainSceneColorBuffer);
	glBindImageTexture(0, pass->dof.dof_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	//BLURRING PASS. Use the circle of confusion from scene's alpha texture to blur into a blurred texture
	if (use_circle_blur)
	{
		composite_pass = true;

		static const float blur_scales[2] = { 8.0, 1.0 };

		float scale = blur_scales[quality];

		Shader_SetDefine(&pass->dof.shader, DOF_DEFINE_BOKEH_CIRCULAR, true);

		Shader_Use(&pass->dof.shader);

		Shader_SetFloat2(&pass->dof.shader, DOF_UNIFORM_VIEWPORTSIZE, backend_data->halfScreenSize[0], backend_data->halfScreenSize[1]);
		Shader_SetFloaty(&pass->dof.shader, DOF_UNIFORM_BLUR_SCALE, scale);
		Shader_SetFloaty(&pass->dof.shader, DOF_UNIFORM_BLUR_SIZE, 8.0);
	
		Pass_DispatchScreenCompute(backend_data->halfScreenSize[0], backend_data->halfScreenSize[1], 8, 8);
	}
	else
	{
		static const int blur_steps[2] = {6, 24};

		int steps = blur_steps[quality];

		//box blur
		Shader_SetDefine(&pass->dof.shader, DOF_DEFINE_BOKEH_BOX, true);
		Shader_Use(&pass->dof.shader);
		
		//first pass
		Shader_SetInt(&pass->dof.shader, DOF_UNIFORM_SECOND_PASS, false);
		Shader_SetFloat2(&pass->dof.shader, DOF_UNIFORM_VIEWPORTSIZE, backend_data->screenSize[0], backend_data->screenSize[1]);
		Shader_SetInt(&pass->dof.shader, DOF_UNIFORM_BLUR_STEPS, steps);
		Shader_SetFloaty(&pass->dof.shader, DOF_UNIFORM_BLUR_SIZE, 8.0);

		Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//second pass
		Shader_SetInt(&pass->dof.shader, DOF_UNIFORM_SECOND_PASS, true);
		glBindTextureUnit(1, pass->dof.dof_texture);
		glBindImageTexture(0, pass->scene.MainSceneColorBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

		Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);
	}
		
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	
	if (composite_pass)
	{
		//COMPOSITE PASS. Combine scene's texture with the blurred texture
		Shader_ResetDefines(&pass->dof.shader);

		Shader_SetDefine(&pass->dof.shader, DOF_DEFINE_COMPOSITE, true);
		Shader_Use(&pass->dof.shader);
		Shader_SetFloat2(&pass->dof.shader, DOF_UNIFORM_VIEWPORTSIZE, backend_data->screenSize[0], backend_data->screenSize[1]);

		glBindTextureUnit(2, pass->dof.dof_texture);
		glBindImageTexture(0, pass->scene.MainSceneColorBuffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);
	}
}


static void Pass_Godrays()
{
	if (!r_cvars.r_enableGodrays->int_value)
	{
		return;
	}

	Shader_ResetDefines(&pass->godray.shader);

	//RAYMARCHING PASS
	Shader_SetDefine(&pass->godray.shader, GODRAY_DEFINE_RAYMARCH_PASS, true);
	
	Shader_Use(&pass->godray.shader);

	Shader_SetFloat2(&pass->godray.shader, GODRAY_UNIFORM_VIEWPORTSIZE, backend_data->halfScreenSize[0], backend_data->halfScreenSize[1]);
	Shader_SetInt(&pass->godray.shader, GODRAY_UNIFORM_MAXSTEPS, 8);
	Shader_SetFloaty(&pass->godray.shader, GODRAY_UNIFORM_SCATTERING, 0.8);

	glBindTextureUnit(0, pass->general.depth_halfsize_texture);
	glBindTextureUnit(3, pass->shadow.depth_maps);
	glBindImageTexture(0, pass->godray.godray_fog_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);

	Pass_DispatchScreenCompute(backend_data->halfScreenSize[0], backend_data->halfScreenSize[1], 8, 8);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	Shader_ResetDefines(&pass->godray.shader);
	//FIRST BLUR PASS
	Shader_SetDefine(&pass->godray.shader, GODRAY_DEFINE_BLUR_PASS, true);

	Shader_Use(&pass->godray.shader);

	Shader_SetFloat2(&pass->godray.shader, GODRAY_UNIFORM_VIEWPORTSIZE, backend_data->halfScreenSize[0], backend_data->halfScreenSize[1]);
	Shader_SetInt(&pass->godray.shader, GODRAY_UNIFORM_SECONDPASS, false);

	glBindTextureUnit(0, pass->general.depth_halfsize_texture);
	glBindTextureUnit(2, pass->godray.godray_fog_texture);
	glBindImageTexture(0, pass->godray.back_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);

	Pass_DispatchScreenCompute(backend_data->halfScreenSize[0], backend_data->halfScreenSize[1], 8, 8);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	
	//SECOND BLUR PASS
	Shader_SetInt(&pass->godray.shader, GODRAY_UNIFORM_SECONDPASS, true);
	glBindTextureUnit(2, pass->godray.back_texture);
	glBindImageTexture(0, pass->godray.godray_fog_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);

	Pass_DispatchScreenCompute(backend_data->halfScreenSize[0], backend_data->halfScreenSize[1], 8, 8);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	Shader_ResetDefines(&pass->godray.shader);
	//COMPOSITE PASS
	Shader_SetDefine(&pass->godray.shader, GODRAY_DEFINE_COMPOSITE_PASS, true);

	Shader_Use(&pass->godray.shader);
	
	Shader_SetFloat2(&pass->godray.shader, GODRAY_UNIFORM_VIEWPORTSIZE, backend_data->screenSize[0], backend_data->screenSize[1]);
	
	glBindTextureUnit(0, pass->general.depth_halfsize_texture);
	glBindTextureUnit(1, pass->deferred.depth_texture);
	glBindTextureUnit(2, pass->godray.godray_fog_texture);
	glBindImageTexture(0, pass->scene.MainSceneColorBuffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

	Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);
}

static void Pass_deferredShading()
{	
	//make sure ao finishes
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);

	//use shader
	glUseProgram(pass->deferred.shading_shader);

	bool use_simple_cubemap_for_reflection = true;

	//setup textures
	glBindTextureUnit(0, pass->deferred.gNormalMetal_texture);
	glBindTextureUnit(1, pass->deferred.gColorRough_texture);
	glBindTextureUnit(2, pass->deferred.depth_texture);
	glBindTextureUnit(3, (r_cvars.r_ssaoHalfSize->int_value == 0) ? pass->ao.ao_texture : pass->ao.back_ao_texture);
	glBindTextureUnit(4, pass->shadow.depth_maps);
	
	//IBL Textures
	glBindTextureUnit(5, pass->ibl.brdfLutTexture);
	glBindTextureUnit(6, pass->ibl.irradianceCubemapTexture);
	glBindTextureUnit(7, (use_simple_cubemap_for_reflection) ? pass->ibl.envCubemapTexture : pass->ibl.prefilteredCubemapTexture);

	//draw quad
	Render_Quad();

	glEnable(GL_DEPTH_TEST);
}

static void Pass_ProcessCubemap()
{
	glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.RBO);
	glViewport(0, 0, 512, 512);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);

	glUseProgram(pass->ibl.sky_compute_shader);
	glBindTextureUnit(0, pass->general.perlin_noise_texture);
	glBindTextureUnit(1, pass->ibl.nightTexture);
	Shader_SetMatrix4(pass->ibl.sky_compute_shader, "u_proj", pass->ibl.cube_proj);

	Shader_SetVector3f_2(pass->ibl.sky_compute_shader, "u_skyColor", scene.environment.sky_color);
	Shader_SetVector3f_2(pass->ibl.sky_compute_shader, "u_skyHorizonColor", scene.environment.sky_horizon_color);
	Shader_SetVector3f_2(pass->ibl.sky_compute_shader, "u_groundHorizonColor", scene.environment.ground_horizon_color);
	Shader_SetVector3f_2(pass->ibl.sky_compute_shader, "u_groundColor", scene.environment.ground_bottom_color);

	static float time = 0;
	time += Core_getDeltaTime();

	Shader_SetFloat(pass->ibl.sky_compute_shader, "u_time", time);
	for (int i = 0; i < 6; i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.envCubemapTexture, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Shader_SetMatrix4(pass->ibl.sky_compute_shader, "u_view", pass->ibl.cube_view_matrixes[i]);
		Render_Cube();
	}
	static bool dirty = true;
	if(dirty)
	{
		RInternal_ProcessIBLCubemap(false, true, true, true);
		dirty = false;
	}
	else
	{
		RInternal_ProcessIBLCubemap(true, true, false, false);
	}
	if (pass->ibl.update_brdf || pass->ibl.update_irradiance || pass->ibl.update_prefilter)
	{
		RInternal_ProcessIBLCubemap(false, pass->ibl.update_irradiance, pass->ibl.update_prefilter, pass->ibl.update_brdf);

		pass->ibl.update_brdf = false;
		pass->ibl.update_irradiance = false;
		pass->ibl.update_prefilter = false;
	}

	//RESET VIEWPROT
	glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
}

static void Pass_Skybox()
{
	if (!r_cvars.r_drawSky->int_value)
		return;

	Pass_ProcessCubemap();
	glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);

	mat4 view_no_translation;
	glm_mat4_identity(view_no_translation);
	glm_mat3_make(scene.camera.view, view_no_translation);

	view_no_translation[2][0] = scene.camera.view[2][0];
	view_no_translation[2][1] = scene.camera.view[2][1];
	view_no_translation[2][2] = scene.camera.view[2][2];
	view_no_translation[2][3] = scene.camera.view[2][3];

	glUseProgram(pass->scene.skybox_shader);
	Shader_SetMatrix4(pass->scene.skybox_shader, "u_proj", scene.camera.proj);
	Shader_SetMatrix4(pass->scene.skybox_shader, "u_view", view_no_translation);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	Render_Cube();
	
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}


static void Pass_Transparents()
{	
	//Render Transparent objects

	glEnable(GL_CULL_FACE);
	//Render_TransparentScene(RPS__STANDART_PASS);

	glEnable(GL_BLEND);
	glDisable(GL_BLEND);
	//Render other objects
	//render water
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(GL_TRUE);
	Render_WorldWaterChunks();



	//render particles
	//glUseProgram(drawData->particles.particle_render_shader);
	//Render_Particles();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

}

static void Pass_WaterPrePass()
{
	//only bother rendering if we have any water chunks in frustrum
	if (scene.cull_data.lc_world.water_in_frustrum <= 0)
	{
		return;
	}

	//copy main scene's buffer to the refraction texture
	glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glBindTexture(GL_TEXTURE_2D, pass->water.refraction_texture);
	//glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);

	//Render into the reflection texture
	glBindFramebuffer(GL_FRAMEBUFFER, pass->water.reflection_fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, pass->water.reflection_rbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pass->shadow.blur_shader);
	glBindTextureUnit(0, pass->scene.MainSceneColorBuffer);
	glBindImageTexture(0, pass->water.refraction_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	Pass_DispatchScreenCompute(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8);


	bool viewport_change = (r_cvars.r_waterReflectionQuality->int_value < 2);
	if (viewport_change)
	{
		glViewport(0, 0, pass->water.reflection_size[0], pass->water.reflection_size[1]);
	}

	//Render skybox
	mat4 view_no_translation;
	glm_mat4_identity(view_no_translation);
	glm_mat3_make(pass->water.reflection_view_matrix, view_no_translation);

	view_no_translation[2][0] = pass->water.reflection_view_matrix[2][0];
	view_no_translation[2][1] = pass->water.reflection_view_matrix[2][1];
	view_no_translation[2][2] = pass->water.reflection_view_matrix[2][2];
	view_no_translation[2][3] = pass->water.reflection_view_matrix[2][3];

	glUseProgram(pass->scene.skybox_shader);
	Shader_SetMatrix4(pass->scene.skybox_shader, "u_proj", scene.camera.proj);
	Shader_SetMatrix4(pass->scene.skybox_shader, "u_view", view_no_translation);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	Render_Cube();
	
	glDrawBuffer(GL_COLOR_ATTACHMENT0);	
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glEnable(GL_CLIP_DISTANCE0);
	
	//Render scene
	Render_OpaqueScene(RPass__REFLECTION_CLIP);
	Render_SemiOpaqueScene(RPass__REFLECTION_CLIP);

	glDisable(GL_CLIP_DISTANCE0);

	if (viewport_change)
	{
		glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
	}

	//Blur the reflection texture
	glUseProgram(pass->general.box_blur_shader);
	glBindTextureUnit(0, pass->water.reflection_texture);
	glBindImageTexture(0, pass->water.reflection_back_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	Shader_SetInteger(pass->general.box_blur_shader, "u_size", 4);
	Shader_SetVector2f(pass->general.box_blur_shader, "u_dir", 0.0, 1.0);
	Shader_SetFloat(pass->general.box_blur_shader, "u_blurScale", 1);

	Pass_DispatchScreenCompute(pass->water.reflection_size[0], pass->water.reflection_size[1], 8, 8);

	glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);
}

static void Pass_OcclussionBoxes()
{
	glUseProgram(pass->lc.occlussion_boxes_shader);
	glDisable(GL_CULL_FACE); //always disable cull face, causes lots of issues otherwise
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -1);

	Render_DrawChunkOcclusionBoxes();

	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0, 0);
}

static void Pass_SimpleGeoPass()
{
	Render_SimpleScene();
}

static void Pass_PostProcess()
{
	//draw the debug texture if needed
	if (r_cvars.r_drawDebugTexture->int_value > -1)
	{
		//-1 disabled, 0 = Normal, 1 = Albedo, 2 = Depth, 3 = Metal, 4 = Rough, 5 = AO, 6 = Bloom
		glUseProgram(pass->post.debug_shader);

		Shader_SetInteger(pass->post.debug_shader, "u_selection", r_cvars.r_drawDebugTexture->int_value);

		glBindTextureUnit(0, pass->deferred.gNormalMetal_texture);
		glBindTextureUnit(1, pass->deferred.gColorRough_texture);
		glBindTextureUnit(2, pass->deferred.depth_texture);
		glBindTextureUnit(3, pass->ao.ao_texture);
	}
	//Normal post process pass
	else
	{
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindTextureUnit(0, pass->deferred.depth_texture);
		glBindTextureUnit(1, pass->scene.MainSceneColorBuffer);
		glBindTextureUnit(2, pass->bloom.mip_textures[0]);

		Shader_SetDefine(&pass->post.post_process_shader, POST_PROCESS_DEFINE_USE_FXAA, r_cvars.r_useFxaa->int_value);
		Shader_SetDefine(&pass->post.post_process_shader, POST_PROCESS_DEFINE_USE_BLOOM, r_cvars.r_useBloom->int_value);
		Shader_SetDefine(&pass->post.post_process_shader, POST_PROCESS_DEFINE_USE_REINHARD_TONEMAP, r_cvars.r_TonemapMode->int_value == 0);
		Shader_SetDefine(&pass->post.post_process_shader, POST_PROCESS_DEFINE_USE_UNCHARTED2_TONEMAP, r_cvars.r_TonemapMode->int_value == 1);
		Shader_SetDefine(&pass->post.post_process_shader, POST_PROCESS_DEFINE_USE_ACES_TONEMAP, r_cvars.r_TonemapMode->int_value == 2);
		Shader_SetDefine(&pass->post.post_process_shader, POST_PROCESS_DEFINE_USE_FOG, scene.environment.depthFogEnabled || scene.environment.heightFogEnabled);

		Shader_Use(&pass->post.post_process_shader);
		
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_BLOOMSTRENGTH, r_cvars.r_bloomStrength->float_value);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_CONTRAST, r_cvars.r_Contrast->float_value);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_SATURATION, r_cvars.r_Saturation->float_value);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_BRIGHTNESS, r_cvars.r_Brightness->float_value);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_GAMMA, r_cvars.r_Gamma->float_value);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_EXPOSURE, r_cvars.r_Exposure->float_value);

		Shader_SetInt(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_HEIGHTFOGENABLED, scene.environment.heightFogEnabled);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_HEIGHTFOGMIN, scene.environment.heightFogMin);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_HEIGHTFOGMAX, scene.environment.heightFogMax);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_HEIGHTFOGCURVE, scene.environment.heightFogCurve);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_HEIGHTFOGDENSITY, scene.environment.heightFogDensity);
		
		Shader_SetInt(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_DEPTHFOGENABLED, scene.environment.depthFogEnabled);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_DEPTHFOGBEGIN, scene.environment.depthFogBegin);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_DEPTHFOGEND, scene.environment.depthFogEnd);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_DEPTHFOGCURVE, scene.environment.depthFogCurve);
		Shader_SetFloaty(&pass->post.post_process_shader, POST_PROCESS_UNIFORM_DEPTHFOGDENSITY, scene.environment.depthFogDensity);
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);
	Render_Quad();
}

static void Pass_UI()
{
	Render_UI();
}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
High level function that goes through all the passes in sequence
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void Pass_Main()
{
	glBindFramebuffer(GL_FRAMEBUFFER, pass->deferred.FBO);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Setup pre opaque render gl state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	
	//depth prepass opaques and semi opaques
	Pass_DepthPrepass();

	///Render all opaque and semi opaque objects into g buffers
	Pass_gBuffer();
	
	//shadow mapping pass
	Pass_shadowMap();

	glDisable(GL_CULL_FACE);

	//Downsample depth and normal(if needed)
	Pass_downsampleDepthNormal();
	
	//SSAO pass, blur pass
	Pass_SSAO();

	//Pass_SSIL();

	//Draw into the main's screen buffer
	//The scene's framebuffer uses deferred's depth texture
	glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);
	glClear(GL_COLOR_BUFFER_BIT);

	//Deferred shading pass
	Pass_deferredShading();
	
	//Render skybox
	Pass_Skybox();

	//Render boxes for oclussion culling
	Pass_OcclussionBoxes();

	//Water render prepass stuff
	Pass_WaterPrePass();

	//Render all transparent objects
	Pass_Transparents();

	glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);
	
	glEnable(GL_DEPTH_TEST);
	//Draw simple scene
	Pass_SimpleGeoPass();
	
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	//Blur the main scene buffer for Depth of field effects
	Pass_DepthOfField();
	
	//Perform godrays
	Pass_Godrays();

	//Bind to the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Downsample and then upsample the scene's color buffer for bloom effect
	Pass_BloomTextureSample();
	
	//Post process pass
	Pass_PostProcess();

	//Render UI and other top level sprites
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	Pass_UI();
}