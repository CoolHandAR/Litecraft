#include "c_common.h"

#include "c_console.h"
#include "stdio.h"

void C_Printf(const char* fmt, ...)
{
	C_ConsolePrintf(fmt);
	printf(fmt);
}
