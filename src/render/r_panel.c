/*
    Debug/Testing Panel for various renderer items
*/

#include "render/r_core.h"
#include "core/core_common.h"

extern NK_Data nk;
extern GLFWwindow* glfw_window;
extern R_Cvars r_cvars;
extern R_Metrics metrics;
extern R_Scene scene;

typedef struct
{
	struct nk_rect panel_bounds;
} PanelCore;

static PanelCore panel_core;

static void RPanel_CvarCheckbox(Cvar* const p_cvar, const char* p_str)
{
	int enabled = !p_cvar->int_value;
	if (nk_checkbox_label(nk.ctx, p_str, &enabled))
	{
		Cvar_setValueDirectInt(p_cvar, !enabled);
	}
}
static void RPanel_CvarSliderf(Cvar* const p_cvar, const char* p_str, float p_minV, float p_maxV, float p_step, float p_incPerPixel)
{
	nk_layout_row_begin(nk.ctx, NK_DYNAMIC, 25, 2);
	
	nk_layout_row_push(nk.ctx, 0.80);
	//slider
	float value = nk_propertyf(nk.ctx, p_str, p_minV, p_cvar->float_value, p_maxV, p_step, p_incPerPixel);

	nk_layout_row_push(nk.ctx, 0.20);
	//reset button
	if (nk_button_label(nk.ctx, "Reset"))
	{
		Cvar_setValueToDefaultDirect(p_cvar);
	}
	else if (value != p_cvar->float_value)
	{
		Cvar_setValueDirectFloat(p_cvar, value);
	}

	nk_layout_row_end(nk.ctx);
}
static void RPanel_CvarSlideri(Cvar* const p_cvar, const char* p_str, int p_minV, int p_maxV, int p_step, float p_incPerPixel)
{
	nk_layout_row_begin(nk.ctx, NK_DYNAMIC, 25, 2);

	nk_layout_row_push(nk.ctx, 0.80);
	//slider
	int value = nk_propertyi(nk.ctx, p_str, p_minV, p_cvar->int_value, p_maxV, p_step, p_incPerPixel);

	nk_layout_row_push(nk.ctx, 0.20);
	//reset button
	if (nk_button_label(nk.ctx, "Reset"))
	{
		Cvar_setValueToDefaultDirect(p_cvar);
	}
	else if (value != p_cvar->int_value)
	{
		Cvar_setValueDirectInt(p_cvar, value);
	}

	nk_layout_row_end(nk.ctx);
}
static void RPanel_CvarDropdown(Cvar* const p_cvar, const char** p_names, const char* p_label, int p_max)
{	
	nk_layout_row_dynamic(nk.ctx, 25, 2);
	nk_label(nk.ctx, p_label, NK_TEXT_ALIGN_LEFT);

	int selected_item = p_cvar->int_value;
	if (nk_combo_begin_label(nk.ctx, p_names[selected_item], nk_vec2(nk_widget_width(nk.ctx), 200)))
	{
		nk_layout_row_dynamic(nk.ctx, 25, 1);

		for (int i = 0; i < p_max; i++)
		{
			if (nk_combo_item_label(nk.ctx, p_names[i], NK_TEXT_LEFT))
			{
				Cvar_setValueDirectInt(p_cvar, i);
			}
		}
		nk_combo_end(nk.ctx);
	}
}

void RPanel_Main()
{
	if (!nk.enabled)
		return;

	if (panel_core.panel_bounds.w == 0 || panel_core.panel_bounds.h == 0)
	{
		panel_core.panel_bounds.w = 300;
		panel_core.panel_bounds.h = 300;
	}

	if (!nk_begin(nk.ctx, "Renderer panel", panel_core.panel_bounds, NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE))
	{
		nk_end(nk.ctx);
		return;
	}

	//update main window bounds
	panel_core.panel_bounds = nk_window_get_bounds(nk.ctx);

	Window_EnableCursor();

	if (nk_window_is_active(nk.ctx, "Renderer panel"))
	{
		Core_BlockInputThisFrame();
	}
	
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "General", NK_MAXIMIZED))
	{
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Window", NK_MINIMIZED))
		{
			RPanel_CvarCheckbox(r_cvars.w_useVsync, "Vsync");

			nk_tree_pop(nk.ctx);
		}

		RPanel_CvarCheckbox(r_cvars.r_drawSky, "Draw sky");	
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Camera", NK_MAXIMIZED))
	{	
		RPanel_CvarSliderf(r_cvars.cam_fov, "Fov", 0.0, 130, 0.1, 0.01);
		RPanel_CvarSliderf(r_cvars.cam_zNear, "ZFar", 50, 4000, 0.1, 0.01);
		RPanel_CvarSliderf(r_cvars.cam_zFar, "ZNear", 0.01, 50, 0.1, 0.01);

		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Debug", NK_MAXIMIZED))
	{
		//DEBUG TEXTURES
		//-1 disabled, 0 = Normal, 1 = Albedo, 2 = Depth, 3 = Metal, 4 = Rough, 5 = AO
		static const char* texture_items[] = { "Disabled", "Normal", "Albedo", "Depth", "Metallic", "Roughness", "SSAO"};
		int selected_item = r_cvars.r_drawDebugTexture->int_value + 1;

		nk_label(nk.ctx, "Draw texture", NK_TEXT_ALIGN_LEFT);
		if (nk_combo_begin_label(nk.ctx, texture_items[selected_item], nk_vec2(nk_widget_width(nk.ctx), 200)))
		{
			nk_layout_row_dynamic(nk.ctx, 25, 1);

			for (int i = 0; i < 7; i++)
			{
				if (nk_combo_item_label(nk.ctx, texture_items[i], NK_TEXT_LEFT))
				{
					int value = i - 1;
					Cvar_setValueDirectInt(r_cvars.r_drawDebugTexture, value);
				}
			}
			nk_combo_end(nk.ctx);
		}

		static const char* wireframe_items[] = { "Disabled", "Wireframe pass", "Wireframe override"};
		int selected_wireframe_mode = r_cvars.r_wireframe->int_value;
		nk_label(nk.ctx, "Wireframe", NK_TEXT_ALIGN_LEFT);
		if (nk_combo_begin_label(nk.ctx, wireframe_items[selected_wireframe_mode], nk_vec2(nk_widget_width(nk.ctx), 200)))
		{
			nk_layout_row_dynamic(nk.ctx, 25, 1);

			for (int i = 0; i < 3; i++)
			{
				if (nk_combo_item_label(nk.ctx, wireframe_items[i], NK_TEXT_LEFT))
				{
					int value = i;
					Cvar_setValueDirectInt(r_cvars.r_wireframe, i);
				}
			}
			nk_combo_end(nk.ctx);
		}

		nk_tree_pop(nk.ctx);
	}
	

	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Shadows", NK_MINIMIZED))
	{
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Directional shadows", NK_MINIMIZED))
		{
			RPanel_CvarCheckbox(r_cvars.r_useDirShadowMapping, "Enabled");
			RPanel_CvarSliderf(r_cvars.r_shadowBias, "#Bias", 0.0, 10.0, 0.01, 0.01);
			RPanel_CvarSliderf(r_cvars.r_shadowNormalBias, "Normal Bias", 0.0, 10.0, 0.01, 0.01);

			RPanel_CvarSlideri(r_cvars.r_shadowQualityLevel, "Quality", 0, r_cvars.r_shadowQualityLevel->max_value, 1, 1);
			RPanel_CvarSlideri(r_cvars.r_shadowBlurLevel, "Blurring", 0, r_cvars.r_shadowBlurLevel->max_value, 1, 1);

			nk_tree_pop(nk.ctx);
		}
	
		nk_tree_pop(nk.ctx);
	}
	
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "SSAO", NK_MINIMIZED))
	{	
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		RPanel_CvarCheckbox(r_cvars.r_useSsao, "Enabled");
		RPanel_CvarCheckbox(r_cvars.r_ssaoHalfSize, "Half size");
		
		RPanel_CvarSliderf(r_cvars.r_ssaoBias, "Bias", 0.0, 1.0, 0.01, 0.005);
		RPanel_CvarSliderf(r_cvars.r_ssaoRadius, "Radius", 0.0, 1.0, 0.01, 0.005);
		RPanel_CvarSliderf(r_cvars.r_ssaoStrength, "Strength", 0.0, 8.0, 0.05, 0.01);
		nk_tree_pop(nk.ctx);
	}
	
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Post process", NK_MINIMIZED))
	{
		RPanel_CvarCheckbox(r_cvars.r_useFxaa, "FXXA Enabled");

		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Monitor", NK_MINIMIZED))
		{
			RPanel_CvarSliderf(r_cvars.r_Gamma, "Gamma", 0.0, 3, 0.01, 0.01);
			RPanel_CvarSliderf(r_cvars.r_Exposure, "Exposure", 0.0, 16, 0.01, 0.01);
			RPanel_CvarSliderf(r_cvars.r_Brightness, "Brightness", 0.0, 8, 0.01, 0.01);
			RPanel_CvarSliderf(r_cvars.r_Contrast, "Contrast", 0.0, 8.0, 0.01, 0.01);
			RPanel_CvarSliderf(r_cvars.r_Saturation, "Saturation", 0.0, 8.0, 0.01, 0.01);

			nk_tree_pop(nk.ctx);
		}
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Depth of field", NK_MINIMIZED))
		{
			RPanel_CvarCheckbox(r_cvars.r_useDepthOfField, "Enabled");

			if (r_cvars.r_useDepthOfField->int_value)
			{
				static const char* DOF_MODES[2] = { "BOX", "CIRCULAR" };
				RPanel_CvarDropdown(r_cvars.r_DepthOfFieldMode, DOF_MODES, "Mode", 2);

				nk_layout_row_dynamic(nk.ctx, 25, 1);

				scene.environment.depthOfFieldBlurScale = nk_propertyf(nk.ctx, "Blur Scale", 0.01, scene.environment.depthOfFieldBlurScale, 1.0, 0.01, 0.01);

				int depth_of_field_near_enabled = !scene.environment.depthOfFieldNearEnabled;
				if (nk_checkbox_label(nk.ctx, "Near Enabled", &depth_of_field_near_enabled))
				{
					scene.environment.depthOfFieldNearEnabled = !scene.environment.depthOfFieldNearEnabled;
				}
				if (scene.environment.depthOfFieldNearEnabled)
				{
					scene.environment.depthOfFieldNearBegin = nk_propertyf(nk.ctx, "Near begin", 0.01, scene.environment.depthOfFieldNearBegin, 1500, 0.1, 0.1);
					scene.environment.depthOfFieldNearEnd = nk_propertyf(nk.ctx, "Near end", 0.01, scene.environment.depthOfFieldNearEnd, 1500, 0.1, 0.1);
				}
				int depth_of_field_far_enabled = !scene.environment.depthOfFieldFarEnabled;
				if (nk_checkbox_label(nk.ctx, "Far Enabled", &depth_of_field_far_enabled))
				{
					scene.environment.depthOfFieldFarEnabled = !scene.environment.depthOfFieldFarEnabled;
				}
				if (scene.environment.depthOfFieldFarEnabled)
				{
					scene.environment.depthOfFieldFarBegin = nk_propertyf(nk.ctx, "Far begin", 0.01, scene.environment.depthOfFieldFarBegin, 1500, 0.1, 0.1);
					scene.environment.depthOfFieldFarEnd = nk_propertyf(nk.ctx, "Far end", 0.01, scene.environment.depthOfFieldFarEnd, 1500, 0.1, 0.1);
				}


			}
			
			nk_tree_pop(nk.ctx);
		}
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Bloom", NK_MINIMIZED))
		{	
			RPanel_CvarCheckbox(r_cvars.r_useBloom, "Enabled");
			
			RPanel_CvarSliderf(r_cvars.r_bloomStrength, "Strength", 0.0, 1.0, 0.001, 0.005);
			RPanel_CvarSliderf(r_cvars.r_bloomThreshold, "Threshold", 0.0, r_cvars.r_bloomThreshold->max_value, 0.01, 0.01);
			RPanel_CvarSliderf(r_cvars.r_bloomSoftThreshold, "Soft threshold", 0.0, r_cvars.r_bloomSoftThreshold->max_value, 0.001, 0.01);

			nk_tree_pop(nk.ctx);
		}
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Scene", NK_MINIMIZED))
	{
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Directional light", NK_MINIMIZED))
		{	
			nk_layout_row_dynamic(nk.ctx, 25, 4);
			nk_label(nk.ctx, "Direction", NK_TEXT_ALIGN_LEFT);
			nk_property_float(nk.ctx, "x", -1, &scene.scene_data.dirLightDirection[0], 1, 0.01, 0.01);
			nk_property_float(nk.ctx, "y", -1, &scene.scene_data.dirLightDirection[1], 1, 0.01, 0.01);
			nk_property_float(nk.ctx, "z", -1, &scene.scene_data.dirLightDirection[2], 1, 0.01, 0.01);

			struct nk_colorf sun_color;
			sun_color.r = scene.scene_data.dirLightColor[0];
			sun_color.g = scene.scene_data.dirLightColor[1];
			sun_color.b = scene.scene_data.dirLightColor[2];

			nk_layout_row_dynamic(nk.ctx, 100, 1);
			sun_color = nk_color_picker(nk.ctx, sun_color, NK_RGB);

			scene.scene_data.dirLightColor[0] = sun_color.r;
			scene.scene_data.dirLightColor[1] = sun_color.g;
			scene.scene_data.dirLightColor[2] = sun_color.b;

			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_labelf_wrap(nk.ctx, "Ambient intensity %.2f", scene.scene_data.dirLightAmbientIntensity);
			nk_slider_float(nk.ctx, 0, &scene.scene_data.dirLightAmbientIntensity, 32, 0.01);
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_labelf_wrap(nk.ctx, "Specular intensity %.2f", scene.scene_data.dirLightSpecularIntensity);
			nk_slider_float(nk.ctx, 0, &scene.scene_data.dirLightSpecularIntensity, 32, 0.01);

			nk_tree_pop(nk.ctx);
		}
		
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Fog", NK_MINIMIZED))
		{	
			int height_fog_enabled = !scene.environment.heightFogEnabled;
			nk_layout_row_dynamic(nk.ctx, 25, 1);
			if (nk_checkbox_label(nk.ctx, "Height Fog Enabled", &height_fog_enabled))
			{
				scene.environment.heightFogEnabled = !scene.environment.heightFogEnabled;
			}
			if (!height_fog_enabled)
			{
				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.heightFogDensity = nk_propertyf(nk.ctx, "Height fog density", 0.0, scene.environment.heightFogDensity, 1.0, 0.01, 0.001);

				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.heightFogMin = nk_propertyf(nk.ctx, "Height fog min", -100000, scene.environment.heightFogMin, 100000, 0.1, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.heightFogMax = nk_propertyf(nk.ctx, "Height fog max", -100000, scene.environment.heightFogMax, 100000, 0.1, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.heightFogCurve = nk_propertyf(nk.ctx, "Height fog curve", 0.0, scene.environment.heightFogCurve, 24.0, 0.01, 0.001);
			}
			
			int depth_fog_enabled = !scene.environment.depthFogEnabled;
			nk_layout_row_dynamic(nk.ctx, 25, 1);
			if (nk_checkbox_label(nk.ctx, "Depth Fog Enabled", &depth_fog_enabled))
			{
				scene.environment.depthFogEnabled = !scene.environment.depthFogEnabled;
			}

			if (!depth_fog_enabled)
			{
				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.depthFogDensity = nk_propertyf(nk.ctx, "Depth fog density", 0.0, scene.environment.depthFogDensity, 1.0, 0.01, 0.001);

				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.depthFogBegin = nk_propertyf(nk.ctx, "Depth fog begin", -100000, scene.environment.depthFogBegin, 100000, 0.1, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.depthFogEnd = nk_propertyf(nk.ctx, "Depth fog end", -100000, scene.environment.depthFogEnd, 100000, 0.1, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.depthFogCurve = nk_propertyf(nk.ctx, "Depth fog curve", 0.0, scene.environment.depthFogCurve, 24.0, 0.01, 0.001);
			}
			
			RPanel_CvarCheckbox(r_cvars.r_enableGodrays, "Godrays enabled");
			if (r_cvars.r_enableGodrays->int_value)
			{
				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.godrayScatteringAmount = nk_propertyf(nk.ctx, "Scattering amount", 0.0, scene.environment.godrayScatteringAmount, 0.99, 0.01, 0.001);

				nk_layout_row_dynamic(nk.ctx, 25, 1);
				scene.environment.godrayFogAmount = nk_propertyf(nk.ctx, "Fog curve", 0.0, scene.environment.godrayFogAmount, 24, 0.01, 0.001);
			}

			nk_tree_pop(nk.ctx);
		}
		nk_tree_pop(nk.ctx);
	}
	nk_end(nk.ctx);
}

void RPanel_Metrics()
{
	nk_style_push_color(nk.ctx, &nk.ctx->style.window.fixed_background.data.color, nk_rgba(1, 1, 1, 1));
	if (!nk_begin(nk.ctx, "Renderer metrics", nk_rect(200, 200, 240, 95), NK_WINDOW_NO_SCROLLBAR))
	{
		nk_end(nk.ctx);
		return;
	}

	nk_style_push_color(nk.ctx, &nk.ctx->style.text.color, nk_rgba(255, 255, 255, 255));
	nk_layout_row_dynamic(nk.ctx, 15, 1);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "Frame time: %f", metrics.frame_time);
	nk_labelf(nk.ctx, NK_TEXT_ALIGN_LEFT, "FPS: %i", metrics.fps);
	nk_style_pop_color(nk.ctx);
	nk_style_pop_color(nk.ctx);
	nk_end(nk.ctx);
}