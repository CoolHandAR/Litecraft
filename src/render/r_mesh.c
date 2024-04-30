#include "r_mesh.h"

#include <glad/glad.h>

#include "r_renderDefs.h"


R_Mesh Mesh_Init(dynamic_array* p_vertices, dynamic_array* p_indices, dynamic_array* p_textures)
{
	R_Mesh mesh;
	memset(&mesh, 0, sizeof(R_Mesh));

	mesh.vertices = p_vertices;
	mesh.indices = p_indices;
	mesh.textures = p_textures;

	//GEN VAO
	glGenVertexArrays(1, &mesh.vao);

	//GEN VBO
	glGenBuffers(1, &mesh.vbo);

	//GEN EBO
	glGenBuffers(1, &mesh.ebo);

	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);

	Vertex* data = mesh.vertices->data;
	uint32_t* indices = mesh.indices->data;

	glBufferData(GL_ARRAY_BUFFER, mesh.vertices->elements_size * sizeof(ModelVertex), &data[0], GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices->elements_size * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

	const int STRIDE_SIZE = sizeof(ModelVertex);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, position)));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, normal)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, tex_coords)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, tangent)));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, STRIDE_SIZE, (void*)(offsetof(ModelVertex, bitangent)));
	glEnableVertexAttribArray(4);

	return mesh;
}

void Mesh_Destruct(R_Mesh* p_mesh)
{
	glDeleteVertexArrays(1, &p_mesh->vao);
	glDeleteBuffers(1, &p_mesh->vbo);
	glDeleteBuffers(1, &p_mesh->ebo);

	if(p_mesh->textures)
		free(p_mesh->textures);

	if(p_mesh->vertices)
		free(p_mesh->vertices);

	free(p_mesh);

	p_mesh = NULL;
}
