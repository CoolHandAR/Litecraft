#ifndef UTILITY_MATH
#define UTILITY_MATH
#pragma once

#include <cglm/cglm.h>
#include <math.h>
#include "render/r_camera.h"


#define Math_PI 3.1415926535897932384626433833
#define CMP_EPSILON 0.00001

static unsigned long long Math_rng_seed = 1;

static void Math_srand(unsigned long long p_seed)
{
	Math_rng_seed = p_seed;
}

static inline uint32_t Math_rand()
{
	Math_rng_seed = (214013 * Math_rng_seed + 2531011);
	return (Math_rng_seed >> 32) & RAND_MAX;
}

static inline bool Math_AABB_PlanesIntersect(vec3 box[2], vec4* planes, int numPlanes)
{
	float* p, dp;
	int    i;

	for (i = 0; i < numPlanes; i++) 
	{
		p = planes[i];
		dp = p[0] * box[p[0] > 0.0f][0]
			+ p[1] * box[p[1] > 0.0f][1]
			+ p[2] * box[p[2] > 0.0f][2];

		if (dp < -p[3])
			return false;
	}

	return true;
}
static inline bool Math_AABB_PlanesFullyContain(vec3 box[2], vec4* planes, int numPlanes)
{
	float* p, dp;
	int    i;

	for (i = 0; i < numPlanes; i++) 
	{
		p = planes[i];
		dp = p[0] * box[p[0] < 0.0f][0]
			+ p[1] * box[p[1] < 0.0f][1]
			+ p[2] * box[p[2] < 0.0f][2];

		if (dp < -p[3])
			return false;
	}

	return true;
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

#endif // !UTILITY_MATH
