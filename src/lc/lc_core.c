#include "lc/lc_core.h"

#include "core/core_common.h"

extern LC_CoreData lc_core_data;

extern void PL_IssueDrawCmds();

void LC_MouseUpdate(double x, double y)
{
	static bool prev_ui_opened = false;

	static double prev_x = 0;
	static double prev_y = 0;

	if (Con_isOpened() || Window_isCursorEnabled())
	{
		if (!prev_ui_opened)
		{
			prev_ui_opened = true;
		}
	}
	else
	{
		if (Core_CheckForBlockedInputState())
		{
			return;
		}

		if (prev_ui_opened)
		{
			glfwSetCursorPos(Window_getPtr(), prev_x, prev_y);
			x = prev_x;
			y = prev_y;

			prev_ui_opened = false;
		}

		Camera_ProcessMouse(&lc_core_data.cam, x, y);
		prev_x = x;
		prev_y = y;
	}
}


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
	LC_World_StartFrame();
}
void LC_EndFrame()
{
	LC_World_EndFrame();
}

void LC_Draw()
{
	Draw_LCWorld();
	PL_IssueDrawCmds();
}

void LC_Exit()
{
	LC_World_Exit();
}

/*
* ~~~~~~~~~~~~~~~~~~~~
	PHYS UPDATE
* ~~~~~~~~~~~~~~~~~~~
*/
void LC_PhysUpdate(float delta)
{
	PhysicsWorld_Step(LC_World_GetPhysWorld(), delta);
}