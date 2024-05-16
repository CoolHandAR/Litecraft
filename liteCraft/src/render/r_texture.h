#pragma once
#include <cglm/cglm.h>

#include "utility/u_math.h"

typedef struct Texture_Format
{
	unsigned int imageFormat;
	unsigned int wrapS;
	unsigned int wrapT;
	unsigned int filterMin;
	unsigned int filterMax;

}Texture_Format;

typedef struct R_Texture
{
	Texture_Format format;
	unsigned int id;
	int width;
	int height;

} R_Texture;

R_Texture Texture_Load(const char* p_texturePath, M_Rect2Di* p_textureRegion);
void Texture_Destruct(R_Texture* p_texture);

typedef struct Cubemap_Faces_Paths
{
	const char* right_face;
	const char* left_face;
	const char* top_face;
	const char* bottom_face;
	const char* back_face;
	const char* front_face;
} Cubemap_Faces_Paths;

R_Texture CubemapTexture_Load(Cubemap_Faces_Paths p_facesPathsData);