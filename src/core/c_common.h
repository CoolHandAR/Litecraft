#pragma once

#include <stdlib.h>
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>
#include <cglm/cglm.h>

#include <stdbool.h>

typedef struct
{
	struct nk_context* ctx;
	struct nk_glfw glfw;
	struct nk_colorf bg;
} NK_Data;


typedef enum
{
	ET__MALLOC_FAILURE,
	ET__OPENGL_ERROR,
	ET__LOAD_FAILURE
} ErrorType;

/*
~~~~~~~~~~~~~~~~~
COMMON FUNCTIONS
~~~~~~~~~~~~~~~~~
*/
//Print to stdout and console
void C_Printf(const char* fmt, ...);
void C_ErrorPrintf(ErrorType p_errorType, const char* fmt, ...);
size_t C_getTicks();
size_t C_getPhysicsTicks();

/*
~~~~~~~~~~~~~~~~~
CONSOLE FUNCTIONS
~~~~~~~~~~~~~~~~~
*/
//Printf to console
void Con_printf(const char* fmt, ...);
//Returns true if console is currently opened
bool Con_isOpened();


/*
~~~~~~~~~~~~~~~~~
WINDOW FUNCTIONS
~~~~~~~~~~~~~~~~~
*/
void Window_setSize(float p_width, float p_height);
void Window_setWidth(float p_width);
void Window_setHeigth(float p_heigth);
void Window_getSize(ivec2 r_windowSize);
void Window_detachGLContext();
void Window_setGLContext();
GLFWwindow* Window_getPtr();