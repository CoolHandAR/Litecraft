#include "u_utility.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* File_ParseString(const char* p_filePath)
{
	FILE* file = NULL;

	fopen_s(&file, p_filePath, "r");
	if (!file)
	{
		printf("Failed to open file for parsing at path: %s!\n", p_filePath);
		return 0;
	}

	size_t buffer_size = 10240;

	char* buffer = malloc(buffer_size);
	if (!buffer)
	{
		printf("MALLOC FAILURE\n");
		return NULL;
	}
	memset(buffer, 0, buffer_size);

	//READ FROM THE JSON FILE
	int c;
	int i = 0;
	while ((c = fgetc(file)) != EOF)
	{
		buffer[i] = c;
		i++;

		//realloc more if we need to 
		if (i + 1 >= buffer_size)
		{
			void* ptr = realloc(buffer, buffer_size + buffer_size);

			//did we fail to realloc for some reason?
			if (!ptr)
			{
				free(buffer);
				return NULL;
			}
			void* ptr_new_data = (char*)ptr + buffer_size;
			memset(ptr_new_data, 0, buffer_size);
			buffer_size += buffer_size;

			buffer = ptr;
		}
	}

	//CLEAN UP
	fclose(file);

	return buffer;
}

bool File_PrintString(const char* p_string, const char* p_filePath)
{
	FILE* file = NULL;

	fopen_s(&file, p_filePath, "w");
	if (!file)
	{
		printf("Failed to open file for printing at path: %s!\n", p_filePath);
		return false;
	}

	fwrite(p_string, sizeof(char), strlen(p_string), file);
	
	return fclose(file) == 0;
}
