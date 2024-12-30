#include "r_model.h"


#define _CRT_SECURE_NO_WARNINGS

#include <assimp/cimport.h> 
#include <assimp/scene.h>  
#include <assimp/postprocess.h>
#include <assimp/texture.h>
#include <stdio.h>
#include <string.h>

#include "r_texture.h"

#include <string.h>
#include "utility/Basic_string.h"
#include "utility/u_utility.h"
#include "core/resource_manager.h"

typedef struct 
{
	vec3 position;
	vec2 tex_coords;
	vec3 tangent;
	vec3 normal;
	vec3 bitangent;
} ModelVertex;


static R_Texture* loadMaterialTexture(struct aiMaterial* p_mat, enum aiTextureType p_textureType, const struct aiScene* p_scene, const char* p_directory)
{
	int tex_count = aiGetMaterialTextureCount(p_mat, p_textureType);

	//no texture found?
	if (tex_count <= 0)
		return NULL;

	struct aiString path;
	enum aitReturn result = aiGetMaterialTexture(p_mat, p_textureType, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
	
	//failed to load for whatever reason?
	if (result != aiReturn_SUCCESS)
		return NULL;

	//check if the texture is embedded
	if (path.data[0] == '*')
	{
		const char* chr_data = path.data + 1;
		int texture_index = strtol(chr_data, (char**)NULL, 10);

		assert(texture_index >= 0 && texture_index < p_scene->mNumTextures);

		struct aiTexture* tex = p_scene->mTextures[texture_index];

		size_t buffer_size = (tex->mHeight == 0) ? tex->mWidth : tex->mWidth * tex->mHeight;

		char embedded_texture_name[1024];

		//create a pseudo handle using the directory and the texture type
		sprintf_s(embedded_texture_name, 1024, "%s/%s.%s", p_directory, aiTextureTypeToString(p_textureType), tex->achFormatHint);

		return Resource_getFromMemory(embedded_texture_name, tex->pcData, buffer_size, RESOURCE__TEXTURE);
	}

	//ptherwise it's a normal texture file
	char texture_path[1024];
	sprintf_s(texture_path, 1024, "%s/%s", p_directory, path.data);
	
	return Resource_get(texture_path, RESOURCE__TEXTURE);;
}

static R_Mesh processMesh(R_Model* const p_model, struct aiMesh* p_mesh, const struct aiScene* p_scene, const char* p_directory)
{
	dynamic_array* vertices = dA_INIT(ModelVertex, 0);
	dynamic_array* indices = dA_INIT(uint32_t, 0);

	size_t total_indices = 0;
	for (size_t i = 0; i < p_mesh->mNumFaces; i++)
	{
		struct aiFace face = p_mesh->mFaces[i];
		total_indices += face.mNumIndices;
	}

	//reserve arrays up front for faster processing / avoid realloc calls
	dA_reserve(vertices, p_mesh->mNumVertices);
	dA_reserve(indices, total_indices);

	//VERTICES
	for (size_t i = 0; i < p_mesh->mNumVertices; i++)
	{
		ModelVertex vertex;
		memset(&vertex, 0, sizeof(ModelVertex));

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
		if (p_mesh->mTangents)
		{
			vertex.tangent[0] = p_mesh->mTangents[i].x;
			vertex.tangent[1] = p_mesh->mTangents[i].y;
			vertex.tangent[2] = p_mesh->mTangents[i].z;
		}
		if (p_mesh->mBitangents)
		{
			vertex.bitangent[0] = p_mesh->mBitangents[i].x;
			vertex.bitangent[1] = p_mesh->mBitangents[i].y;
			vertex.bitangent[2] = p_mesh->mBitangents[i].z;
		}
	
		ModelVertex* ptr = dA_emplaceBack(vertices);
		*ptr = vertex;
	}

	//INDICES
	for (size_t i = 0; i < p_mesh->mNumFaces; i++)
	{
		struct aiFace face = p_mesh->mFaces[i];

		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			uint32_t* ptr = dA_emplaceBack(indices);
			*ptr = face.mIndices[j];
		}
	}

	//SETUP MATERIALS
	struct aiMaterial* material = p_scene->mMaterials[p_mesh->mMaterialIndex];

	R_Mesh mesh;
	
	//BASE COLOR
	//look for either a diffuse type or PBR base_color
	mesh.material.base_color = loadMaterialTexture(material, aiTextureType_BASE_COLOR, p_scene, p_directory);
	//base color not found? try loading phong specular texture
	if (mesh.material.base_color == NULL)
	{
		mesh.material.base_color = loadMaterialTexture(material, aiTextureType_DIFFUSE, p_scene, p_directory);
	}
	//METALLICNES/SPECULAR
	//look for either PBR metallicness or specular texture
	mesh.material.metallic__specular = loadMaterialTexture(material, aiTextureType_METALNESS, p_scene, p_directory);
	//metalness not found? look for specular
	if (mesh.material.metallic__specular == NULL)
	{
		mesh.material.metallic__specular = loadMaterialTexture(material, aiTextureType_SPECULAR, p_scene, p_directory);
	}
	//ROUGHNESS
	mesh.material.roughness = loadMaterialTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, p_scene, p_directory);
	
	//AMBIENT OCCLUSSION
	mesh.material.ao = loadMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, p_scene, p_directory);

	//NORMAL MAP
	mesh.material.normal = loadMaterialTexture(material, aiTextureType_NORMALS, p_scene, p_directory);

	
	mesh.vertices = vertices;
	mesh.indices = indices;

	//Bounding box
	mesh.bounding_box[0][0] = p_mesh->mAABB.mMin.x;
	mesh.bounding_box[0][1] = p_mesh->mAABB.mMin.y;
	mesh.bounding_box[0][2] = p_mesh->mAABB.mMin.z;

	mesh.bounding_box[1][0] = p_mesh->mAABB.mMax.x;
	mesh.bounding_box[1][1] = p_mesh->mAABB.mMax.y;
	mesh.bounding_box[1][2] = p_mesh->mAABB.mMax.z;

	return mesh;
}
static void processNode(R_Model* const p_model, struct aiNode* p_node, const struct aiScene* p_scene, const char* p_directory)
{
	for (unsigned i = 0; i < p_node->mNumMeshes; i++)
	{
		struct aiMesh* mesh = p_scene->mMeshes[p_node->mMeshes[i]];

		R_Mesh processed_mesh = processMesh(p_model, mesh, p_scene, p_directory);

		R_Mesh* ptr = dA_emplaceBack(p_model->meshes);
		*ptr = processed_mesh;
	}
	for (unsigned i = 0; i < p_node->mNumChildren; i++)
	{
		processNode(p_model, p_node->mChildren[i], p_scene, p_directory);
	}
}


R_Model Model_Load(const char* p_mdlPath)
{
	R_Model model;
	memset(&model, 0, sizeof(model));

	unsigned load_flags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenBoundingBoxes
		| aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_RemoveRedundantMaterials;

	const struct aiScene* scene = aiImportFile(p_mdlPath, load_flags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		printf("Failed to load model %s\n", aiGetErrorString());
		return model;
	}

	char directory[1024];
	memset(directory, 0, sizeof(directory));

	int _index = String_findLastOfIndex(p_mdlPath, '/');

	if (_index > -1)
	{
		strncpy_s(directory, 1024, p_mdlPath, _index);
	}

	model.meshes = dA_INIT(R_Mesh, 0);

	dA_reserve(model.meshes, scene->mNumMeshes);

	// process ASSIMP's root node recursively
	processNode(&model, scene->mRootNode, scene, directory);

	//release scene data
	aiReleaseImport(scene);

	return model;
}

void Mesh_Destruct(R_Mesh* p_mesh)
{
	if (p_mesh->vertices)
	{
		dA_Destruct(p_mesh->vertices);
	}
	if (p_mesh->indices)
	{
		dA_Destruct(p_mesh->indices);
	}
}

void Model_Destruct(R_Model* p_mdl)
{
	if (p_mdl->meshes)
	{
		for (int i = 0; i < p_mdl->meshes->elements_size; i++)
		{
			R_Mesh* array = p_mdl->meshes->data;

			Mesh_Destruct(&array[i]);
		}

		dA_Destruct(p_mdl->meshes);
	}

}
