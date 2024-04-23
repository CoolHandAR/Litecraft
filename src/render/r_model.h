#pragma once

#include "utility/dynamic_array.h"


typedef struct R_Model
{
	char directory[2222];
	dynamic_array* meshes;
	dynamic_array* loaded_textures;

} R_Model;

R_Model Model_Load(const char* p_mdlPath);
void Model_Destruct(R_Model* p_mdl);