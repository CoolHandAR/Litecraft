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

extern void Render_Quad();
extern void Render_OpaqueWorldChunks(bool p_TextureDraw);
extern void Render_TransparentWorldChunks();
extern void Render_Cube();
extern void Render_DebugScene();
extern void Render_DrawChunkOcclusionBoxes();

typedef enum
{
	RPS__DEPTH_PREPASS,
	RPS__SHADOW_MAPPING_PASS,
	RPS__GBUFFER_PASS,
	RPS__DEBUG_PASS,
} RenderPassState;

static void Render_OpaqueScene(RenderPassState rs)
{
	switch (rs)
	{
	case RPS__DEPTH_PREPASS:
	{
		//Render opaque world chunks
		glUseProgram(pass->lc.depthPrepass_shader);
		Render_OpaqueWorldChunks(false);
		
		//Render other opaque stuff

		break;
	}
	case RPS__GBUFFER_PASS:
	{
		//Render opaque world chunks
		glUseProgram(pass->lc.gBuffer_shader);
		Render_OpaqueWorldChunks(true);

		//Render other opaque stuff

		break;
	}
	case RPS__SHADOW_MAPPING_PASS:
	{
		//Render opaque world chunks
		glUseProgram(pass->lc.shadow_map_shader);
		Render_OpaqueWorldChunks(false);

		//Render other opaque stuff

		break;
	}
	case RPS__DEBUG_PASS:
	{
		break;
	}
	default:
		break;
	}
}

static void Pass_depthPrepass()
{
	//Disable color mask since we are only writing to the depth buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	
	//draw all opaque objects
	Render_OpaqueScene(RPS__DEPTH_PREPASS);

	//Enable color mask again
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

static void Pass_shadowMap()
{
	//if (!r_cvars.r_useDirShadowMapping->int_value)
		//return;

	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glUseProgram(pass->shadow.depth_shader);

	glBindFramebuffer(GL_FRAMEBUFFER, pass->shadow.FBO);

	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	glClear(GL_DEPTH_BUFFER_BIT);

	Render_OpaqueScene(RPS__SHADOW_MAPPING_PASS);

	glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
}

static void Pass_gBuffer()
{
	//glDepthFunc(GL_EQUAL); //Skip occluded pixels since we did the depth pre pass already

	 glDepthFunc(GL_LESS);
	
	 glEnable(GL_DEPTH_TEST);

	//make sure we disable blend, even if we disabled it before
	glDisable(GL_BLEND);

	//draw opaque scene
	Render_OpaqueScene(RPS__GBUFFER_PASS);

	//glDepthFunc(GL_LESS);
}

static void Pass_AO()
{
	if (!r_cvars.r_useSsao->int_value)
		return;

	int viewport_width = backend_data->screenSize[0];
	int viewport_height = backend_data->screenSize[1];
	
	if (r_cvars.r_ssaoHalfSize->int_value)
	{
		glViewport(0, 0, backend_data->halfScreenSize[0], backend_data->halfScreenSize[1]);

		viewport_width = backend_data->halfScreenSize[0];
		viewport_height = backend_data->halfScreenSize[1];
	}

	//SSAO 
	glBindFramebuffer(GL_FRAMEBUFFER, pass->ao.FBO);
	//glClearColor(0, 0, 0, 0);
	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_BLEND);
	//glDisable(GL_CULL_FACE);
	//glDisable(GL_STENCIL_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTextureUnit(0, pass->deferred.depth_texture);
	glBindTextureUnit(1, pass->ao.noise_texture);
	glBindTextureUnit(2, pass->deferred.gNormalMetal_texture);

	glUseProgram(pass->ao.shader);
	Shader_SetVector2f(pass->ao.shader, "u_viewportSize", viewport_width, viewport_height);

	Render_Quad();

	//Box Blur the ssao texture
	glBindFramebuffer(GL_FRAMEBUFFER, pass->ao.BLURRED_FBO);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(pass->general.box_blur_shader);
	glBindTextureUnit(0, pass->ao.ao_texture);
	Shader_SetInteger(pass->general.box_blur_shader, "u_size", 2);
	Render_Quad();

	//make sure to reset the viewport size if it was changed
	if (r_cvars.r_ssaoHalfSize->int_value)
	{
		glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
	}
}
static void Pass_BloomTextureSample()
{
	//if (!r_cvars.r_useBloom->int_value)
		//return;

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
	Shader_SetFloat(pass->general.upsample_shader, "u_filterRadius", 0.0001);

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

static void Pass_deferredShading()
{
	glDisable(GL_DEPTH_TEST);

	//use shader
	glUseProgram(pass->deferred.shading_shader);

	//setup textures
	glBindTextureUnit(0, pass->deferred.gNormalMetal_texture);
	glBindTextureUnit(1, pass->deferred.gColorRough_texture);
	glBindTextureUnit(2, pass->deferred.depth_texture);
	glBindTextureUnit(3, pass->ao.ao_texture);
	glBindTextureUnit(4, pass->shadow.depth_maps);
	
	//IBL Textures
	glBindTextureUnit(5, pass->ibl.brdfLutTexture);
	glBindTextureUnit(6, pass->ibl.irradianceCubemapTexture);
	glBindTextureUnit(7, pass->ibl.prefilteredCubemapTexture);

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

static void Pass_TransparentForward()
{	
	//Disable cull face, so that transparent faces don't dissapear
	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);

	//Render Transparent objects
	//World Chunks
	//glUseProgram(pass->lc.transparents_forward_pbr_shader);
	Render_TransparentWorldChunks();

	//Render other objects

	//render water
	
	//render particles

	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

static void Pass_OcclussionBoxes()
{
	glUseProgram(pass->lc.occlussion_boxes_shader);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -1);

	//glViewport(0, 0, 1, 1);
	Render_DrawChunkOcclusionBoxes();

	//glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
	//glEnable(GL_CULL_FACE);
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
		//-1 disabled, 0 = Normal, 1 = Albedo, 2 = Depth, 3 = Metal, 4 = Rough, 5 = AO, 6 = BLOOM
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
		glUseProgram(pass->post.post_process_shader);
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);
	Render_Quad();
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

	//Render all opaque objects into the depth buffer
	//Pass_depthPrepass();

	unsigned query = 0;
	glGenQueries(1, &query);
	glBeginQuery(GL_PRIMITIVES_GENERATED, query);
	///Render all opaque objects into g buffers
	Pass_gBuffer();
	glEndQuery(GL_PRIMITIVES_GENERATED);

	int result = 0;
	glGetQueryObjectiv(query, GL_QUERY_RESULT, &result);

	printf("%i \n", result);
	
	//Opaque shadow mapping pass
	Pass_shadowMap();

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

	//Downsample and then upsample the scene's color buffer for bloom effect
	Pass_BloomTextureSample();

	//Render boxes for oclussion culling
	Pass_OcclussionBoxes();

	//Render all transparent objects into the main's scene Buffer
	//Pass_TransparentForward();

	//Blit and blur the main scene buffer for Depth of field effects

	

	//Bind to the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//Post process pass
	Pass_PostProcess();

	
	//Draw debug stuff
	//Pass_DebugPass();


	//Render UI And other sprites
}