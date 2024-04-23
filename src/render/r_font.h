
#include <cglm/cglm.h>
#include "r_texture.h"

#define MAX_FONT_GLYPHS 100


typedef struct R_FontAtlasData
{
	int distance_range;
	float size;
	int width, height;
} R_FontAtlasData;
typedef struct R_FontMetricData
{
	int em_size;
	double line_height;
	double ascender;
	double descender;
	double underline_y;
	double underline_thickness;
} R_FontMetricData;

typedef struct R_FontBounds
{
	double left;
	double bottom;
	double right;
	double top;
} R_FontBounds;

typedef struct R_FontGlyphData
{
	unsigned unicode;
	double advance;
	R_FontBounds plane_bounds;
	R_FontBounds atlas_bounds;
} R_FontGlyphData;

typedef struct R_FontData
{
	R_Texture texture;
	R_FontAtlasData atlas_data;
	R_FontMetricData metrics_data;
	R_FontGlyphData glyphs_data[MAX_FONT_GLYPHS];
} R_FontData;

R_FontData R_loadFontData(const char* p_jsonPath, const char* p_textureFile);
R_FontGlyphData R_getFontGlyphData(const R_FontData* const p_fontData, char p_ch);