#version 460 core

out vec4 FragColor;

in vec2 v_texCoords;

uniform sampler2D depth_texture;
uniform sampler2D noise_texture;
uniform sampler2D focus_texture;
uniform sampler2D unFocus_texture;

uniform float u_minDistance;
uniform float u_maxDistance;
uniform float u_focusInfluence;
uniform vec2 u_focusPoint;

uniform mat4 u_InverseProj;
uniform mat4 u_InverseView;

vec3 calcViewPos(vec2 coords)
{
    float depth = textureLod(depth_texture, coords, 0).r;

    vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_space = u_InverseProj * ndc;

    view_space.xyz = view_space.xyz / view_space.w;

    return view_space.xyz;
}


void main()
{  
    vec3 fragPos = calcViewPos(v_texCoords);
    vec3 focusPos = calcViewPos(u_focusPoint);
    vec3 focusColor = texture(focus_texture, v_texCoords).rgb;
    vec3 unFocusColor = texture(unFocus_texture, v_texCoords).rgb;

    float blur = smoothstep(u_minDistance, u_maxDistance, abs(fragPos.z - (focusPos.z * u_focusInfluence)));

    vec3 color = mix(focusColor, unFocusColor, blur);

    //float fog = clamp((abs(fragPos.z)) / (200), 0.00, 0.99);

    //color = mix(color, vec3(1, 1, 1), fog);

    FragColor = vec4(color, 1.0);
}   