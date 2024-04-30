#pragma once
#include <stdbool.h>

typedef enum C_CvarFlags
{
	CVAR__CONST = 1 << 0, //cvar can't be changed from its initial value
	CVAR__SAVE_TO_FILE = 1 << 1
} C_CvarFlags;

typedef struct C_Cvar
{
	char* name;
	char* help_text;
	char* default_value;
	char* str_value;

	float float_value;
	int int_value;

	int flags;

	float max_value;
	float min_value;

	bool modified;

	struct C_Cvar* next;
	struct C_Cvar* hash_next;

} C_Cvar;

int C_CvarCoreInit();
void C_CvarCoreCleanup();

C_Cvar* C_getCvar(const char* p_varName);
bool C_setCvarValue(const char* p_varName, const char* p_value);
bool C_setCvarValueDirect(C_Cvar* const p_cvar, const char* p_value);
C_Cvar* C_cvarRegister(const char* p_varName, const char* p_value, const char* p_helpText, int p_flags, float p_minValue, float p_maxValue);
void C_cvarResetToDefault(const char* p_varName);
void C_cvarResetAllToDefault();
bool C_cvarPrintAllToFile(const char* p_filePath);
bool C_cvarLoadAllFromFile(const char* p_filePath);