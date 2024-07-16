#include "r_core.h"

static void r_depthPrepass()
{
	//use shader

	//Disable color mask since we are only writing to the depth buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	//draw scene

	//Enable color mask again
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

static void r_gbufferPass()
{
	glDepthFunc(GL_EQUAL); //Skip occluded pixels since we did the depth pre pass already
	glDepthMask(GL_FALSE); 
	//use shader
	
	//draw scene

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}

static void r_deferredProcessPass()
{
	//use shader

	//draw quad
}

static void r_transparentPass()
{

}

static void r_waterTypePass()
{

}

void r_MainPass()
{
	//Depth prepass
	r_depthPrepass();

	//g buffer pass
	r_gbufferPass();

	//deferred pass
	r_deferredProcessPass();
}