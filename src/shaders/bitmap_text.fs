#version 460 core

in VS_Out
{
    vec2 tex_coords;
    vec4 color;
} fs_in;

out vec4 f_color;

uniform sampler2D font_atlas_texture;

float screenPxRange() 
{
    const float pxRange = 2;
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(font_atlas_texture, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(fs_in.tex_coords);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b) 
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    vec3 font_color = texture(font_atlas_texture, fs_in.tex_coords).rgb;

    float sd = median(font_color.r, font_color.g, font_color.b);
    float screen_px_distance = screenPxRange() * (sd - 0.5);
    float opacity = clamp(screen_px_distance + 0.5, 0.0, 1.0);

    if(opacity < 0.1)
        discard;

    f_color = mix(f_color, fs_in.color, opacity);

    if(f_color.a < 0.1)
        discard;

}