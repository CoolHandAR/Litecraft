#pragma once

#include "physics/p_physics_defs.h";
#include "lc_defs.h"
#include <GLFW/glfw3.h>
#include "render/r_camera.h"



void LC_Player_Create(vec3 pos);
void LC_Player_Update(R_Camera* const cam, float delta);
void LC_Player_ProcessInput(GLFWwindow* const window, R_Camera* const cam);
void LC_Player_getPos(vec3 dest);