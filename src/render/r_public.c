#include "r_core.h"
#include "r_public.h"

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Process various public renderer functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

extern R_CMD_Buffer* cmdBuffer;
extern RPass_PassData* pass;
extern R_BackendData* backend_data;
extern R_StorageBuffers storage;
extern R_Scene scene;

extern void Render_Cube();
extern void Render_Quad();

static const vec4 DEFAULT_COLOR = { 1.0, 1.0, 1.0, 1.0 };

static bool _checkCmdLimit(unsigned int p_cmdSize)
{
	assert(cmdBuffer && "Renderer not initisialized");
	if (cmdBuffer->cmds_counter + 1 >= MAX_RENDER_COMMANDS || cmdBuffer->byte_count + p_cmdSize >= RENDER_BUFFER_COMMAND_ALLOC_SIZE)
	{
		printf("Max render command buffer limit reached \n");
		return false;
	}
	return true;
}

static void _copyToCmdBuffer(R_CMD p_cmdEnum, void* p_cmdData, unsigned int p_cmdSize)
{
	if (!_checkCmdLimit(p_cmdSize))
		return;

	cmdBuffer->cmd_enums[cmdBuffer->cmds_counter] = p_cmdEnum;

	memcpy(cmdBuffer->cmds_ptr, p_cmdData, p_cmdSize);
	(char*)cmdBuffer->cmds_ptr += p_cmdSize;

	cmdBuffer->byte_count += p_cmdSize;
	cmdBuffer->cmds_counter++;
}

void Draw_ScreenTexture(R_Texture* p_texture, M_Rect2Df* p_textureRegion, float p_x, float p_y, float p_xScale, float p_yScale, float p_rotation)
{
	if (!p_texture)
	{
		return;
	}

	R_CMD_DrawTexture cmd;
	cmd.proj_type = R_CMD_PT__SCREEN;
	cmd.texture = p_texture;
	cmd.position[0] = p_x;
	cmd.position[1] = p_y;
	
	cmd.scale[0] = p_xScale;
	cmd.scale[1] = p_yScale;

	cmd.rotation = p_rotation;

	glm_vec4_copy(DEFAULT_COLOR, cmd.color);

	if (p_textureRegion)
	{
		cmd.texture_region = *p_textureRegion;
	}
	else
	{
		cmd.texture_region.x = 0;
		cmd.texture_region.y = 0;
		cmd.texture_region.width = p_texture->width;
		cmd.texture_region.height = p_texture->height;
	}

	_copyToCmdBuffer(R_CMD__TEXTURE, &cmd, sizeof(R_CMD_DrawTexture));
}

void Draw_ScreenTextureColored(R_Texture* p_texture, M_Rect2Df* p_textureRegion, float p_x, float p_y, float p_xScale, float p_yScale, float p_rotation, float p_r, float p_g, float p_b, float p_a)
{
	if (!p_texture)
	{
		return;
	}

	R_CMD_DrawTexture cmd;
	cmd.proj_type = R_CMD_PT__SCREEN;
	cmd.texture = p_texture;
	cmd.position[0] = p_x;
	cmd.position[1] = p_y;

	cmd.scale[0] = p_xScale;
	cmd.scale[1] = p_yScale;

	cmd.rotation = p_rotation;

	cmd.color[0] = p_r;
	cmd.color[1] = p_g;
	cmd.color[2] = p_b;
	cmd.color[3] = p_a;

	if (p_textureRegion)
	{
		cmd.texture_region = *p_textureRegion;
	}
	else
	{
		cmd.texture_region.x = 0;
		cmd.texture_region.y = 0;
		cmd.texture_region.width = p_texture->width;
		cmd.texture_region.height = p_texture->height;
	}

	_copyToCmdBuffer(R_CMD__TEXTURE, &cmd, sizeof(R_CMD_DrawTexture));
}

void Draw_ScreenTexture2(R_Texture* p_texture, M_Rect2Df* p_textureRegion, vec2 p_position)
{
	Draw_ScreenTexture(p_texture, p_textureRegion, p_position[0], p_position[1], 0, 0, 0, 0);
}


void Draw_CubeWires(vec3 box[2], vec4 p_color)
{
	R_CMD_DrawCube cmd;

	glm_vec3_copy(box[0], cmd.box[0]);
	glm_vec3_copy(box[1], cmd.box[1]);
	cmd.polygon_mode = R_CMD_PM__WIRES;

	if (!p_color)
		glm_vec4_copy(DEFAULT_COLOR, cmd.color);
	else
		glm_vec4_copy(p_color, cmd.color);

	_copyToCmdBuffer(R_CMD__CUBE, &cmd, sizeof(R_CMD_DrawCube));
}


void Draw_TexturedCube(vec3 p_box[2], R_Texture* p_tex, M_Rect2Df* p_texRegion)
{
	R_CMD_DrawTextureCube cmd;

	memset(&cmd, 0, sizeof(cmd));

	glm_vec3_copy(p_box[0], cmd.box[0]);
	glm_vec3_copy(p_box[1], cmd.box[1]);

	cmd.texture = p_tex;
	if (p_texRegion)
	{
		cmd.texture_region = *p_texRegion;
	}
	else
	{
		if (p_tex)
		{
			cmd.texture_region.width = 1;
			cmd.texture_region.height = 1;
		}
		
	}
	
	glm_vec4_copy(DEFAULT_COLOR, cmd.color);

	_copyToCmdBuffer(R_CMD__TEXTURED_CUBE, &cmd, sizeof(R_CMD_DrawTextureCube));
}

void Draw_TexturedCubeColored(vec3 p_box[2], R_Texture* p_tex, M_Rect2Df* p_texRegion, float p_r, float p_g, float p_b, float p_a)
{
	R_CMD_DrawTextureCube cmd;

	memset(&cmd, 0, sizeof(cmd));

	glm_vec3_copy(p_box[0], cmd.box[0]);
	glm_vec3_copy(p_box[1], cmd.box[1]);

	cmd.texture = p_tex;
	if (p_texRegion)
	{
		cmd.texture_region = *p_texRegion;
	}
	else
	{
		if (p_tex)
		{
			cmd.texture_region.width = 1;
			cmd.texture_region.height = 1;

		}

	}
	
	cmd.color[0] = p_r;
	cmd.color[1] = p_g;
	cmd.color[2] = p_b;
	cmd.color[3] = p_a;


	_copyToCmdBuffer(R_CMD__TEXTURED_CUBE, &cmd, sizeof(R_CMD_DrawTextureCube));
}

void Draw_TexturedQuad(vec3 p_minMax[2], R_Texture* p_tex, M_Rect2Df* p_texRegion)
{
}


void Draw_Line(vec3 p_from, vec3 p_to, vec4 p_color)
{
	R_CMD_DrawLine cmd;
	
	cmd.proj_type = R_CMD_PT__3D;
	glm_vec3_copy(p_from, cmd.from);
	glm_vec3_copy(p_to, cmd.to);
	if (!p_color)
		glm_vec4_copy(DEFAULT_COLOR, cmd.color);
	else
		glm_vec4_copy(p_color, cmd.color);

	_copyToCmdBuffer(R_CMD__LINE, &cmd, sizeof(R_CMD_DrawLine));
}

void Draw_Line2(float x1, float y1, float z1, float x2, float y2, float z2)
{
	vec3 from;
	from[0] = x1;
	from[1] = y1;
	from[2] = z1;

	vec3 to;
	to[0] = x2;
	to[1] = y2;
	to[2] = z2;

	Draw_Line(from, to, NULL);
}

void Draw_Triangle(vec3 p1, vec3 p2, vec3 p3, vec4 p_color)
{
	R_CMD_DrawTriangle cmd;

	cmd.proj_type = R_CMD_PT__3D;
	glm_vec3_copy(p1, cmd.points[0]);
	glm_vec3_copy(p2, cmd.points[1]);
	glm_vec3_copy(p3, cmd.points[2]);

	if (!p_color)
		glm_vec4_copy(DEFAULT_COLOR, cmd.color);
	else
		glm_vec4_copy(p_color, cmd.color);

	_copyToCmdBuffer(R_CMD__TRIANGLE, &cmd, sizeof(R_CMD_DrawTriangle));
}

void Draw_Model(R_Model* const p_model, vec3 p_position)
{

}

void Draw_ModelWires(R_Model* const p_model, vec3 p_position)
{
}

void Draw_LCWorld()
{
	R_CMD_DrawLCWorld cmd;
	cmd.nullData = 0;

	_copyToCmdBuffer(R_CMD__LCWorld, &cmd, sizeof(R_CMD_DrawLCWorld));
}

void RScene_SetDirLight(DirLight p_dirLight)
{
	scene.scene_data.dirLightColor[0] = p_dirLight.color[0];
	scene.scene_data.dirLightColor[1] = p_dirLight.color[1];
	scene.scene_data.dirLightColor[2] = p_dirLight.color[2];

	scene.scene_data.dirLightDirection[0] = p_dirLight.direction[0];
	scene.scene_data.dirLightDirection[1] = p_dirLight.direction[1];
	scene.scene_data.dirLightDirection[2] = p_dirLight.direction[2];
	scene.scene_data.dirLightDirection[3] = 0;

	//glm_vec3_normalize(scene.scene_data.dirLightDirection);

	scene.scene_data.dirLightAmbientIntensity = p_dirLight.ambient_intensity;
	scene.scene_data.dirLightSpecularIntensity = p_dirLight.specular_intensity;
}

void RScene_SetDirLightDirection(vec3 dir)
{
	scene.scene_data.dirLightDirection[0] = dir[0];
	scene.scene_data.dirLightDirection[1] = dir[1];
	scene.scene_data.dirLightDirection[2] = dir[2];
	scene.scene_data.dirLightDirection[3] = 0;
}

void RScene_SetFog(FogSettings p_fog_settings)
{
	glm_vec3_copy(p_fog_settings.fog_color, scene.scene_data.fogColor);
	scene.scene_data.fogDensity = p_fog_settings.density;
	scene.scene_data.heightFogMin = p_fog_settings.heightFogMin;
	scene.scene_data.heightFogMax = p_fog_settings.heightFogMax;
	scene.scene_data.heightFogCurve = p_fog_settings.heightFogCurve;
	scene.scene_data.depthFogBegin = p_fog_settings.depthFogBegin;
	scene.scene_data.depthFogEnd = p_fog_settings.depthFogEnd;
	scene.scene_data.depthFogCurve = p_fog_settings.depthFogCurve;
}

LightID RScene_RegisterPointLight(PointLight2 p_pointLight)
{
	LightID lid = RSB_Request(&storage.point_lights);
	LightID object_pool_id = Object_Pool_Request(storage.point_lights_pool);

	PointLight* ptr = dA_at(storage.point_lights_pool->pool, object_pool_id);

	assert(lid == object_pool_id);

	const float maxBrightness = fmaxf(fmaxf(p_pointLight.color[0], p_pointLight.color[1]), p_pointLight.color[2]);
	p_pointLight.radius = (-p_pointLight.linear + sqrt(p_pointLight.linear * p_pointLight.linear - 4 * p_pointLight.quadratic * (p_pointLight.constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * p_pointLight.quadratic);

	glNamedBufferSubData(storage.point_lights.buffer, lid * sizeof(PointLight2), sizeof(PointLight2), &p_pointLight);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, storage.point_lights.buffer);
	scene.scene_data.numPointLights++;

	memcpy(ptr, &p_pointLight, sizeof(PointLight2));

	AABB aabb;
	aabb.position[0] = p_pointLight.position[0] - (p_pointLight.radius * 0.5);
	aabb.position[1] = p_pointLight.position[1] - (p_pointLight.radius * 0.5);
	aabb.position[2] = p_pointLight.position[2] - (p_pointLight.radius * 0.5);

	aabb.width = p_pointLight.radius;
	aabb.height = p_pointLight.radius;
	aabb.length = p_pointLight.radius;

	AABB_Tree_Insert(&scene.clusters_data.light_tree, &aabb, lid);

	return lid;
}

void RSCene_DeletePointLight(LightID p_lightID)
{
	if (p_lightID > storage.point_lights.used_size)
	{
		//return;
	}

	PointLight2 blank;
	memset(&blank, 0, sizeof(blank));
	glNamedBufferSubData(storage.point_lights.buffer, p_lightID * sizeof(PointLight2), sizeof(PointLight2), &blank);

	RSB_FreeItem(&storage.point_lights, p_lightID, false);
	Object_Pool_Free(storage.point_lights_pool, p_lightID, true);
}

static void RScene_CubemapCompute()
{
	//CONVULUTE CUBEMAP
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
	glUseProgram(pass->ibl.irradiance_convulution_shader);
	Shader_SetInteger(pass->ibl.irradiance_convulution_shader, "environmentMap", 0);
	Shader_SetMatrix4(pass->ibl.irradiance_convulution_shader, "u_proj", pass->ibl.cube_proj);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);

	glViewport(0, 0, 32, 32);
	for (int i = 0; i < 6; i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.irradianceCubemapTexture, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		Shader_SetMatrix4(pass->ibl.irradiance_convulution_shader, "u_view", pass->ibl.cube_view_matrixes[i]);
		Render_Cube();
	}

	//PREFILTER CUBEMAP
	glUseProgram(pass->ibl.prefilter_cubemap_shader);
	Shader_SetInteger(pass->ibl.prefilter_cubemap_shader, "environmentMap", 0);
	Shader_SetMatrix4(pass->ibl.prefilter_cubemap_shader, "u_proj", pass->ibl.cube_proj);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pass->ibl.envCubemapTexture);

	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		unsigned int mipWidth = (unsigned)(128 * pow(0.5, mip));
		unsigned int mipHeight = (unsigned)(128 * pow(0.5, mip));

		glBindRenderbuffer(GL_RENDERBUFFER, pass->ibl.RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		Shader_SetFloat(pass->ibl.prefilter_cubemap_shader, "u_roughness", roughness);

		for (unsigned int i = 0; i < 6; ++i)
		{
			Shader_SetMatrix4(pass->ibl.prefilter_cubemap_shader, "u_view", pass->ibl.cube_view_matrixes[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.prefilteredCubemapTexture, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			Render_Cube();
		}
	}

	//BRDF GENERATION
	glUseProgram(pass->ibl.brdf_shader);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->ibl.brdfLutTexture, 0);
	glViewport(0, 0, 512, 512);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_BLEND);
	Render_Quad();

	//RESET VIEWPROT
	glViewport(0, 0, backend_data->screenSize[0], backend_data->screenSize[1]);
}

void RScene_SetSkyboxTexturePanorama(R_Texture* p_tex)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex->id);
	glUseProgram(pass->ibl.equirectangular_to_cubemap_shader);
	Shader_SetInteger(pass->ibl.equirectangular_to_cubemap_shader, "equirectangularMap", 0);
	Shader_SetMatrix4(pass->ibl.equirectangular_to_cubemap_shader, "u_proj", pass->ibl.cube_proj);

	glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.FBO);
	glViewport(0, 0, 512, 512);
	//CONVERT PANORAMA TO A CUBEMAP
	for (int i = 0; i < 6; i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.envCubemapTexture, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		Shader_SetMatrix4(pass->ibl.equirectangular_to_cubemap_shader, "u_view", pass->ibl.cube_view_matrixes[i]);
		Render_Cube();
	}

	RScene_CubemapCompute();
}

void RScene_SetSkyboxTextureSingleImage(const char* p_path)
{
	R_Texture skybox_tex = HDRTexture_Load(p_path, NULL);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, skybox_tex.id);
	glUseProgram(pass->ibl.single_image_cubemap_convert_shader);
	Shader_SetInteger(pass->ibl.single_image_cubemap_convert_shader, "skyboxTextureMap", 0);
	Shader_SetMatrix4(pass->ibl.single_image_cubemap_convert_shader, "u_proj", pass->ibl.cube_proj);

	glBindFramebuffer(GL_FRAMEBUFFER, pass->ibl.FBO);
	glViewport(0, 0, 512, 512);
	//CONVERT PANORAMA TO A CUBEMAP
	for (int i = 0; i < 6; i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, pass->ibl.envCubemapTexture, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		Shader_SetMatrix4(pass->ibl.single_image_cubemap_convert_shader, "u_view", pass->ibl.cube_view_matrixes[i]);
		Render_Cube();
	}

	RScene_CubemapCompute();
}

ModelID RScene_RegisterModel(R_Model* p_model)
{
	return 0;
}

void RScene_SetAmbientLightInfluence(float p_ratio)
{
	scene.scene_data.ambientLightInfluence = glm_clamp(p_ratio, 0.0, 1.0);
}

ParticleEmitterSettings* Particle_RegisterEmitter()
{
	FL_Node* node = FL_emplaceFront(storage.particle_emitter_clients);

	ParticleEmitterSettings* emitter = node->value;

	emitter->particles = dA_INIT(ParticleCpu, 0);

	return emitter;
}

void Particle_MarkUpdate(ParticleEmitterSettings* p_emitter)
{
	//storage.particle_update_queue[storage.particle_update_index] = p_emitter;

	//storage.particle_update_index++;

	p_emitter->_queue_update = true;
	p_emitter->emitting = true;
}

void Particle_Emit(ParticleEmitterSettings* p_emitter)
{
	//p_emitter->settings.state_flags |= EMITTER_STATE_FLAG__EMITTING;
//
	//Particle_MarkUpdate(p_emitter);

	p_emitter->emitting = true;
	p_emitter->force_restart = true;
}

void Particle_EmitTransformed(ParticleEmitterSettings* p_emitter, vec3 direction, vec3 origin)
{
	p_emitter->direction[0] = direction[0];
	p_emitter->direction[1] = direction[1];
	p_emitter->direction[2] = direction[2];

	p_emitter->xform[3][0] = origin[0];
	p_emitter->xform[3][1] = origin[1];
	p_emitter->xform[3][2] = origin[2];

	//glm_mat4_mulv3(p_emitter->settings.xform, p_emitter->aabb[0], 1.0, p_emitter->settings.aabb[0]);
	//glm_mat4_mulv3(p_emitter->settings.xform, p_emitter->aabb[1], 1.0, p_emitter->settings.aabb[1]);

	Particle_Emit(p_emitter);
}

