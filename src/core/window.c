

#include <glad/glad.h>
#include "lc_core.h"
#include <stdio.h>
#include "c_common.h"
#include "cvar.h"
#include <Windows.h>
#include <assert.h>

#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720
#define WINDOW_NAME "Litecraft"

typedef struct
{
	DWORD gl_context_owner_thread_id;
	bool is_full_screen;
	bool cursor_enabled;
} WindowState;

extern void r_onWindowResize(ivec2 window_size);
extern void RCore_onWindowResize(int width, int height);
extern void PL_onMouseScroll(int yOffset);

GLFWwindow* glfw_window;
static WindowState window_state;

static void WinCallback_Framebuffer(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	
	ivec2 vec;
	vec[0] = width;
	vec[1] = height;

	r_onWindowResize(vec);
	RCore_onWindowResize(width, height);
}

static void WinCallback_MousePosition(GLFWwindow* window, double xpos, double ypos)
{
	LC_MouseUpdate(xpos, ypos);
}

static void WinCallback_ScrollBack(GLFWwindow* window, double xoffset, double yoffset)
{
	nk_gflw3_scroll_callback(window, xoffset, yoffset);

	PL_onMouseScroll(yoffset);
}


int Window_Init()
{
	memset(&window_state, 0, sizeof(window_state));

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	glfw_window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
	
	//fullscreen
	//glfwSetWindowMonitor(glfw_window, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);

	if (glfw_window == NULL)
	{
		printf("Failed to create glfw window\n");
		glfwTerminate();
		return false;
	}
	
	glfwMakeContextCurrent(glfw_window);

	//set callback functions
	glfwSetMonitorCallback(WinCallback_Framebuffer);

	glfwSetCharCallback(glfw_window, nk_glfw3_char_callback);
	glfwSetMouseButtonCallback(glfw_window, nk_glfw3_mouse_button_callback);

	glfwSetScrollCallback(glfw_window, WinCallback_ScrollBack);
	glfwSetFramebufferSizeCallback(glfw_window, WinCallback_Framebuffer);
	glfwSetCursorPosCallback(glfw_window, WinCallback_MousePosition);
	glfwSwapInterval(0); //vsync;

	glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	window_state.gl_context_owner_thread_id = GetCurrentThreadId();

	window_state.cursor_enabled = false;

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

	assert(caller_thread_id == window_state.gl_context_owner_thread_id && "Calling thread mismatch");

	glfwMakeContextCurrent(NULL);
	window_state.gl_context_owner_thread_id = 0;
}

void Window_setGLContext()
{
	assert(glfwGetCurrentContext() == NULL && "The thread must be detached first on the context thread");

	DWORD caller_thread_id = GetCurrentThreadId();

	//is the gl context already set by the same thread?
	if (caller_thread_id == window_state.gl_context_owner_thread_id)
		return;
	
	glfwMakeContextCurrent(glfw_window);
	window_state.gl_context_owner_thread_id = caller_thread_id;
}

void Window_toggleFullScreen()
{
	window_state.is_full_screen = !window_state.is_full_screen;

	if (window_state.is_full_screen)
	{
		glfwSetWindowMonitor(glfw_window, glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);
		glfwSetWindowSize(glfw_window, 1920, 1080);
	}
	else
	{
		glfwSetWindowMonitor(glfw_window, NULL, 0, 0, 1280, 720, GLFW_DONT_CARE);
		glfwSetWindowSize(glfw_window, 1280, 720);
	}
}

bool Window_isFullScreen()
{
	return window_state.is_full_screen;
}

bool Window_isCursorEnabled()
{
	return window_state.cursor_enabled;
}

GLFWwindow* Window_getPtr()
{
	return glfw_window;
}

void Window_EnableCursor()
{
	

	//glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	window_state.cursor_enabled = true;
}

void Window_DisableCursor()
{


	//glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	window_state.cursor_enabled = false;
}

void Window_EndFrame()
{
	if (window_state.cursor_enabled)
	{
		if (glfwGetInputMode(glfw_window, GLFW_CURSOR) != GLFW_CURSOR_NORMAL)
		{
			glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	else if (!window_state.cursor_enabled)
	{
		if (glfwGetInputMode(glfw_window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
		{
			glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	}

	window_state.cursor_enabled = false;
}