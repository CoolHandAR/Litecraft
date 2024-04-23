#pragma once


#include "utility/dynamic_array.h"
#include "render/r_camera.h"
typedef struct GLFWwindow GLFWwindow;

void LC_Init();
void LC_MouseUpdate(double x, double y);
void LC_KBupdate(GLFWwindow* const window);
void LC_Loop(float delta);
void LC_PhysLoop(float delta);
void LC_Cleanup();
void LC_Movement(GLFWwindow* const window);