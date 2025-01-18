#include <stdio.h>
#include <stdbool.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Windows.h>

#define DYNAMIC_ARRAY_IMPLEMENTATION
#include "utility/dynamic_array.h"

#define CHM_IMPLEMENTATION
#include "utility/Custom_Hashmap.h"

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin/stb_perlin.h>

#include "core/cvar.h"
#include "core/core_common.h"

#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_IMPLEMENTATION
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>


/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SUBSYSTEMS INITILIZATIONS AND EXITS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

extern void GLmessage_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param);

extern int Renderer_Init(int p_initWidth, int p_initHeight);
extern int Window_Init();
extern int Sound_Init();
extern bool Input_Init();
extern int Cvar_Init();
extern int Con_Init();
extern void ResourceManager_Init();
extern int ThreadCore_Init();

extern void Renderer_Exit();
extern void Sound_Cleanup();
extern void ResourceManager_Cleanup();
extern void Cvar_Cleanup();
extern void Con_Cleanup();
extern void ThreadCore_Cleanup();

extern NK_Data nk;

static bool Init_Glfw()
{
	if (!glfwInit())
	{
		printf("Failed to init glfw\n");
		return false;
	}
	
	return true;

}
static bool Init_Glad()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to init GLAD\n");
		return false;
	}
	
	printf("GLAD Loaded\n");
	printf("OpenGL loaded\n");
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	return true;
}

static void Init_Nuklear()
{
	memset(&nk, 0, sizeof(NK_Data));

	nk.ctx = nk_glfw3_init(&nk.glfw, Window_getPtr(), NK_GLFW3_DEFAULT);

	struct nk_font_atlas* atlas;
	nk_glfw3_font_stash_begin(&nk.glfw, &atlas);
	nk_glfw3_font_stash_end(&nk.glfw);
}

static void Cleanup_Nuklear()
{
	nk_glfw3_shutdown(&nk.glfw);
	nk_free(nk.ctx);
}

static void Init_setGLStates()
{
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GLmessage_callback, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_FALSE);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

bool Core_init()
{
	if (!Init_Glfw()) return false;
	if (!Window_Init()) return false;
	if (!Init_Glad()) return false;
	if (!Sound_Init()) return false;
	if (!Cvar_Init()) return false;
	if (!Input_Init()) return false;
	if (!Renderer_Init(0, 0))
	{
		MessageBox(NULL, (LPCWSTR)L"Failed to load renderer shaders.\nMake sure there is shader folder", (LPCWSTR)L"Failure to load!", MB_ICONWARNING);

		return false;
	}
	//if (!ThreadCore_Init()) return false;
	if (!Con_Init()) return false;
	
	Init_setGLStates();
	ResourceManager_Init();
	Init_Nuklear();

	return true;
}

void Core_Exit()
{
	//ThreadCore_Cleanup();
	glfwTerminate();
	Sound_Cleanup();
	Cvar_Cleanup();
	Cleanup_Nuklear();
	Con_Cleanup();
	ResourceManager_Cleanup();
	Renderer_Exit();
}