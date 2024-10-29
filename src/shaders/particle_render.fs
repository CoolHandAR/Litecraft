#version 460 core

out vec4 f_color;

in vec2 v_texCoords;
in vec4 v_color;
in vec2 v_frame;
in vec4 v_lightColor;
in vec3 v_lightPos;
in vec3 v_worldPos;

uniform sampler2D textureSampler;

void main(void)
{
    vec2 tex_coords_offsetted = vec2((fract(v_texCoords.x) + v_frame.x) / 5, -(fract(v_texCoords.y) + v_frame.y) / 5);
    vec4 tex_color = texture(textureSampler, tex_coords_offsetted);

    //check for texture transparenancy
   // if(tex_color.a < 0.1)
		  //discard;


    float dist = length(v_lightPos - v_worldPos);
    float attenuation = (1.0 / (1.0 + (0.1 * dist * dist)));

    vec3 lightColor = mix(vec3(1.0), v_lightColor.rgb, 1);

    //f_color = vec4(v_lightColor.rgb, 1.0) * tex_color;
    vec3 color = lightColor;
    color *= tex_color.rgb;

    f_color = vec4(color.rgb, 1.0);
}
