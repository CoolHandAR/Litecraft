#include "r_core.h"

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Performs all the passes that make up the scene
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
extern R_DrawData* drawData;
extern R_RenderPassData* pass;
extern R_BackendData* backend_data;
extern R_Cvars r_cvars;
extern R_Scene scene;

extern void Render_OpaqueScene(RenderPassState rpass_state);
extern void Render_SemiOpaqueScene(RenderPassState rpass_state);
extern void Render_Quad();
extern void Render_Cube();
extern void Render_DebugScene();
extern void Render_Particles();
extern void Render_DrawChunkOcclusionBoxes();
extern void Render_UI();
extern void Init_SetupGLBindingPoints();


static void Pass_GetRoundedDispatchGroupsForScreen(float p_x, float p_y, float p_xLocal, float p_yLocal, unsigned* r_x, unsigned* r_y)
{
	*r_x = ceilf(p_x / p_xLocal);
	*r_y = ceilf(p_y / p_yLocal);
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
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	glDisable(GL_BLEND);
	glCullFace(GL_FRONT);
	glEnable(GL_POLYGON_OFFSET_FILL);

	for (int i = 0; i < 4; i++)
	{
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pass->shadow.moment_maps, 0, i);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pass->shadow.depth_maps, 0, i);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Render_OpaqueScene(RPass__SHADOW_MAPPING_SPLIT1 + i);

		if (allow_transparent_shadows)
		{
			Render_SemiOpaqueScene(RPass__SHADOW_MAPPING_SPLIT1 + i);
		}
		if (allow_particle_shadows)
		{
			glUseProgram(pass->particles.particle_shadow_map_shader);
			Shader_SetMatrix4(pass->particles.particle_shadow_map_shader, "u_matrix", scene.scene_data.shadow_matrixes[i]);
			Render_Particles();
		}

	}
	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_DEPTH_CLAMP);

	glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
}

static void Pass_gBuffer()
{
	//make sure we disable blend, even if we disabled it before
	glDisable(GL_BLEND);
	
	//draw opaque scene
	Render_OpaqueScene(RPass__GBUFFER);

	//draw semi opaque scene
	Render_SemiOpaqueScene(RPass__GBUFFER);
}

static void Pass_downsample()
{
	glBlitNamedFramebuffer(pass->deferred.FBO, pass->general.halfsize_fbo, 0, 0, backend_data->screenSize[0], backend_data->screenSize[1], 0, 0,
			backend_data->halfScreenSize[0], backend_data->halfScreenSize[1], GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

static void Pass_AO()
{
	if (!r_cvars.r_useSsao->int_value)
		return;

	int viewport_width = backend_data->screenSize[0];
	int viewport_height = backend_data->screenSize[1];
	
	unsigned normal_tex = pass->deferred.gNormalMetal_texture;
	unsigned depth_tex = pass->deferred.depth_texture;
	if (r_cvars.r_ssaoHalfSize->int_value)
	{
		viewport_width >>= 1;
		viewport_height >>= 1;

		//Downsample depth and normal to half size
		Pass_downsample();
		
		normal_tex = pass->general.normal_halfsize_texture;
		depth_tex = pass->general.depth_halfsize_texture;
	}
	unsigned dispatch_x = 1;
	unsigned dispatch_y = 1;
	Pass_GetRoundedDispatchGroupsForScreen(viewport_width, viewport_height, 8, 8, &dispatch_x, &dispatch_y);

	//SSAO 
	glBindTextureUnit(0, depth_tex);
	glBindTextureUnit(1, pass->ao.noise_texture);
	glBindTextureUnit(2, normal_tex);

	glUseProgram(pass->ao.shader);
	Shader_SetVector2f(pass->ao.shader, "u_viewportSize", backend_data->screenSize[0], backend_data->screenSize[1]);
	Shader_SetFloat(pass->ao.shader, "u_uvMultiplier", (r_cvars.r_ssaoHalfSize->int_value) ? 2.0 : 1.0);

	glBindImageTexture(0, pass->ao.ao_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

	glDispatchCompute(dispatch_x, dispatch_y, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//Box Blur the ssao texture
	glUseProgram(pass->general.box_blur_shader);
	glBindTextureUnit(0, pass->ao.ao_texture);
	glBindImageTexture(0, pass->ao.blurred_ao_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
	Shader_SetInteger(pass->general.box_blur_shader, "u_size", 2);

	glDispatchCompute(dispatch_x, dispatch_y, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


	//Upsample the texture if needed
	if (r_cvars.r_ssaoHalfSize->int_value)
	{
		dispatch_x = 1;
		dispatch_y = 1;
		Pass_GetRoundedDispatchGroupsForScreen(backend_data->screenSize[0], backend_data->screenSize[1], 8, 8, &dispatch_x, &dispatch_y);

		glUseProgram(pass->general.copy_shader);
		glBindTextureUnit(0, pass->ao.blurred_ao_texture);
		glBindImageTexture(0, pass->ao.ao_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

		Shader_SetVector2f(pass->general.copy_shader, "u_viewportSize", backend_data->screenSize[0], backend_data->screenSize[1]);
		Shader_SetFloat(pass->general.copy_shader, "u_uvMultiplier", 0.5);

		glDispatchCompute(dispatch_x, dispatch_y, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
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
	Shader_SetFloat(pass->general.upsample_shader, "u_filterRadius", 0.1);

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

	glUseProgram(pass->general.box_blur_shader);
	
	glBindTextureUnit(0, pass->scene.MainSceneColorBuffer);
	glBindImageTexture(0, pass->dof.dof_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	Shader_SetInteger(pass->general.box_blur_shader, "u_size", 6);

	glDispatchCompute(backend_data->screenSize[0] / 8, backend_data->screenSize[1] / 8, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

static void Pass_deferredShading()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);

	//use shader
	glUseProgram(pass->deferred.shading_shader);

	//setup textures
	glBindTextureUnit(0, pass->deferred.gNormalMetal_texture);
	glBindTextureUnit(1, pass->deferred.gColorRough_texture);
	glBindTextureUnit(2, pass->deferred.depth_texture);
	glBindTextureUnit(3, (r_cvars.r_ssaoHalfSize->int_value == 1) ? pass->ao.ao_texture : pass->ao.blurred_ao_texture);
	glBindTextureUnit(4, pass->shadow.moment_maps);
	glBindTextureUnit(5, pass->shadow.depth_maps);
	
	//IBL Textures
	glBindTextureUnit(6, pass->ibl.brdfLutTexture);
	glBindTextureUnit(7, pass->ibl.irradianceCubemapTexture);
	glBindTextureUnit(8, pass->ibl.prefilteredCubemapTexture);

	//draw quad
	Render_Quad();

	glEnable(GL_DEPTH_TEST);
}

static void Pass_Skybox()
{
	if (!r_cvars.r_drawSky->int_value)
		return;

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
	//Render other objects
	//render water
	

	
	//render particles
	glDisable(GL_CULL_FACE);
	glUseProgram(pass->particles.particle_render_shader);
	Render_Particles();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

}

void Pass_OcclussionBoxes()
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

static void Pass_DebugPass()
{
	glUseProgram(pass->general.basic_3d_shader);
	Render_DebugScene();
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
		glBindTextureUnit(4, pass->bloom.mip_textures[0]);
	}
	//Normal post process pass
	else
	{
		glBindTextureUnit(0, pass->deferred.depth_texture);
		glBindTextureUnit(1, pass->scene.MainSceneColorBuffer);
		glBindTextureUnit(2, pass->bloom.mip_textures[0]);
		glBindTextureUnit(3, pass->dof.dof_texture);
		glUseProgram(pass->post.post_process_shader);
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
	
	///Render all opaque objects into g buffers
	Pass_gBuffer();
	
	//shadow mapping pass
	Pass_shadowMap();

	glDisable(GL_CULL_FACE);
	
	//SSAO pass, blur pass
	Pass_AO();

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

	//Render all transparent objects
	Pass_Transparents();

	glBindFramebuffer(GL_FRAMEBUFFER, pass->scene.FBO);

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	//Blur the main scene buffer for Depth of field effects
	Pass_DepthOfField();
	
	glEnable(GL_DEPTH_TEST);
	//Draw debug stuff
	Pass_DebugPass();

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