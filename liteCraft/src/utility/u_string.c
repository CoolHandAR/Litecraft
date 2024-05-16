#include "u_utility.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

const char* const empty_string = "";

bool String_StartsWith(const char* p_str, const char* p_starts, bool p_caseSensitive)
{
	int length = strlen(p_starts);
	
	if (length < 0 || length > strlen(p_str))
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

#define HASH_BITS_LONG_LONG ((sizeof (size_t)) * 8)
#define HASH_ROTATE_LEFT(val, n)   (((val) << (n)) | ((val) >> (HASH_BITS_LONG_LONG - (n))))
#define HASH_ROTATE_RIGHT(val, n)  (((val) >> (n)) | ((val) << (HASH_BITS_LONG_LONG - (n))))
size_t Hash_stringSeeded(char* p_str, size_t p_seed, unsigned p_max)
{
	//src: http://web.archive.org/web/20071223173210/http://www.concentric.net/~Ttwang/tech/inthash.htm
	size_t hash = p_seed;

	while (*p_str)
	{
		hash = HASH_ROTATE_LEFT(hash, 9) + (unsigned char)*p_str++;
	}
	hash ^= p_seed;
	const int c2 = 0x27d4eb2d;
	hash = (hash ^ 61) ^ (hash >> 16);
	hash = hash + (hash << 3);
	hash = hash ^ (hash >> 4);
	hash = hash * c2;
	hash = hash ^ (hash >> 15);

	return hash + p_seed;
}

size_t Hash_string(char* p_str, unsigned p_max)
{
	//src: http://web.archive.org/web/20071223173210/http://www.concentric.net/~Ttwang/tech/inthash.htm
	size_t hash = 0;

	while (*p_str)
	{
		hash = HASH_ROTATE_LEFT(hash, 9) + (unsigned char)*p_str++;
	}
	const int c2 = 0x27d4eb2d;
	hash = (hash ^ 61) ^ (hash >> 16);
	hash = hash + (hash << 3);
	hash = hash ^ (hash >> 4);
	hash = hash * c2;
	hash = hash ^ (hash >> 15);

	hash &= p_max;

	return hash;
}
