#include "lc/lc_core.h"

extern void PL_Update();
extern void LC_World_Update();

/*
* ~~~~~~~~~~~~~~~~~~~~
	GAME UPDATE
* ~~~~~~~~~~~~~~~~~~~
*/
void LC_StartFrame()
{
	//Update the player and process input
	PL_Update();

	//Update the game world
	//LC_World_StartFrame();
}
void LC_EndFrame()
{
	//lc_world_endframe();
}

/*
* ~~~~~~~~~~~~~~~~~~~~
	PHYS UPDATE
* ~~~~~~~~~~~~~~~~~~~
*/
void LC_PhysUpdate()
{

}