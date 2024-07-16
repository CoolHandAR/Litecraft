#pragma once

#include <cglm/cglm.h>

typedef struct
{
    vec3 direction;

    vec3 color;
    float ambient_intensity;
    float specular_intensity;
} DirLight;

typedef struct
{
    vec3 position;

    vec3 color;
    float ambient_intensity;
    float specular_intensity;

    float linear;
    float quadratic;
    float radius;
    float constant;
} PointLight;
typedef struct
{
    vec3 position;
    vec3 direction;

    vec3 color;
    float ambient_intensity;
    float specular_intensity;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
} SpotLight;
