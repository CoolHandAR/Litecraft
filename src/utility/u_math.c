#include "u_math.h"

#include <string.h>

#include <cglm/clipspace/persp_rh_zo.h>
#include <cglm/clipspace/ortho_rh_zo.h>
#include <cglm/clipspace/view_rh_zo.h>



void Math_Proj_ReverseZInfinite(float fovy, float aspect, float nearZ, float farZ, mat4 dest)
{
	float f = 1.0 / tanf(fovy / 2.0);

	glm_mat4_zero(dest);

	dest[0][0] = f / aspect;
	dest[1][1] = f;
	dest[2][3] = -1.0;
	dest[3][2] = nearZ;
}

void Math_GLM_FrustrumPlanes_ReverseZ(mat4 m, vec4 dest[6])
{
	mat4 t;

	glm_mat4_transpose_to(m, t);

	//t[3][2] += 1.0;

	glm_vec4_add(t[3], t[0], dest[0]); /* left   */
	glm_vec4_sub(t[3], t[0], dest[1]); /* right  */
	glm_vec4_add(t[3], t[1], dest[2]); /* bottom */
	glm_vec4_sub(t[3], t[1], dest[3]); /* top    */
	glm_vec4_sub(t[3], t[2], dest[4]); /* far   */

	t[2][2] = 0.0;
	glm_vec4_sub(t[2], t[3], dest[5]); /* near    */

	//dest[5][2] += 1.0;

	float far, near;
	glm_persp_decomp_z_rh_zo(m, &far, &near);

	dest[5][3] = far;

	glm_plane_normalize(dest[0]);
	glm_plane_normalize(dest[1]);
	glm_plane_normalize(dest[2]);
	glm_plane_normalize(dest[3]);
	glm_plane_normalize(dest[4]);
	glm_plane_normalize(dest[5]);
}

long double Math_fract(long double x)
{
	return fminl(x - floorl(x), 0.999999940395355224609375);
	
}

void Math_calcLightSpaceMatrix2(const float p_cameraFov, const float p_screenWidth, const float p_screenHeight, const float p_nearPlane, const float p_farPlane, const float p_zMult,
	vec3 p_lightDir, mat4 p_cameraView, mat4 dest, int count)
{
	mat4 proj;
	mat4 inv_view_proj;
	mat4 light_view;
	vec4 frustrum_corners[8];
	vec4 frustrum_center;
	vec3 eye;
	vec3 up;
	vec4 furthest_point;



	glm_perspective(glm_rad(p_cameraFov), p_screenWidth / p_screenHeight, p_nearPlane, p_farPlane, proj);

	glm_mat4_mul(proj, p_cameraView, inv_view_proj);
	glm_mat4_inv(inv_view_proj, inv_view_proj);

	glm_frustum_corners(inv_view_proj, frustrum_corners);

	glm_frustum_center(frustrum_corners, frustrum_center);

	glm_vec4_sub(frustrum_corners[0], frustrum_corners[6], furthest_point);
	float radius = glm_vec4_norm(furthest_point);
	float texels_per_unit = 1280.0 / (radius * 2.0);

	mat4 base_look_at;
	mat4 base_look_at_inv;
	vec3 look_at;

	vec3 zero;
	memset(up, 0, sizeof(vec3));
	up[1] = 1;
	glm_vec3_zero(zero);
	look_at[0] = -p_lightDir[0];
	look_at[1] = -p_lightDir[1];
	look_at[2] = -p_lightDir[2];

	glm_lookat(zero, look_at, up, base_look_at);

	glm_mat4_scale(base_look_at, texels_per_unit);
	glm_mat4_inv(base_look_at, base_look_at_inv);

	glm_mat4_mulv3(base_look_at, frustrum_center, 1.0, frustrum_center);
	frustrum_center[0] = floorf(frustrum_center[0]);
	frustrum_center[1] = floorf(frustrum_center[1]);
	glm_mat4_mulv3(base_look_at_inv, frustrum_center, 1.0, frustrum_center);


	eye[0] = frustrum_center[0] + (p_lightDir[0] * radius * 2.0);
	eye[1] = frustrum_center[1] + (p_lightDir[1] * radius * 2.0);
	eye[2] = frustrum_center[2] + (p_lightDir[2] * radius * 2.0);



	glm_lookat(eye, frustrum_center, up, light_view);

	float min_x = FLT_MAX;
	float min_y = FLT_MAX;
	float min_z = FLT_MAX;

	float max_x = -FLT_MAX;
	float max_y = -FLT_MAX;
	float max_z = -FLT_MAX;


	for (int i = 0; i < 8; i++)
	{
		vec4 trf;
		glm_mat4_mulv(light_view, frustrum_corners[i], trf);

		min_x = min(min_x, trf[0]);
		min_y = min(min_y, trf[1]);
		min_z = min(min_z, trf[2]);

		max_x = max(max_x, trf[0]);
		max_y = max(max_y, trf[1]);
		max_z = max(max_z, trf[2]);
	}


	if (min_z < 0)
	{
		min_z *= p_zMult;
	}
	else
	{
		min_z /= p_zMult;
	}
	if (max_z < 0)
	{
		max_z /= p_zMult;
	}
	else
	{
		max_z *= p_zMult;
	}

	mat4 light_proj;




	glm_ortho(-radius, radius, -radius, radius, 0.0, radius * 6, light_proj);

	glm_mat4_mul(light_proj, light_view, dest);

}
void getAxis(mat4 m, int axis, vec3 dest)
{
	dest[0] = m[0][axis];
	dest[1] = m[1][axis];
	dest[2] = m[2][axis];
}
void getNormalizedBasisAxis(mat4 m, int axis, vec3 dest)
{
	dest[0] = m[0][axis];
	dest[1] = m[1][axis];
	dest[2] = m[2][axis];

	glm_normalize(dest);
}

void OrthoNormalize(mat4 m)
{
	vec3 x, y, z;
	getAxis(m, 0, x);
	getAxis(m, 1, y);
	getAxis(m, 2, z);

	glm_normalize(x);
	y[0] = y[0] - x[0] * (glm_vec3_dot(x, y));
	y[1] = y[1] - x[1] * (glm_vec3_dot(x, y));
	y[2] = y[2] - x[2] * (glm_vec3_dot(x, y));

	glm_normalize(y);

	z[0] = (z[0] - x[0] * (glm_vec3_dot(x, y)) - y[0] * glm_vec3_dot(y, z));
	z[1] = (z[1] - x[1] * (glm_vec3_dot(x, y)) - y[1] * glm_vec3_dot(y, z));
	z[2] = (z[2] - x[2] * (glm_vec3_dot(x, y)) - y[2] * glm_vec3_dot(y, z));
	glm_normalize(z);

	m[0][0] = x[0];
	m[1][0] = x[1];
	m[2][0] = x[2];

	m[0][1] = y[0];
	m[1][1] = y[1];
	m[2][1] = y[2];

	m[0][2] = z[0];
	m[1][2] = z[1];
	m[2][2] = z[2];
}

bool plane_intersect_3(vec4 plane, vec3 plane2, vec3 plane3, vec3 r_result)
{
	vec3 norm0;
	vec3 norm1;
	vec3 norm2;

	glm_vec3_copy(plane, norm0);
	glm_vec3_copy(plane2, norm1);
	glm_vec3_copy(plane3, norm2);

	vec3 crossed;
	glm_vec3_cross(norm0, norm1, crossed);

	float denom = glm_vec3_dot(crossed, norm2);

	if (denom == 0.0)
	{
		return false;
	}

	if (r_result)
	{
		vec3 cross0, cross1, cross2;

		glm_vec3_cross(norm1, norm2, cross0);
		glm_vec3_cross(norm2, norm0, cross1);
		glm_vec3_cross(norm0, norm1, cross2);

		glm_vec3_scale(cross0, plane[3], cross0);
		glm_vec3_scale(cross1, plane2[3], cross1);
		glm_vec3_scale(cross2, plane3[3], cross2);

		vec3 r;
		glm_vec3_add(cross0, cross1, r);
		glm_vec3_add(r, cross2, r);

		r[0] /= denom;
		r[1] /= denom;
		r[2] /= denom;

		glm_vec3_copy(r, r_result);
	}
}

void getFrustrumEndpoints(mat4 proj, mat4 view, vec3 r_points[8])
{
	vec4 planes[6];
	glm_frustum_planes(proj, planes);

	// [left, right, bottom, top, near, far]

	int INTERSECTIONS[8][3] =
	{
		GLM_RIGHT, GLM_BOTTOM, GLM_NEAR,
		GLM_RIGHT, GLM_BOTTOM, GLM_FAR,
		GLM_RIGHT, GLM_TOP, GLM_NEAR,
		GLM_RIGHT, GLM_TOP, GLM_FAR,

		GLM_LEFT, GLM_BOTTOM, GLM_NEAR,
		GLM_LEFT, GLM_BOTTOM, GLM_FAR,
		GLM_LEFT, GLM_TOP, GLM_NEAR,
		GLM_LEFT, GLM_TOP, GLM_FAR,
	};

	for (int i = 0; i < 6; i++)
	{
		planes[i][0] = -planes[i][0];
		planes[i][1] = -planes[i][1];
		planes[i][2] = -planes[i][2];
	}

	for (int i = 0; i < 8; i++)
	{
		vec3 p;

		bool res = plane_intersect_3(planes[INTERSECTIONS[i][0]], planes[INTERSECTIONS[i][1]], planes[INTERSECTIONS[i][2]], p);

		glm_mat4_mulv3(view, p, 1.0, r_points[i]);
	}
}

void Math_calcLightSpaceMatrix(const float p_cameraFov, const float p_screenWidth, const float p_screenHeight, const float p_nearPlane, const float p_farPlane, const float p_zMult,
	vec3 p_lightDir, mat4 p_cameraView, mat4 dest, vec4 frustrum_dest[6])
{
	mat4 proj;
	mat4 inv_view_proj;
	mat4 light_view;
	vec4 frustrum_corners[8];
	vec4 frustrum_center;
	vec3 eye;
	vec3 up;
	vec4 furthest_point;

	glm_perspective(glm_rad(p_cameraFov), p_screenWidth / p_screenHeight, p_nearPlane, p_farPlane, proj);

	glm_mat4_mul(proj, p_cameraView, inv_view_proj);
	glm_mat4_inv(inv_view_proj, inv_view_proj);

	glm_frustum_corners(inv_view_proj, frustrum_corners);

	glm_frustum_center(frustrum_corners, frustrum_center);

	glm_vec4_sub(frustrum_corners[0], frustrum_corners[6], furthest_point);

	float radius = 0.0;
	for (int i = 0; i < 8; i++)
	{
		float dist = glm_vec4_distance(frustrum_corners[i], frustrum_center);
		radius = max(radius, dist);
	}
	//radius = glm_vec4_norm(furthest_point);

	float texels_per_unit = 1280.0 / (radius * 2.0);

	mat4 base_look_at;
	mat4 base_look_at_inv;
	vec3 look_at;

	vec3 zero;
	memset(up, 0, sizeof(vec3));
	up[1] = 1;
	glm_vec3_zero(zero);
	look_at[0] = -p_lightDir[0];
	look_at[1] = -p_lightDir[1];
	look_at[2] = -p_lightDir[2];

	glm_lookat(zero, look_at, up, base_look_at);

	glm_mat4_scale(base_look_at, texels_per_unit);
	glm_mat4_inv(base_look_at, base_look_at_inv);

	glm_mat4_mulv3(base_look_at, frustrum_center, 1.0, frustrum_center);
	frustrum_center[0] = floorf(frustrum_center[0]);
	frustrum_center[1] = floorf(frustrum_center[1]);
	glm_mat4_mulv3(base_look_at_inv, frustrum_center, 1.0, frustrum_center);


	eye[0] = frustrum_center[0] + (p_lightDir[0] * radius);
	eye[1] = frustrum_center[1] + (p_lightDir[1] * radius);
	eye[2] = frustrum_center[2] + (p_lightDir[2] * radius);

	float minX = FLT_MAX;
	float minY = FLT_MAX;
	float minZ = FLT_MAX;

	float maxX = FLT_MIN;
	float maxY = FLT_MIN;
	float maxZ = FLT_MIN;

	glm_lookat(eye, frustrum_center, up, light_view);
	//*Exracted planes order : [left, right, bottom, top, near, far]

	for (int i = 0; i < 8; i++)
	{
		vec4 trf;
		glm_mat4_mulv(light_view, frustrum_corners[i], trf);

		minX = min(minX, trf[0]);
		minY = min(minY, trf[1]);
		minZ = min(minZ, trf[2]);

		maxX = max(maxX, trf[0]);
		maxY = max(maxY, trf[1]);
		maxZ = max(maxZ, trf[2]);
	}

	
	float z_rad = radius;

	float zMult = 2.0f;
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	mat4 light_proj;

	glm_ortho(-radius, radius, -radius, radius, 0.0, maxZ - minZ, light_proj);

	glm_mat4_mul(light_proj, light_view, dest);
}



void Math_getLightSpacesMatrixesForFrustrum(const R_Camera* const p_camera, const float p_screenWidth, const float p_screenHeight,
	const float p_zMult, vec4 p_shadowCascadeLevels, vec3 p_lightDir, mat4 dest[5], vec4 frustrum_planes[5][6])
{

	Math_calcLightSpaceMatrix(p_camera->config.fov, p_screenWidth, p_screenHeight, p_camera->config.zNear, p_shadowCascadeLevels[0], p_zMult, p_lightDir, p_camera->data.view_matrix, dest[0], frustrum_planes[0]);
	Math_calcLightSpaceMatrix(p_camera->config.fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[0], p_shadowCascadeLevels[1], p_zMult, p_lightDir, p_camera->data.view_matrix, dest[1], frustrum_planes[1]);
	Math_calcLightSpaceMatrix(p_camera->config.fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[1], p_shadowCascadeLevels[2], p_zMult, p_lightDir, p_camera->data.view_matrix, dest[2], frustrum_planes[2]);
	Math_calcLightSpaceMatrix(p_camera->config.fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[2], p_shadowCascadeLevels[3], p_zMult, p_lightDir, p_camera->data.view_matrix, dest[3], frustrum_planes[3]);
	//Math_calcLightSpaceMatrix(p_camera->config.fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[3], p_camera->config.zFar, p_zMult, p_lightDir, p_camera->data.view_matrix, dest[4], frustrum_planes[4]);
}



void Math_CalcOrthoProj(const R_Camera* const p_camera, const float p_screenWidth, const float p_screenHeight, float p_shadowCascadeLevels[5])
{
	float ar = p_screenHeight / p_screenWidth;
	float tanHalfHFOV = tanf(glm_rad(p_camera->config.fov / 2.0f));
	float tanHalfVFOV = tanf(glm_rad((p_camera->config.fov * ar) / 2.0f));

	for (int i = 0; i < 5; i++)
	{
		float xn = p_shadowCascadeLevels[i] * tanHalfHFOV;
		float xf = p_shadowCascadeLevels[i + 1] * tanHalfHFOV;
		float yn = p_shadowCascadeLevels[i] * tanHalfVFOV;
		float yf = p_shadowCascadeLevels[i + 1] * tanHalfVFOV;

		vec4 frustrum_corners[8];
		frustrum_corners[0][0] = xn;
		frustrum_corners[0][1] = yn;
		frustrum_corners[0][2] = p_shadowCascadeLevels[i];
		frustrum_corners[0][3] = 1.0;

		for (int j = 0; j < 4; j++)
		{
			frustrum_corners[j][0] = xn;
			frustrum_corners[j][1] = yn;
			frustrum_corners[j][2] = p_shadowCascadeLevels[i];
			frustrum_corners[j][3] = 1.0;

			xn = -xn;
			yn = -yn;
		}
		for (int j = 0; j < 4; j++)
		{
			frustrum_corners[j][0] = xf;
			frustrum_corners[j][1] = yf;
			frustrum_corners[j][2] = p_shadowCascadeLevels[i + 1];
			frustrum_corners[j][3] = 1.0;

			xf = -xf;
			yf = -yf;
		}
		float min_x = FLT_MAX;
		float min_y = FLT_MAX;
		float min_z = FLT_MAX;

		float max_x = -FLT_MAX;
		float max_y = -FLT_MAX;
		float max_z = -FLT_MAX;
		
	}
}


bool AABB_intersectsRay(AABB* const aabb, vec3 p_from, vec3 p_dir, vec3 r_clip, vec3 r_normal, float* r_far, float* r_near)
{
	vec3 c1, c2;
	vec3 end;
	end[0] = aabb->position[0] + aabb->width;
	end[1] = aabb->position[1] + aabb->height;
	end[2] = aabb->position[2] + aabb->length;


	float near = -1e20;
	float far = 1e20;

	int axis = 0;

	for (int i = 0; i < 3; i++) 
	{
		if (p_dir[i] == 0)
		{
			if ((p_from[i] < aabb->position[i]) || (p_from[i] > end[i])) {
				return false;
			}
		}
		else 
		{ // ray not parallel to planes in this direction
			c1[i] = (aabb->position[i] - p_from[i]) / p_dir[i];
			c2[i] = (end[i] - p_from[i]) / p_dir[i];

			if (c1[i] > c2[i]) 
			{
				vec3 temp;
				glm_vec3_copy(c1, temp);

				glm_vec3_copy(c2, c1);
				glm_vec3_copy(temp, c2);

			}
			if (c1[i] > near) 
			{
				near = c1[i];
				axis = i;
			}
			if (c2[i] < far) 
			{
				far = c2[i];
			}
			if ((near > far) || (far < 0)) 
			{
				return false;
			}
		}
	}

	if (r_clip != NULL)
		glm_vec3_copy(c1, r_clip);


	if (r_normal != NULL)
	{
		r_normal[0] = 0;
		r_normal[1] = 0;
		r_normal[2] = 0;

		r_normal[axis] = (p_dir[axis] > 0) ? -1 : 1;
	}

	if (r_far != NULL)
	{
		*r_far = far;
	}

	if (r_near != NULL)
	{
		*r_near = near;
	}

	return true;
}

bool AABB_intersectsOther(AABB* aabb, AABB* other)
{
	if (aabb->position[0] >= (other->position[0] + other->width))
	{
		return false;
	}
	if (aabb->position[0] + aabb->width <= other->position[0])
	{
		return false;
	}
	if (aabb->position[1] >= (other->position[1] + other->height))
	{
		return false;
	}
	if (aabb->position[1] + aabb->height <= other->position[1])
	{
		return false;
	}
	if (aabb->position[2] >= (other->position[2] + other->length))
	{
		return false;
	}
	if (aabb->position[2] + aabb->length <= other->position[2])
	{
		return false;
	}
	return true;
}

void AABB_MergeWith(AABB* aabb, AABB* other, AABB* dest)
{
	vec3 beg_1, beg_2;
	vec3 end_1, end_2;
	vec3 min, max;

	glm_vec3_copy(aabb->position, beg_1);
	glm_vec3_copy(other->position, beg_2);

	end_1[0] = aabb->width + beg_1[0];
	end_1[1] = aabb->height + beg_1[1];
	end_1[2] = aabb->length + beg_1[2];

	end_2[0] = other->width + beg_2[0];
	end_2[1] = other->height + beg_2[1];
	end_2[2] = other->length + beg_2[2];

	
	min[0] = (beg_1[0] < beg_2[0]) ? beg_1[0] : beg_2[0];
	min[1] = (beg_1[1] < beg_2[1]) ? beg_1[1] : beg_2[1];
	min[2] = (beg_1[2] < beg_2[2]) ? beg_1[2] : beg_2[2];

	max[0] = (end_1[0] > end_2[0]) ? end_1[0] : end_2[0];
	max[1] = (end_1[1] > end_2[1]) ? end_1[1] : end_2[1];
	max[2] = (end_1[2] > end_2[2]) ? end_1[2] : end_2[2];

	glm_vec3_copy(min, dest->position);

	dest->width = max[0] - min[0];
	dest->height = max[1] - min[1];
	dest->length = max[2] - min[2];
}

AABB AABB_getIntersectionWithOther(AABB* aabb, AABB* other)
{
	vec3 src_min;
	glm_vec3_copy(aabb->position, src_min);

	vec3 src_max;
	src_max[0] = aabb->position[0] + aabb->width;
	src_max[1] = aabb->position[1] + aabb->length;
	src_max[2] = aabb->position[2] + aabb->length;

	vec3 dst_min;
	glm_vec3_copy(other->position, dst_min);

	vec3 dst_max;
	dst_max[0] = other->position[0] + other->width;
	dst_max[1] = other->position[1] + other->length;
	dst_max[2] = other->position[2] + other->length;

	vec3 min, max;

	AABB box;
	memset(&box, 0, sizeof(AABB));

	if (src_min[0] > dst_max[0] || src_max[0] < dst_min[0])
	{
		return box;
	}
	else
	{
		min[0] = (src_min[0] > dst_min[0]) ? src_min[0] : dst_min[0];
		max[0] = (src_max[0] < dst_max[0]) ? src_max[0] : dst_max[0];
	}
	if (src_min[1] > dst_max[1] || src_max[1] < dst_min[1])
	{
		return box;
	}
	else
	{
		min[1] = (src_min[1] > dst_min[1]) ? src_min[1] : dst_min[1];
		max[1] = (src_max[1] < dst_max[1]) ? src_max[1] : dst_max[1];
	}
	if (src_min[2] > dst_max[2] || src_max[2] < dst_min[2])
	{
		return box;
	}
	else
	{
		min[2] = (src_min[2] > dst_min[2]) ? src_min[2] : dst_min[2];
		max[2] = (src_max[2] < dst_max[2]) ? src_max[2] : dst_max[2];
	}


	glm_vec3_copy(min, box.position);
	
	box.width = max[0] - min[0];
	box.height = max[1] - min[1];
	box.length = max[2] - min[2];

	return box;
}

float AABB_GetSurfaceArea(AABB* aabb)
{
	return aabb->width * aabb->height * aabb->length;
}

AABB AABB_getMinkDiff(AABB* first, AABB* other)
{
	AABB md;
	md.position[0] = first->position[0] - (other->position[0] + other->width);
	md.position[1] = first->position[1] - (other->position[1] + other->height);
	md.position[2] = first->position[2] - (other->position[2] + other->length);

	md.width = first->width + other->width;
	md.height = first->height + other->height;
	md.length = first->length + other->length;
		
	return md;
}

AABB AABB_ExpandedTo(AABB* first, vec3 to)
{
	vec3 begin, end;
	glm_vec3_copy(first->position, begin);
	
	end[0] = first->position[0] + first->width;
	end[1] = first->position[1] + first->height;
	end[2] = first->position[2] + first->length;

	if (to[0] < begin[0]) 
	{
		begin[0] = to[0];
	}
	if (to[1] < begin[1]) 
	{
		begin[1] = to[1];
	}
	if (to[2] < begin[2]) 
	{
		begin[2] = to[2];
	}

	if (to[0] > end[0]) 
	{
		end[0] = to[0];
	}
	if (to[1] > end[1]) 
	{
		end[1] = to[1];
	}
	if (to[2] > end[2]) 
	{
		end[2] = to[2];
	}

	AABB box;
	glm_vec3_copy(begin, box.position);
	
	box.width = end[0] - begin[0];
	box.height = end[1] - begin[1];
	box.length = end[2] - begin[2];

	return box;
}

bool between(float x, float a, float b)
{
	return x >= a && x <= b;
}

float AABB_getFirstRayIntersection(AABB const aabb, vec3 p_dir, vec3 r_intersection, vec3 r_normal)
{
	vec3 n;
	memset(&n, 0, sizeof(vec3));
	float dist = 1;

	Plane p;
	memset(&p, 0, sizeof(Plane));

	vec3 from;
	memset(from, 0, sizeof(vec3));

	
	if (p_dir[0] > 0)
	{
		//X_MIN
		p.normal[0] = -1;
		p.normal[1] = 0;
		p.normal[2] = 0;

		from[0] = aabb.position[0];
		from[1] = aabb.position[1];
		from[2] = aabb.position[2];

		float x_min = 1;
		Plane_IntersectsRay(&p, from, p_dir, r_intersection, &x_min);

		if (x_min < dist && x_min > (float)CMP_EPSILON && between(x_min * p_dir[1], aabb.position[1], aabb.position[1] + aabb.height) &&
			between(x_min * p_dir[2], aabb.position[2], aabb.position[2] + aabb.length))
		{
			dist = x_min;
			n[0] = -1;
			n[1] = 0;
			n[2] = 0;
		}
	}

	else if (p_dir[0] < 0)
	{
		//X MAX
		p.normal[0] = 1;
		p.normal[1] = 0;
		p.normal[2] = 0;
		from[0] = aabb.position[0] + aabb.width;
		from[1] = aabb.position[1];
		from[2] = aabb.position[2];

		float x_max = 1;
		Plane_IntersectsRay(&p, from, p_dir, r_intersection, &x_max);

		if (x_max < dist && x_max > (float)CMP_EPSILON && between(x_max * p_dir[1], aabb.position[1], aabb.position[1] + aabb.height) &&
			between(x_max * p_dir[2], aabb.position[2], aabb.position[2] + aabb.length))
		{
			dist = x_max;
			n[0] = 1;
			n[1] = 0;
			n[2] = 0;
		}
	}
	if (p_dir[1] > 0)
	{
		//Y MIN
		p.normal[0] = 0;
		p.normal[1] = -1;
		p.normal[2] = 0;

		from[0] = aabb.position[0];
		from[1] = aabb.position[1];
		from[2] = aabb.position[2];

		float y_min = 1;
		Plane_IntersectsRay(&p, from, p_dir, r_intersection, &y_min);

		if (y_min < dist && y_min > (float)CMP_EPSILON && between(y_min * p_dir[0], aabb.position[0], aabb.position[0] + aabb.width) &&
			between(y_min * p_dir[2], aabb.position[2], aabb.position[2] + aabb.length))
		{
			dist = y_min;
			n[0] = 0;
			n[1] = -1;
			n[2] = 0;
		}
	}
	else if (p_dir[1] < 0)
	{
		//Y MAX
		p.normal[0] = 0;
		p.normal[1] = 1;
		p.normal[2] = 0;

		from[0] = aabb.position[0];
		from[1] = aabb.position[1] + aabb.height;
		from[2] = aabb.position[2];

		float y_max = 1;
		Plane_IntersectsRay(&p, from, p_dir, r_intersection, &y_max);

		if (y_max < dist && y_max > (float)CMP_EPSILON && between(y_max * p_dir[0], aabb.position[0], aabb.position[0] + aabb.width) &&
			between(y_max * p_dir[2], aabb.position[2], aabb.position[2] + aabb.length))
		{
			dist = y_max;
			n[0] = 0;
			n[1] = 1;
			n[2] = 0;
		}
	}
	if (p_dir[2] > 0)
	{
		//Z MIN
		p.normal[0] = 0;
		p.normal[1] = 0;
		p.normal[2] = -1;

		from[0] = aabb.position[0];
		from[1] = aabb.position[1];
		from[2] = aabb.position[2];

		float z_min = 1;
		Plane_IntersectsRay(&p, from, p_dir, r_intersection, &z_min);

		if (z_min < dist && z_min > (float)CMP_EPSILON && between(z_min * p_dir[0], aabb.position[0], aabb.position[0] + aabb.width) &&
			between(z_min * p_dir[1], aabb.position[1], aabb.position[1] + aabb.height))
		{
			dist = z_min;
			n[0] = 0;
			n[1] = 0;
			n[2] = -1;
		}
	}
	else if (p_dir[2] < 0)
	{
		//Z MAX
		p.normal[0] = 0;
		p.normal[1] = 0;
		p.normal[2] = 1;

		from[0] = aabb.position[0];
		from[1] = aabb.position[1];
		from[2] = aabb.position[2] + aabb.length;

		float z_max = 1;
		Plane_IntersectsRay(&p, from, p_dir, r_intersection, &z_max);

		if (z_max < dist && z_max > (float)CMP_EPSILON && between(z_max * p_dir[0], aabb.position[0], aabb.position[0] + aabb.width) &&
			between(z_max * p_dir[1], aabb.position[1], aabb.position[1] + aabb.height))
		{
			dist = z_max;
			n[0] = 0;
			n[1] = 0;
			n[2] = 1;
		}
	}


	if (r_normal != NULL)
	{
		r_normal[0] = n[0];
		r_normal[1] = n[1];
		r_normal[2] = n[2];
	}

	return dist;
}

float AABB_checkPlaneIntersection(AABB const p_aabb, vec3 p_dir, vec3 p_normal)
{
	Plane p;
	memset(&p, 0, sizeof(Plane));

	vec3 from;
	memset(from, 0, sizeof(vec3));


	

	return 0.0f;
}

void AABB_CLOSESTBOUDNS(AABB* first, vec3 point, vec3 dest)
{
	float min_dist = fabs(point[0] - first->position[0]);
	vec3 bounds_point;
	bounds_point[0] = first->position[0];
	bounds_point[1] = point[1];
	bounds_point[2] = point[2];

	if (fabs((first->position[0] + first->width) - point[0]) < min_dist)
	{
		min_dist = fabs((first->position[0] + first->width) - point[0]);
		bounds_point[0] = first->position[0] + first->width;
		bounds_point[1] = point[1];
		bounds_point[2] = point[2];
	}

	if (fabs((first->position[1] + first->height) - point[1]) < min_dist)
	{
		min_dist = fabs((first->position[1] + first->height) - point[1]);
		
		bounds_point[0] = point[0];
		bounds_point[1] = first->position[1] + first->height;
		bounds_point[2] = point[2];
		
	}
	if (fabs((first->position[1]) - point[1]) < min_dist)
	{
		min_dist = fabs((first->position[1]) - point[1]);
		bounds_point[0] = point[0];
		bounds_point[1] = first->position[1];
		bounds_point[2] = point[2];
	}

	if (fabs((first->position[2] + first->length) - point[2]) < min_dist)
	{
		min_dist = fabs((first->position[2] + first->length) - point[2]);
		bounds_point[0] = point[0];
		bounds_point[1] = point[1];
		bounds_point[2] = first->position[2] + first->length;
	}

	if (fabs(first->position[2] - point[2]) < min_dist)
	{
		min_dist = fabs((first->position[2]) - point[2]);
		bounds_point[0] = point[0];
		bounds_point[1] = point[1];
		bounds_point[2] = first->position[2];
	}


	dest[0] = bounds_point[0];
	dest[1] = bounds_point[1];
	dest[2] = bounds_point[2];
}

void AABB_getPenerationDepth(AABB* first, vec3 dest)
{
	vec3 zero;
	zero[0] = 0;
	zero[1] = 0;
	zero[2] = 0;

	AABB_CLOSESTBOUDNS(first, zero, dest);
}

bool AABB_MinkIntersectionCheck(AABB* mink)
{
	if (mink->position[0] >= 0)
	{
		if (mink->position[0] + 0.5 >= 0)
		{
			return false;
		}
	}

}

bool AABB_hasPoint(AABB* aabb, vec3 point)
{
	if (point[0] < aabb->position[0])
	{
		return false;
	}
	if (point[1] < aabb->position[1])
	{
		return false;
	}
	if (point[2] < aabb->position[2])
	{
		return false;
	}
	if (point[0] > aabb->position[0] + aabb->width)
	{
		return false;
	}
	if (point[1] > aabb->position[1] + aabb->height)
	{
		return false;
	}
	if (point[2] > aabb->position[2] + aabb->length)
	{
		return false;
	}

	return true;
}

void AABB_getCenter(AABB* aabb, vec3 dest)
{
	dest[0] = aabb->position[0] + (aabb->width * 0.5f);
	dest[1] = aabb->position[1] + (aabb->height * 0.5f);
	dest[2] = aabb->position[2] + (aabb->length * 0.5f);
}

bool Plane_IntersectsRay(Plane* const p_plane, vec3 from, vec3 dir, vec3 r_intersection, float* distr)
{
	vec3 segment;
	glm_vec3_copy(dir, segment);
	
	float den = glm_dot(p_plane->normal, segment);

	if (Math_IsZeroApprox(den))
	{
		return false;
	}

	float dist = (glm_dot(p_plane->normal, from) - p_plane->distance) / den;

	*distr = dist;

	if (dist > (float)CMP_EPSILON)
	{
		return false;
	}

	dist = -dist;

	if (r_intersection != NULL)
	{
		r_intersection[0] = from[0] + segment[0] * dist;
		r_intersection[1] = from[1] + segment[1] * dist;
		r_intersection[2] = from[2] + segment[2] * dist;
	}
	

	return true;
}

bool Plane_IntersectsSegment(Plane* const p_plane, vec3 begin, vec3 end, vec3 r_intersection, float* distance)
{
	vec3 segment;
	glm_vec3_sub(begin, end, segment);
	
	float den = glm_dot(p_plane->normal, segment);

	if (Math_IsZeroApprox(den))
	{
		return false;
	}

	float dist = (glm_dot(p_plane->normal, begin) - p_plane->distance) / den;

	if (dist < (float)-CMP_EPSILON || dist >(1.0f + (float)CMP_EPSILON))
	{
		return false;
	}

	dist = -dist;

	r_intersection[0] = begin[0] + segment[0] * dist;
	r_intersection[1] = begin[1] + segment[1] * dist;
	r_intersection[2] = begin[2] + segment[2] * dist;

	*distance = dist;

	return true;
}

void Math_Model(vec3 position, vec3 size, float rotation, mat4 dest)
{
	mat4 m;
	glm_mat4_identity(m);

	vec3 pos;
	pos[0] = position[0];
	pos[1] = position[1];
	pos[2] = position[2];
	glm_translate(m, pos);


	if (rotation != 0)
	{
		pos[0] = size[0] * 0.5f;
		pos[1] = size[1] * 0.5f;
		pos[2] = size[2] * 0.5f;

		glm_translate(m, pos);

		vec3 axis;
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;

		glm_rotate(m, glm_rad(rotation), axis);

		pos[0] = size[0] * -0.5f;
		pos[1] = size[1] * -0.5f;
		pos[2] = size[2] * -0.5f;

		glm_translate(m, pos);
	}

	glm_scale(m, size);

	glm_mat4_copy(m, dest);
}

void Math_Model2D(vec2 position, vec2 size, float rotation, mat4 dest)
{
	vec3 pos;
	pos[0] = position[0];
	pos[1] = position[1];
	pos[2] = 0.0f;

	vec3 scale;
	scale[0] = size[0];
	scale[1] = size[1];
	scale[2] = 0.0f;

	Math_Model(pos, scale, rotation, dest);

}
#define SHADOW_CASCADE_COUNT 5


void CascadeShadow_genMatrix(vec3 p_lightDir, float nearPlane, float farPlane, mat4 view, mat4 proj, mat4 dest[5], float cascades[5])
{
	mat4 viewProj;
	mat4 invViewProj;
	glm_mat4_mul(proj, view, viewProj);
	glm_mat4_inv(viewProj, invViewProj);

	float cascadeSplitLambda = 0.95;

	float cascadeSplits[SHADOW_CASCADE_COUNT];

	float nearClip, farClip;
	
	nearClip = nearPlane;
	farClip = farPlane;
	
	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	float lastSplitDistance = 0;
	for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
	{
		float p = (i + 1) / (float)SHADOW_CASCADE_COUNT;
		float log = minZ * powf(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
		
		float splitDist = cascadeSplits[i];
		vec4 frustrumCorners[8];
		glm_frustum_corners(invViewProj, frustrumCorners);

		for (int j = 0; j < 4; j++)
		{
			vec4 dist;
			glm_vec4_sub(frustrumCorners[j + 4], frustrumCorners[j], dist);

			vec4 splitedDist, lastSplitedDist;
			glm_vec4_scale(dist, splitDist, splitedDist);
			glm_vec4_scale(dist, lastSplitDistance, lastSplitedDist);

			glm_vec4_add(frustrumCorners[j], splitedDist, frustrumCorners[j + 4]);
			glm_vec4_add(frustrumCorners[j], lastSplitedDist, frustrumCorners[j]);
		}

		vec4 frustrumCenter;
		glm_frustum_center(frustrumCorners, frustrumCenter);

		float radius = 0;
		for (int j = 0; j < 8; j++)
		{
			vec4 dist;
			glm_vec4_sub(frustrumCorners[j], frustrumCenter, dist);
			float distance = glm_vec4_norm(dist);
			radius = max(radius, distance);
		}
		radius = ceil(radius * 16.0) / 16.0;

		vec3 maxExtents, minExtents;
		maxExtents[0] = radius;
		maxExtents[1] = radius;
		maxExtents[2] = radius;

		glm_vec3_scale(maxExtents, -1, minExtents);

		vec3 lightDir;
		glm_vec3_scale(p_lightDir, -1, lightDir);
		glm_normalize(lightDir);
		glm_vec3_scale(lightDir, -minExtents[2], lightDir);


		vec3 eye, up;
		glm_vec3_sub(frustrumCenter, lightDir, eye);
		up[0] = 0;
		up[1] = 1;
		up[2] = 0;

		mat4 lightViewMatrix, lightOrthoMatrix;
		glm_lookat(eye, frustrumCenter, up, lightViewMatrix);
		glm_ortho(minExtents[0], maxExtents[0], minExtents[1], maxExtents[1], 0.0, maxExtents[2] - minExtents[2], lightOrthoMatrix);

		float finalSplitDistance = (nearClip + splitDist * clipRange) * -1.0;
		mat4 finalProjMatrix;
		glm_mat4_mul(lightOrthoMatrix, lightViewMatrix, finalProjMatrix);
		glm_mat4_copy(finalProjMatrix, dest[i]);
		cascades[i] = finalSplitDistance;

		lastSplitDistance = cascadeSplits[i];
	}
}
