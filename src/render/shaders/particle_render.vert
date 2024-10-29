#version 460 core 

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_texCoords;

#include "scene_incl.incl"
#include "particles_common.incl"

layout(std430, binding = 5) readonly restrict buffer ParticleEmittersBuffer
{
    ParticleEmitter data[];
} emitters;

layout(std430, binding = 7) readonly restrict buffer TransformsBuffer
{
    vec4 data[];
} instances;

out VS_OUT
{
    vec2 TexCoords;
    vec3 LightColor;
    vec4 Color;
    flat int TexIndex;
} vs_out;


vec3 calculate_light(vec3 light_color, vec3 light_dir, vec3 normal, vec3 view_dir, float ambient_intensity, float diffuse_intensity, float specular_intensity, float attenuation)
{
    //AMBIENT
    vec3 ambient = light_color * ambient_intensity;
    //DIFFUSE
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    float diffuse = diffuse_factor * diffuse_intensity;
    //SPECULAR
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float specular_factor = pow(max(dot(normal, halfway_dir), 0.0), 64);
    float specular = specular_factor * specular_intensity;

    return vec3(ambient + diffuse + specular);
}

void main()
{
     uint read_index = gl_InstanceID * (3 + 1 + 1); //xform + color + custom

     mat4 xform;
     vec4 color;
     vec4 custom;

    //read matrix data from the buffer
    xform[0] = instances.data[read_index + 0];
    xform[1] = instances.data[read_index + 1];
    xform[2] = instances.data[read_index + 2];
    xform[3] = vec4(0.0, 0.0, 0.0, 1.0);

    xform = transpose(xform);

    //color
    color = instances.data[read_index + 3];

    //custom 
    custom = instances.data[read_index + 4];

    uint emitter_index = uint(custom.x);

#define EMITTER emitters.data[emitter_index]

    uint frame = uint(EMITTER.frame) + EMITTER.h_frames;

    vec2 frameOffset = vec2(frame % EMITTER.h_frames, frame / EMITTER.h_frames);

    vec2 coords = a_texCoords;

    coords.x += frameOffset.x;
    coords.y += -frameOffset.y;

    coords.x /= EMITTER.h_frames;
    coords.y /= EMITTER.v_frames;

    vs_out.TexCoords = vec2((coords.x), ((coords.y)));
    vs_out.Color = color;
    vs_out.TexIndex = EMITTER.texture_index;

    mat3 normalMatrix = transpose(inverse(mat3(xform)));
    vec3 normal = normalMatrix * vec3(0, 0, 1);
    vec4 worldPos = xform * vec4(a_Pos, 0.0, 1.0);
    vec4 viewSpace = cam.view * worldPos;
    vec3 viewDir = normalize(cam.position.xyz - worldPos.xyz);
    vec3 lightColor = calculate_light(scene.dirLightDirection.xyz * scene.dirLightAmbientIntensity, normalize(scene.dirLightDirection.xyz), viewDir, normal, EMITTER.ambient_intensity, EMITTER.diffuse_intensity, EMITTER.specular_intensity, 1);

    vs_out.LightColor = lightColor;

    gl_Position = cam.viewProjection * worldPos;
}