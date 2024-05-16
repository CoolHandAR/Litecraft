
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdbool.h>

#include "cvar.h"
#include "render/r_renderer.h"




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
#include "core/c_common.h"

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SUBSYSTEMS INITILIZATIONS AND EXITS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/



static int INIT_WINDOW_WIDTH = 1024;
static int INIT_WINDOW_HEIGHT = 720;
static const char* WINDOW_NAME = "My window";

extern int r_Init1();
extern int Window_Init();
extern int Sound_Init();
extern void Sound_Cleanup();
extern void GLmessage_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param);
extern void RM_Init();
extern bool Input_Init();
extern int Cvar_Init();
extern void Cvar_Cleanup();
extern int Con_Init();
extern void Con_Cleanup();
extern NK_Data nk;
extern GLFWwindow* glfw_window;

static bool C_initGlfw()
{
	if (!glfwInit())
	{
		printf("Failed to init glfw\n");
		return false;
	}
	if (!Window_Init())
		return false;


	return true;

}
bool C_initGlad()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to init GLAD\n");
		return false;
	}
	glViewport(0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);

	printf("GLAD Loaded\n");
	printf("OpenGL loaded\n");
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	return true;
}

static void C_initNuklear()
{
	memset(&nk, 0, sizeof(NK_Data));

	nk.ctx = nk_glfw3_init(&nk.glfw, glfw_window, NK_GLFW3_INSTALL_CALLBACKS);

	struct nk_font_atlas* atlas;
	nk_glfw3_font_stash_begin(&nk.glfw, &atlas);
	nk_glfw3_font_stash_end(&nk.glfw);
}

static void C_NuklearCleanup()
{
	nk_glfw3_shutdown(&nk.glfw);
	nk_free(nk.ctx);
}

void C_setGLStates()
{
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GLmessage_callback, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

bool C_init()
{
	if (!C_initGlfw()) return false;
	if (!C_initGlad()) return false;
	if (!Sound_Init()) return false;
	if (!Cvar_Init()) return false;
	if (!r_Init()) return false;
	if (!Input_Init()) return false;
	//if (!r_Init1()) return false;

	Con_Init();

	C_setGLStates();

	RM_Init();
	C_initNuklear();

	glViewport(0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);


	return true;
}

void C_Exit()
{
	glfwTerminate();
	Sound_Cleanup();
	Cvar_Cleanup();
	C_NuklearCleanup();
	Con_Cleanup();
}