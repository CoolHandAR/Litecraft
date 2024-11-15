#include "r_camera.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cglm/clipspace/persp_rh_zo.h>
#include <cglm/clipspace/view_rh_zo.h>
#include "utility/u_math.h"
#include "core/core_common.h"

R_Camera* current_camera;

static Camera_Data getDefautltData()
{
	Camera_Data data;
	memset(&data, 0, sizeof(Camera_Data));
	
	data.camera_front[2] = -1.0f;
	data.world_up[1] = 1.0f;
	data.position[2] = 3.0f;
	data.yaw = -90;
	data.pitch = 0.0f;

	return data;
}

static Camera_Config getDefaultConfig()
{
	Camera_Config config;
	memset(&config, 0, sizeof(Camera_Config));
	
	config.fov = 67.0f;
	config.speed = 0.2f;
	config.mouseSensitivity = 0.1f;
	config.zNear = 0.05f;
	config.zFar = 4000.0f;

	return config;
}


R_Camera Camera_Init()
{
	R_Camera camera;

	camera.data = getDefautltData();
	camera.config = getDefaultConfig();

	return camera;
}

void Camera_updateFront(R_Camera* const p_cam)
{
	float yaw_in_radians = glm_rad(p_cam->data.yaw);
	float pitch_in_radians = glm_rad(p_cam->data.pitch);

	float cos_yaw = cos(yaw_in_radians);
	float cos_pitch = cos(pitch_in_radians);

	float sin_yaw = sin(yaw_in_radians);
	float sin_pitch = sin(pitch_in_radians);

	vec3 front;
	front[0] = cos_yaw * cos_pitch;
	front[1] = sin_pitch;
	front[2] = sin_yaw * cos_pitch;
	glm_normalize(front);
	glm_vec3_copy(front, p_cam->data.camera_front);
	
	glm_vec3_cross(p_cam->data.camera_front, p_cam->data.world_up, p_cam->data.camera_right);
	glm_normalize(p_cam->data.camera_right);

	glm_vec3_cross(p_cam->data.camera_right, p_cam->data.camera_front, p_cam->data.camera_up);
	glm_normalize(p_cam->data.camera_up);
}

void Camera_ProcessMouse(R_Camera* const p_cam, double x, double y)
{
	if (Core_CheckForBlockedInputState())
	{
		return;
	}

	static bool s_FirstMouse = true;

	static float last_x = 0.0f;
	static float last_y = 0.0f;

	if (s_FirstMouse)
	{
		x = 0;
		y = 0;
		s_FirstMouse = false;
	}

	float xOffset = x - last_x;
	float yOffset = last_y - y;

	last_x = x;
	last_y = y;

	p_cam->data.yaw += xOffset * p_cam->config.mouseSensitivity;
	p_cam->data.pitch += yOffset * p_cam->config.mouseSensitivity;


	if (p_cam->data.pitch > 89.0f)
		p_cam->data.pitch = 89.0f;
	if (p_cam->data.pitch < -89.0f)
		p_cam->data.pitch = -89.0f;

	Camera_updateFront(p_cam);
}

void Camera_ProcessMove(R_Camera* const p_cam, int x, int y)
{
	vec3 camera_right_multipled;
	glm_vec3_scale(p_cam->data.camera_right, p_cam->config.speed, &camera_right_multipled);

	vec3 camera_front_multiplied;
	glm_vec3_scale(p_cam->data.camera_front, p_cam->config.speed, &camera_front_multiplied);

	if (x > 0)
	{
		glm_vec3_add(p_cam->data.position, camera_right_multipled, p_cam->data.position);
	}
	else if (x < 0)
	{
		glm_vec3_sub(p_cam->data.position, camera_right_multipled, p_cam->data.position);
	}
	if (y > 0)
	{
		glm_vec3_add(p_cam->data.position, camera_front_multiplied, p_cam->data.position);
	}
	else if (y < 0)
	{
		glm_vec3_sub(p_cam->data.position, camera_front_multiplied, p_cam->data.position);
	}

}

void Camera_UpdateMatrices(R_Camera* const p_cam, float screen_width, float screen_height)
{
	return;
	vec3 center;
	glm_vec3_add(p_cam->data.position, p_cam->data.camera_front, center);

	glm_lookat(p_cam->data.position, center, p_cam->data.camera_up, p_cam->data.view_matrix);

	glm_perspective(glm_rad(p_cam->config.fov), screen_width / screen_height, p_cam->config.zNear, p_cam->config.zFar, p_cam->data.proj_matrix);
	
	mat4 view_proj;
	glm_mat4_mul(p_cam->data.proj_matrix, p_cam->data.view_matrix, view_proj);

	glm_frustum_planes(view_proj, p_cam->data.frustrum_planes);
}

void Camera_setCurrent(R_Camera* const p_cam)
{
	current_camera = p_cam;
}

R_Camera* Camera_getCurrent()
{
	return current_camera;
}

