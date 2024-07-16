/*
~~~~~~~~~~~~~~~~~~
INIT GAME RESOURCES, GAME WORLD, PHYSICS WORLD, ETC...
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


int LC_Init2()
{
	if (!_loadResources()) return false;

}