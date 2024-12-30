#ifndef CORE_COMMON_H
#define CORE_COMMON_H
#pragma once

#include <stdbool.h>
#include <cglm/cglm.h>

#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>


typedef struct
{
	struct nk_context* ctx;
	struct nk_glfw glfw;
	struct nk_colorf bg;
	bool enabled;
} NK_Data;


typedef enum
{
	ET__MALLOC_FAILURE,
	ET__OPENGL_ERROR,
	ET__LOAD_FAILURE
} ErrorType;

/*
~~~~~~~~~~~~~~~~~
PUBLIC FUNCTIONS
~~~~~~~~~~~~~~~~~
*/
//Print to stdout and console
void Core_Printf(const char* fmt, ...);
void Core_ErrorPrintf(ErrorType p_errorType, const char* fmt, ...);
size_t Core_getTicks();
size_t Core_getPhysicsTicks();
double Core_getDeltaTime();
double Core_getPhysDeltaTime();
double Core_getLerpFraction();
bool Core_CheckForBlockedInputState();
void Core_BlockInputThisFrame();


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
void Window_toggleFullScreen();
bool Window_isFullScreen();
bool Window_isCursorEnabled();
GLFWwindow* Window_getPtr();
void Window_EnableCursor();
void Window_DisableCursor();
void Window_SetVsync(bool enabled);

/*
~~~~~~~~~~~~~~~~~
THREADING
~~~~~~~~~~~~~~~~~
*/
typedef void* (*ThreadTask_fun)(void);
typedef int WorkHandleID;
#define THREAD_TASK_ARG_DATA_MAX_SIZE 256

typedef union
{
	long long int_value;
	double long real_value;
	void* mem_value;
} ReturnResult;

typedef enum
{
	TASK_FLAG__FIRE_AND_FORGET = 1 << 0, //Does not return a value and automatically discards the result
	TASK_FLAG__PRIORITY_ABOVE_NORMAL = 1 << 1,
	TASK_FLAG__PRIORITY_HIGHEST = 1 << 2,
	TASK_FLAG__PRIORITY_BELOW_NORMAL = 1 << 2,
	TASK_FLAG__PRIORITY_LOWEST = 1 << 3,
	__INTERNAL_TASK_FLAG__INT_TYPE = 1 << 4,
	__INTERNAL_TASK_FLAG__VOID_TYPE = 1 << 5
}WorkTask_Flags;

#define Thread_AssignTask(FUNCTION, FLAGS) __Internal_Thread_AssignTaskArgCopy(FUNCTION, NULL, 0, FLAGS | __INTERNAL_TASK_FLAG__VOID_TYPE);
#define Thread_AssignTaskArgs(FUNCTION, ARG_DATA, FLAGS) __Internal_Thread_AssignTaskArgCopy(FUNCTION, &ARG_DATA, sizeof(ARG_DATA), FLAGS)
#define Thread_AssignTaskIntType(FUNCTION, INT_TYPE, FLAGS) __Internal_Thread_AssignTaskArgCopy(FUNCTION, INT_TYPE, sizeof(INT_TYPE), FLAGS | __INTERNAL_TASK_FLAG__INT_TYPE)
WorkHandleID __Internal_Thread_AssignTaskArgCopy(ThreadTask_fun p_taskFun, void* p_argData, size_t p_allocSize, WorkTask_Flags p_taskFlags);

bool Thread_IsCompleted(WorkHandleID p_workHandleID);
void Thread_ReleaseWorkHandle(WorkHandleID p_workHandleID);
ReturnResult Thread_WaitForResult(WorkHandleID p_workHandleID, size_t p_waitTime);
#endif