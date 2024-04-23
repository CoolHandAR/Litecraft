#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdbool.h>

#include "c_console.h"
#include "c_cvars.h"

static int INIT_WINDOW_WIDTH = 800;
static int INIT_WINDOW_HEIGHT = 600;
static const char* WINDOW_NAME = "My window";

extern int s_InitSoundEngine();
extern void C_setWindowPtr(GLFWwindow* ptr);
extern void C_resizeCallback(GLFWwindow* window, int width, int height);
extern void C_mouseCallback(GLFWwindow* window, double xpos, double ypos);
extern void C_kbCallback(GLFWwindow* Window, int key, int scancode, int action, int mods);
extern void C_charCallback(GLFWwindow* window, unsigned int codepoint);
extern void GLmessage_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param);
extern void RM_Init();

bool C_initGlfw()
{
	if (!glfwInit())
	{
		printf("Failed to init glfw\n");
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);

	if (window == NULL)
	{
		printf("Failed to create glfw window\n");
		glfwTerminate();
		return false;
	}
	C_setWindowPtr(window);
	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, C_resizeCallback);
	glfwSetCursorPosCallback(window, C_mouseCallback);
	glfwSetKeyCallback(window, C_kbCallback);
	glfwSetCharCallback(window, C_charCallback);
	glfwSwapInterval(1); //vsync;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
	if (!s_InitSoundEngine()) return false;
	if(!C_CvarCoreInit()) return false;

	C_ConsoleInit();

	C_setGLStates();

	RM_Init();

	return true;
}