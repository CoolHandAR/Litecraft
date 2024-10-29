#version 450 core

layout(location = 0) out vec4 FragColor;

struct VecOutput
{
   vec2 v_texCoords;
   vec2 v_texOffset;
   float v_ao;
};

in float v_Depth;

layout(location = 0) in VecOutput v_Output;

uniform sampler2D texture_atlas;


void main()
{
   vec4 tex_color = vec4(0.0);
	vec2 coords = vec2(0.0);
   vec2 tex_coords_offsetted = vec2(0.0);
   
    if(v_Depth < 0.0)
    {
        discard;
    }

  

   coords = v_Output.v_texCoords;

   coords = fract(coords);

   tex_coords_offsetted = vec2((coords.x + v_Output.v_texOffset.x) / 25, -(coords.y + v_Output.v_texOffset.y) / 25);
   
   tex_color = texture(texture_atlas, tex_coords_offsetted);

    FragColor = vec4(tex_color.rgb, 1.0);

    
}