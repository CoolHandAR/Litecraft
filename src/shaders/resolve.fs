#version 460 core

out vec4 f_color;

in vec2 v_texCoords;

uniform sampler2D specularBuffer;
uniform sampler2D diffuseBuffer;
uniform sampler2D ssrBuffer;



void main()
{
    vec4 specularColor = texture(specularBuffer, v_texCoords);
    vec4 diffuseColor = texture(diffuseBuffer, v_texCoords);
    vec4 ssrColor = texture(ssrBuffer, v_texCoords);
    
   // specularColor.rgb = mix(specularColor.rgb, ssrColor.rgb * specularColor.a, ssrColor.a);

    vec3 finalColor = diffuseColor.rgb;

    f_color = vec4(finalColor.rgb, 1.0);
}