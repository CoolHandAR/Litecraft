#include "c_cvars.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

/*
	Code inspired by quake3 cvar system
*/


#define MAX_CVARS 1024

#define FILE_HASH_SIZE 256

typedef struct C_CvarCore
{
	C_Cvar cvars[MAX_CVARS];
	int index_count;

	C_Cvar* hash_table[FILE_HASH_SIZE];
	C_Cvar* next;
} C_CvarCore;

static const char* s_empty_string;
static const char* s_number_string;
static C_CvarCore s_cvarCore;

static void _updateCvarNumValues(C_Cvar* p_cvar)
{
	if (!_strcmpi(p_cvar->str_value, "true"))
	{
		p_cvar->int_value = 1;
		p_cvar->float_value = 1;
	}
	else if (!_strcmpi(p_cvar->str_value, "false"))
	{
		p_cvar->int_value = 1;
		p_cvar->float_value = 1;
	}
	else
	{
		p_cvar->int_value = strtol(p_cvar->str_value, (char**)NULL, 10);
		p_cvar->float_value = strtod(p_cvar->str_value, (char**)NULL);
	}
}

static bool _validateString(const char *p_string)
{
	if (!p_string)
	{
		return false;
	}
	if (strchr(p_string, '\\'))
	{
		return false;
	}
	if (strchr(p_string, '\"'))
	{
		return false;
	}
	if (strchr(p_string, ';'))
	{
		return false;
	}

	return true;
}

static long _genHashValueForCvar(const char *p_string)
{
	int i = 0;
	long hash = 0;
	char letter = 0;

	while (p_string[i] != '0')
	{
		letter = tolower(p_string[i]);
		hash += (long)(letter) * (i + 119);
		i++;

		hash &= (FILE_HASH_SIZE - 1);
		return hash;
	}
}

static char* _safeCopyString(const char* p_source)
{
	char* out;

	if (!p_source[0])
	{
		return s_empty_string;
	}
	//else if (!p_source[1])
	//{
	//	if (p_source[0] >= '0' && p_source[0] <= '9')
	//	{
	//		return s_number_string;
	//	}
	//}

	out = malloc(strlen(p_source) + 1);
	if (out)
	{
		strcpy(out, p_source);
		return out;
	}

	return s_empty_string;
}

static C_Cvar* _findCvar(const char* p_cvarName)
{
	C_Cvar* cvar;
	long hash = 0;

	hash = _genHashValueForCvar(p_cvarName);

	for (cvar = s_cvarCore.hash_table[hash]; cvar; cvar = cvar->hash_next)
	{
		if (!_strcmpi(p_cvarName, cvar->name))
		{
			return cvar;
		}
	}

	return NULL;
}

static C_Cvar* _createCvar(const char* p_name, const char* p_value, int p_flags)
{
	if (s_cvarCore.index_count >= MAX_CVARS)
	{
		return;
	}
	if (!p_name || !p_value)
	{
		return NULL;
	}
	if (!_validateString(p_name))
	{
		return NULL;
	}
	if (!_validateString(p_value))
	{
		return NULL;
	}

	C_Cvar* var = NULL;
	
	var = &s_cvarCore.cvars[s_cvarCore.index_count];
	s_cvarCore.index_count++;

	var->name = _safeCopyString(p_name);
	var->str_value = _safeCopyString(p_value);
	var->default_value = _safeCopyString(p_value);
	_updateCvarNumValues(var);

	var->next = s_cvarCore.next;
	s_cvarCore.next = var;
	
	var->flags = p_flags;

	long hash = _genHashValueForCvar(var->name);
	var->hash_next = s_cvarCore.hash_table[hash];
	s_cvarCore.hash_table[hash] = var;

	return var;
}


int C_CvarCoreInit()
{
	memset(&s_cvarCore, 0, sizeof(C_CvarCore));

	s_empty_string = malloc(sizeof("EMPTY STRING!"));

	if (!s_empty_string)
	{
		C_CvarCoreCleanup();
		return -1;
	}
	s_number_string = malloc(sizeof("0000"));
	if (!s_number_string)
	{
		C_CvarCoreCleanup();
		return -1;
	}

	_createCvar("r_draw", "true", 0);
	_createCvar("sensitivity", "0.8", 0);

	return 1;
}

void C_CvarCoreCleanup()
{
	if (s_empty_string)
	{
		free(s_empty_string);
	}
	if (s_number_string)
	{
		free(s_number_string);
	}

	for (int i = 0; i < s_cvarCore.index_count; i++)
	{
		if (s_cvarCore.cvars[i].name && s_cvarCore.cvars[i].name != s_empty_string)
		{
			free(s_cvarCore.cvars[i].name);
		}
		if (s_cvarCore.cvars->str_value && s_cvarCore.cvars[i].name != s_empty_string)
		{
			free(s_cvarCore.cvars[i].str_value);
		}
		if (s_cvarCore.cvars->default_value && s_cvarCore.cvars[i].name != s_empty_string)
		{
			free(s_cvarCore.cvars[i].default_value);
		}
	}
}

C_Cvar* C_getCvar(const char* p_varName)
{
	return _findCvar(p_varName);
}


bool C_setCvarValueDirect(C_Cvar* const p_cvar, const char* p_value)
{
	if (!p_cvar)
	{
		return false;
	}
	if (!_validateString(p_value))
	{
		return false;
	}
	//can't change with this flag
	if (p_cvar->flags & CVAR__CONST)
	{
		return false;
	}
	//free the previous allocated str
	if (p_cvar->str_value)
	{
		free(p_cvar->str_value);
		p_cvar->str_value = _safeCopyString(p_value);

		if (p_cvar->str_value)
		{
			_updateCvarNumValues(p_cvar);
		}
		else 
		{
			p_cvar->float_value = 0;
			p_cvar->int_value = 0;

			return false;
		}
	}

	return true;
}

C_Cvar* C_cvarRegister(const char* p_varName, const char* p_value, int p_flags)
{
	return _createCvar(p_varName, p_value, p_flags);
}

bool C_setCvarValue(const char* p_varName, const char* p_value)
{
	if (!_validateString(p_varName))
	{
		return false;
	}
	if (!_validateString(p_value))
	{
		return false;
	}

	C_Cvar* cvar = C_getCvar(p_varName);

	if (!cvar)
	{
		return false;
	}

	return C_setCvarValueDirect(cvar, p_value);
}