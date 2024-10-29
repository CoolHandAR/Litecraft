#include "lc/lc_core.h"

extern void PL_Update();
extern void LC_World_Update();

/*
* ~~~~~~~~~~~~~~~~~~~~
	MAIN GAME LOOP
* ~~~~~~~~~~~~~~~~~~~
*/
void LC_Loop2()
{
	//Update the player and process input
	PL_Update();
}