/*
    Debug/Testing Panel for various renderer items
*/

#include "render/r_core.h"
#include "core/c_common.h"

extern NK_Data nk;
extern GLFWwindow* glfw_window;
extern R_Cvars r_cvars;

void RPanel_Main()
{
	glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	if (!nk_begin(nk.ctx, "Renderer panel", nk_rect(200, 20, 300, 300), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_CLOSABLE))
	{
		nk_end(nk.ctx);
		return;
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "General", NK_MAXIMIZED))
	{
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
		if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Bloom", NK_MINIMIZED))
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
			nk_tree_pop(nk.ctx);
		}
		*/
		nk_tree_pop(nk.ctx);
	}
	
	nk_end(nk.ctx);
}

void RPanel_Metrics()
{

}