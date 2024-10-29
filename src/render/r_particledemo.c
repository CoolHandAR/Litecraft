#include "r_public.h"
#include "input.h"
#include "core/c_common.h"
#include <core/resource_manager.h>

extern NK_Data nk;
extern GLFWwindow* glfw_window;

static ParticleEmitterSettings* e;

static bool s_opened;

void ParticleDemo_Init()
{

	s_opened = false;
	Input_registerAction("Open particle demo");
	Input_setActionBinding("Open particle demo", IT__KEYBOARD, GLFW_KEY_O, 0);

	e = Particle_RegisterEmitter();

	e->texture = Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE);

	e->settings.particle_amount = 1;
	glm_mat4_identity(e->settings.xform);
	e->settings.state_flags |= EMITTER_STATE_FLAG__EMITTING;
	e->settings.life_time = 4;
	e->settings.direction[0] = -1;
	e->settings.initial_velocity = 5;
	e->settings.speed_scale = 1;
	e->settings.explosiveness = 0;
	e->settings.randomness = 0.5;
	e->settings.frame_count = 0;
	e->settings.frame = 13;
	e->settings.scale = 1.1;
	e->settings.life_time = 8.5;
	e->settings.anim_speed_scale = 0;
	e->settings.h_frames = 25;
	e->settings.v_frames = 25;

	e->settings.xform[3][0] += 14.5;
	e->settings.xform[3][1] += 16.5;
	e->settings.xform[3][2] += 8.5;

	e->aabb[0][0] = -5;
	e->aabb[0][1] = -5;
	e->aabb[0][2] = -5;
	e->aabb[1][0] = 5;
	e->aabb[1][1] = 5;
	e->aabb[1][2] = 5;

	glm_mat4_mulv3(e->settings.xform, e->aabb[0], 1.0, e->settings.aabb[0]);
	glm_mat4_mulv3(e->settings.xform, e->aabb[1], 1.0, e->settings.aabb[1]);

	Particle_MarkUpdate(e);
}

void ParticleDemo_Main()
{
	if (Input_IsActionJustPressed("Open particle demo") && !Con_isOpened())
	{
		s_opened = !s_opened;
	}

	if (!s_opened)
	{
		return;
	}


	if (!nk_begin(nk.ctx, "Particle demo panel", nk_rect(200, 20, 300, 300), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_CLOSABLE))
	{
		nk_end(nk.ctx);
		return;
	}

	if (nk_window_is_active(nk.ctx, "Particle demo panel") && nk_window_is_hovered(nk.ctx))
	{
		C_BlockInputThisFrame();
	}
	
	bool update = false;

	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "General", NK_MAXIMIZED))
	{
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		int playing = !(e->settings.state_flags & EMITTER_STATE_FLAG__EMITTING);
		if (nk_checkbox_label(nk.ctx, "Playing", &playing))
		{
			e->settings.state_flags ^= EMITTER_STATE_FLAG__EMITTING;
			update = true;
		}
		int one_shot = !(e->settings.settings_flags & EMITTER_SETTINGS_FLAG__ONE_SHOT);
		if (nk_checkbox_label(nk.ctx, "One shot", &one_shot))
		{
			e->settings.settings_flags ^= EMITTER_SETTINGS_FLAG__ONE_SHOT;
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Amount %d", e->settings.particle_amount);
		if (nk_slider_int(nk.ctx, 0, &e->settings.particle_amount, 128, 1))
		{
			update = true;
		}

		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Lifetime %.2f", e->settings.life_time);
		if (nk_slider_float(nk.ctx, 0.1, &e->settings.life_time, 500, 0.1))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Speed scale %.2f", e->settings.speed_scale);
		if (nk_slider_float(nk.ctx, 0, &e->settings.speed_scale, 64, 0.1))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Explosivness %.2f", e->settings.explosiveness);
		if (nk_slider_float(nk.ctx, 0, &e->settings.explosiveness, 1, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Randomness %.2f", e->settings.randomness);
		if (nk_slider_float(nk.ctx, 0, &e->settings.randomness, 1, 0.01))
		{
			update = true;
		}
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Flags", NK_MINIMIZED))
	{
		int cast_shadows = !(e->settings.settings_flags & EMITTER_SETTINGS_FLAG__CAST_SHADOWS);
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		if (nk_checkbox_label(nk.ctx, "Cast shadows", &cast_shadows))
		{
			e->settings.settings_flags ^= EMITTER_SETTINGS_FLAG__CAST_SHADOWS;
			update = true;
		}
		int animation_enabled = !(e->settings.settings_flags & EMITTER_SETTINGS_FLAG__ANIMATION);
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		if (nk_checkbox_label(nk.ctx, "Animation", &animation_enabled))
		{
			e->settings.settings_flags ^= EMITTER_SETTINGS_FLAG__ANIMATION;
			update = true;
		}

		nk_tree_pop(nk.ctx);
	}

	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Velocity", NK_MINIMIZED))
	{
		nk_layout_row_dynamic(nk.ctx, 25, 4);
		nk_labelf_wrap(nk.ctx, "Direction");
		float x = e->settings.direction[0];
		float y = e->settings.direction[1];
		float z = e->settings.direction[2];
		nk_property_float(nk.ctx, "X", -1, &x, 1, 0.01, 0.01);
		nk_property_float(nk.ctx, "Y", -1, &y, 1, 0.01, 0.01);
		nk_property_float(nk.ctx, "Z", -1, &z, 1, 0.01, 0.01);

		if (x != e->settings.direction[0]) { e->settings.direction[0] = x; update = true; };
		if (y != e->settings.direction[1]) { e->settings.direction[1] = y; update = true; };
		if (z != e->settings.direction[2]) { e->settings.direction[2] = z; update = true; };
		
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Spread %.2f", e->settings.spread);
		if (nk_slider_float(nk.ctx, 0, &e->settings.spread, 180, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Gravity %.2f", e->settings.gravity);
		if (nk_slider_float(nk.ctx, -100, &e->settings.gravity, 100, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Friction %.2f", e->settings.friction);
		if (nk_slider_float(nk.ctx, 0, &e->settings.friction, 100, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Initial velocity %.2f", e->settings.initial_velocity);
		if (nk_slider_float(nk.ctx, 0, &e->settings.initial_velocity, 1000, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Linear accel %.2f", e->settings.linear_accel);
		if (nk_slider_float(nk.ctx, -100, &e->settings.linear_accel, 100, 0.01))
		{
			update = true;
		}
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Emission", NK_MAXIMIZED))
	{
		static const char* emission_items[] = { "Point", "Box", "Sphere" };
		int selected_item = e->settings.emission_shape;

		nk_label(nk.ctx, "Shape", NK_TEXT_ALIGN_LEFT);
		if (nk_combo_begin_label(nk.ctx, emission_items[selected_item], nk_vec2(nk_widget_width(nk.ctx), 200)))
		{
			nk_layout_row_dynamic(nk.ctx, 25, 1);

			for (int i = 0; i < 3; i++)
			{
				if (nk_combo_item_label(nk.ctx, emission_items[i], NK_TEXT_LEFT))
				{
					e->settings.emission_shape = i;
				}
			}
			nk_combo_end(nk.ctx);
		}

		switch (e->settings.emission_shape)
		{
		case EES__POINT:
		{
			//do nothing
			break;
		}
		case EES__BOX:
		{
			nk_layout_row_dynamic(nk.ctx, 25, 4);
			nk_labelf_wrap(nk.ctx, "Box size");
			float x = e->settings.emission_size[0];
			float y = e->settings.emission_size[1];
			float z = e->settings.emission_size[2];
			nk_property_float(nk.ctx, "X", -100000, &x, 100000, 0.01, 0.01);
			nk_property_float(nk.ctx, "Y", -100000, &y, 100000, 0.01, 0.01);
			nk_property_float(nk.ctx, "Z", -100000, &z, 100000, 0.01, 0.01);

			if (x != e->settings.emission_size[0]) { e->settings.emission_size[0] = x; update = true; };
			if (y != e->settings.emission_size[1]) { e->settings.emission_size[1] = y; update = true; };
			if (z != e->settings.emission_size[2]) { e->settings.emission_size[2] = z; update = true; };

			break;
		}
		case EES__SPHERE:
		{
			nk_layout_row_dynamic(nk.ctx, 25, 2);
			nk_labelf_wrap(nk.ctx, "Radius %.2f", e->settings.emission_size[0]);
			
			if (nk_slider_float(nk.ctx, 0.01, &e->settings.emission_size[0], 128, 0.01))
			{
				update = true;
			}

			break;
		}
		default:
			break;
		}


		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Transform", NK_MINIMIZED))
	{
		nk_layout_row_dynamic(nk.ctx, 25, 4);
		nk_labelf_wrap(nk.ctx, "Origin");
		float x = e->settings.xform[3][0];
		float y = e->settings.xform[3][1];
		float z = e->settings.xform[3][2];
		nk_property_float(nk.ctx, "X", -100000, &x, 100000, 0.01, 0.01);
		nk_property_float(nk.ctx, "Y", -100000, &y, 100000, 0.01, 0.01);
		nk_property_float(nk.ctx, "Z", -100000, &z, 100000, 0.01, 0.01);

		if (x != e->settings.xform[3][0]) { e->settings.xform[3][0] = x; update = true; };
		if (y != e->settings.xform[3][1]) { e->settings.xform[3][1] = y; update = true; };
		if (z != e->settings.xform[3][2]) { e->settings.xform[3][2] = z; update = true; };

		glm_mat4_mulv3(e->settings.xform, e->aabb[0], 1.0, e->settings.aabb[0]);
		glm_mat4_mulv3(e->settings.xform, e->aabb[1], 1.0, e->settings.aabb[1]);

		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Scale %.2f", e->settings.scale);
		if (nk_slider_float(nk.ctx, 0.01, &e->settings.scale, 1000, 0.01))
		{
			update = true;

		}
		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Animation", NK_MINIMIZED))
	{
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		int looping = !(e->settings.settings_flags & EMITTER_SETTINGS_FLAG__LOOP_ANIMATION);
		if (nk_checkbox_label(nk.ctx, "Loop", &looping))
		{
			e->settings.settings_flags ^= EMITTER_SETTINGS_FLAG__LOOP_ANIMATION;
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 4);
		nk_labelf_wrap(nk.ctx, "H Frames %d", e->settings.h_frames);
		if (nk_slider_int(nk.ctx, 0, &e->settings.h_frames, 1000, 1))
		{
			update = true;
		}
		nk_labelf_wrap(nk.ctx, "V Frames %d", e->settings.v_frames);
		if (nk_slider_int(nk.ctx, 0, &e->settings.v_frames, 1000, 1))
		{
			update = true;
		}

		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Frame count %d", e->settings.frame_count);
		if (nk_slider_int(nk.ctx, 0, &e->settings.frame_count, 1000, 1))
		{
			update = true;
		}

		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Animation speed scale %.2f", e->settings.anim_speed_scale);
		if (nk_slider_float(nk.ctx, 0, &e->settings.anim_speed_scale, 1000, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Frame %d", e->settings.frame);
		if (nk_slider_int(nk.ctx, 0, &e->settings.frame, e->settings.frame_count, 1))
		{
			update = true;
		}

		nk_tree_pop(nk.ctx);
	}
	if (nk_tree_push(nk.ctx, NK_TREE_NODE, "Lighting", NK_MINIMIZED))
	{	
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Ambient intensity %.2f", e->settings.ambient_intensity);
		if (nk_slider_float(nk.ctx, 0, &e->settings.ambient_intensity, 1, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Diffuse intensity %.2f", e->settings.diffuse_intensity);
		if (nk_slider_float(nk.ctx, 0, &e->settings.diffuse_intensity, 1, 0.01))
		{
			update = true;
		}
		nk_layout_row_dynamic(nk.ctx, 25, 2);
		nk_labelf_wrap(nk.ctx, "Specular intensity %.2f", e->settings.specular_intensity);
		if (nk_slider_float(nk.ctx, 0, &e->settings.specular_intensity, 1, 0.01))
		{
			update = true;
		}
		nk_tree_pop(nk.ctx);
	}
	if (update)
	{
		Particle_MarkUpdate(e);
	}

	nk_end(nk.ctx);
}