#version 460 core

#define KERNEL_SAMPLES_SIZE 64

out vec4 f_color;

in vec2 v_texCoords;
in vec2  v_viewRay;

uniform sampler2D depth_texture;
uniform sampler2D noise_texture;

uniform mat4 u_InverseProj;
uniform mat4 u_proj;
uniform vec3 u_kernelSamples[KERNEL_SAMPLES_SIZE];

float calcViewZ(vec2 coords)
{
    float depth = texture(depth_texture, coords).r;

    float view_z = u_proj[3][2] / (2 * depth - 1 - u_proj[2][2]);

    return view_z;
}


vec3 calcViewPos(vec2 coords)
{
    float depth = texture(depth_texture, coords).r;
 
    vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_space = u_InverseProj * ndc;

    view_space.xyz = view_space.xyz / view_space.w;

    return view_space.xyz;
}   

vec3 depthToNormal(vec2 tex_coords)
{
    const vec2 offset1 = vec2(0.0, 0.001);
    const vec2 offset2 = vec2(0.001, 0.0);

    float depth0 = texture(depth_texture, tex_coords).r;
    float depth1 = texture(depth_texture, tex_coords + offset1).r;
    float depth2 = texture(depth_texture, tex_coords + offset2).r;

    vec3 p1 = vec3(offset1, depth1 - depth0);
    vec3 p2 = vec3(offset2, depth2 - depth0);

    vec3 normal = cross(p1, p2);
    normal.z = -normal.z;

    return normalize(normal);
}

float radius = 0.5;
float bias = 0.025;
const vec2 noiseScale = vec2(1024.0/4.0, 720.0/4.0); 
void main()
{   
    float view_z = calcViewZ(v_texCoords);
    //vec3 view_pos = vec3(v_viewRay.x * view_z, v_viewRay.y * view_z, view_z);
    vec3 view_pos = calcViewPos(v_texCoords);

    vec3 view_normal = cross(dFdy(view_pos.xyz), dFdx(view_pos.xyz));
    view_normal = normalize(view_normal * -1.0);

    vec3 noise_vec = texture(noise_texture, v_texCoords * noiseScale).xyz;

    vec3 tangent = normalize(noise_vec - view_normal * dot(noise_vec, view_normal));
    vec3 biTangent = cross(view_normal, tangent);
    mat3 TBN = mat3(tangent, biTangent, view_normal);
    float occlusion_factor = 0.0;

    for(int i = 0; i < KERNEL_SAMPLES_SIZE; i++)
    {
        vec3 sample_pos = TBN * u_kernelSamples[i];

        sample_pos = view_pos + sample_pos * radius;

        vec4 offset = vec4(sample_pos, 1.0);
        offset = u_proj * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * vec2(0.5) + vec2(0.5);

        float depth = calcViewPos(offset.xy).z;

        float range_check = smoothstep(0.0, 1.0, radius / abs(view_pos.z - depth));
        occlusion_factor += (depth >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;
    }
    occlusion_factor = 1.0 - (occlusion_factor / KERNEL_SAMPLES_SIZE);

    f_color = vec4(pow(occlusion_factor, 3), 1, 1, 1);
}   