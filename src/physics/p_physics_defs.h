#pragma once

#include "utility/u_math.h"
#include "lc_defs.h"
#include "lc/lc_block_defs.h"

#define MAX_BLOCK_CONTACTS 4

typedef enum Physics_Type
{
	PT__NONE,
	PT__STATIC,
	PT__KINEMATIC
} Physics_Type;

typedef struct Block_Contact
{
	ivec3 position;
	vec3 normal;
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
	PF__Ducking = 1 << 2
} Phys_Flags;

typedef enum
{
	PSF__Stuck = 1 << 0,
	PSF__SkipGroundCheck = 1 << 1
} Phys_StateFlags;

typedef struct
{
	float ground_friction;
	float air_friction;
	float flying_friction;
	float water_friction;

	float ground_accel;
	float air_accel;
	float water_accel;

	float ducking_scale;
	float stop_speed;
	float flying_speed;

	float jump_height;
} Phys_Config;

typedef struct Kinematic_Body
{
	AABB box;

	vec3 velocity;
	vec3 direction;
	vec3 _prev_valid_pos;
	
	Phys_Config config;

	int mask;
	int layer;
	int aabb_tree_index;

	int flags;
	int _state_flags;

	int water_level;
	int contact_count;

	bool on_ground;
	bool force_update_on_frame;

	LC_Entity* handle;

	Block_Contact block_contacts[MAX_BLOCK_CONTACTS];
	Block_Contact ground_contact;

} Kinematic_Body;