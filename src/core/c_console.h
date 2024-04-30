#pragma once

#include <stdbool.h>

void C_ConsoleInit();
void C_KB_Input(int uni_code);
void C_ToggleConsole();
void C_DrawConsole();
void C_UpdateConsole();

void C_ConsolePrint(const char* p_msg);
void C_ConsoleNewline();
void C_ConsolePrintf(const char* fmt, ...);