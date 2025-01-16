#include "utility/u_utility.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned char* File_Parse(const char* p_filePath, int* r_length)
{
	FILE* file = NULL;

	fopen_s(&file, p_filePath, "rb"); //use rb because otherwise it can cause reading issues
	if (!file)
	{
		printf("Failed to open file for parsing at path: %s!\n", p_filePath);
		return 0;
	}
	int file_length = File_GetLength(file);
	unsigned char* buffer = malloc(file_length + 1);
	if (!buffer)
	{
		return NULL;
	}
	memset(buffer, 0, file_length + 1);
	fread_s(buffer, file_length, 1, file_length, file);

	//CLEAN UP
	fclose(file);

	if (r_length)
	{
		*r_length = file_length;
	}

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

int File_GetLength(FILE* p_file)
{
	int pos;
	int end;

	pos = ftell(p_file);
	fseek(p_file, 0, SEEK_END);
	end = ftell(p_file);
	fseek(p_file, pos, SEEK_SET);

	return end;
}
