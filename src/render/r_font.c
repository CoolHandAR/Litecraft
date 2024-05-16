#include "r_font.h"

#include "cJSON/cJSON.h"

#include "utility/u_utility.h"
#include <string.h>
#include <assert.h>


R_FontData R_loadFontData(const char* p_jsonPath, const char* p_textureFile)
{
	R_FontData font_data;
	memset(&font_data, 0, sizeof(R_FontData));

	//load font texture
	font_data.texture = Texture_Load(p_textureFile, NULL);

	//read from the json file
	char* json_char_data = File_ParseString(p_jsonPath);

	if (!json_char_data)
	{
		return font_data;
	}
    //try to load json
    cJSON* monitor_json = cJSON_Parse(json_char_data);
    if (monitor_json == NULL)
    {
        const char* error_ptr = cJSON_GetErrorPtr();
        fprintf(stderr, "Error before: %s \n", error_ptr);
		free(json_char_data);
        return font_data;
    }

    //Parse atlas data
    const cJSON* atlas_name = cJSON_GetObjectItem(monitor_json, "atlas");
    if (atlas_name && atlas_name->child)
    {
        const cJSON* child = atlas_name->child;
		while (child)
		{
			if (!strcmp(child->string, "distanceRange"))
			{
				font_data.atlas_data.distance_range = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "size"))
			{
				font_data.atlas_data.size = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "width"))
			{
				font_data.atlas_data.width = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "height"))
			{
				font_data.atlas_data.height = cJSON_GetNumberValue(child);
			}

			child = child->next;
		}
    }
    //Parse metrics
	const cJSON* metrics = cJSON_GetObjectItem(monitor_json, "metrics");
	if (metrics && metrics->child)
	{
		const cJSON* child = metrics->child;
		while (child)
		{
			if (!strcmp(child->string, "emSize"))
			{
				font_data.metrics_data.em_size = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "lineHeight"))
			{
				font_data.metrics_data.line_height = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "ascender"))
			{
				font_data.metrics_data.ascender = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "descender"))
			{
				font_data.metrics_data.descender = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "underlineY"))
			{
				font_data.metrics_data.underline_y = cJSON_GetNumberValue(child);
			}
			else if (!strcmp(child->string, "underlineThickness"))
			{
				font_data.metrics_data.underline_thickness = cJSON_GetNumberValue(child);
			}

			child = child->next;
		}
	}
	//Parse glyphs
	const cJSON* glyphs = cJSON_GetObjectItem(monitor_json, "glyphs");
	if (glyphs && glyphs->child && cJSON_IsArray(glyphs))
	{
		int array_size = cJSON_GetArraySize(glyphs);
		
		for (int i = 0; i < array_size; i++)
		{
			const cJSON* array_child = cJSON_GetArrayItem(glyphs, i);

			if (!array_child)
				break;

			if (array_child->child)
				array_child = array_child->child;

			while (array_child)
			{
				if (!strcmp(array_child->string, "unicode"))
				{
					font_data.glyphs_data[i].unicode = cJSON_GetNumberValue(array_child);
				}
				else if (!strcmp(array_child->string, "advance"))
				{
					font_data.glyphs_data[i].advance = cJSON_GetNumberValue(array_child);
				}
				else if (!strcmp(array_child->string, "planeBounds"))
				{
					const cJSON* nested_child = array_child->child;

					for (int j = 0; j < 4; j++)
					{
						if (!nested_child)
							break;

						if (!strcmp(nested_child->string, "left"))
						{
							font_data.glyphs_data[i].plane_bounds.left = cJSON_GetNumberValue(nested_child);
						}
						else if (!strcmp(nested_child->string, "bottom"))
						{
							font_data.glyphs_data[i].plane_bounds.bottom = cJSON_GetNumberValue(nested_child);
						}
						else if (!strcmp(nested_child->string, "right"))
						{
							font_data.glyphs_data[i].plane_bounds.right = cJSON_GetNumberValue(nested_child);
						}
						else if (!strcmp(nested_child->string, "top"))
						{
							font_data.glyphs_data[i].plane_bounds.top = cJSON_GetNumberValue(nested_child);
						}

						nested_child = nested_child->next;

					}
				}
				else if (!strcmp(array_child->string, "atlasBounds"))
				{
					const cJSON* nested_child = array_child->child;

					for (int j = 0; j < 4; j++)
					{
						if (!nested_child)
							break;

						if (!strcmp(nested_child->string, "left"))
						{
							font_data.glyphs_data[i].atlas_bounds.left = cJSON_GetNumberValue(nested_child);
						}
						else if (!strcmp(nested_child->string, "bottom"))
						{
							font_data.glyphs_data[i].atlas_bounds.bottom = cJSON_GetNumberValue(nested_child);
						}
						else if (!strcmp(nested_child->string, "right"))
						{
							font_data.glyphs_data[i].atlas_bounds.right = cJSON_GetNumberValue(nested_child);
						}
						else if (!strcmp(nested_child->string, "top"))
						{
							font_data.glyphs_data[i].atlas_bounds.top = cJSON_GetNumberValue(nested_child);
						}

						nested_child = nested_child->next;
					}
				}
				array_child = array_child->next;
			}
		}
	}

    cJSON_Delete(monitor_json);
	free(json_char_data);

	return font_data;
}

R_FontGlyphData R_getFontGlyphData(const R_FontData* const p_fontData, char p_ch)
{
	//glyphs start with unicode 32 and so that means that 0 index of the array is 32
	//so we subtract the unicode of the char with 32 to get the index of our array
	int glyph_index = (int)p_ch - 32;
	
	assert(glyph_index >= 0 && glyph_index < MAX_FONT_GLYPHS && "Error when retreaving font glyph data");

	//glyph not found? return the glyph of a question mark 
	if (glyph_index < 0 || glyph_index >= MAX_FONT_GLYPHS)
	{
		char qstmark_ch = '?';
		return R_getFontGlyphData(p_fontData, qstmark_ch);
	}
	

	return p_fontData->glyphs_data[glyph_index];
}
