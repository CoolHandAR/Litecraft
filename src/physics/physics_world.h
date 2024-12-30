#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H
#pragma once

#include "p_physics_defs.h"
#include "utility/forward_list.h"
#include "lc/lc_defs.h"

typedef struct
{
	FL_Head* static_bodies;
	FL_Head* kinematic_bodies;
	float gravity_scale;
} PhysicsWorld;


void PhysicsWorld_Step(PhysicsWorld* world, float delta);
PhysicsWorld* PhysicsWorld_Create(float p_gravityScale);
void PhysicsWorld_Destruct(PhysicsWorld* world);

Static_Body* PhysicsWorld_AddStaticBody(PhysicsWorld* const world, AABB* const p_initBox, LC_Entity* ent_handle);
Kinematic_Body* PhysicsWorld_AddKinematicBody(PhysicsWorld* const world, AABB* const p_initBox, LC_Entity* ent_handle);

void PhysicsWorld_RemoveStaticBody(Static_Body* s_body);
void PhysicsWorld_RemoveKinematicBody(Kinematic_Body* k_body);

#endif // !PHYSICS_WORLD_H