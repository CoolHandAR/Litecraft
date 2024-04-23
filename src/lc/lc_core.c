#include "lc_core.h"

#include "render/r_model.h"

#include "render/r_renderer.h"

#include "utility/u_gl.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>



#include "render/r_texture.h"

#include "render/r_camera.h"

#include "lc_chunk.h"

#include "physics/p_aabb_tree.h"
#include "physics/p_physics_defs.h"
#include "physics/p_physics_world.h"
#include "lc_world.h"
#include "lc_player.h"
#include "render/r_sprite.h"

#include "sound/s_sound.h"


extern void r_startFrame(R_Camera* const p_cam, ivec2 window_size, LC_World* const world);
extern void r_endFrame();

typedef struct G_GameData
{
	R_Model model;
	R_Camera camera;
	LC_World* WORLD;
	R_Sprite sprite;
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
extern GLFWwindow* s_Window;
extern ivec2 s_windowSize;
#include "core/c_resource_manager.h"
#include "render/r_shader.h"

extern ma_engine sound_engine;

void LC_Init()
{
	s_gameData.camera = Camera_Init();
	s_gameData.WORLD = LC_World_Generate();
	//s_gameData.model = Model_Load("assets/backpack/backpack.obj");

	r_Init();
	
	vec3 player_pos;
	player_pos[0] = -5;
	player_pos[1] = 0;
	player_pos[2] = 5;
	

	glClearColor(0.4, 0.7, 1, 1.0);


	g_gui_atlas_texture = Texture_Load("assets/ui/gui_atlas.png", NULL);
	g_block_atlas = Texture_Load("assets/cube_textures/block_atlas.png", NULL);

	LC_Player_Create(s_gameData.WORLD, player_pos);
	
	s_gameData.camera.config.fov = 85;
	s_gameData.camera.config.zFar = 500;
	s_gameData.camera.config.zNear = 0.1F;

	


	vec3 position;
	position[0] = 0;
	position[1] = 0;
	position[2] = 0;
	vec3 direction;
	direction[0] = 1;
	direction[1] = 0;
	direction[2] = 0;

	s_LoadSpatialSound(&s_gameData.sound, "assets/sounds/jump.wav", 0, 1, position, direction, NULL);
	


	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);
}

void LC_MouseUpdate(double x, double y)
{
	Camera_ProcessMouse(&s_gameData.camera, x, y);
}

void LC_KBupdate(GLFWwindow* const window)
{
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
	{
		ma_sound_start(&s_gameData.sound);
	}

}

void LC_Movement(GLFWwindow* const window)
{

	
	
}


void LC_Loop(float delta)
{
	LC_KBupdate(s_Window);


	LC_Player_ProcessInput(s_Window, &s_gameData.camera);
	Camera_UpdateMatrices(&s_gameData.camera, s_windowSize[0], s_windowSize[1]);
	LC_Player_Update(&s_gameData.camera, delta);

	LC_World_Update(delta);

	r_startFrame(&s_gameData.camera, s_windowSize, s_gameData.WORLD);

	r_endFrame();
	
	vec3 player_pos;
	LC_Player_getPos(player_pos);
	
	
	ma_engine_listener_set_position(&sound_engine, 0, player_pos[0], player_pos[1], player_pos[2]);
	
	
	
	
	
	


	



}
void LC_PhysLoop(float delta)
{
	PhysWorld_Step(s_gameData.WORLD->phys_world, delta);

}
void LC_Cleanup()
{
	LC_World_Destruct(s_gameData.WORLD);
	//Model_Destruct(&s_gameData.model);
	
}
