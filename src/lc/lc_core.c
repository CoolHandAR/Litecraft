#include "lc_core.h"

#include "render/r_model.h"

#include "render/r_renderer.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>


#include "utility/dynamic_array.h"

#include "render/r_texture.h"

#include "render/r_camera.h"


#include "physics/p_aabb_tree.h"
#include "physics/p_physics_defs.h"
#include "physics/p_physics_world.h"

#include "lc_player.h"


#include "sound/sound.h"

#include <threads.h>

#include "core/cvar.h"
#include "input.h"
#include "core/core_common.h"
#include "render/r_public.h"


typedef struct G_GameData
{
	R_Model model;
	R_Camera camera;
	ma_sound sound;

} G_GameData;

typedef struct LC_GameTextures
{
	R_Texture gui_atlas;
	R_Texture block_atlas;

} LC_GameTextures;

R_Texture g_gui_atlas_texture;
R_Texture g_block_atlas;

static G_GameData s_gameData;
extern GLFWwindow* glfw_window;
extern ivec2 s_windowSize;
#include "core/resource_manager.h"
#include "render/r_shader.h"
#include "utility/u_utility.h"
#include <time.h>

#include "core/resource_manager.h"
extern ma_engine sound_engine;

Cvar* mouse_sensitivity;
Cvar* cam_fov;

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024
char input_buffer[256];
char scrollback_buffer[1024];

#include "utility/Circle_buffer.h"

#define USE_NEW_RENDERER


typedef struct
{
	int value_int;
	float value_float;
} Functionargs;



#include <stdarg.h>
#define VA(...) __VA_ARGS__

#include <Windows.h>

#include "lc/lc_world.h"

extern void PL_initPlayer(vec3 p_spawnPos);

static R_Model model2;
extern void ParticleDemo_Init();
extern void ParticleDemo_Main();
static ParticleEmitterSettings* emitter;
extern int LC_Init2();
R_Texture* test_tex;
//#define PARTICLE_DEMO


void get_vogel_disk(float* r_kernel, int p_sample_count) {
	const float golden_angle = 2.4;

	for (int i = 0; i < p_sample_count; i++) {
		float r = sqrt((float)(i) + 0.5) / sqrt((float)(p_sample_count));
		float theta = (float)(i) * golden_angle;

		r_kernel[i * 4] = cos(theta) * r;
		r_kernel[i * 4 + 1] = (theta) * r;
	}
}


void LC_Init()
{
	vec4 samples[64];
	get_vogel_disk(samples, 8);

	LC_Init2();

	Circle_Buffer buf = CircleBuf_Init(sizeof(int), 5);

	int write_to = 69;
	int read_back = 0;

	CircleBuf_Write(&buf, &write_to, 1);
	CircleBuf_Read(&buf, &read_back, 1, true);

	//ParticleDemo_Init();

	int test = 34 % 32;

	VA(2);

	s_gameData.camera = Camera_Init();
	Camera_setCurrent(&s_gameData.camera);

	//model2 = Model_Load("assets/map/e1m1.obj");

	vec3 player_pos;
	player_pos[0] = -5;
	player_pos[1] = 16;
	player_pos[2] = 5;
	memset(input_buffer, 0, sizeof(input_buffer));
	memset(scrollback_buffer, 0, sizeof(scrollback_buffer));

	LC_World_Create(8, 8, 8);
	//LC_World_Create(1, 1, 1);



	

	int xs = 25;
	int ys = 8;

	xs = 25 * 64;
	ys = 8 * 32;

	//WorkHandleID work_id = Thread_AssignTask(test_func, NULL, 0);
	//WorkHandleID work_id2 = Thread_AssignTask(test_func, 12, 0);
	//WorkHandleID work_id3 = Thread_AssignTask(test_func, NULL, 0);

	int8_t packed = 0;

	int8_t hp = 7;
	int8_t norm = 5;

	packed = hp;
	packed = packed | (norm << 3);


	int8_t unpacked_hp = (packed & ~(7 << 3));
	int8_t unpacked_norm = packed >> 3;

	int i = 1;

	if (*((char*)&i) == 1) puts("little endian");
	else puts("big endian");

	
	
	
	//WorkHandleID id1 = Thread_AssignTask(test_func, 0);
	//WorkHandleID id2 = Thread_AssignTask(test_func, 0);


	//ReturnResult r = Thread_WaitForResult(work_id, INFINITE);
	//Thread_ReleaseWorkHandle(work_id);


	g_gui_atlas_texture = Texture_Load("assets/ui/gui_atlas.png", NULL);
	g_block_atlas = Texture_Load("assets/cube_textures/block_atlas.png", NULL);


	

	//LC_Player_Create(player_pos);

	PL_initPlayer(player_pos);
	
	s_gameData.camera.config.fov = 90;
	s_gameData.camera.config.zFar = 1500;
	s_gameData.camera.config.zNear = 0.1F;


	mouse_sensitivity = Cvar_Register("mouse_sensitivity", "0.1", "Mouse sensitivity help text", CVAR__SAVE_TO_FILE, 0.0, 1000);
	cam_fov = Cvar_Register("cam_fov", "90", NULL, CVAR__SAVE_TO_FILE, 45, 95);
	

	DWORD id = GetCurrentThreadId();

	vec3 position;
	position[0] = 16;
	position[1] = 16;
	position[2] = 0;
	vec3 direction;
	direction[0] = 1;
	direction[1] = 0;
	direction[2] = 0;

	
	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, position);

	vec4 normal;
	normal[0] = 0;
	normal[1] = 1;
	normal[2] = 0;
	normal[3] = 1;
	glm_mat4_mulv(model, normal, normal);



#ifdef PARTICLE_DEMO
	
	
#endif // PARTICLE DEMO


	Input_saveAllToFile("assets/inputtest.cfg");
	Input_loadAllFromFile("assets/inputtest.cfg");
#ifdef USE_NEW_RENDERER
	RScene_SetSkyboxTexturePanorama(Resource_get("assets/cubemaps/hdr/industrial.hdr", RESOURCE__TEXTURE_HDR));

	//RScene_SetSkyboxTextureSingleImage("assets/cubemaps/skybox/skybox-day.png");

	DirLight dirlight;
	dirlight.color[0] = 1;
	dirlight.color[1] = 1;
	dirlight.color[2] = 1;

	dirlight.ambient_intensity = 8;
	dirlight.specular_intensity = 1;
	
	dirlight.direction[0] = 1;
	dirlight.direction[1] = 1;
	dirlight.direction[2] = 1;
	//glm_normalize(dirlight.direction);
	RScene_SetDirLight(dirlight);
#endif
	float x = 0;
	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);
}

void LC_MouseUpdate(double x, double y)
{
	static bool prev_ui_opened = false;
	
	static double prev_x = 0;
	static double prev_y = 0;

	if (Con_isOpened() || Window_isCursorEnabled())
	{
		if (!prev_ui_opened)
		{
			prev_ui_opened = true;
		}
	}
	else
	{
		if (Core_CheckForBlockedInputState())
		{
			return;
		}

		if (prev_ui_opened)
		{
			glfwSetCursorPos(glfw_window, prev_x, prev_y);
			x = prev_x;
			y = prev_y;

			prev_ui_opened = false;
		}

		Camera_ProcessMouse(&s_gameData.camera, x, y);
		prev_x = x;
		prev_y = y;
	}	
}


#include <stdalign.h>


extern void PL_Update();
//extern void RPanel_Main();
void LC_Loop(float delta)
{
	AABB box;
	box.width = 1;
	box.height = 1;
	box.length = 1;
	box.position[0] = -30;
	box.position[1] = 0;
	box.position[2] = 0;

	vec4 color;
	memset(color, 0, sizeof(vec4));
	//ParticleDemo_Main();
	//r_drawAABB(box, color);

	//Draw_ScreenSprite(&sprite_test);
	//Draw_ScreenSprite(&sprite_test);
	//Draw_ScreenSprite(&sprite_2);

	int ind_len = (6 * sizeof(int));
	int vtx_len = (4 * 16);

	int aligment = 0;
	glGetIntegerv(GL_PACK_ALIGNMENT, &aligment);

	//RPanel_Main();

	s_gameData.camera.config.mouseSensitivity = mouse_sensitivity->float_value;
	s_gameData.camera.config.fov = cam_fov->float_value;


	PL_Update();

	//LC_Player_ProcessInput(glfw_window, &s_gameData.camera);
	//LC_Player_Update(&s_gameData.camera, delta);

	ivec2 window_size;
	Window_getSize(window_size);


	if(!Con_isOpened())
		Camera_UpdateMatrices(&s_gameData.camera, window_size[0], window_size[1]);

	vec3 pos;
	LC_Player_getPosition(pos);
	LC_getBiomeType(pos[0], pos[2]);

#ifdef USE_NEW_RENDERER
	//Draw_AABBWires1(aabb, NULL);
	//Draw_Line2(0, 0, 0, 25, 0, 0);
#else
	r_Update(&s_gameData.camera);
#endif
}
void LC_PhysLoop(float delta)
{
	PhysWorld_Step(LC_World_GetPhysWorld(), delta);

}
void LC_Cleanup()
{
	
	//Model_Destruct(&s_gameData.model);
	
}

extern void PL_IssueDrawCmds();
void LC_Draw()
{
	AABB aabb;
	aabb.width = 25;
	aabb.height = 25;
	aabb.length = 25;

	aabb.position[0] = 5;
	aabb.position[1] = 0;
	aabb.position[2] = 0;

#ifdef USE_NEW_RENDERER

	Draw_LCWorld();
	PL_IssueDrawCmds();
	LC_World_Draw();

	//Draw_AABB(aabb, NULL);

	vec3 box[2];
	
	memset(box, 0, sizeof(box));

	box[1][0] = 16;
	box[1][1] = 16;
	box[1][2] = 16;

	//Draw_TexturedCube(box, Resource_get("assets/cube_textures/simple_block_atlas.png", RESOURCE__TEXTURE), NULL);

	vec3 p1, p2, p3;
	p1[0] = 0;
	p1[1] = 0;
	p1[2] = 1;

	p2[0] = -5;
	p2[1] = 0;
	p2[2] = 0;

	p3[0] = -3;
	p3[1] = 5;
	p3[2] = 0;

	//Draw_Triangle(p1, p2, p3, NULL);
#else
	r_Update(&s_gameData.camera);
#endif
}
