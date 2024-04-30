#include "c_console.h"

#include "render/r_texture.h"
#include "render/r_renderer.h"
#include <string.h>
#include <stdarg.h>

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

	bool is_open;

} C_Console;

C_Console s_console;

static void _WriteCharToInputBuffer(char ch)
{
	if (!s_console.is_open)
	{
		return;
	}

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
	if (!s_console.is_open)
	{
		return;
	}

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

static void _submitInput()
{
	if (!s_console.is_open)
	{
		return;
	}

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
		C_ConsolePrint(" Variable not found");
		C_ConsoleNewline();
	}
	else if (cvar->flags & CVAR__CONST)
	{
		C_ConsolePrint(" Variable can't be changed");
		C_ConsoleNewline();
	}
	else if (found_space)
	{
		C_setCvarValueDirect(cvar, &value_buffer);
		C_ConsolePrint(" ");
		C_ConsolePrint(&value_buffer);
		C_ConsoleNewline();
	}
	else
	{
		C_ConsolePrint(" == ");
		C_ConsolePrint(cvar->str_value);
		C_ConsolePrint("  ");
		C_ConsolePrint(cvar->help_text);
		C_ConsoleNewline();
	}
	
	memset(s_console.input_buffer, 0, sizeof(s_console.input_buffer));
	s_console.input_position = -1;
}

void C_ConsoleInit()
{
	memset(&s_console, 0, sizeof(C_Console));
	s_console.input_position = -1;

	s_console.bg_texture = Texture_Load("assets/ui/console_bg.png", NULL);

	vec3 pos;
	pos[0] = 400;
	pos[1] = 0;
	pos[2] = 0;

	vec2 scale;
	scale[0] = 0.6;
	scale[1] = 2;
	

	Sprite_Init(&s_console.bg_sprite, pos, scale, 0, &s_console.bg_texture);
	

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
	//toggle console
	else if (uni_code == 96)
	{
		s_console.is_open = !s_console.is_open;
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
	if (!s_console.is_open)
		return;

	//draw background
	r_DrawScreenSprite(&s_console.bg_sprite);

	//draw input
	r_DrawScreenText2(s_console.input_buffer, 0, 265, 16, 18, 1, 1, 1, 1, 0.0, 0.0);

	//draw scroll backLines
	r_DrawScreenText2(s_console.scroll_back_buffer, 0, 210 - (s_console.total_text_lines * 18), 18, 18, 1, 1, 1, 1, 0.0, 0.0);
}

void C_UpdateConsole()
{
}

void C_ConsolePrint(const char* p_msg)
{
	const int str_len = strlen(p_msg);

	if (str_len + 1 >= MAX_SCROLL_BACK_BUFFER_SIZE)
	{
		return;
	}

	strncat(s_console.scroll_back_buffer, p_msg, str_len);
}

void C_ConsoleNewline()
{
	char new_line_ch = '\n';

	strncat(s_console.scroll_back_buffer, &new_line_ch, sizeof(char));
	s_console.total_text_lines++;
}

static void _printCharToScrollBackBuffer(char ch)
{
	if (1 >= MAX_SCROLL_BACK_BUFFER_SIZE)
	{
		return;
	}
	
	strncat(s_console.scroll_back_buffer, &ch, sizeof(char));
}

void C_ConsolePrintf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char ch = 0;

	while (*fmt != '\0')
	{
		ch = *fmt;

		//look for special characters
		if (ch == '%')
		{
			fmt++;
			ch = *fmt;

			char num_buf[24];
			memset(num_buf, 0, sizeof(num_buf));

			switch (ch)
			{
			/* FALLTHROUGH */
			case 'i':
			case 'd':
			{
				int integer = va_arg(args, int);
				sprintf(num_buf, "%i", integer);
				C_ConsolePrint(num_buf);
				break;
			}
			case 'u':
			{
				unsigned int u_integer = va_arg(args, unsigned int);
				sprintf(num_buf, "%i", u_integer);
				C_ConsolePrint(num_buf);
				break;
			}
			/* FALLTHROUGH */
			case 'f':
			case 'F':
			{
				float floating_point = va_arg(args, float);
				sprintf(num_buf, "%f", floating_point);
				C_ConsolePrint(num_buf);
				break;
			}
			case 's':
			{
				char* ch = va_arg(args, char*);
				C_ConsolePrint(ch);
			}
			default:
				break;
			}
		}
		//otherwise we print
		else
		{
			_printCharToScrollBackBuffer(ch);
		}
		fmt++;		
	}

	va_end(args);
}
