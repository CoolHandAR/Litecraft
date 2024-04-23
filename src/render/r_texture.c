#include "r_texture.h"

#include <glad/glad.h>

#include <string.h>

#define STBI_FAILURE_USERMSG
#include <stb_image/stb_image.h>

static unsigned char* loadTextureData(R_Texture* const p_tex, const char* p_path, bool p_flipOnLoad)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(p_flipOnLoad);

	unsigned char* data = stbi_load(p_path, &width, &height, &nrChannels, 0);

	if (!data)
	{
		printf("Failed to load texture. Reason: %s ", stbi_failure_reason());
		stbi_image_free(data);
		return NULL;
	}

	if (width < 1 || height < 1)
	{
		printf("Failed to load texture. Reason invalid size");
		stbi_image_free(data);
		return NULL;
	}

	p_tex->width = width;
	p_tex->height = height;

	//from stb_image.h
	// An output image with N components has the following components interleaved
	// in this order in each pixel:
	//
	//     N=#comp     components
	//       1           grey
	//       2           grey, alpha
	//       3           red, green, blue
	//       4           red, green, blue, alpha
	//set correct color channel format

	if (nrChannels < 4)
	{
		p_tex->format.imageFormat = GL_RGB;
	}
	else
	{
		p_tex->format.imageFormat = GL_RGBA;
	}

	return data;
}

static void genTexture(R_Texture* const p_tex, unsigned int p_width, unsigned int p_height, unsigned char* p_data)
{
	//create texture
	glGenTextures(1, &p_tex->id);
	glBindTexture(GL_TEXTURE_2D, p_tex->id);

	int mipmap_levels = 2;
	int format = GL_RGBA8;

	//only works at openGL 4.2 and above
	glTexStorage2D(GL_TEXTURE_2D, mipmap_levels, GL_RGBA8, p_width, p_height);

	//set texture wrap and filter modes
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, p_tex->format.wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, p_tex->format.wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, p_tex->format.filterMin);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, p_tex->format.filterMax);


	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, p_width, p_height, p_tex->format.imageFormat, GL_UNSIGNED_BYTE, p_data);

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
}


R_Texture Texture_Load(const char* p_texturePath, M_Rect2Di* p_textureRegion)
{
	R_Texture texture;
	memset(&texture, 0, sizeof(R_Texture));

	texture.format.imageFormat = GL_RGBA;
	texture.format.wrapS = GL_REPEAT;
	texture.format.wrapT = GL_REPEAT;
	texture.format.filterMin = GL_NEAREST;
	texture.format.filterMax = GL_NEAREST;

	unsigned char* texture_data = loadTextureData(&texture, p_texturePath, true);

	if (!texture_data)
	{
		texture.id = 0;
		return texture;
	}

	//check if we want to generate the whole texture or only a subset
	bool render_entire_image;

	if (p_textureRegion == NULL)
	{
		render_entire_image = true;
	}
	else
	{
		if ((p_textureRegion->width == 0 || p_textureRegion->height == 0) || ((p_textureRegion->x <= 0) && (p_textureRegion->y <= 0) && (p_textureRegion->width >= texture.width) && (p_textureRegion->height >= texture.height)))
		{
			render_entire_image = true;
		}
		else
		{
			render_entire_image = false;
		}
	}

	if (render_entire_image)
	{
		genTexture(&texture, texture.width, texture.height, texture_data);
	}
	else
	{
		M_Rect2Di rect = *p_textureRegion;

		if (rect.x < 0) rect.x = 0;
		if (rect.y < 0) rect.y = 0;
		if (rect.x + rect.width > texture.width) rect.width = texture.width - rect.x;
		if (rect.y + rect.height > texture.height) rect.height = texture.height - rect.y;

		//setup for future storage with nullptr
		genTexture(&texture, rect.width, rect.y, texture_data, NULL);

		glBindTexture(GL_TEXTURE_2D, texture.id);

		//Need to multiply the width with multiplier since otherwise it causes crashes and/or broken diagonal lines
		//READ: https://www.khronos.org/opengl/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
		//READ: https://www.khronos.org/opengl/wiki/Pixel_Transfer#Pixel_layout

		const int PIXEL_ARRAY_MULTIPLIER = (texture.format.imageFormat == GL_RGB) ? 3 : 4;

		const uint8_t* pixels = texture_data + PIXEL_ARRAY_MULTIPLIER * (rect.x + (texture.width * rect.y));

		for (int i = 0; i < rect.height; ++i)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, i, rect.width, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			pixels += PIXEL_ARRAY_MULTIPLIER * texture.width;
		}
	}

	stbi_image_free(texture_data);

	glFlush();

	return texture;
}

void Texture_Destruct(R_Texture* p_texture)
{
	glDeleteTextures(1, p_texture->id);

}

R_Texture CubemapTexture_Load(Cubemap_Faces_Paths p_facesPathsData)
{
	R_Texture texture;
	texture.width = 0;
	texture.height = 0;
	texture.id = 0;
	
	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id);
	unsigned char* data;
	//RIGHT FACE
	data = loadTextureData(&texture, p_facesPathsData.right_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//LEFT FACE
	data = loadTextureData(&texture, p_facesPathsData.left_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//TOP FACE
	data = loadTextureData(&texture, p_facesPathsData.top_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//BOTTOM FACE
	data = loadTextureData(&texture, p_facesPathsData.bottom_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//BACK FACE
	data = loadTextureData(&texture, p_facesPathsData.back_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//FRONT FACE
	data = loadTextureData(&texture, p_facesPathsData.front_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);


	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return texture;
}
