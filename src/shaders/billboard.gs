#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};

out vec2 v_texCoords;

uniform vec3 u_viewPos;
uniform float u_billboardSize;

void main()
{
    vec3 position = gl_in[0].gl_Position.xyz;
    vec3 to_camera = normalize(u_viewPos - position);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(to_camera, up) * u_billboardSize;

    position -= right;                                                                   
    gl_Position = g_viewProjectionMatrix * vec4(position, 1.0);                                             
    v_texCoords = vec2(0.0, 0.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    position.y += u_billboardSize;                                                        
    gl_Position = g_viewProjectionMatrix * vec4(position, 1.0);                                             
   v_texCoords = vec2(0.0, 1.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    position.y -= u_billboardSize;                                                        
    position += right;                                                                   
    gl_Position = g_viewProjectionMatrix * vec4(position, 1.0);                                             
    v_texCoords= vec2(1.0, 0.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    position.y += u_billboardSize;                                                        
    gl_Position = g_viewProjectionMatrix * vec4(position, 1.0);                                             
    v_texCoords = vec2(1.0, 1.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    EndPrimitive();              
}