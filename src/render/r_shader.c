#include "r_shader.h"

#include <glad/glad.h>
#include <stdio.h>
#include <string.h>
#include "utility/u_utility.h"

#define PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION 1

static bool Shader_checkCompileErrors(unsigned int p_object, const char* p_type)
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

static char* handleIncludeDirective(const char* p_srcPath, const char* p_buffer, int* p_bufferSize)
{
	if (!p_srcPath || !p_buffer)
	{
		return;
	}

	//current active buffer
	char* cur_buf = p_buffer;

	int buf_size = *p_bufferSize;

	//Look up if we have any includes
	char* look_up = strstr(cur_buf, "#include ");

	while (look_up)
	{
		char directory[256];
		memset(directory, 0, sizeof(directory));

		//Find and set the directory of this shader file
		int _index = String_findLastOfIndex(p_srcPath, '/');

		if (_index > -1)
		{
			strncpy_s(directory, 256, p_srcPath, _index);
		}

		char file_name_buf[256];
		char file_loc_buf[256];

		memset(file_name_buf, 0, sizeof(file_name_buf));
		memset(file_loc_buf, 0, sizeof(file_loc_buf));

		//copy the included file name into the buffer
		_memccpy(file_name_buf, look_up + strlen("#include \'"), '\n', 256);

		//remove the last " mark
		void* find = strchr(file_name_buf, '\'"');
		if (find)
		{
			memset(find, 0, 1);
		}
		
		//concat the directory and the file name
		sprintf_s(file_loc_buf, 256, "%s/%s", directory, file_name_buf);

		int parsed_length = 0;

		//try to parse the data
		char* parsed_data = File_Parse(file_loc_buf, &parsed_length);

		//we succeeded
		if (parsed_data)
		{	
			//we need to reallocate for the increased size. Current buffer size + parsed buffer size
			char* reallocated_buffer = realloc(cur_buf, buf_size + parsed_length + 1);

			if (reallocated_buffer)
			{
				buf_size = buf_size + parsed_length;

				//look up the include derictive again
				look_up = strstr(reallocated_buffer, "#include ");

				if (look_up)
				{
					//remove the include derictive
					char* ch = look_up;
					while (*ch != '\n')
					{
						if (ch != '\n')
							memset(ch, ' ', 1);
						ch++;
					}


					//copy the data after the include directive forward
					memmove(look_up + parsed_length, look_up, strlen(look_up));
					//copy the parsed data into the main buffer
					memcpy(look_up, parsed_data, parsed_length);
					//make sure the complete buffer is null terminated
					memset(reallocated_buffer + buf_size, 0, 1);
				
				}
				cur_buf = reallocated_buffer;
			}
			//make sure to free the parsed data
			free(parsed_data);
		}
		else
		{
			printf("Failed to process shader include: %s \n", file_name_buf);
			break;
		}
		//look for another include derictive
		if(cur_buf)
			look_up = strstr(cur_buf, "#include ");
	}

	*p_bufferSize = buf_size;

	return cur_buf;
}

static unsigned char* Shader_InsertDefinesIntoCharBuf(const char* p_src, int p_bufferSize, const char** p_defines, int p_defineCount)
{
	if (p_defineCount <= 0)
	{
		return p_src;
	}
	const char* def_char = "#define ";
	int def_strlen = strlen(def_char);

	const char* parsed_defines[32];
	int defines_lengths[32];
	int total_length = 0;

	for (int i = 0; i < 32 && i < p_defineCount; i++)
	{
		parsed_defines[i] = (const char*)p_defines[i];
		defines_lengths[i] = (int)strlen(p_defines[i]);

		if (parsed_defines[i] == "")
		{
			total_length += defines_lengths[i] + 1;
		}
		else
		{
			total_length += defines_lengths[i] + def_strlen + 1; // '\n' char
		}
	}
	int new_length = p_bufferSize + total_length + 1 + 1 + 32; // two '\n' and extra for carefulness sake
	//we need to reallocate for the increased size. Current buffer size + total define strlen count + 1
	char* reallocated_buffer = realloc(p_src, new_length);

	if (!reallocated_buffer)
	{
		return NULL;
	}

	memset(reallocated_buffer + new_length - 1, 0, 1); //null terminate the buffer

	char* find = reallocated_buffer;
	size_t offset = 0;
	if (!strncmp(reallocated_buffer, "#version", strlen("#version")))
	{
		for (int i = 0; i < 50; i++)
		{
			if (find[0] == '\n')
			{
				find++;
				break;
			}
			find++;

			offset++;
		}
	}
	else
	{
		printf("Failed to include defines into shader \n");
		return NULL;
	}

	memmove(find + total_length, find, p_bufferSize - offset);

	for (int i = 0; i < 32 && i < p_defineCount && i < total_length; i++)
	{
		if (parsed_defines[i] == "")
		{
			//Add a new line
			memset(find, 10, 1);
			find += 1;
			continue;
		}

		//Add the #DEFINE
		memcpy(find, def_char, def_strlen);
		find += def_strlen;

		//Copy the define str
		memcpy(find, parsed_defines[i], defines_lengths[i]);
		find += defines_lengths[i];

		//Add a new line
		memset(find, 10, 1);
		find += 1;
	}

	return reallocated_buffer;
}


R_Shader Shader_CompileFromMemory(const char* p_vertexShader, const char* p_fragmentShader, const char* p_geometryShader)
{
	unsigned int vertex_shader, fragment_shader, geometry_shader;

	//vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &p_vertexShader, NULL);
	glCompileShader(vertex_shader);
	if (!Shader_checkCompileErrors(vertex_shader, "VERTEX"))
		return 0;

	//fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &p_fragmentShader, NULL);
	glCompileShader(fragment_shader);
	if (!Shader_checkCompileErrors(fragment_shader, "FRAGMENT"))
		return 0;

	if (p_geometryShader != NULL)
	{
		geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry_shader, 1, &p_geometryShader, NULL);
		glCompileShader(geometry_shader);
		if (!Shader_checkCompileErrors(geometry_shader, "GEOMETRY"))
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
	if (!Shader_checkCompileErrors(shader_id, "PROGRAM"))
		return false;

	//CLEANUP
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	if (p_geometryShader != NULL)
	{
		glDeleteShader(geometry_shader);
	}

	return shader_id;
}

R_Shader Shader_CompileFromFile(const char* p_vertexShaderPath, const char* p_fragmentShaderPath, const char* p_geometryShaderPath)
{
	int vertex_length = 0;
	int fragment_length = 0;
	int geo_length = 0;

	char* vertex_buf = File_Parse(p_vertexShaderPath, &vertex_length);
	char* fragment_buf = File_Parse(p_fragmentShaderPath, &fragment_length);
	char* geo_buf = NULL;

	if (p_geometryShaderPath)
	{
		geo_buf = File_Parse(p_geometryShaderPath, &geo_length);
	}
	if (!vertex_buf)
	{
		printf("Failed to open vertex shader file!\n");
		return 0;
	}
	if (!fragment_buf)
	{
		printf("Failed to open fragment shader file!\n");
		free(vertex_buf);
		return 0;
	}
	if (p_geometryShaderPath)
	{
		if (!geo_buf)
		{
			free(vertex_buf);
			free(fragment_buf);
			printf("Failed to open geometry shader file!\n");
			return 0;
		}
	}
	
	//HANDLE INCLUDES
	vertex_buf = handleIncludeDirective(p_vertexShaderPath, vertex_buf, &vertex_length);
	fragment_buf = handleIncludeDirective(p_fragmentShaderPath, fragment_buf, &fragment_length);

	if (p_geometryShaderPath && geo_buf)
	{
		geo_buf = handleIncludeDirective(p_geometryShaderPath, geo_buf, &geo_length);
	}

	//COMPILE
	R_Shader compiled_shader = Shader_CompileFromMemory(vertex_buf, fragment_buf, geo_buf);

	//CLEAN UP
	if (vertex_buf)
	{
		free(vertex_buf);
	}
	if (fragment_buf)
	{
		free(fragment_buf);
	}
	if (p_geometryShaderPath && geo_buf)
	{
		free(geo_buf);
	}

	return compiled_shader;
}

R_Shader Shader_CompileFromFileDefine(const char* p_vertexShaderPath, const char* p_fragmentShaderPath, const char* p_geometryShaderPath, const char** p_defines, int p_defineCount)
{
	int vertex_length = 0;
	int fragment_length = 0;
	int geo_length = 0;

	char* vertex_buf = File_Parse(p_vertexShaderPath, &vertex_length);
	char* fragment_buf = File_Parse(p_fragmentShaderPath, &fragment_length);
	char* geo_buf = NULL;

	if (p_geometryShaderPath)
	{
		geo_buf = File_Parse(p_geometryShaderPath, &geo_length);
	}
	if (!vertex_buf)
	{
		printf("Failed to open vertex shader file!\n");
		return 0;
	}
	if (!fragment_buf)
	{
		printf("Failed to open fragment shader file!\n");
		free(vertex_buf);
		return 0;
	}
	if (p_geometryShaderPath)
	{
		if (!geo_buf)
		{
			free(vertex_buf);
			free(fragment_buf);
			printf("Failed to open geometry shader file!\n");
			return 0;
		}
	}
	//HANDLE INCLUDES
	vertex_buf = handleIncludeDirective(p_vertexShaderPath, vertex_buf, &vertex_length);
	fragment_buf = handleIncludeDirective(p_fragmentShaderPath, fragment_buf, &fragment_length);

	if (p_geometryShaderPath && geo_buf)
	{
		geo_buf = handleIncludeDirective(p_geometryShaderPath, geo_buf, &geo_length);
	}

	//HANDLE DEFINES
	vertex_buf = Shader_InsertDefinesIntoCharBuf(vertex_buf, vertex_length, p_defines, p_defineCount);
	fragment_buf = Shader_InsertDefinesIntoCharBuf(fragment_buf, fragment_length, p_defines, p_defineCount);
	if (p_geometryShaderPath && geo_buf)
	{
		geo_buf = Shader_InsertDefinesIntoCharBuf(geo_buf, geo_length, p_defines, p_defineCount);
	}

	//COMPILE
	R_Shader compiled_shader = Shader_CompileFromMemory(vertex_buf, fragment_buf, geo_buf);

	//CLEAN UP
	if (vertex_buf)
	{
		free(vertex_buf);
	}
	if (fragment_buf)
	{
		free(fragment_buf);
	}
	if (p_geometryShaderPath && geo_buf)
	{
		free(geo_buf);
	}

	return compiled_shader;
}



R_Shader ComputeShader_CompileFromMemory(const char* p_computeShader)
{
	unsigned compute_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute_shader, 1, &p_computeShader, NULL);
	glCompileShader(compute_shader);
	if (!Shader_checkCompileErrors(compute_shader, "COMPUTE"))
		return 0;


	R_Shader program = glCreateProgram();
	glAttachShader(program, compute_shader);
	glLinkProgram(program);
	if (!Shader_checkCompileErrors(program, "PROGRAM"))
		return 0;

	glDeleteShader(compute_shader);

	return program;
}

R_Shader ComputeShader_CompileFromFile(const char* p_computeShaderPath)
{
	int comp_length = 0;

	char* comp_buf = File_Parse(p_computeShaderPath, &comp_length);

	if (!comp_buf)
	{
		printf("Failed to open compute shader file: %s !\n", p_computeShaderPath);
		return 0;
	}

	//HANDLE INCLUDES
	comp_buf = handleIncludeDirective(p_computeShaderPath, comp_buf, &comp_length);

	//COMPILE
	R_Shader compiled_shader = ComputeShader_CompileFromMemory(comp_buf);

	//CLEAN UP
	if (comp_buf)
	{
		free(comp_buf);
	}

	return compiled_shader;
}

R_Shader ComputeShader_CompileFromFileDefine(const char* p_computeShaderPath, const char** p_defines, int p_defineCount)
{
	int comp_length = 0;

	char* comp_buf = File_Parse(p_computeShaderPath, &comp_length);

	if (!comp_buf)
	{
		printf("Failed to open compute shader file: %s !\n", p_computeShaderPath);
		return 0;
	}

	//HANDLE INCLUDES
	comp_buf = handleIncludeDirective(p_computeShaderPath, comp_buf, &comp_length);
	
	//HANDLE DEFINES
	comp_buf = Shader_InsertDefinesIntoCharBuf(comp_buf, comp_length, p_defines, p_defineCount);

	//COMPILE
	R_Shader compiled_shader = ComputeShader_CompileFromMemory(comp_buf);

	//CLEAN UP
	if (comp_buf)
	{
		free(comp_buf);
	}

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

void Shader_SetUnsigned(R_Shader shader, const char* name, unsigned value)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif
	glUniform1ui(uniform_location, value);
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

void Shader_SetVector3f_2(R_Shader shader, const char* name, vec3 const value)
{
	GLint uniform_location = glGetUniformLocation(shader, name);

#if PRINT_AND_FAIL_ON_INVALID_UNIFORM_LOCATION

	if (uniform_location == -1)
	{
		printf("GL_ERROR::Invalid Uniform Location: %s \n", name);
		return;
	}
#endif

	glUniform3f(uniform_location, value[0], value[1], value[2]);
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
