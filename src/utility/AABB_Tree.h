#pragma once

#include <cglm/cglm.h>
#include "u_object_pool.h"

typedef struct
{
	Object_Pool _node_pool;
	int _root_index;
} AABB_Tree;

AABB_Tree AABB_Tree_Create();
void AABBTree_Insert(vec3 box[2], const void* p_customData);