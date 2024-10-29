#include "cvar.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

#include "utility/u_utility.h"
#include <assert.h>

/*
	Code inspired by quake3 cvar system
*/

#define MAX_CVARS 1024
#define MAX_CVAR_CHAR_SIZE 256
#define FILE_HASH_SIZE 256

typedef struct CvarCore
{
	Cvar cvars[MAX_CVARS];
	int index_count;

	Cvar* hash_table[FILE_HASH_SIZE];
	Cvar* next;

} CvarCore;

static CvarCore s_cvarCore;

static void Cvar_CopyNumValueToStrValue(Cvar* p_cvar)
{
	int v_len = 0;
	char value_buf[256];
	memset(value_buf, 0, sizeof(value_buf));

	//Determine if we should it's a true floating point number or a int one
	float fraction = fmodf(p_cvar->float_value, 1.0);

	//int type
	if (fraction == 0)
	{
		v_len = snprintf(value_buf, 256, "%i", p_cvar->int_value);
	}
	//float type
	else
	{
		v_len = snprintf(value_buf, 256, "%f", p_cvar->float_value);
	}

	//alloc and copy
	if (v_len > 0)
	{
		p_cvar->str_value = malloc(v_len + 1);

		if (p_cvar->str_value)
		{
			strncpy(p_cvar->str_value, value_buf, v_len + 1);
		}
	}
}

static bool _updateCvarNumValues(Cvar* p_cvar, const char* p_strValue)
{
	bool num_value = false;

	//check for boolean types
	if (!_strcmpi(p_strValue, "true"))
	{
		p_cvar->int_value = 1;
		p_cvar->float_value = 1;
	}
	else if (!_strcmpi(p_strValue, "false"))
	{
		p_cvar->int_value = 0;
		p_cvar->float_value = 0;
	}
	else
	{
		//both of these functions return 0 if they can't convert the values to a number
		p_cvar->int_value = strtol(p_strValue, (char**)NULL, 10);
		p_cvar->float_value = strtod(p_strValue, (char**)NULL);

		//limit values
		if (p_cvar->float_value > p_cvar->max_value)
		{
			p_cvar->float_value = p_cvar->max_value;
			p_cvar->int_value = p_cvar->max_value;
			num_value = true;
		}
		else if (p_cvar->float_value < p_cvar->min_value)
		{
			p_cvar->float_value = p_cvar->min_value;
			p_cvar->int_value = p_cvar->min_value;
			num_value = true;
		}
	}
	return num_value;
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
	if (strlen(p_string) == 0)
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
	}

	hash &= (FILE_HASH_SIZE - 1);

	//hash = Hash_string(p_string);

	return hash;
}


static Cvar* _findCvar(const char* p_cvarName)
{
	Cvar* cvar;
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


static Cvar* _createCvar(const char* p_name, const char* p_value, const char* p_helpText, int p_flags, float p_minValue, float p_maxValue)
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
	//if the cvar exists return it
	Cvar* find_cvar = _findCvar(p_name);
	if (find_cvar)
	{
		return find_cvar;
	}

	Cvar* var = NULL;
	
	var = &s_cvarCore.cvars[s_cvarCore.index_count];
	s_cvarCore.index_count++;

	var->name = String_safeCopy(p_name);
	var->str_value = String_safeCopy(p_value);
	var->default_value = String_safeCopy(p_value);
	var->help_text = String_safeCopy(p_helpText);
	var->min_value = p_minValue;
	var->max_value = p_maxValue;
	var->modified = true;

	_updateCvarNumValues(var, p_value);

	var->next = s_cvarCore.next;
	s_cvarCore.next = var;
	
	var->flags = p_flags;

	long hash = _genHashValueForCvar(var->name);
	var->hash_next = s_cvarCore.hash_table[hash];
	s_cvarCore.hash_table[hash] = var;

	return var;
}


int Cvar_Init()
{
	memset(&s_cvarCore, 0, sizeof(CvarCore));


	return 1;
}

void Cvar_Cleanup()
{
	for (int i = 0; i < s_cvarCore.index_count; i++)
	{
		Cvar* cvar = &s_cvarCore.cvars[i];

		if (cvar->name && !String_usingEmptyString(cvar->name))
		{
			free(cvar->name);
		}
		if (cvar->str_value && !String_usingEmptyString(cvar->str_value))
		{
			free(cvar->str_value);
		}
		if (cvar->default_value && !String_usingEmptyString(cvar->default_value))
		{
			free(cvar->default_value);
		}
	}

}

Cvar* Cvar_get(const char* p_varName)
{
	return _findCvar(p_varName);
}


Cvar* Cvar_Register(const char* p_varName, const char* p_value, const char* p_helpText, int p_flags, float p_minValue, float p_maxValue)
{
	return _createCvar(p_varName, p_value, p_helpText, p_flags, p_minValue, p_maxValue);
}


void Cvar_ResetAllToDefault()
{
	for (int i = 0; i < s_cvarCore.index_count; i++)
	{
		Cvar_setValueDirect(&s_cvarCore.cvars[i], s_cvarCore.cvars[i].default_value);
	}
}

bool Cvar_PrintAllToFile(const char* p_filePath)
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
		Cvar* cvar = &s_cvarCore.cvars[i];
		
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

		fprintf(out_file, "%s \"%s\" \n", cvar->name, cvar->str_value);
	}

	return fclose(out_file) == 0;
}

bool Cvar_LoadAllFromFile(const char* p_filePath)
{
	char* parsed_string = File_Parse(p_filePath, NULL);

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
				Cvar* cvar = Cvar_get(name_buf);

				if (cvar)
				{
					Cvar_setValueDirect(cvar, value_buf);
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

int Cvar_StartWith(const char* p_startsWith, Cvar* r_cvars[5])
{
	if (!_validateString(p_startsWith))
	{
		return 0;
	}

	int matching_cvars_count = 0;

	for (int i = 0; i < s_cvarCore.index_count; i++)
	{
		Cvar* cvar = &s_cvarCore.cvars[i];

		if (String_StartsWith(cvar->name, p_startsWith, false))
		{
			r_cvars[matching_cvars_count] = cvar;
			matching_cvars_count++;

			if (matching_cvars_count + 1 >= 5)
			{
				break;
			}
		}
	}

	return matching_cvars_count;
}

bool Cvar_setValueDirect(Cvar* const p_cvar, const char* p_value)
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
		if (!String_usingEmptyString(p_cvar->str_value))
			free(p_cvar->str_value);
		
		//if the value was clamped in any way that mean's it was a number
		//so we can then copy the clamped value from the num values
		if (_updateCvarNumValues(p_cvar, p_value))
		{
			Cvar_CopyNumValueToStrValue(p_cvar);
		}
		//otherwise simple str copy
		else
		{
			p_cvar->str_value = String_safeCopy(p_value);
		}	

		//failed for whatever reason
		if(!p_cvar->str_value)
		{
			p_cvar->float_value = 0;
			p_cvar->int_value = 0;

			return false;
		}

		p_cvar->modified = true;
	}
	return true;
}

bool Cvar_setValueDirectInt(Cvar* const p_cvar, int p_value)
{
	char buf[12];
	memset(buf, 0, sizeof(buf));

	sprintf(buf, "%i", p_value);

	Cvar_setValueDirect(p_cvar, buf);
}

bool Cvar_setValueDirectFloat(Cvar* const p_cvar, float p_value)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));

	sprintf(buf, "%f", p_value);

	Cvar_setValueDirect(p_cvar, buf);
}

void Cvar_setValueToDefaultDirect(Cvar* const p_cvar)
{
	Cvar_setValueDirect(p_cvar, p_cvar->default_value);
}

void Cvar_setValueToDefault(const char* p_varName)
{
	Cvar* cvar = Cvar_get(p_varName);

	if (!cvar)
	{
		return;
	}
	
	Cvar_setValueDirect(cvar, cvar->default_value);
}

bool Cvar_setValue(const char* p_varName, const char* p_value)
{
	if (!_validateString(p_varName))
	{
		return false;
	}
	if (!_validateString(p_value))
	{
		return false;
	}

	Cvar* cvar = Cvar_get(p_varName);

	if (!cvar)
	{
		return false;
	}

	return Cvar_setValueDirect(cvar, p_value);
}