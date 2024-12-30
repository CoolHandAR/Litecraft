#include "r_texture.h"

#include <glad/glad.h>

#define STBI_FAILURE_USERMSG
#include <stb_image/stb_image.h>

static unsigned nrChannelToImageFormat(int p_nrChannel)
{
	//from stb_image.h
	// An output image with N components has the following components interleaved
	// in this order in each pixel:
	//
	//     N=#comp     components
	//       1           grey
	//       2           grey, alpha
	//       3           red, green, blue
	//       4           red, green, blue, alpha
	//
	switch (p_nrChannel)
	{
	case 1:
	{
		return GL_RED;
	}
	case 2:
	{
		return GL_RG;
	}
	case 3:
	{
		return GL_RGB;
	}
	case 4:
	{
		return GL_RGBA;
	}
	}

	return 0;
}

static unsigned char* loadTextureDataFromFile(int* r_width, int* r_height, unsigned* r_imageFormat, const char* p_path, bool p_flipOnLoad)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(p_flipOnLoad);

	unsigned char* data = stbi_load(p_path, &width, &height, &nrChannels, 0);

	if (!data)
	{
		printf("Failed to load texture. Reason: %s ", stbi_failure_reason());
		//stbi_image_free(data);
		return NULL;
	}

	if (width < 1 || height < 1)
	{
		printf("Failed to load texture. Reason invalid size");
		stbi_image_free(data);
		return NULL;
	}

	*r_width = width;
	*r_height = height;

	*r_imageFormat = nrChannelToImageFormat(nrChannels);
	
	return data;
}

static float* loadTextureDataFromFileFloat(int* r_width, int* r_height, unsigned* r_imageFormat, const char* p_path, bool p_flipOnLoad)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(p_flipOnLoad);

	float* data = stbi_loadf(p_path, &width, &height, &nrChannels, 0);

	if (!data)
	{
		printf("Failed to load texture. Reason: %s ", stbi_failure_reason());
		//stbi_image_free(data);
		return NULL;
	}

	if (width < 1 || height < 1)
	{
		printf("Failed to load texture. Reason invalid size");
		stbi_image_free(data);
		return NULL;
	}

	*r_width = width;
	*r_height = height;

	*r_imageFormat = nrChannelToImageFormat(nrChannels);

	return data;
}

static R_Texture genTexture(unsigned char* p_data, unsigned p_texWidth, unsigned p_texHeight, unsigned p_imageFormat, M_Rect2Di* p_textureRegion, bool p_isFloat)
{
	R_Texture texture;
	memset(&texture, 0, sizeof(R_Texture));

	//Default format
	texture.format.imageFormat = p_imageFormat;
	texture.format.wrapS = GL_REPEAT;
	texture.format.wrapT = GL_REPEAT;
	texture.format.filterMin = GL_NEAREST;
	texture.format.filterMax = GL_NEAREST;

	texture.width = p_texWidth;
	texture.height = p_texHeight;

	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);

	//glTexStorage2D(GL_TEXTURE_2D, 1, (p_isFloat == true) ? GL_RGBA16F : GL_RGBA8, texture.width, texture.height);

	glTexImage2D(GL_TEXTURE_2D, 0, (p_isFloat == true) ? GL_RGBA16F : GL_RGBA8, texture.width, texture.height, 0, texture.format.imageFormat, (p_isFloat == true) ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);

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
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.width, texture.height, texture.format.imageFormat, (p_isFloat == true) ? GL_FLOAT : GL_UNSIGNED_BYTE, p_data);
	}
	else
	{
		M_Rect2Di rect = *p_textureRegion;

		if (rect.x < 0) rect.x = 0;
		if (rect.y < 0) rect.y = 0;
		if (rect.x + rect.width > texture.width) rect.width = texture.width - rect.x;
		if (rect.y + rect.height > texture.height) rect.height = texture.height - rect.y;

		//Need to multiply the width with multiplier since otherwise it causes crashes and/or broken diagonal lines
		//READ: https://www.khronos.org/opengl/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
		//READ: https://www.khronos.org/opengl/wiki/Pixel_Transfer#Pixel_layout

		const int PIXEL_ARRAY_MULTIPLIER = (texture.format.imageFormat == GL_RGB) ? 3 : 4;

		const uint8_t* pixels = p_data + PIXEL_ARRAY_MULTIPLIER * (rect.x + (texture.width * rect.y));

		for (int i = 0; i < rect.height; ++i)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, i, rect.width, 1, GL_RGB, (p_isFloat) ? GL_FLOAT : GL_UNSIGNED_BYTE, pixels);
			pixels += PIXEL_ARRAY_MULTIPLIER * texture.width;
		}
	}


	//set texture wrap and filter modes
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture.format.wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture.format.wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.format.filterMin);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.format.filterMax);

	glFlush();

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);


	return texture;
}



R_Texture Texture_LoadFromData(unsigned char* p_data, size_t p_bufLen, M_Rect2Di* p_textureRegion)
{
	R_Texture texture;
	texture.id = 0;

	int width, height;
	int nr_channels;
	
	unsigned char* texture_data = stbi_load_from_memory(p_data, p_bufLen, &width, &height, &nr_channels, 4);

	if (!texture_data)
	{
		printf("Failed to load texture. Reason: %s ", stbi_failure_reason());
		return texture;
	}

	if (width < 1 || height < 1)
	{
		printf("Failed to load texture. Reason: invalid size");
		stbi_image_free(texture_data);
		return texture;
	}

	unsigned image_format = nrChannelToImageFormat(nr_channels);

	texture = genTexture(texture_data, width, height, image_format, p_textureRegion, false);

	stbi_image_free(texture_data);

	return texture;
}

R_Texture Texture_Load(const char* p_texturePath, M_Rect2Di* p_textureRegion)
{
	R_Texture texture;
	texture.id = 0;

	int width, height;
	unsigned image_format;

	unsigned char* texture_data = loadTextureDataFromFile(&width, &height, &image_format, p_texturePath, true);

	if (!texture_data)
	{
		return texture;
	}
	
	texture = genTexture(texture_data, width, height, image_format, p_textureRegion, false);

	stbi_image_free(texture_data);

	return texture;
}

void Texture_Destruct(R_Texture* p_texture)
{
	glDeleteTextures(1, p_texture->id);

}

R_Texture HDRTexture_Load(const char* p_texturePath, M_Rect2Di* p_textureRegion)
{
	R_Texture texture;
	texture.id = 0;

	int width, height;
	unsigned image_format;

	float* texture_data = loadTextureDataFromFileFloat(&width, &height, &image_format, p_texturePath, true);

	if (!texture_data)
	{
		return texture;
	}

	texture = genTexture(texture_data, width, height, image_format, p_textureRegion, true);

	stbi_image_free(texture_data);

	return texture;
}

R_Texture CubemapTexture_Load(Cubemap_Faces_Paths p_facesPathsData)
{
	R_Texture texture;
	texture.width = 0;
	texture.height = 0;
	texture.id = 0;
	
	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id);
	unsigned char* data = NULL;
	//RIGHT FACE
	data = loadTextureDataFromFile(&texture.width, &texture.height, &texture.format.imageFormat, p_facesPathsData.right_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//LEFT FACE
	data = loadTextureDataFromFile(&texture.width, &texture.height, &texture.format.imageFormat, p_facesPathsData.left_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//TOP FACE
	data = loadTextureDataFromFile(&texture.width, &texture.height, &texture.format.imageFormat, p_facesPathsData.top_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//BOTTOM FACE
	data = loadTextureDataFromFile(&texture.width, &texture.height, &texture.format.imageFormat, p_facesPathsData.bottom_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//BACK FACE
	data = loadTextureDataFromFile(&texture.width, &texture.height, &texture.format.imageFormat, p_facesPathsData.back_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);
	//FRONT FACE
	data = loadTextureDataFromFile(&texture.width, &texture.height, &texture.format.imageFormat, p_facesPathsData.front_face, false);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, texture.format.imageFormat, texture.width, texture.height, 0, texture.format.imageFormat, GL_UNSIGNED_BYTE, data);


	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return texture;
}
