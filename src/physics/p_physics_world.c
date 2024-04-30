#include "p_physics_world.h"

#include "lc/lc_chunk.h"

#include <stdio.h>
#include "lc_world.h"

#include "render/r_renderer.h"

#include "lc/lc_block_defs.h"

P_PhysWorld* phys_world = NULL;

static void Calc_KinematicVel(vec3 wish_dir, vec3 current_vel, float wish_speed, float accel, float delta, vec3 out)
{
	float add_speed = 0;
	float accel_speed = 0;
	float current_speed = 0;

	current_speed = glm_vec3_dot(current_vel, wish_dir);
	add_speed = wish_speed - current_speed;

	if (add_speed <= 0)
	{
		return 0;
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

#define MAX_BUMPS 4
static void Solve_KinematicBodies(P_PhysWorld* world, float delta)
{
	FL_Node* node = world->kinematic_bodies->next;
	while (node != NULL)
	{
		Kinematic_Body* k_body = node->value;
		
		float accel = (k_body->on_ground) ? k_body->accel : k_body->air_accel;

		float adjusted_h_speed = 400;

		vec3 desired_vel;
		//better to memset to avoid crazy values 
		memset(desired_vel, 0, sizeof(vec3));

		//desired_vel[0] = Math_move_towardf(k_body->velocity[0], k_body->max_speed * k_body->direction[0], 2222) * delta;
		//desired_vel[2] = Math_move_towardf(k_body->velocity[2], k_body->max_speed * k_body->direction[2], 2222) * delta;
		int move_h = (k_body->direction[0] != 0 || k_body->direction[2] != 0) ? 1 : 0;

		//k_body->current_speed = Math_move_towardf(k_body->current_speed, move_h * k_body->max_speed, 200) * delta;
		
	
		if (k_body->direction[0] != 0 || k_body->direction[2] != 0)
		{
			float max_speed = (k_body->flags & PF__Ducking) ? k_body->max_ducking_speed : k_body->max_speed;

			k_body->current_speed = Math_move_towardf(k_body->current_speed, max_speed, 20000);
		}
		else
		{
			k_body->current_speed = Math_move_towardf(k_body->current_speed, 0, 200);
		}
		//k_body->raw_velocity[0] = k_body->direction[0] * k_body->current_speed;
		//k_body->raw_velocity[2] = k_body->direction[2] * k_body->current_speed;
		k_body->raw_velocity[0] = Math_move_towardf(k_body->raw_velocity[0], k_body->current_speed * k_body->direction[0], 2);
		k_body->raw_velocity[2] = Math_move_towardf(k_body->raw_velocity[2], k_body->current_speed * k_body->direction[2], 2);

		desired_vel[0] = k_body->raw_velocity[0] * delta;
		desired_vel[2] = k_body->raw_velocity[2] * delta;
			 
		//not affected by gravity
		if (!(k_body->flags & PF__AffectedByGravity))
		{
			desired_vel[1] = (k_body->direction[1] * adjusted_h_speed) * delta;
			

			if (k_body->on_ground)
			{
				//make sure we don't try to force ourselves into the ground
				if (desired_vel[1] < 0)
				{
					desired_vel[1] = 0;
				}
			}
		}
		//affected by gravity
		else
		{
			if (k_body->on_ground)
			{
				//are we jumping?
				if (k_body->direction[1] > 0)
				{
					desired_vel[1] = 0.2;
					k_body->on_ground = false;
				}
				else
				{
					desired_vel[1] = 0;
				}
			}
			//we are falling
			else
			{
				//k_body->velocity[1] = Math_move_towardf(k_body->velocity[1], -FLT_MAX, 111) * delta;

				//adjusted_y_speed = Math_move_towardf(k_body->current_y_speed, -100, 0.1) * delta;
				//desired_vel[1] = -(world->gravity_scale * delta);
				//desired_vel[1] = Math_move_towardf(k_body->current_y_speed, -100, 0.1) * delta;
				desired_vel[1] = Math_move_towardf(k_body->velocity[1], -0.1, 10 * delta);
			}

		}

		
		//printf("%f \n", k_body->raw_velocity[2]);

		//reset contacts
		memset(&k_body->ground_contact, 0, sizeof(Block_Contact));
		memset(k_body->block_contacts, 0, sizeof(k_body->block_contacts));

		//reset the velocity vector
		//memset(k_body->velocity, 0, sizeof(vec3));
		
		//skip if the desired velocity is zero
		if (desired_vel[0] == 0 && desired_vel[1] == 0 && desired_vel[2] == 0 && !k_body->force_update_on_frame && !(k_body->flags & PF__Ducking))
		{
			//reset direction vector
			memset(k_body->direction, 0, sizeof(vec3));
			k_body->current_speed = 0;
			k_body->current_y_speed = 0;
			k_body->flags = k_body->flags & ~PF__Ducking;
			node = node->next;
			continue;
		}
		k_body->force_update_on_frame = false;
		k_body->flags = k_body->flags & ~PF__Stuck;

		//reset on ground state
		k_body->on_ground = false;

		//Expand box by the desired velocity
		AABB expanded_box;
		expanded_box.width = (k_body->box.width) + fabsf(desired_vel[0]);
		expanded_box.height = (k_body->box.height) + fabsf(desired_vel[1]);
		expanded_box.length = (k_body->box.length) + fabsf(desired_vel[2]);

		expanded_box.position[0] = k_body->box.position[0] + (desired_vel[0]);
		expanded_box.position[1] = k_body->box.position[1] + (desired_vel[1]);
		expanded_box.position[2] = k_body->box.position[2] + (desired_vel[2]);

		int min_x = roundf(expanded_box.position[0]);
		int min_y = roundf(expanded_box.position[1] - 3); //offset a little so we can do "is on ground" check
		int min_z = roundf(expanded_box.position[2]);

		int max_x = roundf(expanded_box.position[0] + expanded_box.width);
		//offset a little so we can do a up check if we are ducking
		int max_y = (k_body->flags & PF__Ducking) ? roundf(expanded_box.position[1] + expanded_box.height + 3) : roundf(expanded_box.position[1] + expanded_box.height);
		int max_z = roundf(expanded_box.position[2] + expanded_box.length);

		vec3 max_normal;
		float max_dist = FLT_MAX;

		AABB block_box;
		block_box.width = 1;
		block_box.height = 1;
		block_box.length = 1;

		vec3 bumps[MAX_BUMPS];
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
						LC_Block* block = LC_World_getBlock(x, y, z, NULL, NULL);

						if (block == NULL)
							continue;

						if (block->type == LC_BT__NONE)
							continue;

						LC_Block_Collision_Data block_collision_data = LC_BLOCK_COLLISION_DATA[block->type];

						if (block_collision_data.collidable == false)
						{
							continue;
						}

						block_box.position[0] = x - 0.5;
						block_box.position[1] = y - 0.5;
						block_box.position[2] = z - 0.5;

						//Mink diff
						AABB mink = AABB_getMinkDiff(&block_box, &k_body->box);
						
						//do a is on ground check
						if (!k_body->on_ground)
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
								k_body->ground_contact.position[0] = block_box.position[0];
								k_body->ground_contact.position[1] = block_box.position[1];
								k_body->ground_contact.position[2] = block_box.position[2];

								//clip velocity against the ground plane
								if (k_body->flags & PF__AffectedByGravity)
								{
									clipVelocity(desired_vel, normal, 1.001f, desired_vel);
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

							k_body->flags |= PF__Stuck;

							printf("stuck \n");

							glm_vec3_normalize(peneration_depth);
							glm_vec3_sign(peneration_depth, peneration_depth);
							k_body->box.position[0] += peneration_depth[0];
							k_body->box.position[1] += peneration_depth[1];
							k_body->box.position[2] += peneration_depth[2];

							glm_vec3_zero(desired_vel);
				
						}
						//otherwise check if we will collide
						else
						{
							vec3 normal;

							float dist = AABB_getFirstRayIntersection(mink, desired_vel, NULL, normal);

							//we will collide
							if (dist < 1.0)
							{
								//collect other possible collisions
								if (bump_count < MAX_BUMPS && bump_count < MAX_BLOCK_CONTACTS)
								{
									bumps[bump_count][0] = block_box.position[0];
									bumps[bump_count][1] = block_box.position[1];
									bumps[bump_count][2] = block_box.position[2];

									bump_count++;

									k_body->block_contacts[bump_count].position[0] = block_box.position[0];
									k_body->block_contacts[bump_count].position[1] = block_box.position[1];
									k_body->block_contacts[bump_count].position[2] = block_box.position[2];
								}

								if (dist < max_dist)
								{
									max_dist = dist;
									glm_vec3_copy(normal, max_normal);
								}

								//another ground check won't hurt
								if (normal[1] > 0)
								{
									k_body->on_ground = true;
								}
							}
						}

					}
				}
			}
		}

		//we will collide
		if (max_dist < 1.0)
		{
			float over_bounce = 1.001f;

			//clip the desired velocity to the nearest collision
			clipVelocity(desired_vel, max_normal, over_bounce, desired_vel);
			
			//clip the velocity with other possible bumps
			for (int i = 0; i < bump_count; i++)
			{
				block_box.position[0] = bumps[i][0];
				block_box.position[1] = bumps[i][1];
				block_box.position[2] = bumps[i][2];

				//Mink diff
				AABB mink = AABB_getMinkDiff(&block_box, &k_body->box);

				vec3 normal;

				float dist = AABB_getFirstRayIntersection(mink, desired_vel, NULL, normal);

				// we will not collide so skip this
				if (dist == 1)
				{
					continue;
				}

				if (dist < 1.0)
				{
					over_bounce -= delta;

					if (glm_dot(max_normal, normal) > 0.99)
					{
						glm_vec3_add(normal, desired_vel, desired_vel);
					}

					clipVelocity(desired_vel, normal, over_bounce, desired_vel);
				}
			}
			glm_vec3_copy(desired_vel, k_body->velocity);
			glm_vec3_copy(k_body->box.position, k_body->prev_pos);
			k_body->box.position[0] += k_body->velocity[0];
			k_body->box.position[1] += k_body->velocity[1];
			k_body->box.position[2] += k_body->velocity[2];

		}
		//No collisions found?
		//move by the desired velocity
		else
		{
			k_body->velocity[0] = desired_vel[0];
			k_body->velocity[1] = desired_vel[1];
			k_body->velocity[2] = desired_vel[2];

			glm_vec3_copy(k_body->box.position, k_body->prev_pos);
			k_body->box.position[0] += desired_vel[0];
			k_body->box.position[1] += desired_vel[1];
			k_body->box.position[2] += desired_vel[2];
		}
		
		if (!force_ducking)
		{
			k_body->flags = k_body->flags & ~PF__Ducking;
		}

		//reset direction vector
		memset(k_body->direction, 0, sizeof(vec3));

		//move to the next kinematic body
		node = node->next;
	}
}


void PhysWorld_Step(P_PhysWorld* world, float delta)
{
	Solve_KinematicBodies(world, delta);
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

AABB_RayQueryResult PhysWorld_AABBRayQueryNearest(vec3 from, vec3 dir)
{
	dynamic_array* query = AABB_Tree_QueryRay(&phys_world->tree, from, dir);

	AABB_RayQueryResult nearest;
	memset(&nearest, 0, sizeof(AABB_RayQueryResult));

	if (query->elements_size == 0)
	{
		dA_Destruct(query);
		return nearest;
	}

	float max_near = FLT_MAX;
	

	for (int i = 0; i < query->elements_size; i++)
	{
		AABB_RayQueryResult* array = query->data;

		if (array[i].near < max_near)
		{
			max_near = array[i].near;
			nearest = array[i];
		}

	}
	
	dA_Destruct(query);

	return nearest;
}

void PhysWorld_AABBRayQueryNearestPlane(vec3 from, vec3 dir, vec3 normal, float distance)
{
	dynamic_array* query = AABB_Tree_QueryPlaneRay(&phys_world->tree, from, dir, normal, distance);

	if (query->elements_size == 0)
	{
		dA_Destruct(query);
		return;
	}

	

	dA_Destruct(query);
}

LC_Block* PhysWorld_RayNearestBlock(vec3 from, vec3 dir, int max_dist, vec3 pos_dist)
{
	
	vec3 s;


	

	

	return NULL;
}
