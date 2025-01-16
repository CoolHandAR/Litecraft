#include "render/r_shader.h"

#include <string.h>
#include <glad/glad.h>

#include "utility/u_utility.h"

static RShader* s_currentActiveShader = NULL;
static uint64_t s_currentVariantKey = 0;
static GLuint s_currentProgramID = 0;


static uint32_t Shader_HashWrapper(const void* _key)
{
	uint64_t x = *(uint64_t*)_key;

	return Hash_uint64(x);
}

static char* Shader_HandleIncludes(const char* p_srcPath, const char* p_buffer, int* p_bufferSize)
{
	if (!p_srcPath || !p_buffer)
	{
		return NULL;
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
		if (cur_buf)
			look_up = strstr(cur_buf, "#include ");
	}

	*p_bufferSize = buf_size;

	return cur_buf;
}

static unsigned char* Shader_InsertDefines(const char* p_src, int p_bufferSize, const char** p_defines, int p_defineCount, bool* p_result)
{
	if (p_defineCount <= 0)
	{
		*p_result = true;
		return p_src;
	}
	const char* def_char = "#define ";
	int def_strlen = strlen(def_char);

	const char* parsed_defines[64];
	int defines_lengths[64];
	int total_length = 0;

	for (int i = 0; i < 64 && i < p_defineCount; i++)
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
	int new_length = p_bufferSize + total_length + 1 + 1 + 64; // two '\n' and extra for carefulness sake
	//we need to reallocate for the increased size. Current buffer size + total define strlen count + 1
	char* reallocated_buffer = realloc(p_src, new_length);

	if (!reallocated_buffer)
	{
		*p_result = false;
		return p_src;
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
		*p_result = false;
		return p_src;
	}

	memmove(find + total_length, find, p_bufferSize - offset);

	for (int i = 0; i < 64 && i < p_defineCount && i < total_length; i++)
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

	*p_result = true;

	return reallocated_buffer;
}

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

static unsigned Shader_CompileFinal(const char* vert_src, const char* frag_src, const char* geo_src, const char* comp_src)
{
	//check if compute shader
	if (comp_src)
	{
		unsigned compute_shader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(compute_shader, 1, &comp_src, NULL);
		glCompileShader(compute_shader);
		if (!Shader_checkCompileErrors(compute_shader, "COMPUTE"))
			return 0;


		unsigned program = glCreateProgram();
		glAttachShader(program, compute_shader);
		glLinkProgram(program);
		if (!Shader_checkCompileErrors(program, "PROGRAM"))
			return 0;

		glDeleteShader(compute_shader);

		return program;
	}
	//pixel shader
	else if (vert_src && frag_src)
	{
		unsigned vertex_shader, fragment_shader, geometry_shader;

		//vertex shader
		vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_shader, 1, &vert_src, NULL);
		glCompileShader(vertex_shader);
		if (!Shader_checkCompileErrors(vertex_shader, "VERTEX"))
			return 0;

		//fragment shader
		fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment_shader, 1, &frag_src, NULL);
		glCompileShader(fragment_shader);
		if (!Shader_checkCompileErrors(fragment_shader, "FRAGMENT"))
			return 0;

		//geo check
		if (geo_src)
		{
			geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometry_shader, 1, &geo_src, NULL);
			glCompileShader(geometry_shader);
			if (!Shader_checkCompileErrors(geometry_shader, "GEOMETRY"))
				return 0;
		}

		unsigned program_id;

		program_id = glCreateProgram();
		glAttachShader(program_id, vertex_shader);
		glAttachShader(program_id, fragment_shader);

		if (geo_src != NULL)
		{
			glAttachShader(program_id, geometry_shader);
		}

		glLinkProgram(program_id);
		if (!Shader_checkCompileErrors(program_id, "PROGRAM"))
			return false;

		//CLEANUP
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		if (geo_src != NULL)
		{
			glDeleteShader(geometry_shader);
		}

		return program_id;
	}
		
	return 0;
}

static bool Shader_SetupUniformLocations(RShader* const shader, ShaderVariant* const variant)
{	
	if (shader->max_uniforms > 0)
	{
		int* locations = malloc(shader->max_uniforms * sizeof(GLint));

		if (!locations)
		{
			return false;
		}

		for (int i = 0; i < shader->max_uniforms; i++)
		{
			locations[i] = glGetUniformLocation(variant->program_id, shader->uniform_names[i]);
		}

		variant->uniforms_locations = locations;
	}
	if (shader->max_tex_units > 0)
	{	
		for (int i = 0; i < shader->max_tex_units; i++)
		{
			int loc = glGetUniformLocation(variant->program_id, shader->tex_unit_names[i]);

			if (loc != -1)
			{
				glProgramUniform1i(variant->program_id, loc, i);
			}
		}
	}
	int loc = glGetUniformLocation(variant->program_id, "texture_arr");

	if (loc != -1)
	{
		int32_t samplers[32];

		for (int j = 0; j < 32; j++)
		{
			samplers[j] = j;
		}
		glProgramUniform1iv(variant->program_id, loc, 32, samplers);
	}

	return true;
}

static bool Shader_CreateVariant(RShader* const shader, uint64_t key, ShaderVariant** r_variant)
{
	ShaderVariant variant;
	variant.key = key;
	
	unsigned char* vertex_buf = NULL;
	unsigned char* fragment_buf = NULL;
	unsigned char* geo_buf = NULL;
	unsigned char* compute_buf = NULL;

	if (shader->is_compute)
	{
		compute_buf = malloc(shader->compute_length + 1);

		if (!compute_buf)
		{
			return false;
		}

		memcpy(compute_buf, shader->compute_code, shader->compute_length + 1);
	}
	else
	{	
		assert((shader->vertex_length > 0 && shader->fragment_length > 0));

		vertex_buf = malloc(shader->vertex_length + 1);

		if (!vertex_buf)
		{
			return false;
		}

		fragment_buf = malloc(shader->fragment_length + 1);
		
		if (!fragment_buf)
		{
			free(vertex_buf);
			return false;
		}

		if (shader->geo_code && shader->geo_length > 0)
		{
			geo_buf = malloc(shader->geo_length + 1);

			if (!geo_buf)
			{
				free(vertex_buf);
				free(fragment_buf);
				return false;
			}
			
			memcpy(geo_buf, shader->geo_code, shader->geo_length + 1);
		}

		memcpy(vertex_buf, shader->vertex_code, shader->vertex_length + 1);
		memcpy(fragment_buf, shader->fragment_code, shader->fragment_length + 1);
	}
	
	bool include_result = false;
	bool has_includes = false;

	//setup defines
	if (key != 0)
	{
		const char* variant_defines[64];
		int variant_index = 0;
		for (int i = 0; i < 64; i++)
		{
			//has define
			if ((1 << i) & key)
			{
				variant_defines[variant_index] = shader->define_names[i];
				variant_index++;

				//remove bit
				key &= ~(1 << i);
			}
			if (key == 0)
			{
				break;
			}
		}

		if (variant_index > 0)
		{
			if (shader->is_compute)
			{
				compute_buf = Shader_InsertDefines(compute_buf, shader->compute_length + 1, variant_defines, variant_index, &include_result);
			}
			else
			{
				vertex_buf = Shader_InsertDefines(vertex_buf, shader->vertex_length + 1, variant_defines, variant_index, &include_result);
				fragment_buf = Shader_InsertDefines(fragment_buf, shader->fragment_length +1 , variant_defines, variant_index, &include_result);

				if (shader->geo_code && shader->geo_length > 0)
				{
					geo_buf = Shader_InsertDefines(geo_buf, shader->geo_length + 1, variant_defines, variant_index, &include_result);
				}
			}
			has_includes = true;
		}
	}

	//compile, this function will check for null ptrs
	variant.program_id = Shader_CompileFinal(vertex_buf, fragment_buf, geo_buf, compute_buf);
	
	//clean up buffers
	if (shader->is_compute && compute_buf)
	{
		free(compute_buf);
	}
	else
	{
		if (vertex_buf)
		{
			free(vertex_buf);
		}
		if (fragment_buf)
		{
			free(fragment_buf);
		}
		if (geo_buf)
		{
			free(geo_buf);
		}
	}
	

	if (variant.program_id <= 0)
	{
		return false;
	}

	variant.uniforms_locations = NULL;
	//setup uniforms
	if (!Shader_SetupUniformLocations(shader, &variant))
	{
		return false;
	}

	//insert to hashmap
	*r_variant = (ShaderVariant*)CHMap_Insert(&shader->variant_map, &variant.key, &variant);

	return true;
}

static bool Shader_InitAndLoad(const char* vert_path, const char* frag_path, const char* geo_path, const char* comp_path, int max_defines, int max_uniforms, 
	int max_texunits, const char** defines, const char** uniforms, const char** texunits, RShader* r_shader)
{
	RShader shader;
	memset(&shader, 0, sizeof(shader));

	//check if compute shader
	if (comp_path)
	{
		int comp_length = 0;

		char* comp_buf = File_Parse(comp_path, &comp_length);

		if (!comp_buf)
		{
			printf("Failed to open compute shader file: %s !\n", comp_path);
			return false;
		}

		//HANDLE INCLUDES
		comp_buf = Shader_HandleIncludes(comp_path, comp_buf, &comp_length);

		shader.compute_code = comp_buf;
		shader.compute_length = comp_length;

		shader.is_compute = true;
	}
	//pixel shader
	else if (vert_path && frag_path)
	{
		int vertex_length = 0;
		int fragment_length = 0;
		int geo_length = 0;

		char* vertex_buf = File_Parse(vert_path, &vertex_length);
		char* fragment_buf = NULL;
		char* geo_buf = NULL;

		if (!vertex_buf)
		{
			printf("Failed to open vertex shader file!\n");
			return false;
		}
		
		fragment_buf = File_Parse(frag_path, &fragment_length);

		if (!fragment_buf)
		{
			printf("Failed to open fragment shader file!\n");
			free(vertex_buf);
			return false;
		}

		if (geo_path)
		{
			geo_buf = File_Parse(geo_path, &geo_length);

			if (!geo_buf)
			{
				free(vertex_buf);
				free(fragment_buf);
				printf("Failed to open geometry shader file!\n");
				return false;
			}
		}
		//HANDLE INCLUDES
		vertex_buf = Shader_HandleIncludes(vert_path, vertex_buf, &vertex_length);
		fragment_buf = Shader_HandleIncludes(frag_path, fragment_buf, &fragment_length);

		if (geo_path && geo_buf)
		{
			geo_buf = Shader_HandleIncludes(geo_path, geo_buf, &geo_length);
		}

		if (!vertex_buf || !fragment_buf)
		{
			return false;
		}
		if (geo_path && !geo_buf)
		{
			return false;
		}
		shader.vertex_code = vertex_buf;
		shader.fragment_code = fragment_buf;
		shader.geo_code = geo_buf;
		
		shader.vertex_length = vertex_length;
		shader.fragment_length = fragment_length;
		shader.geo_length = geo_length;
	}

	shader.variant_map = CHMAP_INIT(Shader_HashWrapper, NULL, uint64_t, ShaderVariant, 0);
	shader.max_defines = max_defines;
	shader.max_uniforms = max_uniforms;
	shader.max_tex_units = max_texunits;
	shader.define_names = defines;
	shader.uniform_names = uniforms;
	shader.tex_unit_names = texunits;
	shader.is_loaded = true;

	*r_shader = shader;

	return true;
}

static ShaderVariant* Shader_FindVariant(RShader* const shader, uint64_t key)
{
	return CHMap_Find(&shader->variant_map, &key);
}

static void Shader_DestroyVariant(RShader* const shader, ShaderVariant* const variant, bool p_removeFromMap)
{
	if (variant->uniforms_locations)
	{
		free(variant->uniforms_locations);
	}
	if (variant->program_id > 0)
	{
		glDeleteProgram(variant->program_id);
	}

	if (p_removeFromMap)
	{
		CHMap_Erase(&shader->variant_map, variant->key);
	}
}

static void Shader_SetCurrent(RShader* const shader, ShaderVariant* const variant)
{
	s_currentActiveShader = shader;
	s_currentProgramID = variant->program_id;
	s_currentVariantKey = variant->key;

	shader->active_variant = variant;
	shader->current_variant_key = variant->key;
}

RShader Shader_PixelCreate(const char* vert_src, const char* frag_src, int max_defines, int max_uniforms, int max_texunits, 
	const char** defines, const char** uniforms, const char** tex_units, bool* r_result)
{	
	RShader shader;

	bool result = Shader_InitAndLoad(vert_src, frag_src, NULL, NULL, max_defines, max_uniforms, max_texunits, defines, uniforms, tex_units, &shader);

	if (!result)
	{
		memset(&shader, 0, sizeof(shader));

		if (r_result)
		{
			*r_result = false;
		}
		return shader;
	}

	*r_result = true;

	return shader;
}

RShader Shader_ComputeCreate(const char* comp_src, int max_defines, int max_uniforms, int max_texunits, const char** defines, const char** uniforms, const char** tex_units, bool* r_result)
{
	RShader shader;

	bool result = Shader_InitAndLoad(NULL, NULL, NULL, comp_src, max_defines, max_uniforms, max_texunits, defines, uniforms, tex_units, &shader);

	if (!result)
	{
		memset(&shader, 0, sizeof(shader));

		if (r_result)
		{
			*r_result = false;
		}
		return shader;
	}

	*r_result = true;

	return shader;
}

RShader Shader_PixelCreateStatic(const char* vert_src, const char* frag_src)
{	
	RShader shader;

	return shader;
}

void Shader_Destruct(RShader* const shader)
{
	for (int i = 0; i < dA_size(shader->variant_map.item_data); i++)
	{
		ShaderVariant* variant = dA_at(shader->variant_map.item_data, i);

		Shader_DestroyVariant(shader, variant, false);
	}

	CHMap_Destruct(&shader->variant_map);

	if (shader->compute_code && shader->compute_length > 0)
	{
		free(shader->compute_code);
	}
	if (shader->vertex_code && shader->vertex_length > 0)
	{
		free(shader->vertex_code);
	}
	if (shader->fragment_code && shader->fragment_length > 0)
	{
		free(shader->fragment_code);
	}
	if (shader->geo_code && shader->geo_length > 0)
	{
		free(shader->geo_code);
	}
}

void Shader_Use(RShader* const shader)
{
	//if it's already active, do nothing and save a gl call
	if (s_currentActiveShader && s_currentActiveShader == shader)
	{
		if (shader->active_variant && shader->active_variant->program_id == s_currentProgramID)
		{
			if (shader->current_variant_key == shader->new_variant_key)
			{
				//return;
			}
		}
	}

	//do we need another variant?
	if (shader->current_variant_key != shader->new_variant_key)
	{
		//look for it in the hash map
		ShaderVariant* variant = Shader_FindVariant(shader, shader->new_variant_key);
		//if in hashmap, bind it
		if (variant)
		{
			glUseProgram(variant->program_id);

			//set as current
			Shader_SetCurrent(shader, variant);
		}
		else
		{
			//otherwise, compile new variant
			ShaderVariant* new_variant;
			if (Shader_CreateVariant(shader, shader->new_variant_key, &new_variant))
			{
				glUseProgram(new_variant->program_id);

				//set as current
				Shader_SetCurrent(shader, new_variant);
			}
			
		}
	}
	else
	{
		//do we have active variant
		if (shader->active_variant)
		{
			glUseProgram(shader->active_variant->program_id);
			Shader_SetCurrent(shader, shader->active_variant);
		}
		else
		{
			//we need to create for current variant
			ShaderVariant* current_variant;
			if (Shader_CreateVariant(shader, shader->current_variant_key, &current_variant))
			{
				glUseProgram(current_variant->program_id);

				//set as current
				Shader_SetCurrent(shader, current_variant);
			}
		}
		
	}
	
}

int Shader_GetUniformLocation(RShader* const shader, int uniform)
{
	assert(uniform < shader->max_uniforms);
	
	if (!shader->active_variant)
	{
		return -1;
	}

	return shader->active_variant->uniforms_locations[uniform];
}

void Shader_SetInt(RShader* const shader, int uniform, int value)
{
	int loc = Shader_GetUniformLocation(shader, uniform);

	if (loc == -1)
	{
		return;
	}

	glUniform1i(loc, value);
}

void Shader_SetUint(RShader* const shader, int uniform, unsigned value)
{
	int loc = Shader_GetUniformLocation(shader, uniform);

	if (loc == -1)
	{
		return;
	}

	glUniform1ui(loc, value);
}

void Shader_SetFloaty(RShader* const shader, int uniform, float value)
{
	int loc = Shader_GetUniformLocation(shader, uniform);

	if (loc == -1)
	{
		return;
	}

	glUniform1f(loc, value);
}


void Shader_SetFloat2(RShader* const shader, int uniform, float x, float y)
{
	int loc = Shader_GetUniformLocation(shader, uniform);

	if (loc == -1)
	{
		return;
	}

	glUniform2f(loc, x, y);
}

void Shader_SetFloat3(RShader* const shader, int uniform, float x, float y, float z)
{
	int loc = Shader_GetUniformLocation(shader, uniform);

	if (loc == -1)
	{
		return;
	}

	glUniform3f(loc, x, y, z);
}

void Shader_SetFloat4(RShader* const shader, int uniform, float x, float y, float z, float w)
{
	int loc = Shader_GetUniformLocation(shader, uniform);

	if (loc == -1)
	{
		return;
	}

	glUniform4f(loc, x, y, z, w);
}

void Shader_SetVec2(RShader* const shader, int uniform, vec2 value)
{
	Shader_SetFloat2(shader, uniform, value[0], value[1]);
}

void Shader_SetVec3(RShader* const shader, int uniform, vec3 value)
{
	Shader_SetFloat3(shader, uniform, value[0], value[1], value[2]);
}

void Shader_SetVec4(RShader* const shader, int uniform, vec4 value)
{
	Shader_SetFloat4(shader, uniform, value[0], value[1], value[2], value[3]);
}

void Shader_SetMat4(RShader* const shader, int uniform, mat4 value)
{
	int loc = Shader_GetUniformLocation(shader, uniform);

	if (loc == -1)
	{
		return;
	}

	glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

