#include "core/core_common.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>

#include "cvar.h"
#include "utility/u_utility.h"
#include "utility/u_math.h"

typedef struct
{
	struct nk_text_edit scroll_edit;
	struct nk_text_edit input_edit;
	char scroll_buffer[40000];
	char input_buffer[1024];
	char prev_input_buffer[1024];

	int scroll_total_lines;

	int suggestion_selection;
	int suggestion_count;

	struct nk_rect main_window_bounds;

	bool opened;
	bool force_input_edit_focus;
} ConsoleCore;

static ConsoleCore con_core;

extern NK_Data nk;
extern GLFWwindow* glfw_window;

static void Con_processCvarInput()
{
	bool name_found = false;
	bool value_found = false;
	char name_buffer[1024];
	char value_buffer[1024];

	char* token;
	char* search = " ";
	token = strtok(con_core.input_buffer, search); //bug strtok modifies the buffer

	if (token)
	{
		strncpy_s(name_buffer, 1024, token, strlen(token));
		name_found = true;
	}
		
	token = strtok(NULL, search);

	if (token)
	{
		strncpy_s(value_buffer, 1024, token, strlen(token));
		value_found = true;
	}
	
	if (!name_found)
		return;

	Cvar* cvar = Cvar_get(name_buffer);

	//not found?
	if (!cvar)
	{
		Con_printf("%s Variable not found", con_core.input_buffer);
	}
	//name found but not value?
	else if (!value_found)
	{
		//print the current cvar value and the help text if it exists
		if (cvar->help_text && !String_usingEmptyString(cvar->help_text))
		{
			Con_printf("%s %s \"%s\"", con_core.input_buffer, cvar->str_value, cvar->help_text);
		}
		else
		{
			Con_printf("%s %s", con_core.input_buffer, cvar->str_value);
		}
	}
	//value found but cvar is const?
	else if (value_found && cvar->flags & CVAR__CONST)
	{
		Con_printf("%s The cvar is marked const and can't be changed", con_core.input_buffer);
	}
	//if value is found, set it
	else if (value_found)
	{
		Cvar_setValueDirect(cvar, value_buffer);
		Con_printf("%s %s", con_core.input_buffer, cvar->str_value);
	}
}

static void Con_submitToScrollBuffer()
{
	if (nk_str_len(&con_core.input_edit.string) > 0)
	{
		//need to move memory back
		if (nk_str_len(&con_core.input_edit.string) + nk_str_len(&con_core.scroll_edit.string) >= 50 - 2)
		{
			//memmove_s(con_core.scroll_buff_c, 50, con_core.scroll_buff_c, 50);
		}
		
		Con_processCvarInput();

		//store the prev input
		memset(con_core.prev_input_buffer, 0, sizeof(con_core.prev_input_buffer));
		memcpy(con_core.prev_input_buffer, con_core.input_buffer, strlen(con_core.input_buffer));

		//clean up the input buffer
		memset(con_core.input_buffer, 0, sizeof(con_core.input_buffer));
		nk_textedit_delete(&con_core.input_edit, 0, nk_str_len(&con_core.input_edit.string));
		con_core.input_edit.select_start = 0;
		con_core.input_edit.select_end = 0;
	}
}
static void Con_handleInput()
{
	if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_CUT))
	{
		nk_textedit_cut(&con_core.input_edit);
	}
	else if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_TEXT_UNDO))
	{
		nk_textedit_undo(&con_core.input_edit);
	}
	else if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_TEXT_REDO))
	{
		nk_textedit_redo(&con_core.input_edit);
	}
	else if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_TEXT_SELECT_ALL))
	{
		nk_textedit_select_all(&con_core.input_edit);
	}
	else if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_DOWN) && con_core.suggestion_count > 0)
	{
		con_core.suggestion_selection++;
	}
	else if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_UP) && con_core.suggestion_count > 0)
	{
		con_core.suggestion_selection--;
	}
	else if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_ENTER) && con_core.suggestion_selection == -1)
	{
		Con_submitToScrollBuffer();
	}
	else if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_DOWN) && con_core.suggestion_count <= 0)
	{
		memcpy(con_core.input_buffer, con_core.prev_input_buffer, sizeof(con_core.prev_input_buffer));
		nk_textedit_text(&con_core.input_edit, con_core.input_buffer, strlen(con_core.input_buffer));
	}

	//Hack!!! this is so that when you press backspace to delete input, the nk code only check if pressed on this frame
	//so we need to reset the backspace state to false
	static float backspace_timer = 0;
	backspace_timer += Core_getDeltaTime();
	
	//make backspace delete faster when, backspace is held longer
	static float backspace_threshold = 0.0;
	if (nk_input_is_key_down(&nk.ctx->input, NK_KEY_BACKSPACE))
	{
		if (backspace_timer > backspace_threshold)
		{
			nk_input_key(nk.ctx, NK_KEY_BACKSPACE, nk_false);
			backspace_timer = 0;
		}

		backspace_threshold = max(backspace_threshold - Core_getDeltaTime(), 0.03);
	}
	else
	{
		backspace_threshold = 0.7;
		backspace_timer = 0;
	}

}

static void Con_HandleTextEditContextMenu(struct nk_rect p_triggerBounds)
{
	if (!nk_contextual_begin(nk.ctx, NK_WINDOW_BORDER, nk_vec2(100, 300), p_triggerBounds))
		return;

	nk_layout_row_dynamic(nk.ctx, 25, 1);

	if (nk_button_label(nk.ctx, "Cut"))
	{
		nk_textedit_cut(&con_core.input_edit);
		nk_contextual_close(nk.ctx);
	}

	if (nk_button_label(nk.ctx, "Undo"))
	{
		nk_textedit_undo(&con_core.input_edit);
		nk_contextual_close(nk.ctx);
	}

	nk_contextual_end(nk.ctx);
}

static void Con_drawSuggestionsWindow()
{
	const int input_str_len = nk_str_len(&con_core.input_edit.string);

	if (input_str_len <= 0)
	{
		con_core.suggestion_selection = -1;
		return;
	}

	char buffer[1024];
	//we need to copy to a c buffer since nk str is not null terminated
	strncpy_s(buffer, 1024, con_core.input_buffer, input_str_len);

	Cvar* cvars_suggestions[5];
	
	int suggestion_count = Cvar_StartWith(buffer, cvars_suggestions);
	con_core.suggestion_count = suggestion_count;

	//no suggestions found??
	if (suggestion_count <= 0)
	{
		con_core.suggestion_selection = -1;
		return;
	}

	//strech the window width to the max cvar name and value len 
	int max_str_len = 0;
	for (int i = 0; i < suggestion_count; i++)
	{
		Cvar* cvar = cvars_suggestions[i];

		int comb_len = strlen(cvar->name) + strlen(cvar->str_value);

		if (comb_len > max_str_len)
		{
			max_str_len = comb_len;
		}
	}

	//draw the suggestion box always below the main window
	if (!nk_begin(nk.ctx, "Cvar-Suggestions", nk_rect(con_core.main_window_bounds.x, con_core.main_window_bounds.y + (con_core.main_window_bounds.h),
		9.5 * max_str_len, 100), NK_WINDOW_DYNAMIC | NK_WINDOW_NO_SCROLLBAR))
		return;

	//clamp selection index
	con_core.suggestion_selection = Math_Clamp(con_core.suggestion_selection, 0, suggestion_count - 1);

	//also check for enter input
	if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_ENTER))
	{
		nk_str_clear(&con_core.input_edit.string);
		nk_textedit_text(&con_core.input_edit, cvars_suggestions[con_core.suggestion_selection]->name, strlen(cvars_suggestions[con_core.suggestion_selection]->name));
		nk_textedit_text(&con_core.input_edit, " ", 1); //add space after entering, so it is easier to input the value after

		con_core.input_edit.cursor = strlen(cvars_suggestions[con_core.suggestion_selection]->name) + 1;

		con_core.input_edit.active = 1;

		con_core.force_input_edit_focus = true;
	}

	static bool selected = false;
	nk_layout_row_dynamic(nk.ctx, 12, 1);
	for (int i = 0; i < suggestion_count; i++)
	{
		if (con_core.suggestion_selection == i || nk_widget_is_hovered(nk.ctx))
		{
			selected = true;
		}
		else
		{
			selected = false;
		}
		//concat name and value
		sprintf_s(buffer, 1024, "%s \"%s\"", cvars_suggestions[i]->name, cvars_suggestions[i]->str_value);
		
		if (nk_selectable_label(nk.ctx, buffer, NK_TEXT_LEFT, &selected))
		{
			con_core.suggestion_selection = i;

			nk_str_clear(&con_core.input_edit.string);
			nk_textedit_text(&con_core.input_edit, cvars_suggestions[con_core.suggestion_selection]->name, strlen(cvars_suggestions[con_core.suggestion_selection]->name));
			nk_textedit_text(&con_core.input_edit, " ", 1); //add space after entering, so it is easier to input the value after

			con_core.input_edit.cursor = strlen(cvars_suggestions[con_core.suggestion_selection]->name) + 1;

			con_core.input_edit.active = 1;

			con_core.force_input_edit_focus = true;
		}
	}
	//always make sure the window is focused
	nk_window_set_focus(nk.ctx, "Console");

	nk_end(nk.ctx);
}

static void Con_drawWindow()
{
	if (!nk_begin(nk.ctx, "Console", con_core.main_window_bounds,
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE
		| NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_CLOSABLE))
	{
		con_core.opened = false;
		nk_end(nk.ctx);
		return;
	}

	//always make sure the window is focused
	nk_window_set_focus(nk.ctx, "Console");

	//Draw scroll buffer log
	struct nk_vec2 window_size = nk_window_get_size(nk.ctx);
	nk_layout_row_dynamic(nk.ctx, window_size.y - 85, 1);

	//this allows scrolling the console log
	if (nk_group_begin(nk.ctx, "Log", NK_WINDOW_BORDER))
	{
		float height = (con_core.scroll_total_lines * 15.3 < window_size.y - 105) ? window_size.y - 105: con_core.scroll_total_lines * 15.3;
		nk_layout_row_dynamic(nk.ctx, height, 1);
		nk_edit_buffer(nk.ctx, NK_EDIT_MULTILINE | NK_EDIT_SELECTABLE, &con_core.scroll_edit, 0);

		nk_group_end(nk.ctx);
	}
	
	//Draw input buffer and sumbit button
	nk_layout_row_begin(nk.ctx, NK_DYNAMIC, 32, 2);
	nk_layout_row_push(nk.ctx, 0.85f);
	
	//input buffer bounds
	struct nk_rect input_edit_bounds = nk_widget_bounds(nk.ctx);

	//input buffer
	nk_edit_focus(nk.ctx, 0);

	nk_edit_buffer(nk.ctx, NK_EDIT_FIELD , &con_core.input_edit, 0);

	//text edit context menu
	Con_HandleTextEditContextMenu(input_edit_bounds);

	nk_layout_row_push(nk.ctx, 0.15f);
	//sumbit button
	if (nk_button_label(nk.ctx, "Sumbit"))
	{
		Con_submitToScrollBuffer();
	}
	nk_layout_row_end(nk.ctx);

	//update main window bounds
	con_core.main_window_bounds = nk_window_get_bounds(nk.ctx);

	nk_end(nk.ctx);

}


void Con_printf(const char* fmt, ...)
{
	assert(!strchr(fmt, '\n') && "New line character not allowed in Con_printf(..)");

    va_list args;
    va_start(args, fmt);

    char buffer[2048];
	memset(buffer, 0, sizeof(buffer));

    vsprintf(buffer, fmt, args);

    va_end(args);
	
	nk_str_append_str_char(&con_core.scroll_edit.string, buffer);

	//append new line 
	nk_rune rune = '\n';
	nk_str_append_text_runes(&con_core.scroll_edit.string, &rune, 1);

	con_core.scroll_total_lines++;
}

bool Con_isOpened()
{
	return con_core.opened;
}

void Con_Update()
{
	if (!nk.enabled)
		return;

	//check for input
	if (nk_input_is_key_pressed(&nk.ctx->input, NK_KEY_TAB))
	{
		con_core.opened = !con_core.opened;
	}

	if (con_core.opened)
	{
		Window_EnableCursor();
	}
	
	if (!con_core.opened)
		return;


	//Handle kb input
	Con_handleInput();
	
	if (con_core.input_edit.active)
	{
		Con_drawSuggestionsWindow();
	}

	Con_drawWindow();
}

int Con_Init()
{
	memset(&con_core, 0, sizeof(ConsoleCore));

	nk_textedit_init_fixed(&con_core.input_edit, con_core.input_buffer, sizeof(con_core.input_buffer));
	nk_textedit_init_fixed(&con_core.scroll_edit, con_core.scroll_buffer, sizeof(con_core.scroll_buffer));
	
	con_core.suggestion_selection = -1;

	con_core.main_window_bounds = nk_rect(520, 50, 500, 500);
}

void Con_Cleanup()
{
	nk_textedit_free(&con_core.scroll_edit);
	nk_textedit_free(&con_core.input_edit);
}


