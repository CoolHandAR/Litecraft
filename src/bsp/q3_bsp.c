
typedef unsigned char byte;
#define	MAX_QPATH		64

#define BSP_IDENT	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
// little-endian "IBSP"

#define BSP_VERSION			46


// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define	MAX_MAP_MODELS		0x400
#define	MAX_MAP_BRUSHES		0x8000
#define	MAX_MAP_ENTITIES	0x800
#define	MAX_MAP_ENTSTRING	0x40000
#define	MAX_MAP_SHADERS		0x400

#define	MAX_MAP_AREAS		0x100	// MAX_MAP_AREA_BYTES in q_shared must match!
#define	MAX_MAP_FOGS		0x100
#define	MAX_MAP_PLANES		0x20000
#define	MAX_MAP_NODES		0x20000
#define	MAX_MAP_BRUSHSIDES	0x20000
#define	MAX_MAP_LEAFS		0x20000
#define	MAX_MAP_LEAFFACES	0x20000
#define	MAX_MAP_LEAFBRUSHES 0x40000
#define	MAX_MAP_PORTALS		0x20000
#define	MAX_MAP_LIGHTING	0x800000
#define	MAX_MAP_LIGHTGRID	0x800000
#define	MAX_MAP_VISIBILITY	0x200000

#define	MAX_MAP_DRAW_SURFS	0x20000
#define	MAX_MAP_DRAW_VERTS	0x80000
#define	MAX_MAP_DRAW_INDEXES	0x80000


// key / value pair sizes in the entities lump
#define	MAX_KEY				32
#define	MAX_VALUE			1024

// the editor uses these predefined yaw angles to orient entities up or down
#define	ANGLE_UP			-1
#define	ANGLE_DOWN			-2

#define	LIGHTMAP_WIDTH		128
#define	LIGHTMAP_HEIGHT		128

#define MAX_WORLD_COORD		( 128*1024 )
#define MIN_WORLD_COORD		( -128*1024 )
#define WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

//=============================================================================


typedef struct {
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_SHADERS		1
#define	LUMP_PLANES			2
#define	LUMP_NODES			3
#define	LUMP_LEAFS			4
#define	LUMP_LEAFSURFACES	5
#define	LUMP_LEAFBRUSHES	6
#define	LUMP_MODELS			7
#define	LUMP_BRUSHES		8
#define	LUMP_BRUSHSIDES		9
#define	LUMP_DRAWVERTS		10
#define	LUMP_DRAWINDEXES	11
#define	LUMP_FOGS			12
#define	LUMP_SURFACES		13
#define	LUMP_LIGHTMAPS		14
#define	LUMP_LIGHTGRID		15
#define	LUMP_VISIBILITY		16
#define	HEADER_LUMPS		17

typedef float vec3_t[3];

typedef struct {
	int			ident;
	int			version;

	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct {
	float		mins[3], maxs[3];
	int			firstSurface, numSurfaces;
	int			firstBrush, numBrushes;
} dmodel_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			surfaceFlags;
	int			contentFlags;
} dshader_t;

// planes x^1 is allways the opposite of plane x

typedef struct {
	float		normal[3];
	float		dist;
} dplane_t;

typedef struct {
	int			planeNum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	int			mins[3];		// for frustom culling
	int			maxs[3];
} dnode_t;

typedef struct {
	int			cluster;			// -1 = opaque cluster (do I still store these?)
	int			area;

	int			mins[3];			// for frustum culling
	int			maxs[3];

	int			firstLeafSurface;
	int			numLeafSurfaces;

	int			firstLeafBrush;
	int			numLeafBrushes;
} dleaf_t;

typedef struct {
	int			planeNum;			// positive plane side faces out of the leaf
	int			shaderNum;
} dbrushside_t;

typedef struct {
	int			firstSide;
	int			numSides;
	int			shaderNum;		// the shader that determines the contents flags
} dbrush_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

typedef struct {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
} drawVert_t;

typedef enum {
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE
} mapSurfaceType_t;

typedef struct {
	int			shaderNum;
	int			fogNum;
	int			surfaceType;

	int			firstVert;
	int			numVerts;

	int			firstIndex;
	int			numIndexes;

	int			lightmapNum;
	int			lightmapX, lightmapY;
	int			lightmapWidth, lightmapHeight;

	vec3_t		lightmapOrigin;
	vec3_t		lightmapVecs[3];	// for patches, [0] and [1] are lodbounds

	int			patchWidth;
	int			patchHeight;
} dsurface_t;


int			nummodels;
dmodel_t	dmodels[MAX_MAP_MODELS];

int			numShaders;
dshader_t	dshaders[MAX_MAP_SHADERS];

int			entdatasize;
char		dentdata[MAX_MAP_ENTSTRING];

int			numleafs;
dleaf_t		dleafs[MAX_MAP_LEAFS];

int			numplanes;
dplane_t	dplanes[MAX_MAP_PLANES];

int			numnodes;
dnode_t		dnodes[MAX_MAP_NODES];

int			numleafsurfaces;
int			dleafsurfaces[MAX_MAP_LEAFFACES];

int			numleafbrushes;
int			dleafbrushes[MAX_MAP_LEAFBRUSHES];

int			numbrushes;
dbrush_t	dbrushes[MAX_MAP_BRUSHES];

int			numbrushsides;
dbrushside_t	dbrushsides[MAX_MAP_BRUSHSIDES];

int			numLightBytes;
byte		lightBytes[MAX_MAP_LIGHTING];

int			numGridPoints;
byte		gridData[MAX_MAP_LIGHTGRID];

int			numVisBytes;
byte		visBytes[MAX_MAP_VISIBILITY];

int			numDrawVerts;
drawVert_t	drawVerts[MAX_MAP_DRAW_VERTS];

int			numDrawIndexes;
int			drawIndexes[MAX_MAP_DRAW_INDEXES];

int			numDrawSurfaces;
dsurface_t	drawSurfaces[MAX_MAP_DRAW_SURFS];

int			numFogs;
dfog_t		dfogs[MAX_MAP_FOGS];

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <utility/u_utility.h>

static int CopyLump(dheader_t* header, int lump, void* dest, int size) {
	int		length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;

	if (length % size) {
		assert(0 && "error");
	}

	memcpy(dest, (byte*)header + ofs, length);

	return length / size;
}

static void SwapBlock(int* block, int sizeOfBlock) {
	int		i;

	sizeOfBlock >>= 2;
	for (i = 0; i < sizeOfBlock; i++) {
		block[i] = (block[i]);
	}
}

static void SwapBSPFile(void) {
	int				i;

	// models	
	SwapBlock((int*)dmodels, nummodels * sizeof(dmodels[0]));

	// shaders (don't swap the name)
	for (i = 0; i < numShaders; i++) {
		dshaders[i].contentFlags = (dshaders[i].contentFlags);
		dshaders[i].surfaceFlags = (dshaders[i].surfaceFlags);
	}

	// planes
	SwapBlock((int*)dplanes, numplanes * sizeof(dplanes[0]));

	// nodes
	SwapBlock((int*)dnodes, numnodes * sizeof(dnodes[0]));

	// leafs
	SwapBlock((int*)dleafs, numleafs * sizeof(dleafs[0]));

	// leaffaces
	SwapBlock((int*)dleafsurfaces, numleafsurfaces * sizeof(dleafsurfaces[0]));

	// leafbrushes
	SwapBlock((int*)dleafbrushes, numleafbrushes * sizeof(dleafbrushes[0]));

	// brushes
	SwapBlock((int*)dbrushes, numbrushes * sizeof(dbrushes[0]));

	// brushsides
	SwapBlock((int*)dbrushsides, numbrushsides * sizeof(dbrushsides[0]));

	// vis
	((int*)&visBytes)[0] = (((int*)&visBytes)[0]);
	((int*)&visBytes)[1] = (((int*)&visBytes)[1]);

	// drawverts (don't swap colors )
	for (i = 0; i < numDrawVerts; i++) {
		drawVerts[i].lightmap[0] = (drawVerts[i].lightmap[0]);
		drawVerts[i].lightmap[1] = (drawVerts[i].lightmap[1]);
		drawVerts[i].st[0] = (drawVerts[i].st[0]);
		drawVerts[i].st[1] = (drawVerts[i].st[1]);
		drawVerts[i].xyz[0] = (drawVerts[i].xyz[0]);
		drawVerts[i].xyz[1] = (drawVerts[i].xyz[1]);
		drawVerts[i].xyz[2] = (drawVerts[i].xyz[2]);
		drawVerts[i].normal[0] = (drawVerts[i].normal[0]);
		drawVerts[i].normal[1] = (drawVerts[i].normal[1]);
		drawVerts[i].normal[2] = (drawVerts[i].normal[2]);
	}

	// drawindexes
	SwapBlock((int*)drawIndexes, numDrawIndexes * sizeof(drawIndexes[0]));

	// drawsurfs
	SwapBlock((int*)drawSurfaces, numDrawSurfaces * sizeof(drawSurfaces[0]));

	// fogs
	for (i = 0; i < numFogs; i++) {
		dfogs[i].brushNum = (dfogs[i].brushNum);
		dfogs[i].visibleSide = (dfogs[i].visibleSide);
	}
}

void	LoadBSPFile(const char* filename) {
	dheader_t* header;

	// load the file header
	header = File_Parse(filename, NULL);

	// swap the header
	SwapBlock((int*)header, sizeof(*header));

	if (header->ident != BSP_IDENT) {
		assert(0 && "error");
	}
	if (header->version != BSP_VERSION) {
		assert(0 && "error");
	}

	numShaders = CopyLump(header, LUMP_SHADERS, dshaders, sizeof(dshader_t));
	nummodels = CopyLump(header, LUMP_MODELS, dmodels, sizeof(dmodel_t));
	numplanes = CopyLump(header, LUMP_PLANES, dplanes, sizeof(dplane_t));
	numleafs = CopyLump(header, LUMP_LEAFS, dleafs, sizeof(dleaf_t));
	numnodes = CopyLump(header, LUMP_NODES, dnodes, sizeof(dnode_t));
	numleafsurfaces = CopyLump(header, LUMP_LEAFSURFACES, dleafsurfaces, sizeof(dleafsurfaces[0]));
	numleafbrushes = CopyLump(header, LUMP_LEAFBRUSHES, dleafbrushes, sizeof(dleafbrushes[0]));
	numbrushes = CopyLump(header, LUMP_BRUSHES, dbrushes, sizeof(dbrush_t));
	numbrushsides = CopyLump(header, LUMP_BRUSHSIDES, dbrushsides, sizeof(dbrushside_t));
	numDrawVerts = CopyLump(header, LUMP_DRAWVERTS, drawVerts, sizeof(drawVert_t));
	numDrawSurfaces = CopyLump(header, LUMP_SURFACES, drawSurfaces, sizeof(dsurface_t));
	numFogs = CopyLump(header, LUMP_FOGS, dfogs, sizeof(dfog_t));
	numDrawIndexes = CopyLump(header, LUMP_DRAWINDEXES, drawIndexes, sizeof(drawIndexes[0]));

	numVisBytes = CopyLump(header, LUMP_VISIBILITY, visBytes, 1);
	numLightBytes = CopyLump(header, LUMP_LIGHTMAPS, lightBytes, 1);
	entdatasize = CopyLump(header, LUMP_ENTITIES, dentdata, 1);

	numGridPoints = CopyLump(header, LUMP_LIGHTGRID, gridData, 8);


	free(header);		// everything has been copied out

	// swap everything
	SwapBSPFile();
}

#include <glad/glad.h>
#include "render/r_shader.h"
#include "render/r_texture.h"

unsigned BSP_VAO;
unsigned BSP_VBO, BSP_EBO;
unsigned BSP_Shader;
unsigned DRAW_COMMANDS;
unsigned VALID_SURFACES;
unsigned INVALID_TEXTURE;

static unsigned lightmap_texture;
static R_Texture textures[200];

unsigned BSP_CreateVertexBuffer()
{
	unsigned gl_buffer = 0;
	glCreateBuffers(1, &gl_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(drawVert_t) * numDrawVerts, drawVerts, GL_STATIC_DRAW);

	return gl_buffer;
}

unsigned BSP_CreateIndexBuffer()
{
	unsigned gl_buffer = 0;
	glCreateBuffers(1, &gl_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * numDrawIndexes, drawIndexes, GL_STATIC_DRAW);

	return gl_buffer;
}
void BSP_CreateTextures()
{
	memset(textures, 0, sizeof(textures));

	char buffer[256];
	
	for (int i = 0; i < numShaders; i++)
	{
		memset(buffer, 0, sizeof(buffer));

		sprintf(buffer, "assets/bsp/%s.jpg", dshaders[i].shader);

		if (!strcmp(dshaders[i].shader, "noshader"))
		{
			continue;
		}

		textures[i] = Texture_Load(buffer, NULL);

		if (textures[i].id == 0)
		{
			memset(buffer, 0, sizeof(buffer));

			sprintf(buffer, "assets/bsp/%s.tga", dshaders[i].shader);

			textures[i] = Texture_Load(buffer, NULL);
		}
	}

	if (lightBytes > 0)
	{
		glGenTextures(1, &lightmap_texture);
		glBindTexture(GL_TEXTURE_2D, lightmap_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, lightBytes);

			//set texture wrap and filter modes
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture.format.wrapS);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture.format.wrapT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.format.filterMin);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.format.filterMax);
	}

	glGenTextures(1, &INVALID_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, INVALID_TEXTURE);

	unsigned data = 0xffffffff;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &data);
}
typedef  struct {
	unsigned  count;
	unsigned  instanceCount;
	unsigned  firstIndex;
	int  baseVertex;
	unsigned  baseInstance;
} DrawElementsIndirectCommand;
void BSP_InitDrawCmds()
{
	DrawElementsIndirectCommand* s = malloc(sizeof(DrawElementsIndirectCommand) * numDrawSurfaces);

	if (!s)
	{
		return;
	}
	memset(s, 0, sizeof(DrawElementsIndirectCommand) * numDrawSurfaces);

	int index = 0;
	for (int i = 0; i < numDrawSurfaces; i++)
	{
		dsurface_t* surface = &drawSurfaces[i];

		if (surface->surfaceType != 1) continue;

		s[index].baseInstance = 0;
		s[index].baseVertex = surface->firstVert;
		s[index].count = surface->numIndexes;
		s[index].instanceCount = 1;
		s[index].firstIndex = surface->firstIndex;
		index++;

		VALID_SURFACES++;
	}

	glCreateBuffers(1, &DRAW_COMMANDS);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, DRAW_COMMANDS);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * VALID_SURFACES, s, GL_STATIC_DRAW);

	free(s);
}
int numClusters;
int clusterBytes;
int    LittleLong(int l)
{
	byte    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}
void BSP_InitAllRenderData()
{
	glGenVertexArrays(1, &BSP_VAO);
	glBindVertexArray(BSP_VAO);
	BSP_VBO = BSP_CreateVertexBuffer();
	BSP_EBO = BSP_CreateIndexBuffer();

	glBindBuffer(GL_ARRAY_BUFFER, BSP_VBO);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(drawVert_t), (void*)offsetof(drawVert_t, xyz));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(drawVert_t), (void*)offsetof(drawVert_t, st));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(drawVert_t), (void*)offsetof(drawVert_t, lightmap));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BSP_EBO);
	BSP_InitDrawCmds();
	BSP_CreateTextures();
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(drawVert_t), (void*)offsetof(drawVert_t, st));

	BSP_Shader = Shader_CompileFromFile("src/shaders/bsp/bsp_shader.vs", "src/shaders/bsp/bsp_shader.fs", NULL);

	glUseProgram(BSP_Shader);
	Shader_SetInteger(BSP_Shader, "tex_image", 0);
	Shader_SetInteger(BSP_Shader, "light_map", 1);

	numClusters = ((int*)visBytes)[0];
	clusterBytes = ((int*)visBytes)[1];
}


static int BSP_FindLeaf(vec3 point)
{
	int index = 0;
	dnode_t* node = NULL;
	dplane_t* plane = NULL;
	float dist = 0;
	while (index >= 0)
	{
		node = &dnodes[index];
		plane = &dplanes[node->planeNum];

		dist = glm_dot(point, plane->normal) - plane->dist;

		if (dist > 0)
		{
			index = node->children[0];
		}
		else
		{
			index = node->children[1];
		}
	}

	return -index - 1;
}


static const byte* BSP_ClusterPVS(int cluster) {
	if (numVisBytes == 0 || cluster < 0 || cluster >= numClusters) {
		return NULL;
	}

	return visBytes + cluster * clusterBytes;
}

bool BSP_IsClusterVisible(int viewCluster, int testCluster)
{
	if (viewCluster < 0) return false;
	if (testCluster < 0) return false;
	if (numVisBytes == 0) return false;

	byte visSet = visBytes[(viewCluster * clusterBytes) +
		(testCluster / 8)];;

	return visSet & (1 << (testCluster & 7));
}

void BSP_RenderAll(vec3 viewPos)
{
	vec3 copy;
	glm_vec3_copy(viewPos, copy);

	float temp = copy[1];
	copy[1] = copy[2];
	copy[2] = temp;

	glBindVertexArray(BSP_VAO);
	glUseProgram(BSP_Shader);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, DRAW_COMMANDS);
	//glMultiDrawElementsIndirect(GL_LINES, GL_UNSIGNED_INT, 0, VALID_SURFACES, sizeof(DrawElementsIndirectCommand));
	//glBindTextureUnit(1, lightmap_texture);

	int leafIndex = BSP_FindLeaf(copy);

	if (leafIndex == -1)
		return;

	int clusterIndex = dleafs[leafIndex].cluster;

	if (clusterIndex == -1)
		return;

	byte* vis = BSP_ClusterPVS(clusterIndex);

	if (!vis)
	{
		return;
	}

	for (int i = 0; i < numleafs; i++)
	{	
		dleaf_t* leaf = &dleafs[i];
		int otherCluster = leaf->cluster;

		if (otherCluster < 0 || otherCluster >= numClusters)
		{
			continue;
		}
		if (!(vis[otherCluster >> 3] & (1 << (otherCluster & 7))))
		{
			//continue;
		}
		if (!BSP_IsClusterVisible(clusterIndex, otherCluster))
		{
			//continue;
		}

		int surfIndex = leaf->firstLeafSurface;
		for (int j = 0; j < leaf->numLeafSurfaces; j++)
		{
			dsurface_t* surface = &drawSurfaces[surfIndex];
			surfIndex++;

			if (surface->surfaceType == 0 || surface->surfaceType == 2 || surface->surfaceType == 4)
			{
				continue;
			}

			if (textures[surface->shaderNum].id == 0)
			{
				glBindTextureUnit(0, INVALID_TEXTURE);
			}
			else
			{
				glBindTextureUnit(0, textures[surface->shaderNum].id);
			}

			glDrawElementsBaseVertex(GL_TRIANGLES, surface->numIndexes, GL_UNSIGNED_INT, (void*)(sizeof(unsigned) * surface->firstIndex), surface->firstVert);
		}

	}
}