#pragma once
#include <stdbool.h>

typedef enum CvarFlags
{
	CVAR__CONST = 1 << 0, //cvar can't be changed from its initial value
	CVAR__SAVE_TO_FILE = 1 << 1
} CvarFlags;

typedef struct Cvar
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

	struct Cvar* next;
	struct Cvar* hash_next;

} Cvar;


int Cvar_Init();
void Cvar_Cleanup();

Cvar* Cvar_get(const char* p_varName);
bool Cvar_setValue(const char* p_varName, const char* p_value);
bool Cvar_setValueDirect(Cvar* const p_cvar, const char* p_value);
Cvar* Cvar_Register(const char* p_varName, const char* p_value, const char* p_helpText, int p_flags, float p_minValue, float p_maxValue);
void Cvar_ResetAllToDefault();
bool Cvar_PrintAllToFile(const char* p_filePath);
bool Cvar_LoadAllFromFile(const char* p_filePath);
int Cvar_StartWith(const char* p_startsWith, Cvar* r_cvars[5]);