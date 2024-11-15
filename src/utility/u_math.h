#pragma once

#include <cglm/cglm.h>
#include <math.h>
#include "render/r_camera.h"


#define Math_PI 3.1415926535897932384626433833
#define CMP_EPSILON 0.00001

static unsigned long long Math_rng_seed = 1;

void Math_Proj_ReverseZInfinite(float fovy, float aspect, float nearZ, float farZ, mat4 dest);

void Math_GLM_FrustrumPlanes_ReverseZ(mat4 m, vec4 dest[6]);

static void Math_srand(unsigned long long p_seed)
{
	Math_rng_seed = p_seed;
}

static inline uint32_t Math_rand()
{
	Math_rng_seed = (214013 * Math_rng_seed + 2531011);
	return (Math_rng_seed >> 32) & RAND_MAX;
}

static inline float Math_randf()
{
	return (float)Math_rand() / (float)RAND_MAX;
}

static inline double Math_randd()
{
	return (double)Math_rand() / (double)RAND_MAX;
}

inline float Math_Clamp(float v, float min_v, float max_v)
{
	return v < min_v ? min_v : (v > max_v ? max_v : v);
}
inline double Math_Clampd(double v, double min_v, double max_v)
{
	return v < min_v ? min_v : (v > max_v ? max_v : v);
}

inline bool Math_IsZeroApprox(float s)
{
	return fabs(s) < (float)CMP_EPSILON;
}
inline int Math_signf(float x)
{
	return (x < 0) ? -1 : 1;
}
inline float Math_sign_float(float x)
{
	return x > 0 ? +1.0f : (x < 0 ? -1.0f : 0.0f);
}

inline float Math_move_towardf(float from, float to, float delta)
{
	return fabsf(to - from) <= delta ? to : from + Math_sign_float(to - from) * delta;
}

long double Math_fract(long double x);

inline long double Math_fract2(long double x)
{
	return x - floor(x);
}

inline int Math_step(float edge, float x)
{
	return x < edge ? 0 : 1;
}

inline void Math_vec3_dir_to(vec3 from, vec3 to, vec3 dest)
{
	vec3 v;
	v[0] = to[0] - from[0];
	v[1] = to[1] - from[1];
	v[2] = to[2] - from[2];

	glm_vec3_normalize(v);
	glm_vec3_copy(v, dest);
};

inline void Math_vec3_slerp(vec3 from, vec3 to, float weight, vec3 dest)
{
	
}

inline void Math_mat4_mulv2(mat4 m, vec2 v, float third, float last, vec2 dest)
{
	vec4 res;
	res[0] = v[0];
	res[1] = v[1];
	res[2] = third;
	res[3] = last;
	glm_mat4_mulv(m, res, res);
	glm_vec2(res, dest);
}

inline void Math_vec3_scaleadd(vec3 va, vec3 vb, float scale, vec3 dest)
{
	dest[0] = va[0] + scale * vb[0];
	dest[1] = va[1] + scale * vb[1];
	dest[2] = va[2] + scale * vb[2];
}

inline bool Math_vec3_is_zero(vec3 v)
{
	for (int i = 0; i < 3; i++)
	{
		if (v[i] != 0)
		{
			return false;
		}
	}
	return true;
}

void Math_calcLightSpaceMatrix(const float p_cameraFov, const float p_screenWidth, const float p_screenHeight, const float p_nearPlane, const float p_farPlane, const float p_zMult,
	vec3 p_lightDir, mat4 p_cameraView, mat4 dest);

void Math_getLightSpacesMatrixesForFrustrum(const R_Camera* const p_camera, const float p_screenWidth, const float p_screenHeight, 
	const float p_zMult, vec4 p_shadowCascadeLevels, vec3 p_lightDir, mat4 dest[5], vec4 frustrum_planes[5][6]);




typedef struct M_Rect2Df
{
	float x, y;
	float width, height;
} M_Rect2Df;

typedef struct M_Rect2Di
{
	int x, y;
	int width, height;
} M_Rect2Di;


typedef struct AABB
{
	vec3 position;
	float width, height, length;
} AABB;

bool AABB_intersectsRay(AABB* const aabb, vec3 p_from, vec3 p_dir, vec3 r_clip, vec3 r_normal, float* r_far, float* r_near);
bool AABB_intersectsOther(AABB* aabb, AABB* other);
void AABB_MergeWith(AABB* aabb, AABB* other, AABB* dest);
AABB AABB_getIntersectionWithOther(AABB* aabb, AABB* other);
float AABB_GetSurfaceArea(AABB* aabb);
AABB AABB_getMinkDiff(AABB* first, AABB* other);
AABB AABB_ExpandedTo(AABB* first, vec3 to);
float AABB_getFirstRayIntersection(AABB const aabb, vec3 p_dir, vec3 r_intersection, vec3 r_normal);
float AABB_checkPlaneIntersection(AABB const p_aabb, vec3 p_dir, vec3 p_normal);
void AABB_CLOSESTBOUDNS(AABB* first, vec3 point, vec3 dest);
void AABB_getPenerationDepth(AABB* first, vec3 dest);
bool AABB_MinkIntersectionCheck(AABB* mink);
bool AABB_hasPoint(AABB* aabb, vec3 point);
void AABB_getCenter(AABB* aabb, vec3 dest);

typedef struct Plane
{
	vec3 normal;
	float distance;

} Plane;

bool Plane_rayIntersection(int pn_X, int pn_Y, int pn_Z, float plane_distance, float ray_from, vec3 p_dir, vec3 r_intersection, float* r_dist);
bool Plane_IntersectsRay(Plane* const p_plane, vec3 from, vec3 dir, vec3 r_intersection, float* distr);
bool Plane_IntersectsSegment(Plane* const p_plane, vec3 begin, vec3 end, vec3 r_intersection, float* distance);

void Math_Model(vec3 position, vec3 size, float rotation, mat4 dest);
void Math_Model2D(vec2 position, vec2 size, float rotation, mat4 dest);


typedef struct
{
	mat4 projView;
	float splitDist;
} CascadeShadow;

void CascadeShadow_genMatrix(vec3 p_lightDir, float nearPlane, float farPlane, mat4 view, mat4 proj, mat4 dest[5], float cascades[5]);