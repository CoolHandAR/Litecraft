#ifndef R_CAMERA_H
#define R_CAMERA_H

#include <cglm/cglm.h>
#include <stdbool.h>

typedef struct Camera_Config
{
	float fov;
	float speed;
	float mouseSensitivity;
	float zNear;
	float zFar;
} Camera_Config;

typedef struct Camera_Data
{
	mat4 view_matrix;
	mat4 proj_matrix;
	vec3 camera_front;
	vec3 camera_right;
	vec3 camera_up;
	vec3 world_up;
	vec3 position;

	float yaw;
	float pitch;

} Camera_Data;

typedef struct R_Camera
{
	Camera_Config config;
	Camera_Data data;

} R_Camera;


R_Camera Camera_Init();
void Camera_ProcessMouse(R_Camera* const p_cam, double x, double y);
void Camera_ProcessMove(R_Camera* const p_cam, int x, int y);
void Camera_UpdateMatrices(R_Camera* const p_cam, float screen_width, float screen_height);





#endif // !R_CAMERA_H
