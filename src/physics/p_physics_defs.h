#pragma once

#include "utility/u_math.h"
#include "lc_defs.h"
#include "lc_block_defs.h"

#define MAX_BLOCK_CONTACTS 4


typedef enum Physics_Type
{
	PT__NONE,
	PT__STATIC,
	PT__KINEMATIC
} Physics_Type;

typedef struct Block_Contact
{
	vec3 position;
	LC_BlockType block_type;
} Block_Contact;

typedef struct Static_Body
{
	AABB box;

	int mask;
	int layer;
	int aabb_tree_index;

	LC_Entity* handle;

	bool collidable;


} Static_Body;

typedef enum Phys_Flags
{
	PF__AffectedByGravity = 1 << 0,
	PF__Collidable = 1 << 1,
	PF__Stuck = 1 << 2,
	PF__ForceUpdateOnFrame = 1 << 3,
	PF__Ducking = 1 << 4
} Phys_Flags;

typedef struct Kinematic_Body
{
	AABB box;
	vec3 velocity;
	vec3 raw_velocity;
	vec3 direction;
	vec3 view_dir;
	vec3 prev_pos;
	float max_speed;
	float current_speed;
	float max_ducking_speed;
	float accel;
	float air_accel;

	float current_y_speed;
	float jump_height;

	int mask;
	int layer;
	int aabb_tree_index;

	int flags;

	bool on_ground;
	bool force_update_on_frame;

	LC_Entity* handle;

	Block_Contact block_contacts[MAX_BLOCK_CONTACTS];
	Block_Contact ground_contact;

} Kinematic_Body;