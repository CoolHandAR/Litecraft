#include "r_model.h"


#define _CRT_SECURE_NO_WARNINGS

#include <assimp/cimport.h> 
#include <assimp/scene.h>  
#include <assimp/postprocess.h>
#include <stdio.h>
#include <string.h>

#include "r_texture.h"

#include <string.h>
#include "utility/Basic_string.h"

#include "r_renderDefs.h"

#include "r_mesh.h"



static dynamic_array* loadMaterialTextures(R_Model* const p_model, struct aiMaterial* p_mat, enum aiTextureType p_textureType, const char* p_typeName)
{
	unsigned int tex_count = aiGetMaterialTextureCount(p_mat, p_textureType);
	
	dynamic_array* textures = dA_INIT(TextureData, 0);

	dA_reserve(textures, tex_count);
	
	bool skip = false;

	for (unsigned i = 0; i < tex_count; i++)
	{
		struct aiString str;
		enum aitReturn tex = aiGetMaterialTexture(p_mat, p_textureType, i, &str, NULL, NULL, NULL, NULL, NULL, NULL);

		for (unsigned j = 0; j < p_model->loaded_textures->elements_size; j++)
		{
			TextureData* textures_array = textures->data;
			TextureData* textures_loaded_array = p_model->loaded_textures->data;

			if (strcmp(&textures_loaded_array[j].path, &str.data) == 0)
			{
				TextureData* ptr = dA_emplaceBack(textures);
				*ptr = textures_loaded_array[j];

				skip = true;
				break;
			}
		}

		if (!skip)
		{
			char filename[1024];
			memset(filename, 0, 1024);
			
			strncpy(&filename, p_model->directory, strlen(p_model->directory) + 1);
			strncat(&filename, "/", 1);
			strncat(&filename, str.data, str.length);
			

			R_Texture texture = Texture_Load(filename, NULL);

			if (texture.id != 0)
			{
				dA_emplaceBack(textures);
				TextureData* last = dA_getLast(textures);

				strncpy(last->path, &str.data, str.length);
				strncpy(last->type, p_typeName, strlen(p_typeName));
				
				last->texture = texture;

				dA_emplaceBack(p_model->loaded_textures);
				
				TextureData* last_loaded = dA_getLast(p_model->loaded_textures);

				strncpy(last_loaded->path, &str.data, str.length);
				strncpy(last_loaded->type, p_typeName, strlen(p_typeName));
				last_loaded->texture = texture;
			}
		}
	}


	return textures;
}

static R_Mesh processMesh(R_Model* const p_model, struct aiMesh* p_mesh, const struct aiScene* p_scene)
{
	dynamic_array* vertices = dA_INIT(Vertex, 0);
	dynamic_array* indices = dA_INIT(uint32_t, 0);
	dynamic_array* textures = dA_INIT(TextureData, 0);

	dA_reserve(vertices, p_mesh->mNumVertices);
	

	for (unsigned i = 0; i < p_mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.position[0] = p_mesh->mVertices[i].x;
		vertex.position[1] = p_mesh->mVertices[i].y;
		vertex.position[2] = p_mesh->mVertices[i].z;

		
		if (p_mesh->mNormals)
		{
			vertex.normal[0] = p_mesh->mNormals[i].x;
			vertex.normal[1] = p_mesh->mNormals[i].y;
			vertex.normal[2] = p_mesh->mNormals[i].z;
		}

		if (p_mesh->mTextureCoords[0])
		{
			vertex.tex_coords[0] = p_mesh->mTextureCoords[0][i].x;
			vertex.tex_coords[1] = p_mesh->mTextureCoords[0][i].y;
		}
		else
		{
			vertex.tex_coords[0] = 0.0f;
			vertex.tex_coords[1] = 0.0f;
		}
		Vertex* ptr = dA_emplaceBack(vertices);
		*ptr = vertex;
	}

	for (unsigned i = 0; i < p_mesh->mNumFaces; i++)
	{
		struct aiFace face = p_mesh->mFaces[i];

		for (unsigned j = 0; j < face.mNumIndices; j++)
		{
			uint32_t* ptr = dA_emplaceBack(indices);
			*ptr = face.mIndices[j];
		}
	}

	struct aiMaterial* material = p_scene->mMaterials[p_mesh->mMaterialIndex];

	//DIFFUSE MAPS
	dynamic_array* diffuseMaps = loadMaterialTextures(p_model, material, aiTextureType_DIFFUSE, "texture_diffuse");
	for (int i = 0; i < diffuseMaps->elements_size; i++)
	{
		TextureData* ptr = dA_emplaceBack(textures);

		TextureData* array = diffuseMaps->data;

		*ptr = array[i];
	}
	//SPECULAR MAPS
	dynamic_array* specularMaps = loadMaterialTextures(p_model, material, aiTextureType_SPECULAR, "texture_specular");
	for (int i = 0; i < specularMaps->elements_size; i++)
	{
		TextureData* ptr = dA_emplaceBack(textures);

		TextureData* array = specularMaps->data;

		*ptr = array[i];
	}
	//NORMAL MAPS
	dynamic_array* normalMaps = loadMaterialTextures(p_model, material, aiTextureType_HEIGHT, "texture_normal");
	for (int i = 0; i < normalMaps->elements_size; i++)
	{
		TextureData* ptr = dA_emplaceBack(textures);

		TextureData* array = normalMaps->data;

		*ptr = array[i];
	}
	//HEIGHT MAPS
	dynamic_array* heightMaps = loadMaterialTextures(p_model, material, aiTextureType_AMBIENT, "texture_height");
	for (int i = 0; i < heightMaps->elements_size; i++)
	{
		TextureData* ptr = dA_emplaceBack(textures);

		TextureData* array = heightMaps->data;

		*ptr = array[i];
	}

	//clean up
	dA_Destruct(diffuseMaps);
	dA_Destruct(specularMaps);
	dA_Destruct(normalMaps);
	dA_Destruct(heightMaps);
	
	R_Mesh mesh = Mesh_Init(vertices, indices, textures);

	return mesh;
}

static void processNode(R_Model* const p_model, struct aiNode* p_node, const struct aiScene* p_scene)
{
	for (unsigned i = 0; i < p_node->mNumMeshes; i++)
	{
		struct aiMesh* mesh = p_scene->mMeshes[p_node->mMeshes[i]];

		R_Mesh processed_mesh = processMesh(p_model, mesh, p_scene);
		
		R_Mesh* ptr = dA_emplaceBack(p_model->meshes);
		*ptr = processed_mesh;
	}
	for (unsigned i = 0; i < p_node->mNumChildren; i++)
	{
		processNode(p_model, p_node->mChildren[i], p_scene);
	}
}

R_Model Model_Load(const char* p_mdlPath)
{
	R_Model model;

	const struct aiScene* scene = aiImportFile(p_mdlPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		printf("Failed to load model\n");
		return model;
	}

	int find_index = 0;
	for (int i = 0; i < strlen(p_mdlPath); i++)
	{
		if (p_mdlPath[i] == '/')
		{
			find_index = i;	
		}
	}

	memset(&model.directory, 0, 2222);
	strncpy(&model.directory, p_mdlPath, find_index);
	
	//set up the arrays
	model.loaded_textures = dA_INIT(TextureData, 0);
	model.meshes = dA_INIT(R_Mesh, 0);

	// process ASSIMP's root node recursively
	processNode(&model, scene->mRootNode, scene);

	return model;
}

void Model_Destruct(R_Model* p_mdl)
{
	if (p_mdl->loaded_textures)
	{
		free(p_mdl->loaded_textures);
	}

	if (p_mdl->meshes)
	{
		for (int i = 0; i < p_mdl->meshes->elements_size; i++)
		{
			R_Mesh* array = p_mdl->meshes->data;

			Mesh_Destruct(&array[i]);
		}

		free(p_mdl->meshes);
	}

}
