#include "lc2/lc_world.h"
#include "lc2/lc_chunk.h"
#include "utility/u_utility.h"

#define LC_IDENT (('R'<<24)+('C'<<16)+('L'<<8)+'I') //ILCR

#define LC_REGION_MAX_WIDTH 32
#define LC_REGION_MAX_HEIGTH 32
#define LC_REGION_MAX_LENGTH 32

typedef struct
{
	int offset;
	int length;
} LC_ChunkLump;

typedef struct
{
	int ident;
	LC_ChunkLump chunk_lumps[LC_REGION_MAX_WIDTH][LC_REGION_MAX_HEIGTH][LC_REGION_MAX_LENGTH];
} LC_RegionHeader;

static FILE* loaded_file;
static LC_RegionHeader* loaded_header;
static int buffer_size;

static void _getNormalizedChunkPosition(int p_gx, int p_gy, int p_gz, ivec3 dest)
{
	dest[0] = (int)floorf((p_gx / (float)LC_CHUNK_WIDTH));
	dest[1] = (int)floorf((p_gy / (float)LC_CHUNK_HEIGHT));
	dest[2] = (int)floorf((p_gz / (float)LC_CHUNK_LENGTH));
}

static int LittleLong(int l)
{
	unsigned char b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

static int BigLong(int l)
{
	unsigned char b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

static void AddLump(FILE* _lc_file, LC_RegionHeader* _header, ivec3 _chunk_pos, const void* _data, int _len)
{
	LC_ChunkLump* lump = &_header->chunk_lumps[_chunk_pos[0]][_chunk_pos[1]][_chunk_pos[2]];

	lump->offset = ftell(_lc_file);
	lump->length = _len;
	fwrite(_data, 1, (_len), _lc_file);
}

static int CopyLump(LC_RegionHeader* _header, ivec3 _chunk_pos, void* _dest, int _size)
{
	int length, offset;
	length = _header->chunk_lumps[_chunk_pos[0]][_chunk_pos[1]][_chunk_pos[2]].length;
	offset = _header->chunk_lumps[_chunk_pos[0]][_chunk_pos[1]][_chunk_pos[2]].offset;

	if (length % _size)
	{
		//odd
		return 0;
	}

	memcpy(_dest, (unsigned char*)_header + offset, length);

	return length / _size;
}

int LC_File_Begin()
{
	fopen_s(&loaded_file, "assets/chunk.lc", "a+");

	if (!loaded_file)
	{
		return 0;
	}
	int length = File_GetLength(loaded_file);
	if (length > 0)
	{
		loaded_header = malloc(length + 1);
		if (!loaded_header)
		{
			return 0;
		}
		memset(loaded_header, 0, length + 1);
		fread_s(loaded_header, length, 1, length, loaded_file);
		buffer_size = length + 1;
	}
	else
	{
		loaded_header = malloc(sizeof(LC_RegionHeader));

		if (!loaded_header)
		{
			return 0;
		}
		memset(loaded_header, 0, sizeof(LC_RegionHeader));
		buffer_size = sizeof(LC_RegionHeader);
	}
	return 1;
}

void LC_File_End()
{
	loaded_header->ident = LC_IDENT;
	fwrite(loaded_header, 1, sizeof(LC_RegionHeader), loaded_file);

	fseek(loaded_file, 0, SEEK_SET);
	fwrite(loaded_header, 1, sizeof(LC_RegionHeader), loaded_file);
	fclose(loaded_file);

	free(loaded_header);
	
	buffer_size = 0;
}


int LC_File_GetChunk(int p_gX, int p_gY, int p_gZ, LC_Chunk* r_chunk)
{
	ivec3 norm_chunk_pos;
	_getNormalizedChunkPosition(p_gX, p_gY, p_gZ, norm_chunk_pos);

	if (norm_chunk_pos[0] >= LC_REGION_MAX_WIDTH)
	{
		return 0;
	}
	if (norm_chunk_pos[1] >= LC_REGION_MAX_HEIGTH)
	{
		return 0;
	}
	if (norm_chunk_pos[2] >= LC_REGION_MAX_LENGTH)
	{
		return 0;
	}

	int v = CopyLump(loaded_header, norm_chunk_pos, r_chunk, sizeof(LC_Chunk));

	if (v == 0)
	{
		return 0;
	}

	return 1;
}

int LC_File_WriteChunk(LC_Chunk* const p_chunk)
{
	ivec3 norm_chunk_pos;
	_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], norm_chunk_pos);

	if (norm_chunk_pos[0] >= LC_REGION_MAX_WIDTH)
	{
		return 0;
	}
	if (norm_chunk_pos[1] >= LC_REGION_MAX_HEIGTH)
	{
		return 0;
	}
	if (norm_chunk_pos[2] >= LC_REGION_MAX_LENGTH)
	{
		return 0;
	}

	AddLump(loaded_file, loaded_header, norm_chunk_pos, p_chunk, sizeof(LC_Chunk));

	return 1;
}

void LC_File_PrintChunk(const char* p_filename, LC_Chunk* const p_chunk)
{
	FILE* lc_file;
	LC_RegionHeader header;
	memset(&header, 0, sizeof(LC_RegionHeader));

	fopen_s(&lc_file, p_filename, "w");

	if (!lc_file)
	{
		return;
	}
	header.ident = LC_IDENT;
	fwrite(&header, 1, sizeof(header), lc_file);
	
	ivec3 norm_chunk_pos;
	_getNormalizedChunkPosition(p_chunk->global_position[0], p_chunk->global_position[1], p_chunk->global_position[2], norm_chunk_pos);

	AddLump(lc_file, &header, norm_chunk_pos, p_chunk, sizeof(LC_Chunk));

	fseek(lc_file, 0, SEEK_SET);
	fwrite(&header, 1, sizeof(header), lc_file);
	fclose(lc_file);
}

LC_Chunk LC_File_LoadChunk(const char* p_filename)
{
	LC_RegionHeader* header = File_Parse(p_filename, NULL);

	if (!header->ident)
	{
		
	}

	LC_Chunk chunk;
	ivec3 norm_chunk_pos;
	_getNormalizedChunkPosition(0, 0, 0, norm_chunk_pos);
	CopyLump(header, norm_chunk_pos, &chunk, sizeof(LC_Chunk));
	
	free(header);

	return chunk;
}