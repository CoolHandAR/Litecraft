#include "u_math.h"

#include <string.h>


long double Math_fract(long double x)
{
	
	return fminl(x - floorl(x), 0.999999940395355224609375);
	
}

void Math_calcLightSpaceMatrix(const float p_cameraFov, const float p_screenWidth, const float p_screenHeight, const float p_nearPlane, const float p_farPlane, const float p_zMult,
	vec3 p_lightDir, mat4 p_cameraView, mat4 dest)
{
	mat4 proj;
	mat4 inv_view_proj;
	mat4 light_view;
	vec4 frustrum_corners[8];
	vec4 frustrum_center;
	vec3 eye;
	vec3 up;

	glm_perspective(glm_rad(p_cameraFov), p_screenWidth / p_screenHeight, p_nearPlane, p_farPlane, proj);

	glm_mat4_mul(proj, p_cameraView, inv_view_proj);
	glm_mat4_inv(inv_view_proj, inv_view_proj);

	glm_frustum_corners(inv_view_proj, frustrum_corners);

	glm_frustum_center(frustrum_corners, frustrum_center);
	
	eye[0] = frustrum_center[0] + p_lightDir[0];
	eye[1] = frustrum_center[1] + p_lightDir[1];
	eye[2] = frustrum_center[2] + p_lightDir[2];

	memset(up, 0, sizeof(vec3));
	up[1] = 1;

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
	
	glm_ortho(min_x, max_x, min_y, max_y, min_z, max_z, light_proj);

	glm_mat4_mul(light_proj, light_view, dest);
}

void Math_getLightSpacesMatrixesForFrustrum(const R_Camera* const p_camera, const float p_screenWidth, const float p_screenHeight,
	const float p_zMult, vec4 p_shadowCascadeLevels, vec3 p_lightDir, mat4 dest[5])
{
	float cam_fov = p_camera->config.fov;
	mat4 cam_view;
	glm_mat4_copy(p_camera->data.view_matrix, cam_view);

	Math_calcLightSpaceMatrix(cam_fov, p_screenWidth, p_screenHeight, p_camera->config.zNear, p_shadowCascadeLevels[0], p_zMult, p_lightDir, cam_view, dest[0]);
	Math_calcLightSpaceMatrix(cam_fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[0], p_shadowCascadeLevels[1], p_zMult, p_lightDir, cam_view, dest[1]);
	Math_calcLightSpaceMatrix(cam_fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[1], p_shadowCascadeLevels[2], p_zMult, p_lightDir, cam_view, dest[2]);
	Math_calcLightSpaceMatrix(cam_fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[2], p_shadowCascadeLevels[3], p_zMult, p_lightDir, cam_view, dest[3]);
	Math_calcLightSpaceMatrix(cam_fov, p_screenWidth, p_screenHeight, p_shadowCascadeLevels[3], p_camera->config.zFar, p_zMult, p_lightDir, cam_view, dest[4]);
}

double Noise_SimplexNoise2D(float x, float y, struct osn_context* osn_ctx, int octaves, float persistence)
{
	double sum = 0;
	float strength = 1.0;
	float scale = 1.0;

	double noise = open_simplex_noise2(osn_ctx, x, y);

	for (int i = 0; i < octaves; i++) {
		sum += strength * noise * scale;
		scale *= 2.0;
		strength *= persistence;
	}


	return sum;
}

double Noise_SimplexNoise3Dabs(float x, float y, float z, struct osn_context* osn_ctx, int octaves, float persistence)
{
	double sum = 0;
	float strength = 1.0;
	float scale = 1.0;

	double noise = fabs(open_simplex_noise3(osn_ctx, x, y, z));
	
	for (int i = 0; i < octaves; i++) {
		sum += strength * noise * scale;
		scale *= 2.0;
		strength *= persistence;
	}

	return sum;
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
		Plane_IntersectsRay(&p, from, p_dir, NULL, &x_min);

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
		Plane_IntersectsRay(&p, from, p_dir, NULL, &x_max);

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
		Plane_IntersectsRay(&p, from, p_dir, NULL, &y_min);

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
		Plane_IntersectsRay(&p, from, p_dir, NULL, &y_max);

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
		Plane_IntersectsRay(&p, from, p_dir, NULL, &z_min);

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
		Plane_IntersectsRay(&p, from, p_dir, NULL, &z_max);

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
