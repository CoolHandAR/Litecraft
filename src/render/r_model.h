#pragma once

#include "utility/dynamic_array.h"

typedef struct R_Mesh
{
	unsigned vao, vbo, ebo;
	dynamic_array* vertices;
	dynamic_array* indices;
	dynamic_array* textures;

} R_Mesh;

R_Mesh Mesh_Init(dynamic_array* p_vertices, dynamic_array* p_indices, dynamic_array* p_textures);
void Mesh_Destruct(R_Mesh* p_mesh);

typedef struct R_Model
{
	char directory[2222];
	dynamic_array* meshes;
	dynamic_array* loaded_textures;

} R_Model;

R_Model Model_Load(const char* p_mdlPath);
void Model_Destruct(R_Model* p_mdl);