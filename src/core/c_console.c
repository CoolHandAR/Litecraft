#include "c_console.h"

#include "render/r_texture.h"
#include "render/r_renderer.h"
#include <string.h>

#include "c_cvars.h"

#define MAX_SCROLL_BACK_BUFFER_SIZE 10240
#define MAX_INPUT_BUFFER_SIZE 256

typedef struct C_Console
{
	R_Texture bg_texture;
	R_Sprite bg_sprite;

	char scroll_back_buffer[MAX_SCROLL_BACK_BUFFER_SIZE];
	int scroll_back_position;

	int total_text_lines;

	char input_buffer[MAX_INPUT_BUFFER_SIZE];
	int input_position;

	float text_height;

} C_Console;

C_Console s_console;

static void _WriteCharToInputBuffer(char ch)
{
	s_console.input_position++;
	if (s_console.input_position >= MAX_INPUT_BUFFER_SIZE)
	{
		s_console.input_position = MAX_INPUT_BUFFER_SIZE - 1;
		return;
	}
	s_console.input_buffer[s_console.input_position] = ch;
}
static void _backSpaceInputBuffer()
{
	if (s_console.input_buffer[s_console.input_position] == '\n')
	{
		return;
	}

	s_console.input_buffer[s_console.input_position] = 0;

	if (s_console.input_position >= 0)
	{
		s_console.input_position--;
	}
}




static bool _validateChar()
{

}

static void _submitInput()
{
	if (s_console.input_position + 2 >= MAX_SCROLL_BACK_BUFFER_SIZE - 1 || s_console.input_position < 0)
	{
		return;
	}


	char value_buffer[256];
	memset(value_buffer, 0, sizeof(value_buffer));
	int value_index = 0;

	bool found_space = false;
	for (int i = 0; i < s_console.input_position + 1; i++)
	{
		int j = (i + 1) % s_console.input_position;
		char ch = s_console.input_buffer[i];
		char next_char = s_console.input_buffer[j];

		if (ch == ' ')
		{
			s_console.input_buffer[i] = 0;
			found_space = true;
			continue;
		}
		
		if (found_space)
		{
			s_console.input_buffer[i] = 0;
			if (ch == ' ' || ch == '\n')
			{
				break;
			}

			value_buffer[value_index] = ch;
			value_index++;
		}

	}


	C_Cvar* cvar = C_getCvar(s_console.input_buffer);

	C_ConsolePrint(s_console.input_buffer, false);

	//cvar not found?
	if (!cvar)
	{
		C_ConsolePrint(" Variable not found", true);
	}
	else if (cvar->flags & CVAR__CONST)
	{
		C_ConsolePrint(" Variable can't be changed", true);
	}
	else if (found_space)
	{
		C_setCvarValueDirect(cvar, &value_buffer);
		C_ConsolePrint(" ", false);
		C_ConsolePrint(&value_buffer, true);
	}
	else
	{
		C_ConsolePrint(" == ", false);
		C_ConsolePrint(cvar->str_value, true);
	}
	
	memset(s_console.input_buffer, 0, sizeof(s_console.input_buffer));
	s_console.input_position = -1;
}

void C_ConsoleInit()
{
	memset(&s_console, 0, sizeof(C_Console));
	s_console.input_position = -1;

	s_console.bg_texture = Texture_Load("assets/ui/console_bg.png", NULL);

	Sprite_setTexture(&s_console.bg_sprite, &s_console.bg_texture);

	s_console.bg_sprite.scale[0] = 1;
	s_console.bg_sprite.scale[1] = 1;

	s_console.text_height = 356;
}

void C_KB_Input(int uni_code)
{
	//backspace
	if (uni_code == 259)
	{
		_backSpaceInputBuffer();
	}
	//new line
	else if (uni_code == 257)
	{
		_submitInput();
	}
	else if (uni_code >= 32 && uni_code <= 126)
	{
		_WriteCharToInputBuffer(uni_code);
	}

}

void C_ToggleConsole()
{
}

void C_DrawConsole()
{
	//printf("%i \n", s_console.input_position);
	vec2 pos;
	pos[0] = 200;
	pos[1] = 0;


	//draw background
	r_DrawScreenSprite(pos, &s_console.bg_sprite);

	//draw input
	r_DrawScreenText2(s_console.input_buffer, 0, 358, 18, 18, 1, 1, 1, 1, 0.0, 0.0);

	//draw scroll backLines
	r_DrawScreenText2(s_console.scroll_back_buffer, 0, 345 - (s_console.total_text_lines * 18), 18, 18, 1, 1, 1, 1, 0.0, 0.0);
}

void C_UpdateConsole()
{
}

void C_ConsolePrint(const char* p_msg, bool p_newLine)
{
	const int str_len = strlen(p_msg);

	if (str_len + 1 >= MAX_SCROLL_BACK_BUFFER_SIZE)
	{
		return;
	}

	strncat(s_console.scroll_back_buffer, p_msg, str_len);

	if (p_newLine)
	{
		char new_line_ch = '\n';

		strncat(s_console.scroll_back_buffer, &new_line_ch, sizeof(char));
	}
}
