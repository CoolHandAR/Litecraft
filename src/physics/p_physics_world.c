#include "p_physics_world.h"

#include "lc/lc_chunk.h"

#include <stdio.h>
#include "lc_world.h"

#include "render/r_renderer.h"

#include "lc/lc_block_defs.h"

#define VELOCITY_ITERATIONS 6
#define POSITION_ITERATION 2

P_PhysWorld* phys_world = NULL;

float Calc_KinematicVel(vec3 wish_dir, vec3 current_vel, float wish_speed, float accel, float delta)
{
	float add_speed = 0;
	float accel_speed = 0;
	float current_speed = 0;

	current_speed = glm_vec3_dot(current_vel, wish_dir);
	add_speed = wish_speed - current_speed;

	if (add_speed <= 0)
	{
		return wish_speed;
	}
	accel_speed = accel * delta * wish_speed;
	if (accel_speed > add_speed)
	{
		accel_speed = add_speed;
	}

	return accel_speed;
}

static void Update_onGroundState(P_PhysWorld* world, Kinematic_Body* k_body)
{
	vec3 center;
	AABB_getCenter(&k_body->box, center);

	center[0] -= 0.5;
	center[1] -= 0.5;
	center[2] -= 0.5;

	vec3 dir;
	dir[0] = 0;
	dir[1] = -5;
	dir[2] = 0;

	ivec3 block_pos;

	LC_Block* block = LC_World_getBlockByRay(center, dir, block_pos, NULL);

	vec3 block_pos_centered;
	block_pos_centered[0] = block_pos[0] + 0.5;
	block_pos_centered[1] = block_pos[1] + 0.5;
	block_pos_centered[2] = block_pos[2] + 0.5;

	float d = glm_vec3_distance(center, block_pos_centered);

	//printf("%f \n", d);

	if (block && block->type != LC_BT__NONE && d < 1.5)
	{	
		//k_body->on_ground = true;
	
	}
	else
	{
		//k_body->on_ground = false;
	}

	//printf("%i \n", k_body->on_ground);
	
}


static void Solve_KinematicBodies(P_PhysWorld* world, float delta)
{
	FL_Node* node = world->kinematic_bodies->next;
	while (node != NULL)
	{
		Kinematic_Body* k_body = node->value;
		
		Kinematic_Body f_body = *k_body; // on stack for faster cache access

		float adjusted_speed = Math_move_towardf(k_body->current_speed, k_body->max_speed, k_body->accel);

		vec3 desired_vel;
		desired_vel[0] = (k_body->direction[0] * adjusted_speed) * delta;

		//not affected by gravity
		if (k_body->gravity_free)
		{
			desired_vel[1] = (k_body->direction[1] * adjusted_speed) * delta;
		}
		//affected by gravity
		else
		{
			if (k_body->on_ground)
			{
				//are we jumping?
				if (k_body->direction[1] > 0)
				{
					desired_vel[1] = k_body->jump_height * delta;
				}
				else
				{
					desired_vel[1] = 0;
				}
			}
			//we are falling
			else
			{
				desired_vel[1] = -(world->gravity_scale * delta);
				desired_vel[1] += k_body->velocity[1];
			}

		}
		desired_vel[2] = (k_body->direction[2] * adjusted_speed) * delta;
		
		//reset the velocity vector
		memset(k_body->velocity, 0, sizeof(vec3));
		
		//skip if the desired velocity is zero
		if (desired_vel[0] == 0 && desired_vel[1] == 0 && desired_vel[2] == 0 && !k_body->force_update_on_frame)
		{
			//reset direction vector
			memset(k_body->direction, 0, sizeof(vec3));
			k_body->current_speed = 0;
			node = node->next;
			continue;
		}
		k_body->force_update_on_frame = false;
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
		int min_y = roundf(expanded_box.position[1] - 1); //offset a little so we can do "is on ground" check
		int min_z = roundf(expanded_box.position[2]);

		int max_x = roundf(expanded_box.position[0] + expanded_box.width);
		int max_y = roundf(expanded_box.position[1] + expanded_box.height);
		int max_z = roundf(expanded_box.position[2] + expanded_box.length);

		vec3 max_normal;
		float max_dist = FLT_MAX;

		AABB block_box;
		block_box.width = 1;
		block_box.height = 1;
		block_box.length = 1;

		vec3 to_move;
		memset(to_move, 0, sizeof(vec3));

		if (k_body->collidable)
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
							Plane p;
							memset(&p, 0, sizeof(Plane));
							p.normal[0] = 0;
							p.normal[1] = 1;
							p.normal[2] = 0;

							vec3 from;
							from[0] = mink.position[0];
							from[1] = mink.position[1] + mink.height;
							from[2] = mink.position[2];

							vec3 dir;
							dir[0] = 0;
							dir[1] = -1;
							dir[2] = 0;
							float d = 2;

							Plane_IntersectsRay(&p, from, dir, NULL, &d);

							if (d < 1.0 && d >= 0)
							{
								k_body->on_ground = true;
							}
						}
						
						//Are we currently inside the other box?
						if (mink.position[0] <= 0 && mink.position[0] + mink.width >= 0 && mink.position[1] <= 0 && mink.position[1] + mink.height >= 0 &&
							mink.position[2] <= 0 && mink.position[2] + mink.length >= 0)
						{
							
							vec3 peneration_depth;

							AABB_getPenerationDepth(&mink, peneration_depth);

							printf("stuck \n");

							//push the body back
							k_body->box.position[0] += peneration_depth[0];
							k_body->box.position[1] += peneration_depth[1];
							k_body->box.position[2] += peneration_depth[2];

							to_move[0] += peneration_depth[0];
							to_move[1] += peneration_depth[1];
							to_move[2] += peneration_depth[2];

							if (peneration_depth[0] != 0 || peneration_depth[1] != 0 || peneration_depth[2] != 0)
							{
								glm_vec3_normalize(peneration_depth);
								glm_vec3_sign(peneration_depth, peneration_depth);

								k_body->box.position[0] += (float)CMP_EPSILON * peneration_depth[0];
								k_body->box.position[1] += (float)CMP_EPSILON * peneration_depth[1];
								k_body->box.position[2] += (float)CMP_EPSILON * peneration_depth[2];
								to_move[0] += (float)CMP_EPSILON * peneration_depth[0];
								to_move[1] += (float)CMP_EPSILON * peneration_depth[1];
								to_move[2] += (float)CMP_EPSILON * peneration_depth[2];

								vec3 tangent;
								//tangent[0] = -peneration_depth[1];
								//tangent[1] = peneration_depth[0];
								//tangent[2] = peneration_depth[2];

							}

							
						}
						//Check if we will collide
						else
						{
							vec3 normal;

							float dist = AABB_getFirstRayIntersection(mink, desired_vel, NULL, normal);

							//we will collide
							if (dist < 1.0)
							{
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
			//check if we can slide
			float normal_dot = glm_dot(max_normal, max_normal);
			if (normal_dot != 0)
			{
				//Need to fix issue if the player box is bigger than 1 the slide goes crazy
				float slide_dot = (1.0f - max_dist) * glm_dot(desired_vel, max_normal);

				

				k_body->velocity[0] = (1.0f - max_dist) * desired_vel[0] - (slide_dot / normal_dot) * max_normal[0];
				k_body->velocity[1] = (1.0f - max_dist) * desired_vel[1] - (slide_dot / normal_dot) * max_normal[1];
				k_body->velocity[2] = (1.0f - max_dist) * desired_vel[2] - (slide_dot / normal_dot) * max_normal[2];

				
				k_body->box.position[0] += k_body->velocity[0];
				k_body->box.position[1] += k_body->velocity[1];
				k_body->box.position[2] += k_body->velocity[2];

				k_body->box.position[0] += ((float)CMP_EPSILON * max_normal[0]);
				k_body->box.position[1] += ((float)CMP_EPSILON * max_normal[1]);
				k_body->box.position[2] += ((float)CMP_EPSILON * max_normal[2]);
			}
		}
		//No collisions found?
		//move by the desired velocity
		else
		{
			k_body->velocity[0] = desired_vel[0];
			k_body->velocity[1] = desired_vel[1];
			k_body->velocity[2] = desired_vel[2];

			k_body->box.position[0] += desired_vel[0];
			k_body->box.position[1] += desired_vel[1];
			k_body->box.position[2] += desired_vel[2];

			k_body->current_speed = adjusted_speed;
		}
		
		//reset direction vector
		memset(k_body->direction, 0, sizeof(vec3));

		node = node->next;
	}
}


void PhysWorld_Step(P_PhysWorld* world, float delta)
{
	for (int i = 0; i < 1; i++)
	{
		Solve_KinematicBodies(world, delta);
	}

	
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
