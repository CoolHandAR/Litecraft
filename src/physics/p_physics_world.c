#include "p_physics_world.h"

#include "lc/lc_world.h"
#include <stdio.h>

#include "render/r_renderer.h"

#include "lc/lc_block_defs.h"
#include "core/core_common.h"


P_PhysWorld* phys_world = NULL;
float g_delta;
#define	OVERCLIP 1.001f
static void Calc_KinematicVel(vec3 wish_dir, vec3 current_vel, float wish_speed, float accel, float delta, vec3 out)
{
	float add_speed = 0;
	float accel_speed = 0;
	float current_speed = 0;

	current_speed = glm_vec3_dot(current_vel, wish_dir);
	add_speed = wish_speed - current_speed;

	if (add_speed <= 0)
	{
		glm_vec3_copy(current_vel, out);
		return;
	}
	accel_speed = accel * delta * wish_speed;
	if (accel_speed > add_speed)
	{
		accel_speed = add_speed;
	}

	for (int i = 0; i < 3; i++)
	{
		out[i] += accel_speed * wish_dir[i];
	}
}

static void clipVelocity(vec3 in, vec3 normal, float overbounce, vec3 dest)
{
	float backoff = 0;
	float change = 0;

	backoff = glm_dot(in, normal);

	if (backoff < 0)
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for (int i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		dest[i] = in[i] - change;
	}
}


static void Apply_Friction(Kinematic_Body* const k_body)
{
	if (!k_body->on_ground)
	{
		return;
	}

	float current_speed = glm_vec3_norm(k_body->velocity);

	//no speed?
	if (current_speed < CMP_EPSILON)
	{
		if (k_body->on_ground && k_body->water_level <= 0)
		{
			k_body->velocity[0] = 0;
			k_body->velocity[2] = 0;
		}
		return;
	}

	float drop = 0;

	//ground friction
	if (k_body->on_ground)
	{
		if (k_body->water_level <= 0)
		{
			float control = current_speed < k_body->config.stop_speed ? k_body->config.stop_speed : current_speed;
			drop += control * k_body->config.ground_friction;
		}
	}
	//air friction
	else
	{
		if(k_body->water_level <= 0)
			drop += k_body->config.air_friction;
	}

	//water friction
	if (k_body->water_level >= 1)
	{
		drop += k_body->config.water_friction;
	}
	
	drop *= g_delta;

	float new_speed = current_speed - drop;

	if (new_speed < 0) new_speed = 0;

	new_speed /= max(current_speed, 0.0001);

	glm_vec3_scale(k_body->velocity, new_speed, k_body->velocity);
}

static bool Process_Jump(Kinematic_Body* const k_body)
{
	if (k_body->direction[1] <= 0 || !k_body->on_ground)
	{
		return false;
	}
	k_body->on_ground = false;
	k_body->velocity[1] = k_body->config.jump_height;
	k_body->_state_flags |= PSF__SkipGroundCheck;

	return true;
}

static void Water_Move(Kinematic_Body* const k_body)
{
	Apply_Friction(k_body);

	float wish_speed = glm_vec3_norm(k_body->direction);

	if (k_body->on_ground && k_body->direction[1] <= 0)
	{
		//are we ducking
		if (k_body->flags & PF__Ducking)
		{
			wish_speed *= k_body->config.ducking_scale;
		}

		Calc_KinematicVel(k_body->direction, k_body->velocity, wish_speed, k_body->config.ground_accel, g_delta, k_body->velocity);
		clipVelocity(k_body->velocity, k_body->ground_contact.normal, OVERCLIP, k_body->velocity);
	}
	//Swimming
	else
	{	
		Calc_KinematicVel(k_body->direction, k_body->velocity, wish_speed, k_body->config.water_accel, g_delta, k_body->velocity);
	}
	

	float water_pull = 1;

	//apply gravity and pull
	//k_body->velocity[1] -= water_pull * g_delta;

	
}

static void Air_Move(Kinematic_Body* const k_body)
{
	Apply_Friction(k_body);

	k_body->direction[1] = 0;

	float wish_speed = glm_vec3_norm(k_body->direction);

	Calc_KinematicVel(k_body->direction, k_body->velocity, wish_speed, k_body->config.air_accel, g_delta, k_body->velocity);

	//apply gravity
	k_body->velocity[1] -= phys_world->gravity_scale * g_delta;
}

static void Ground_Move(Kinematic_Body* const k_body)
{
	Apply_Friction(k_body);

	k_body->direction[1] = 0;
	clipVelocity(k_body->direction, k_body->ground_contact.normal, OVERCLIP, k_body->direction);
	
	float wish_speed = glm_vec3_norm(k_body->direction);

	//are we ducking
	if (k_body->flags & PF__Ducking)
	{
		wish_speed *= k_body->config.ducking_scale;
	}

	Calc_KinematicVel(k_body->direction, k_body->velocity, wish_speed, k_body->config.ground_accel, g_delta, k_body->velocity);

	clipVelocity(k_body->velocity, k_body->ground_contact.normal, OVERCLIP, k_body->velocity);
}


static void FreeFly_Move(Kinematic_Body* const k_body)
{
	if (k_body->on_ground)
	{
		//make sure we don't try to force ourselves into the ground
		if (k_body->direction[1] < 0)
		{
			clipVelocity(k_body->direction, k_body->ground_contact.normal, OVERCLIP, k_body->direction);
		}
	}
	
	Apply_Friction(k_body);

	for (int i = 0; i < 3; i++)
	{
		k_body->velocity[i] = k_body->direction[i] * k_body->config.flying_speed * g_delta;
	}
	
}

static void Calc_Move(Kinematic_Body* const k_body)
{
	//make sure direction is normalized
	glm_normalize(k_body->direction);

	//not affected by gravity?
	if (!(k_body->flags & PF__AffectedByGravity))
	{
		FreeFly_Move(k_body);
		return;
	}
	//in swimming height
	if (k_body->water_level >= 2 && k_body->in_water)
	{
		Water_Move(k_body);
		return;
	}

	//are we jumping?
	if (Process_Jump(k_body) || !k_body->on_ground)
	{
		Air_Move(k_body);
		return;
	}

	if (k_body->on_ground)
	{
		Ground_Move(k_body);
		return;
	}
}

#define MAX_BUMPS 24
static void Solve_KinematicBodies()
{
	float delta = g_delta;
	FL_Node* node = NULL;
	for(node = phys_world->kinematic_bodies->next; node; node = node->next)
	{
		Kinematic_Body* k_body = node->value;
		
		if (k_body->in_water)
		{
			k_body->water_level = LC_World_calcWaterLevelFromPoint(k_body->box.position[0], k_body->box.position[1], k_body->box.position[2]);
		}
		else
		{
			k_body->water_level = 0;
		}

		//reset all of our state falgs
		k_body->_state_flags = 0;

		//calculate the velocity and water level
		Calc_Move(k_body);

		//reset ground contact count
		k_body->contact_count = 0;
		
		//reset contacts
		memset(&k_body->ground_contact, 0, sizeof(Block_Contact));
		memset(k_body->block_contacts, 0, sizeof(k_body->block_contacts));
	
		//reset on ground state
		k_body->on_ground = false;

		//reset in water state
		k_body->in_water = false;

		//Expand box by the desired velocity
		AABB expanded_box;
		expanded_box.width = (k_body->box.width) + fabsf(k_body->velocity[0]);
		expanded_box.height = (k_body->box.height) + fabsf(k_body->velocity[1]);
		expanded_box.length = (k_body->box.length) + fabsf(k_body->velocity[2]);

		expanded_box.position[0] = k_body->box.position[0] + (k_body->velocity[0]);
		expanded_box.position[1] = k_body->box.position[1] + (k_body->velocity[1]);
		expanded_box.position[2] = k_body->box.position[2] + (k_body->velocity[2]);

		int min_x = roundf(expanded_box.position[0]);
		int min_y = roundf(expanded_box.position[1] - 3); //offset a little so we can do "is on ground" check
		int min_z = roundf(expanded_box.position[2]);

		int max_x = roundf(expanded_box.position[0] + expanded_box.width);
		//offset a little so we can do a up check if we are ducking
		int max_y = (k_body->flags & PF__Ducking) ? roundf(expanded_box.position[1] + expanded_box.height + 3) : roundf(expanded_box.position[1] + expanded_box.height);
		int max_z = roundf(expanded_box.position[2] + expanded_box.length);

		vec3 max_normal;
		float max_dist = FLT_MAX;

		vec3 clip_velocity;
		glm_vec3_copy(k_body->velocity, clip_velocity);

		AABB block_box;
		block_box.width = 1;
		block_box.height = 1;
		block_box.length = 1;

		vec3 bumps[MAX_BUMPS];
		uint8_t bump_types[MAX_BUMPS];
		int bump_count = 0;

		bool force_ducking = false;

	
		if (k_body->flags & PF__Collidable)
		{
			for (int x = min_x; x <= max_x; x++)
			{
				for (int y = min_y; y <= max_y; y++)
				{
					for (int z = min_z; z <= max_z; z++)
					{
						LC_Block* block = LC_World_GetBlock(x, y, z, NULL, NULL);

						if (block == NULL)
							continue;

						if (block->type == LC_BT__NONE)
							continue;

						if (!LC_isBlockCollidable(block->type))
						{
							continue;
						}

						block_box.position[0] = x - 0.5;
						block_box.position[1] = y - 0.5;
						block_box.position[2] = z - 0.5;

						//special case for water
						if (block->type == LC_BT__WATER)
						{
							if (AABB_intersectsOther(&k_body->box, &block_box))
							{
								k_body->in_water = true;
							}
							continue;
						}

						//Mink diff
						AABB mink = AABB_getMinkDiff(&block_box, &k_body->box);

						//do a is on ground check
						if (!k_body->on_ground && !(k_body->_state_flags & PSF__SkipGroundCheck))
						{
							vec3 down_vel;
							down_vel[0] = 0;
							down_vel[1] = -1;
							down_vel[2] = 0;

							vec3 normal;

							float dist = AABB_getFirstRayIntersection(mink, down_vel, NULL, normal);

							if (dist < 1.0)
							{
								k_body->on_ground = true;
								k_body->ground_contact.block_type = block->type;
								k_body->ground_contact.position[0] = x;
								k_body->ground_contact.position[1] = y;
								k_body->ground_contact.position[2] = z;

								glm_vec3_copy(normal, k_body->ground_contact.normal);


								//clip velocity against the ground plane
								if (k_body->flags & PF__AffectedByGravity)
								{
									clipVelocity(k_body->velocity, normal, 1.001f, k_body->velocity);
								}

							}
						}
						//if we are ducking do a up check so that we wont stand up into a block
						if (k_body->flags & PF__Ducking)
						{
							vec3 up_vel;
							up_vel[0] = 0;
							up_vel[1] = 1;
							up_vel[2] = 0;

							vec3 normal;

							float dist = AABB_getFirstRayIntersection(mink, up_vel, NULL, normal);

							if (dist < 1.0)
							{
								force_ducking = true;
							}
						}

						//Are we currently inside the other box?
						if (mink.position[0] <= 0 && mink.position[0] + mink.width >= 0 && mink.position[1] <= 0 && mink.position[1] + mink.height >= 0 &&
							mink.position[2] <= 0 && mink.position[2] + mink.length >= 0)
						{
							//we are stuck!!

							vec3 peneration_depth;

							AABB_getPenerationDepth(&mink, peneration_depth);

							k_body->_state_flags |= PSF__Stuck;

							printf("stuck \n");

							glm_vec3_normalize(peneration_depth);
							glm_vec3_sign(peneration_depth, peneration_depth);
							glm_vec3_add(k_body->box.position, peneration_depth, k_body->box.position);

							//zero out the velocity
							glm_vec3_zero(k_body->velocity);

							//do another stuck check
							AABB mink_possible_stuck = AABB_getMinkDiff(&block_box, &k_body->box);

							if (mink_possible_stuck.position[0] <= 0 && mink_possible_stuck.position[0] + mink_possible_stuck.width >= 0 && mink_possible_stuck.position[1] <= 0 && 
								mink_possible_stuck.position[1] + mink_possible_stuck.height >= 0 && mink_possible_stuck.position[2] <= 0 && 
								mink_possible_stuck.position[2] + mink_possible_stuck.length >= 0)
								{
								//we are stuck again
								//last resort, set the position to last valid location
								glm_vec3_copy(k_body->_prev_valid_pos, k_body->box.position);
									
								}
						}
						//otherwise check if we will collide
						vec3 normal;
						vec3 intersection;
						float dist = AABB_getFirstRayIntersection(mink, k_body->velocity, intersection, normal);

						//we will collide
						if (dist < 1.0)
						{
							if (dist < max_dist)
							{
								max_dist = dist;
								glm_vec3_copy(normal, max_normal);
							}
							//collect other possible collisions
							if (bump_count < MAX_BUMPS)
							{
								bumps[bump_count][0] = block_box.position[0];
								bumps[bump_count][1] = block_box.position[1];
								bumps[bump_count][2] = block_box.position[2];

								bump_types[bump_count] = block->type;

								bump_count++;
							}


						}

					}
				}
			}
		}

		//we will collide
		if (max_dist < 1.0)
		{
			int num_collided = 0;

			float over_bounce = 1.001f;

			float time_left = delta;

			//clip the desired velocity to the nearest collision
			clipVelocity(k_body->velocity, max_normal, over_bounce, k_body->velocity);
			
			//clip the velocity with other possible bumps
			for (int i = 0; i < bump_count; i++)
			{
				block_box.position[0] = bumps[i][0];
				block_box.position[1] = bumps[i][1];
				block_box.position[2] = bumps[i][2];

				//Mink diff
				AABB mink = AABB_getMinkDiff(&block_box, &k_body->box);

				vec3 normal;

				float dist = AABB_getFirstRayIntersection(mink, k_body->velocity, NULL, normal);

				// we will not collide so skip this
				if (dist == 1)
				{
					continue;
				}

				//we will collide
				if (dist < 1.0)
				{	
					over_bounce -= delta;

					if (glm_dot(max_normal, normal) > 0.99)
					{
						glm_vec3_add(normal, k_body->velocity, k_body->velocity);
					}

					clipVelocity(k_body->velocity, normal, OVERCLIP, k_body->velocity);

					//add to contacts
					if (k_body->contact_count < MAX_BLOCK_CONTACTS)
					{
						glm_vec3_copy(block_box.position, k_body->block_contacts[k_body->contact_count].position);
						k_body->block_contacts[k_body->contact_count].block_type = bump_types[i];
						k_body->contact_count++;
					}
				}
			}
			glm_vec3_copy(k_body->box.position, k_body->_prev_valid_pos);
			glm_vec3_add(k_body->box.position, k_body->velocity, k_body->box.position);

		}
		//No collisions found?
		//move by the desired velocity
		else
		{
			glm_vec3_copy(k_body->box.position, k_body->_prev_valid_pos);
			glm_vec3_add(k_body->box.position, k_body->velocity, k_body->box.position);
		}
		
		if (!force_ducking)
		{
			k_body->flags = k_body->flags & ~PF__Ducking;
		}
		//printf("%i \n", k_body->water_level);
		//reset direction vector
		memset(k_body->direction, 0, sizeof(vec3));
	}
}


void PhysWorld_Step(P_PhysWorld* world, float delta)
{
	phys_world = world;
	g_delta = delta;

	//Thread_AssignTask(Solve_KinematicBodies, TASK_FLAG__FIRE_AND_FORGET);
	Solve_KinematicBodies();
}

P_PhysWorld* PhysWorld_Create(float p_gravityScale)
{
	P_PhysWorld* world = malloc(sizeof(P_PhysWorld));

	if (!world)
	{
		printf("malloc failed\n");
		return NULL;
	}

	memset(world, 0, sizeof(P_PhysWorld));

	world->gravity_scale = p_gravityScale;

	world->tree = AABB_Tree_Create();
	
	world->static_bodies = FL_INIT(Static_Body);
	world->kinematic_bodies = FL_INIT(Kinematic_Body);

	phys_world = world;

	return world;
}

void PhysWorld_Destruct(P_PhysWorld* world)
{
	AABB_Tree_Destruct(&world->tree);
	FL_Destruct(world->static_bodies);
	FL_Destruct(world->kinematic_bodies);

	free(world);

	phys_world = NULL;

	world = NULL;
}

Static_Body* PhysWorld_AddStaticBody(P_PhysWorld* const world, AABB* const p_initBox, LC_Entity* ent_handle)
{
	int tree_index = AABB_Tree_Insert(&world->tree, p_initBox, ent_handle);

	Static_Body* sb_node = FL_emplaceFront(world->static_bodies)->value;
	
	sb_node->box = *p_initBox;
	sb_node->handle = ent_handle;
	sb_node->aabb_tree_index = tree_index;

	return sb_node;
}

Kinematic_Body* PhysWorld_AddKinematicBody(P_PhysWorld* const world, AABB* const p_initBox, LC_Entity* ent_handle)
{
	int tree_index = AABB_Tree_Insert(&world->tree, p_initBox, ent_handle);

	Kinematic_Body* kb_node = FL_emplaceFront(world->kinematic_bodies)->value;
	
	kb_node->box = *p_initBox;
	kb_node->handle = ent_handle;
	kb_node->aabb_tree_index = tree_index;

	return kb_node;
}

void PhysWorld_RemoveStaticBody(Static_Body* s_body)
{
	FL_Node* node = phys_world->static_bodies->next;

	while (node != NULL)
	{
		Static_Body* cast = node->value;

		if (cast == s_body)
		{
			AABB_Tree_Remove(&phys_world->tree, s_body->aabb_tree_index);
			FL_remove(phys_world->static_bodies, node);
			return;
		}

		node = node->next;
	}
}

void PhysWorld_RemoveKinematicBody(Kinematic_Body* k_body)
{
	FL_Node* node = phys_world->kinematic_bodies->next;

	while (node != NULL)
	{
		Kinematic_Body* cast = node->value;

		if (cast == k_body)
		{
			AABB_Tree_Remove(&phys_world->tree, k_body->aabb_tree_index);
			FL_remove(phys_world->kinematic_bodies, node);
			return;
		}

		node = node->next;
	}
}


