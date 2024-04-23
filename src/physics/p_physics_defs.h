#pragma once

#include "utility/u_math.h"
#include "lc_defs.h"

#define Phys_BLOCK_LAYER 0x008
#define Phys_ACTOR_LAYER 0x016


typedef enum Physics_Type
{
	PT__NONE,
	PT__STATIC,
	PT__KINEMATIC
} Physics_Type;

typedef struct Phys_Contact
{
	LC_Entity* handle;

} Phys_Contact;

typedef struct Static_Body
{
	AABB box;

	int mask;
	int layer;
	int aabb_tree_index;

	LC_Entity* handle;

	bool collidable;


} Static_Body;

typedef struct Kinematic_Body
{
	AABB box;
	vec3 velocity;
	vec3 direction;
	float max_speed;
	float current_speed;
	float accel;

	float jump_height;

	int mask;
	int layer;
	int aabb_tree_index;

	bool on_ground;
	bool collidable;
	bool gravity_free;

	bool force_update_on_frame;

	LC_Entity* handle;

} Kinematic_Body;