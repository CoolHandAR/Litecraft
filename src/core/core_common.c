#include "core/core_common.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

void Core_Printf(const char* fmt, ...)
{
	assert(!strchr(fmt, '\n') && "New line character not allowed in Core_Printf(..)");

	//print to std out
	printf("%s \n", fmt);
	//print to console
	Con_printf(fmt);
}
void Core_ErrorPrintf(ErrorType p_errorType, const char* fmt, ...)
{
	assert(!strchr(fmt, '\n') && "New line character not allowed in Core_ErrorPrintf(..)");
	switch (p_errorType)
	{
	case ET__MALLOC_FAILURE:
	{
		printf("ERROR::MALLOC_FAILURE: \"%s\" \n", fmt);
		Con_printf("ERROR::MALLOC_FAILURE: \"%s\"", fmt);
		break;
	}
	case ET__LOAD_FAILURE:
	{
		printf("ERROR::LOAD_FAILURE: \"%s\" \n", fmt);
		Con_printf("ERROR::MALLOC_FAILURE: \"%s\"", fmt);
		break;
	}
	case ET__OPENGL_ERROR:
	{
		printf("ERROR::OPENGL_ERROR: \"%s\" \n", fmt);
		Con_printf("ERROR::OPENGL_ERROR: \"%s\"", fmt);
		break;
	}
	default:
		break;
	}
}
