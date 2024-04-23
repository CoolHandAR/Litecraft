#pragma once

#include "p_aabb_tree.h"
#include "p_physics_defs.h"
#include "utility/forward_list.h"
#include "lc_defs.h"
#include "lc/lc_chunk.h"

typedef struct P_PhysWorld
{
	AABB_Tree tree;
	FL_Head* static_bodies;
	FL_Head* kinematic_bodies;
	float gravity_scale;

} P_PhysWorld;


void PhysWorld_Step(P_PhysWorld* world, float delta);
P_PhysWorld* PhysWorld_Create(float p_gravityScale);
void PhysWorld_Destruct(P_PhysWorld* world);

Static_Body* PhysWorld_AddStaticBody(P_PhysWorld* const world, AABB* const p_initBox, LC_Entity* ent_handle);
Kinematic_Body* PhysWorld_AddKinematicBody(P_PhysWorld* const world, AABB* const p_initBox, LC_Entity* ent_handle);

void PhysWorld_RemoveStaticBody(Static_Body* s_body);
void PhysWorld_RemoveKinematicBody(Kinematic_Body* k_body);

AABB_RayQueryResult PhysWorld_AABBRayQueryNearest(vec3 from, vec3 dir);
void PhysWorld_AABBRayQueryNearestPlane(vec3 from, vec3 dir, vec3 normal, float distance);

LC_Block* PhysWorld_RayNearestBlock(vec3 from, vec3 dir, int max_dist, vec3 pos_dist);