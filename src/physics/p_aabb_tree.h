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

typedef struct 
{
	int data;
} AABB_FrustrumCullQueryResult;

typedef struct
{
	void* data;
} AABB_FrustrumCullQueryResultTreeData;

AABB_Tree AABB_Tree_Create();
void AABB_Tree_Destruct(AABB_Tree* p_tree);
int AABB_Tree_Insert(AABB_Tree* const p_tree, AABB* const p_AABB, void* const p_data);
void AABB_Tree_Remove(AABB_Tree* const p_tree, int p_index);
void* AABB_Tree_GetIndexData(AABB_Tree* const p_tree, int p_index);
dynamic_array* AABB_Tree_QueryOverlaps(AABB_Tree* const p_tree, AABB* const p_AABB, void* const p_data);
dynamic_array* AABB_Tree_QueryRay(AABB_Tree* const p_tree, vec3 from, vec3 dir);
dynamic_array* AABB_Tree_QueryPlaneRay(AABB_Tree* const p_tree, vec3 from, vec3 dir, vec3 normal, float distance);
int AABB_Tree_IntersectsFrustrumPlanes(AABB_Tree* const p_tree, vec4 frusturm_planes[6], int p_maxCullCount, AABB_FrustrumCullQueryResult* r_query, AABB_FrustrumCullQueryResult* r_queryVisibles);
int AABB_Tree_IntersectsFrustrumBox(AABB_Tree* const p_tree, vec3 p_box[2], int p_maxCullCount, AABB_FrustrumCullQueryResult* r_query, AABB_FrustrumCullQueryResult* r_queryVisibles, AABB_FrustrumCullQueryResultTreeData* r_queryVertexIndex);