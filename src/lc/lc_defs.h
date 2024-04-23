#pragma once

#include <cglm/cglm.h>
//#include "physics/p_physics_defs.h"

typedef enum LC_EntityType
{
	LC_ET__BLOCK,
	LC_ET__ACTOR,
	LC_ET__PICKUP

} LC_EnityType;

typedef struct LC_Entity
{
	void* usr_data;
	int id;
	LC_EnityType type;
	//Physics_Type phys_type;

} LC_Entity;