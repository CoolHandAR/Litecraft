#version 460 core 

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_texCoords;

out vec2 v_texCoords;
out vec4 v_color;
out vec2 v_frame;
out vec4 v_lightColor;
out vec3 v_lightPos;
out vec3 v_worldPos;


layout (std140, binding = 0) uniform Camera
{
	mat4 g_viewProjectionMatrix;
};
layout(std430, binding = 12) readonly restrict buffer Transform
{
    vec4 data[];
} instances;

struct DirLight 
{
    vec3 direction;
	
    vec3 color;
    float ambient_intensity;
    float diffuse_intensity;
};

uniform DirLight u_dirLight;
uniform vec3 u_viewPos;
uniform mat4 u_view;

vec3 calculate_light(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, float specular_intensity, float attenuation)
{
    //AMBIENT
    vec3 ambient = light_color;
    //DIFFUSE
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light_color * diffuse_factor;
    //SPECULAR
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 64);
    vec3 specular = light_color * specular_factor * specular_intensity;

    //APPLY ATTENUATION
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient * (diffuse + specular));
}

vec3 calculate_light2(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, float specular_intensity, float attenuation)
{

  //AMBIENT
    vec3 ambient = light_color;
    //DIFFUSE
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light_color * diffuse_factor;
    //SPECULAR
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 64);
    vec3 specular = light_color * specular_factor * specular_intensity;

    //APPLY ATTENUATION
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

void main()
{
    uint read_index = gl_InstanceID * (3 + 1 + 1); //xform + color + custom

    //read matrix data from the buffer
    mat4 xform;
    xform[0] = instances.data[read_index + 0];
    xform[1] = instances.data[read_index + 1];
    xform[2] = instances.data[read_index + 2];
    xform[3] = vec4(0.0, 0.0, 0.0, 1.0);

    xform = transpose(xform);

    //Color
    v_color = instances.data[read_index + 3];

    //Custom
    vec4 custom = instances.data[read_index + 4];
    
    int frame = int(custom.a);
    int h_Frames = 5;

    v_frame = vec2(frame % h_Frames, frame / h_Frames);


    v_texCoords = -a_texCoords;


     xform[0].xyz *= 5;
    xform[1].xyz *= 5;
    xform[2].xyz *= 5;
    xform[3].xyz -= 5;

    vec3 worldPos = (xform * vec4(a_Pos, 1.0)).xyz;

    
    
    vec3 normal = vec3(0, 0, 0);
    vec3 viewPos = (u_view * vec4(worldPos, 1.0)).xyz;
    vec3 viewDir = normalize(-viewPos);
    mat3 normalMatrix = transpose(inverse(mat3(xform)));
    normal = normalMatrix * a_Normal;

    vec3 light_pos = vec3(0.0);
    float dist = length(light_pos - worldPos);
    float attenuation = (1.0 / (1.0 + (0.1 * dist * dist)));
    v_lightColor.rgb = calculate_light2(u_dirLight.color * 5, normalize(u_dirLight.direction),  normal, normalize(u_viewPos - worldPos), 1, 1).rgb;
   // v_lightColor.rgb = mix(vec3(1.0), v_lightColor.rgb, attenuation);
    v_lightPos = light_pos;
    v_worldPos = worldPos;
    //v_lightColor *= 0.001;
    //xform = mat4(1.0);

    gl_Position = g_viewProjectionMatrix * xform * vec4(a_Pos, 1.0);

                
}
