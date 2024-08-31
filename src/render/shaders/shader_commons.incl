
vec3 depthToViewPos(sampler2D depth_texture, mat4 inverseProj, vec2 coords)
{
    float depth = textureLod(depth_texture, coords, 0.0).r;
 
    vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_space = inverseProj * ndc;

    view_space.xyz = view_space.xyz / view_space.w;

    return view_space.xyz;
}

float depthToViewSpace(sampler2D depth_texture, mat4 inverseProj, vec2 coords)
{
    float depth = textureLod(depth_texture, coords, 0).r;

    float ndc_depth = depth * 2.0 - 1.0;

    float w_component = 1.0;

    float viewSpaceDepth = inverseProj[2][2] * ndc_depth + inverseProj[3][2] * w_component;
    float viewSpaceW = inverseProj[2][3] * ndc_depth + inverseProj[3][3] * w_component;

    viewSpaceDepth /= viewSpaceW;
    
    return viewSpaceDepth;
}