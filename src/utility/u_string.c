#include "u_utility.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static const char* const empty_string = "";

bool String_StartsWith(const char* p_str, const char* p_starts, bool p_caseSensitive)
{
	int length = strlen(p_starts);
	
	if (length <= 0 || length > strlen(p_str))
	{
		return false;
	}

	for (int i = 0; i < length; i++)
	{
		char ch_1 = p_str[i];
		char ch_2 = p_starts[i];

		if (p_caseSensitive)
		{
			if (ch_1 != ch_2)
			{
				return false;
			}
		}
		else
		{
			if (toupper(ch_1) != toupper(ch_2))
			{
				return false;
			}
		}
	}
	return true;	
}

bool String_Contains(const char* p_str, const char* p_contains)
{
	int length = strlen(p_contains);

	if (length <= 0 || length > strlen(p_str))
	{
		return false;
	}

	if (strstr(p_str, p_contains))
	{
		return true;
	}

	return false;
}


char* String_safeCopy(const char* p_source)
{
	char* out;

	if (!p_source)
	{
		return empty_string;
	}
	out = malloc(strlen(p_source) + 1);
	if (out)
	{
		strcpy(out, p_source);
		return out;
	}

	return empty_string;
}

bool String_usingEmptyString(const char* p_str)
{
	//compare memory adresses
	if (p_str == empty_string)
		return true;

	return false;
}

int String_findLastOfIndex(const char* p_str, int p_char)
{
	int str_len = strlen(p_str);

	int last_index = -1;

	for (int i = 0; i < str_len; i++)
	{
		char ch = p_str[i];

		if (ch == p_char)
		{
			last_index = i;
		}
	}

	return last_index;
}

int String_findFirstOfIndex(const char* p_str, int p_char)
{
	int str_len = strlen(p_str);

	for (int i = 0; i < str_len; i++)
	{
		char ch = p_str[i];

		if (ch == p_char)
		{
			return i;
		}
	}

	return -1;
}
