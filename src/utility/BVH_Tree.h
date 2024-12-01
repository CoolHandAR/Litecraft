#pragma once

#include "utility/u_object_pool.h"
#include <cglm/cglm.h>

typedef int BVH_ID;

typedef struct
{
	Object_Pool* nodes;
	float thickness;
	int root;
} BVH_Tree;

BVH_Tree BVH_Tree_Create(float p_thickness);
void BVH_Tree_Destruct(BVH_Tree* const p_tree);
BVH_ID BVH_Tree_Insert(BVH_Tree* const p_tree, vec3 p_box[2], const void* p_data);
void* BVH_Tree_Remove(BVH_Tree* const p_tree, BVH_ID p_bvhID);
bool BVH_Tree_UpdateBounds(BVH_Tree* const p_tree, BVH_ID p_bvhID, vec3 p_newBox[2]);
void* BVH_Tree_GetData(BVH_Tree* const p_tree, BVH_ID p_bvhID);
void BVH_Tree_DrawNodes(BVH_Tree* const p_tree, vec4 p_Leafcolor, vec4 p_ParentColor, bool p_onlyLeafs);


typedef void (*BVH_RegisterFun)(const void* _data, BVH_ID _index);

int BVH_Tree_Cull_Box(BVH_Tree* const p_tree, vec3 p_box[2], int p_maxHitCount, BVH_RegisterFun p_registerFun);
int BVH_Tree_Cull_Planes(BVH_Tree* const p_tree, vec4* p_planes, int p_numPlanes, int p_maxHitCount, BVH_RegisterFun p_registerFun);
int BVH_Tree_Cull_Segment(BVH_Tree* const p_tree, vec3 p_begin, vec3 p_end, int p_maxHitCount, BVH_RegisterFun p_registerFun);
int BVH_Tree_Cull_Point(BVH_Tree* const p_tree, vec3 p_point, int p_maxHitCount, BVH_RegisterFun p_registerFun);