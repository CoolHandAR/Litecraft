#include "r_shader.h"

#include <glad/glad.h>
#include <stdio.h>
#include <string.h>

#define PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION 1
#define READ_BUFFER_SIZE 20480

static bool checkCompileErrors(unsigned int p_object, const char* p_type)
{
	int success;
	char infoLog[1024];
	if (p_type != "PROGRAM")
	{
		glGetShaderiv(p_object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(p_object, 1024, NULL, infoLog);
			printf("Failed to Compile %s Shader. Reason: %s", p_type, infoLog);

			return false;
		}
	}
	else
	{
		glGetProgramiv(p_object, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(p_object, 1024, NULL, infoLog);
			printf("Failed to Compile %s Shader. Reason: %s", p_type, infoLog);
			return false;
		}
	}
	return true;
}

R_Shader Shader_CompileFromMemory(const char* p_vertexShader, const char* p_fragmentShader, const char* p_geometryShader)
{
	unsigned int vertex_shader, fragment_shader, geometry_shader;

	//vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &p_vertexShader, NULL);
	glCompileShader(vertex_shader);
	if (!checkCompileErrors(vertex_shader, "VERTEX"))
		return 0;

	//fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &p_fragmentShader, NULL);
	glCompileShader(fragment_shader);
	if (!checkCompileErrors(fragment_shader, "FRAGMENT"))
		return 0;

	if (p_geometryShader != NULL)
	{
		geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry_shader, 1, &p_geometryShader, NULL);
		glCompileShader(geometry_shader);
		if (!checkCompileErrors(geometry_shader, "GEOMETRY"))
			return 0;
	}

	R_Shader shader_id = 0;

	//shader program
	shader_id = glCreateProgram();
	glAttachShader(shader_id, vertex_shader);
	glAttachShader(shader_id, fragment_shader);

	if (p_geometryShader != NULL)
	{
		glAttachShader(shader_id, geometry_shader);
	}

	glLinkProgram(shader_id);
	if (!checkCompileErrors(shader_id, "PROGRAM"))
		return false;

	//CLEANUP
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	if (p_geometryShader != NULL)
	{
		glDeleteShader(geometry_shader);
	}

	printf("Shader compiled successfully\n");

	return shader_id;
}

R_Shader Shader_CompileFromFile(const char* p_vertexShaderPath, const char* p_fragmentShaderPath, const char* p_geometryShaderPath)
{
	FILE* vertex_file = NULL;
	FILE* fragment_file = NULL;
	FILE* geometry_file = NULL;

	fopen_s(&vertex_file, p_vertexShaderPath, "r");
	if (!vertex_file)
	{
		printf("Failed to open vertex shader file!\n");
		return 0;
	}

	fopen_s(&fragment_file, p_fragmentShaderPath, "r");
	if (!fragment_file)
	{
		printf("Failed to open fragment shader file!\n");
		return 0;
	}

	if (p_geometryShaderPath != NULL)
	{
		fopen_s(&geometry_file, p_geometryShaderPath, "r");
		if (!geometry_file)
		{
			printf("Failed to open geometry shader file!\n");
			return 0;
		}
	}

	char* vertex_buffer = malloc(READ_BUFFER_SIZE);
	if (!vertex_buffer)
	{
		printf("MALLOC FAILURE\n");
		return 0;
	}	
	memset(vertex_buffer, 0, READ_BUFFER_SIZE);


	char* fragment_buffer = malloc(READ_BUFFER_SIZE);
	if (!fragment_buffer)
	{
		free(fragment_buffer);
		printf("MALLOC FAILURE\n");
		return 0;
	}
	memset(fragment_buffer, 0, READ_BUFFER_SIZE);


	char* geometry_buffer = NULL;
	if (p_geometryShaderPath != NULL)
	{
		geometry_buffer = malloc(READ_BUFFER_SIZE);

		if (!geometry_buffer)
		{
			free(vertex_buffer);
			free(fragment_buffer);
			printf("MALLOC FAILURE\n");
			return 0;
		}
		memset(geometry_buffer, 0, READ_BUFFER_SIZE);
	}


	//READ FROM VERTEX_FILE
	int c;
	int i = 0;
	while ((c = fgetc(vertex_file)) != EOF)
	{
		vertex_buffer[i] = c;
		i++;
	}
	//READ FROM FRAGMENT_FILE
	i = 0;
	while ((c = fgetc(fragment_file)) != EOF)
	{
		fragment_buffer[i] = c;
		i++;
	}
	//READ FROM GEOEMETRY FILE
	if (p_geometryShaderPath != NULL && geometry_file != NULL)
	{
		i = 0;
		while ((c = fgetc(geometry_file)) != EOF)
		{
			geometry_buffer[i] = c;
			i++;
		}
	}
	
	//COMPILE
	R_Shader compiled_shader = Shader_CompileFromMemory(vertex_buffer, fragment_buffer, geometry_buffer);
	
	//CLEAN UP
	fclose(vertex_file);
	fclose(fragment_file);
	if (geometry_file != NULL)
	{
		fclose(geometry_file);
	}
	
	free(vertex_buffer);
	free(fragment_buffer);

	if (geometry_buffer != NULL)
	{
		free(geometry_buffer);
	}

	return compiled_shader;
}

R_Shader ComputeShader_CompileFromMemory(const char* p_computeShader)
{
	unsigned compute_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute_shader, 1, &p_computeShader, NULL);
	glCompileShader(compute_shader);
	if (!checkCompileErrors(compute_shader, "COMPUTE"))
		return 0;


	R_Shader program = glCreateProgram();
	glAttachShader(program, compute_shader);
	glLinkProgram(program);
	if (!checkCompileErrors(program, "PROGRAM"))
		return 0;

	glDeleteShader(compute_shader);

	return program;
}

R_Shader ComputeShader_CompileFromFile(const char* p_computeShaderPath)
{
	FILE* compute_file = NULL;
	
	fopen_s(&compute_file, p_computeShaderPath, "r");
	if (!compute_file)
	{
		printf("Failed to open compute shader file!\n");
		return 0;
	}

	char* compute_buffer = malloc(READ_BUFFER_SIZE);
	if (!compute_buffer)
	{
		printf("MALLOC FAILURE\n");
		return 0;
	}
	memset(compute_buffer, 0, READ_BUFFER_SIZE);

	//READ FROM SHADER_FILE
	int c;
	int i = 0;
	while ((c = fgetc(compute_file)) != EOF)
	{
		compute_buffer[i] = c;
		i++;
	}

	//COMPILE
	R_Shader compiled_shader = ComputeShader_CompileFromMemory(compute_buffer);

	//CLEAN UP
	fclose(compute_file);

	free(compute_buffer);

	return compiled_shader;
}


void Shader_SetFloat(R_Shader shader, const char* name, float value)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform1f(uniform_location, value);
}

void Shader_SetInteger(R_Shader shader, const char* name, int value)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform1i(uniform_location, value);
}

void Shader_SetVector2f(R_Shader shader, const char* name, float x, float y)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform2f(uniform_location, x, y);
}

void Shader_SetVector2f_2(R_Shader shader, const char* name, vec2* const value)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform2f(uniform_location, *value[0], *value[1]);
}

void Shader_SetVector3f(R_Shader shader, const char* name, float x, float y, float z)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform3f(uniform_location, x, y, z);
}

void Shader_SetVector3f_2(R_Shader shader, const char* name, vec3* const value)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform3f(uniform_location, *value[0], *value[1], *value[2]);
}

void Shader_SetVector4f(R_Shader shader, const char* name, float x, float y, float z, float w)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform4f(uniform_location, x, y, z, w);
}

void Shader_SetVector4f_2(R_Shader shader, const char* name, vec4* const value)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform4f(uniform_location, *value[0], *value[1], *value[2], *value[3]);
}

void Shader_SetMatrix4(R_Shader shader, const char* name, mat4 const matrix)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniformMatrix4fv(uniform_location, 1, false, matrix);
}
