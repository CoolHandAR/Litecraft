//#version 460

struct CameraData
{
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
    vec4 frustrum_planes[6];
    vec4 position;
    ivec2 screen_size;

    float z_near;
    float z_far;
};

struct SceneData
{
    uint numPointLights;
    uint numSpotLights;
};

layout (std140, binding = 0) uniform Scene
{
	SceneData scene;
};
layout (std140, binding = 1) uniform Camera
{
	CameraData cam;
};