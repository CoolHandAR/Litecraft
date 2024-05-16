/*
~~~~~~~~~~~~~~~~~~
INIT GAME RESOURCES, INPUTS, GAME WORLD, PHYSICS WORLD, ETC...
~~~~~~~~~~~~~~~~~~
*/

#include "input.h"
#include "sound/sound.h"


static void _initPlayer()
{

}

static int _loadResources()
{
	//Textures



	//Sounds
	Sound_load("Fall_crunch", "assets/sounds/player/fall.png", MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_ASYNC);

}

static void _initInputs()
{
	Input_registerAction("Jump");
	Input_registerAction("Forward");
	Input_registerAction("Backward");
	Input_registerAction("Left");
	Input_registerAction("Right");
	Input_registerAction("Attack");
	Input_registerAction("Special");
	Input_registerAction("Duck");
}

int LC_Init2()
{
	if (!_loadResources()) return false;

	_initInputs();
}