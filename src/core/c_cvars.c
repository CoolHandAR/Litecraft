#include "c_cvars.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

#include "utility/u_file.h"
#include <assert.h>

/*
	Code inspired by quake3 cvar system
*/

#define MAX_CVARS 1024
#define MAX_CVAR_CHAR_SIZE 256
#define FILE_HASH_SIZE 256

typedef struct C_CvarCore
{
	C_Cvar cvars[MAX_CVARS];
	int index_count;

	C_Cvar* hash_table[FILE_HASH_SIZE];
	C_Cvar* next;

	char* empty_string;
} C_CvarCore;

static C_CvarCore s_cvarCore;

static void _updateCvarNumValues(C_Cvar* p_cvar)
{
	//check for boolean types
	if (!_strcmpi(p_cvar->str_value, "true"))
	{
		p_cvar->int_value = 1;
		p_cvar->float_value = 1;
	}
	else if (!_strcmpi(p_cvar->str_value, "false"))
	{
		p_cvar->int_value = 0;
		p_cvar->float_value = 0;
	}
	else
	{
		//both of these functions return 0 if they can't convert the values to a number
		p_cvar->int_value = strtol(p_cvar->str_value, (char**)NULL, 10);
		p_cvar->float_value = strtod(p_cvar->str_value, (char**)NULL);

		//limit values
		if (p_cvar->float_value > p_cvar->max_value)
		{
			p_cvar->float_value = p_cvar->max_value;
			p_cvar->int_value = p_cvar->max_value;
		}
		else if (p_cvar->float_value < p_cvar->min_value)
		{
			p_cvar->float_value = p_cvar->min_value;
			p_cvar->int_value = p_cvar->min_value;
		}
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
	if (strchr(p_string, '\n'))
	{
		return false;
	}
	if (strlen(p_string) >= MAX_CVAR_CHAR_SIZE)
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

	while (p_string[i] != '\0')
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

	if (!p_source)
	{
		return s_cvarCore.empty_string;
	}
	out = malloc(strlen(p_source) + 1);
	if (out)
	{
		strcpy(out, p_source);
		return out;
	}

	return s_cvarCore.empty_string;
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

static C_Cvar* _createCvar(const char* p_name, const char* p_value, const char* p_helpText, int p_flags, float p_minValue, float p_maxValue)
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
	var->help_text = _safeCopyString(p_helpText);
	var->min_value = p_minValue;
	var->max_value = p_maxValue;
	var->modified = false;

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

	s_cvarCore.empty_string = malloc(sizeof(""));

	if (!s_cvarCore.empty_string)
	{
		C_CvarCoreCleanup();
		return -1;
	}

	memcpy(s_cvarCore.empty_string, "", sizeof(""));

	return 1;
}

void C_CvarCoreCleanup()
{
	for (int i = 0; i < s_cvarCore.index_count; i++)
	{
		C_Cvar* cvar = &s_cvarCore.cvars[i];

		if (cvar->name && cvar->name != s_cvarCore.empty_string)
		{
			free(cvar->name);
		}
		if (cvar->str_value && cvar->str_value != s_cvarCore.empty_string)
		{
			free(cvar->str_value);
		}
		if (cvar->default_value && cvar->default_value != s_cvarCore.empty_string)
		{
			free(cvar->default_value);
		}
	}

	if (s_cvarCore.empty_string)
	{
		free(s_cvarCore.empty_string);
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

		p_cvar->modified = true;
	}
	return true;
}

C_Cvar* C_cvarRegister(const char* p_varName, const char* p_value, const char* p_helpText, int p_flags, float p_minValue, float p_maxValue)
{
	return _createCvar(p_varName, p_value, p_helpText, p_flags, p_minValue, p_maxValue);
}

void C_cvarResetToDefault(const char* p_varName)
{
}

void C_cvarResetAllToDefault()
{
	for (int i = 0; i < s_cvarCore.index_count; i++)
	{
		C_setCvarValueDirect(&s_cvarCore.cvars[i], s_cvarCore.cvars[i].default_value);
	}
}

bool C_cvarPrintAllToFile(const char* p_filePath)
{
	FILE* out_file = NULL;
	fopen_s(&out_file, p_filePath, "w");
	if (!out_file)
	{
		printf("Failed to open file!\n");
		return false;
	}
	//use this since since fwrite causes error when using directly
	const char quote_char = '"';

	for (int i = 0; i < s_cvarCore.index_count; i++)
	{
		C_Cvar* cvar = &s_cvarCore.cvars[i];
		
		//skip
		if (!(cvar->flags & CVAR__SAVE_TO_FILE))
		{
			continue;
		}
		//skip if the value is same as default
		if (!_strcmpi(cvar->str_value, cvar->default_value))
		{
			continue;
		}

		fwrite(cvar->name, sizeof(char), strlen(cvar->name), out_file);
		fwrite(" ", sizeof(char), 1, out_file);
		
		//wrap the value in quotes
		fwrite(&quote_char, sizeof(char), 1, out_file);
		fwrite(cvar->str_value, sizeof(char), strlen(cvar->str_value), out_file);
		fwrite(&quote_char, sizeof(char), 1, out_file);

		fwrite("\n", sizeof(char), 1, out_file);
	}

	return fclose(out_file) == 0;
}

bool C_cvarLoadAllFromFile(const char* p_filePath)
{
	//maybe move this to json later

	char* parsed_string = File_ParseString(p_filePath);

	if (!parsed_string)
	{
		return false;
	}
	const int str_len = strlen(parsed_string);

	char name_buf[MAX_CVAR_CHAR_SIZE];
	char value_buf[MAX_CVAR_CHAR_SIZE];
	memset(name_buf, 0, sizeof(name_buf));
	memset(value_buf, 0, sizeof(value_buf));
	
	int str_index = 0;
	int buf_index = 0;
	int parse_stage = 1;
	while (str_index < str_len)
	{
		char ch = parsed_string[str_index];
		str_index++;

		if (ch == ' ')
		{
			continue;
		}
		if (ch == '\0')
		{
			break;
		}
		//starting to parse value
		if (ch == '"')
		{
			parse_stage = 2;
			buf_index = 0;
			continue;
		}
		//new line? validate the string, set the value, then continue to the next line
		else if (ch == '\n')
		{
			if (_validateString(name_buf) && _validateString(value_buf))
			{
				C_Cvar* cvar = C_getCvar(name_buf);

				if (cvar)
				{
					C_setCvarValueDirect(cvar, value_buf);
				}
				else
				{
					printf("Invalid cvar %s \n", name_buf);
				}
			}
			else
			{
				printf("Failed to read cvar %s from file \n", name_buf);
			}
			
			parse_stage = 1;
			memset(name_buf, 0, sizeof(name_buf));
			memset(value_buf, 0, sizeof(value_buf));
			buf_index = 0;
			continue;
		}

		//name stage
		if (parse_stage == 1)
		{
			name_buf[buf_index] = ch;
			buf_index++;
		}
		//value stage
		else if (parse_stage == 2)
		{
			value_buf[buf_index] = ch;
			buf_index++;
		}

		if (buf_index >= MAX_CVAR_CHAR_SIZE)
		{
			return false;
		}

	}
	free(parsed_string);

	return true;
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