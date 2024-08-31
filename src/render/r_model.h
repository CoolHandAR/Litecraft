#pragma once

#include "utility/dynamic_array.h"
#include "utility/u_math.h"
#include "r_texture.h"

typedef struct
{
	R_Texture* base_color;
	R_Texture* metallic__specular;
	R_Texture* roughness;
	R_Texture* normal;
	R_Texture* ao;
} Material;

typedef struct R_Mesh
{
	mat4 xform;
	vec3 bounding_box[2];
	dynamic_array* vertices;
	dynamic_array* indices;
	Material material;
	unsigned storage_index;
} R_Mesh;

void Mesh_Destruct(R_Mesh* p_mesh);

typedef struct R_Model
{
	dynamic_array* meshes;
	unsigned id;
	bool registered;
} R_Model;

R_Model Model_Load(const char* p_mdlPath);
void Model_Destruct(R_Model* p_mdl);