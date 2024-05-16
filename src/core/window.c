

#include <glad/glad.h>
#include "lc_core.h"
#include <stdio.h>
#include "c_common.h"
#include "cvar.h"
#include <Windows.h>

#define DEFAULT_WINDOW_WIDTH 1024
#define DEFAULT_WINDOW_HEIGHT 720
#define WINDOW_NAME "Litecraft"

extern void r_onWindowResize(ivec2 window_size);

GLFWwindow* glfw_window;
static DWORD gl_context_thread_id;

static void WinCallback_Framebuffer(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	
	ivec2 vec;
	vec[0] = width;
	vec[1] = height;

	r_onWindowResize(vec);
}

static void WinCallback_MousePosition(GLFWwindow* window, double xpos, double ypos)
{
	LC_MouseUpdate(xpos, ypos);
}


int Window_Init()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfw_window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);

	if (glfw_window == NULL)
	{
		printf("Failed to create glfw window\n");
		glfwTerminate();
		return false;
	}
	
	glfwMakeContextCurrent(glfw_window);

	//set callback functions
	glfwSetFramebufferSizeCallback(glfw_window, WinCallback_Framebuffer);
	glfwSetCursorPosCallback(glfw_window, WinCallback_MousePosition);
	glfwSwapInterval(1); //vsync;

	glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	gl_context_thread_id = GetCurrentThreadId();

	return true;
}

void Window_setSize(float p_width, float p_height)
{
	glfwSetWindowSize(glfw_window, p_width, p_height);
}

void Window_setWidth(float p_width)
{
	int old_width, old_heigth;
	glfwGetWindowSize(glfw_window, &old_width, &old_heigth);
	glfwSetWindowSize(glfw_window, p_width, old_heigth);
}

void Window_setHeigth(float p_heigth)
{
	int old_width, old_heigth;
	glfwGetWindowSize(glfw_window, &old_width, &old_heigth);
	glfwSetWindowSize(glfw_window, old_width, p_heigth);
}

void Window_getSize(ivec2 r_windowSize)
{
	int width, height;
	glfwGetWindowSize(glfw_window, &width, &height);

	r_windowSize[0] = width;
	r_windowSize[1] = height;
}

void Window_detachGLContext()
{
	if (glfwGetCurrentContext() == NULL)
		return;

	DWORD caller_thread_id = GetCurrentThreadId();

	assert(caller_thread_id == gl_context_thread_id && "Calling thread mismatch");

	glfwMakeContextCurrent(NULL);
	gl_context_thread_id = 0;
}

void Window_setGLContext()
{
	assert(glfwGetCurrentContext() == NULL && "The thread must be detached first on the context thread");

	DWORD caller_thread_id = GetCurrentThreadId();

	//is the gl context already set by the same thread?
	if (caller_thread_id == gl_context_thread_id)
		return;
	
	glfwMakeContextCurrent(glfw_window);
	gl_context_thread_id = caller_thread_id;
}

GLFWwindow* Window_getPtr()
{
	return glfw_window;
}

