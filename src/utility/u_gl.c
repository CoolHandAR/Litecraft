#include <glad/glad.h>

#include <string.h>
#include <stdio.h>
#include "u_gl.h"

static const char* GL_sourceChar(GLenum source)
{
	switch (source)
	{
	case GL_DEBUG_SOURCE_API: return "API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
	case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
	case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
	case GL_DEBUG_SOURCE_OTHER: return "OTHER";
	}

	return "NULL";
}

static const char* GL_typeChar(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR: return "ERROR";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
	case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
	case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
	case GL_DEBUG_TYPE_MARKER: return "MARKER";
	case GL_DEBUG_TYPE_OTHER: return "OTHER";
	}

	return "NULL";
}

static const char* GL_severityChar(GLenum severity)
{
	switch (severity) 
	{
	case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
	case GL_DEBUG_SEVERITY_LOW: return "LOW";
	case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
	case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
	}

	return "NULL";
}

void GLmessage_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
{
	char src_buf[64];
	char type_buf[64];
	char sever_buf[64];

	memset(&src_buf, 0, 64);
	memset(&type_buf, 0, 64);
	memset(&sever_buf, 0, 64);

	strcpy(&src_buf, GL_sourceChar(source));
	strcpy(&type_buf, GL_typeChar(type));
	strcpy(&sever_buf, GL_severityChar(severity));

	printf("%s, %s, %s, %i, %s \n", src_buf, type_buf, sever_buf, id, message);
	
}

GL_UniformBuffer GL_UBufferCreate(int p_bindingIndex)
{
	GL_UniformBuffer ubo;

	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, 1, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, p_bindingIndex, ubo, 0, 1);

	return ubo;
}
