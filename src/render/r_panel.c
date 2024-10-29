/*
    Debug/Testing Panel for various renderer items
*/

#include "render/r_core.h"
#include "core/c_common.h"

extern NK_Data nk;
extern GLFWwindow* glfw_window;
extern R_Cvars r_cvars;
extern R_Metrics metrics;
extern R_Scene scene;

static void RPanel_SliderImpl(const char* p_str, float p_minV, float p_maxV, float p_step)
{

}

void RPanel_Main()
{
	if (!nk_begin(nk.ctx, "Renderer panel", nk_rect(200, 20, 300, 300), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE))
	{
		nk_end(nk.ctx);
		return;
	}
	Window_EnableCursor();

	if (nk_window_is_active(nk.ctx, "Renderer panel"))
	{
		C_BlockInputThisFrame();
	}

	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "General", NK_MAXIMIZED))
	{

		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Window", NK_MINIMIZED))
		{
			int vsync_enabled = !r_cvars.w_useVsync->int_value;
			if (nk_checkbox_label(nk.ctx, "Vsync", &vsync_enabled))
			{
				Cvar_setValueDirectInt(r_cvars.w_useVsync, !r_cvars.w_useVsync->int_value);
			}


			nk_tree_pop(nk.ctx);
		}

		int enabled = !r_cvars.r_drawSky->int_value;
		nk_layout_row_dynamic(nk.ctx, 25, 1);
		if (nk_checkbox_label(nk.ctx, "Draw sky", &enabled))
		{
			Cvar_setValueDirectInt(r_cvars.r_drawSky, !r_cvars.r_drawSky->int_value);
		}
		
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Camera", NK_MAXIMIZED))
	{
		float fov_slider = r_cvars.cam_fov->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Fov %.2f", fov_slider);
		if (nk_slider_float(nk.ctx, 1, &fov_slider, 130, 0.1))
		{
			Cvar_setValueDirectFloat(r_cvars.cam_fov, fov_slider);
		}
		float zFar_slider = r_cvars.cam_zFar->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "zFar %.2f", zFar_slider);
		if (nk_slider_float(nk.ctx, 100, &zFar_slider, 4000, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.cam_zFar, zFar_slider);
		}
		float zNear_slider = r_cvars.cam_zNear->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "zNear %.2f", zNear_slider);
		if (nk_slider_float(nk.ctx, 0.01, &zNear_slider, 10, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.cam_zNear, zNear_slider);
		}
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Debug", NK_MAXIMIZED))
	{
		//DEBUG TEXTURES
		//-1 disabled, 0 = Normal, 1 = Albedo, 2 = Depth, 3 = Metal, 4 = Rough, 5 = AO, 6 = BLOOM
		static const char* texture_items[] = { "Disabled", "Normal", "Albedo", "Depth", "Metallic", "Roughness", "SSAO", "Bloom"};
		int selected_item = r_cvars.r_drawDebugTexture->int_value + 1;

		nk_label(nk.ctx, "Draw texture", NK_TEXT_ALIGN_LEFT);
		if (nk_combo_begin_label(nk.ctx, texture_items[selected_item], nk_vec2(nk_widget_width(nk.ctx), 200)))
		{
			nk_layout_row_dynamic(nk.ctx, 25, 1);

			for (int i = 0; i < 8; i++)
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
		int enabled = !r_cvars.r_useDirShadowMapping->int_value;
		nk_layout_row_dynamic(nk.ctx, 25, 1);
		if (nk_checkbox_label(nk.ctx, "Enabled", &enabled))
		{
			Cvar_setValueDirectInt(r_cvars.r_useDirShadowMapping, !r_cvars.r_useDirShadowMapping->int_value);
		}
		float bias_slider = r_cvars.r_shadowBias->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Bias %.2f", bias_slider);
		if (nk_slider_float(nk.ctx, -10.0, &bias_slider, 10.0, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.r_shadowBias, bias_slider);
		}
		float normal_bias_slider = r_cvars.r_shadowNormalBias->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Normal Bias %.2f", normal_bias_slider);
		if (nk_slider_float(nk.ctx, 0.0, &normal_bias_slider, 10.0, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.r_shadowNormalBias, normal_bias_slider);
		}
		float variance_min_slider = r_cvars.r_shadowVarianceMin->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Variance min %f", variance_min_slider);
		if (nk_slider_float(nk.ctx, 0.0, &variance_min_slider, 0.2, 0.001))
		{
			Cvar_setValueDirectFloat(r_cvars.r_shadowVarianceMin, variance_min_slider);
		}
		float light_bleed_reduction_slider = r_cvars.r_shadowLightBleedReduction->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Light bleed reduction %.2f", light_bleed_reduction_slider);
		if (nk_slider_float(nk.ctx, 0.0, &light_bleed_reduction_slider, 1.0, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.r_shadowLightBleedReduction, light_bleed_reduction_slider);
		}
		static const char* resolution_items[] = { "1", "640", "1280", "2560", "5120" };
	


		nk_tree_pop(nk.ctx);
	}
	
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "SSAO", NK_MINIMIZED))
	{
		int enabled = !r_cvars.r_useSsao->int_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		if (nk_checkbox_label(nk.ctx, "Enabled", &enabled))
		{
			Cvar_setValueDirectInt(r_cvars.r_useSsao, !r_cvars.r_useSsao->int_value);
		}
		int enabled_half_size = !r_cvars.r_ssaoHalfSize->int_value;
		if (nk_checkbox_label(nk.ctx, "Half size", &enabled_half_size))
		{
			Cvar_setValueDirectInt(r_cvars.r_ssaoHalfSize, !r_cvars.r_ssaoHalfSize->int_value);
		}
		float bias_slider = r_cvars.r_ssaoBias->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_label(nk.ctx, "Bias", NK_TEXT_ALIGN_LEFT);
		if (nk_slider_float(nk.ctx, 0.0, &bias_slider, 1.0, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.r_ssaoBias, bias_slider);
		}
		float radius_slider = r_cvars.r_ssaoRadius->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_label(nk.ctx, "Radius", NK_TEXT_ALIGN_LEFT);
		if (nk_slider_float(nk.ctx, 0, &radius_slider, 1.0, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.r_ssaoRadius, radius_slider);
		}
		float strength_slider = r_cvars.r_ssaoStrength->float_value;
		nk_layout_row_dynamic(nk.ctx, 25, 3);
		nk_label(nk.ctx, "Strength", NK_TEXT_ALIGN_LEFT);
		if (nk_button_label(nk.ctx, "Reset"))
		{
			Cvar_setValueToDefaultDirect(r_cvars.r_ssaoStrength);
		}
		if (nk_slider_float(nk.ctx, 0, &strength_slider, 8.0, 0.01))
		{
			Cvar_setValueDirectFloat(r_cvars.r_ssaoStrength, strength_slider);
		}
		nk_tree_pop(nk.ctx);
	}
	
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Post process", NK_MINIMIZED))
	{
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Monitor", NK_MINIMIZED))
		{
			float gamma_slider = r_cvars.r_Gamma->float_value;
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_labelf_wrap(nk.ctx, "Gamma %.2f", gamma_slider);
			if (nk_slider_float(nk.ctx, 0, &gamma_slider, 3, 0.01))
			{
				Cvar_setValueDirectFloat(r_cvars.r_Gamma, gamma_slider);
			}
			float exposure_slider = r_cvars.r_Exposure->float_value;
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_label(nk.ctx, "Exposure", NK_TEXT_ALIGN_LEFT);
			if (nk_slider_float(nk.ctx, 0, &exposure_slider, 16, 0.1))
			{
				Cvar_setValueDirectFloat(r_cvars.r_Exposure, exposure_slider);
			}
			float brightness_slider = r_cvars.r_Brightness->float_value;
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_label(nk.ctx, "Brightness", NK_TEXT_ALIGN_LEFT);
			if (nk_slider_float(nk.ctx, 0.01, &brightness_slider, 8, 0.1))
			{
				Cvar_setValueDirectFloat(r_cvars.r_Brightness, brightness_slider);
			}
			float contrast_slider = r_cvars.r_Contrast->float_value;
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_label(nk.ctx, "Contrast", NK_TEXT_ALIGN_LEFT);
			if (nk_slider_float(nk.ctx, 0.01, &contrast_slider, 8, 0.1))
			{
				Cvar_setValueDirectFloat(r_cvars.r_Contrast, contrast_slider);
			}
			float saturation_slider = r_cvars.r_Saturation->float_value;
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_label(nk.ctx, "Saturation", NK_TEXT_ALIGN_LEFT);
			if (nk_slider_float(nk.ctx, 0.01, &saturation_slider, 8, 0.1))
			{
				Cvar_setValueDirectFloat(r_cvars.r_Saturation, saturation_slider);
			}
			nk_tree_pop(nk.ctx);
		}
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Bloom", NK_MINIMIZED))
		{
			int enabled = !r_cvars.r_useBloom->int_value;
			nk_layout_row_dynamic(nk.ctx, 25, 1);
			if (nk_checkbox_label(nk.ctx, "Enabled", &enabled))
			{
				Cvar_setValueDirectInt(r_cvars.r_useBloom, !r_cvars.r_useBloom->int_value);
			}
			float strength_slider = r_cvars.r_bloomStrength->float_value;
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_labelf_wrap(nk.ctx, "Strength %.2f", strength_slider);
			if (nk_slider_float(nk.ctx, 0, &strength_slider, 1, 0.01))
			{
				Cvar_setValueDirectFloat(r_cvars.r_bloomStrength, strength_slider);
			}
			nk_tree_pop(nk.ctx);
		}
		/*
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Depth of Field", NK_MINIMIZED))
		{
			bool w = false;
			nk_layout_row_dynamic(nk.ctx, 25, 1);
			if (nk_checkbox_label(nk.ctx, "Enabled", &w))
			{
				
			}
			float slider = 0;
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_label(nk.ctx, "Strength", NK_TEXT_ALIGN_LEFT);
			if (nk_slider_float(nk.ctx, 0, &slider, 10, 1))
			{

			}
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_label(nk.ctx, "Focal intensity", NK_TEXT_ALIGN_LEFT);
			if (nk_slider_float(nk.ctx, 0, &slider, 10, 1))
			{

			}

			nk_tree_pop(nk.ctx);
		}
		*/
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Scene", NK_MINIMIZED))
	{
		nk_layout_row_dynamic(nk.ctx, 25, 4);
		nk_label(nk.ctx, "Direction", NK_TEXT_ALIGN_LEFT);
		nk_property_float(nk.ctx, "X", -1, &scene.scene_data.dirLightDirection[0], 1, 0.01, 0.01);
		nk_property_float(nk.ctx, "y", -1, &scene.scene_data.dirLightDirection[1], 1, 0.01, 0.01);
		nk_property_float(nk.ctx, "z", -1, &scene.scene_data.dirLightDirection[2], 1, 0.01, 0.01);
		nk_tree_pop(nk.ctx);

		struct nk_colorf sun_color;
		sun_color.r = scene.scene_data.dirLightColor[0];
		sun_color.g = scene.scene_data.dirLightColor[1];
		sun_color.b = scene.scene_data.dirLightColor[2];

		nk_layout_row_dynamic(nk.ctx, 150, 1);
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

		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Fog", NK_MINIMIZED))
		{
			{
				nk_layout_row_dynamic(nk.ctx, 25, 2);
				nk_labelf_wrap(nk.ctx, "Density %.2f", scene.scene_data.fogDensity);
				nk_slider_float(nk.ctx, 0, &scene.scene_data.fogDensity, 1, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 2);
				nk_labelf_wrap(nk.ctx, "Height fog min %.2f", scene.scene_data.heightFogMin);
				nk_slider_float(nk.ctx, 0, &scene.scene_data.heightFogMin, 4000, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 2);
				nk_labelf_wrap(nk.ctx, "Height fog max %.2f", scene.scene_data.heightFogMax);
				nk_slider_float(nk.ctx, 0, &scene.scene_data.heightFogMax, 4000, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 2);
				nk_labelf_wrap(nk.ctx, "Height fog curve %.2f", scene.scene_data.heightFogCurve);
				nk_slider_float(nk.ctx, 0, &scene.scene_data.heightFogCurve, 24, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 2);
				nk_labelf_wrap(nk.ctx, "Depth fog begin %.2f", scene.scene_data.depthFogBegin);
				nk_slider_float(nk.ctx, 0, &scene.scene_data.depthFogBegin, 4000, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 2);
				nk_labelf_wrap(nk.ctx, "Depth fog end %.2f", scene.scene_data.depthFogEnd);
				nk_slider_float(nk.ctx, 0, &scene.scene_data.depthFogEnd, 4000, 0.01);

				nk_layout_row_dynamic(nk.ctx, 25, 2);
				nk_labelf_wrap(nk.ctx, "Depth fog curve %.2f", scene.scene_data.depthFogCurve);
				nk_slider_float(nk.ctx, 0, &scene.scene_data.depthFogCurve, 24, 0.01);
			}

			nk_tree_pop(nk.ctx);
		}
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