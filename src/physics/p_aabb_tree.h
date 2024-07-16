#pragma once


#include "utility/u_math.h"
#include "utility/u_object_pool.h"
#include "p_physics_defs.h"

typedef struct AABB_Tree
{
	Object_Pool* nodes;
	int root;
} AABB_Tree;

typedef struct AABB_QueryResult
{
	AABB box;
	void* data;
} AABB_QueryResult;

typedef struct AABB_RayQueryResult
{
	AABB box;
	float near2;
	float far2;
	vec3 normal;
	vec3 clip;
	void* data;
} AABB_RayQueryResult;

typedef struct AABB_PlaneRayQueryResult
{
	vec3 intersection;
	float distance;
} AABB_PlaneRayQueryResult;

AABB_Tree AABB_Tree_Create();
void AABB_Tree_Destruct(AABB_Tree* p_tree);
int AABB_Tree_Insert(AABB_Tree* const p_tree, AABB* const p_AABB, void* const p_data);
void AABB_Tree_Remove(AABB_Tree* const p_tree, int p_index);
dynamic_array* AABB_Tree_QueryOverlaps(AABB_Tree* const p_tree, AABB* const p_AABB, void* const p_data);
dynamic_array* AABB_Tree_QueryRay(AABB_Tree* const p_tree, vec3 from, vec3 dir);
dynamic_array* AABB_Tree_QueryPlaneRay(AABB_Tree* const p_tree, vec3 from, vec3 dir, vec3 normal, float distance);