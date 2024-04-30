#version 450 core

out vec4 f_color;

in vec2 v_TexCoords;

uniform sampler2D u_position;
uniform sampler2D u_normal;
uniform sampler2D u_colorSpec;

struct Light
{
    vec3 position;
    vec3 color;

    float linear;
    float quadratic;
    float radius;
};

const int MAX_LIGHTS = 1;
uniform Light u_light;
uniform vec3 u_viewPos;


void main()
{
    //Gbuffer data
    vec3 g_frag_pos = texture(u_position, v_TexCoords).rgb;
    vec3 g_normal = texture(u_normal, v_TexCoords).rgb;
    vec3 g_diffuse = texture(u_colorSpec, v_TexCoords).rgb;
    float g_specular = texture(u_colorSpec, v_TexCoords).a;

    //calc lighting
    vec3 lighting = g_diffuse * 0.1;
    vec3 view_dir = normalize(u_viewPos - g_frag_pos);
   
        float distance = length(u_light.position - g_frag_pos);

        
            //diffuse
            vec3 light_dir = normalize(u_light.position - g_frag_pos);
            vec3 diffuse = max(dot(g_normal, light_dir), 0.0) * g_diffuse * u_light.color;
            // specular
            vec3 halfway_dir = normalize(light_dir + view_dir);  
            float spec = pow(max(dot(g_normal, halfway_dir), 0.0), 16.0);
            vec3 specular = u_light.color * spec * g_specular;
            //attenuation
            float attenuation = 1.0 / (1.0 + u_light.linear * distance + u_light.quadratic * distance * distance);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;   
        
    
    f_color = vec4(lighting, 1.0);
}
