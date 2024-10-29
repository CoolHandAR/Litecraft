#version 460 core

#define KERNEL_SAMPLES_SIZE 32

out float f_color;

in vec2 v_texCoords;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D noise_texture;
uniform sampler2D depth_texture;

uniform mat4 u_InverseProj;
uniform mat4 u_proj;
uniform mat4 u_InverseView;
uniform mat4 u_view;
uniform vec3 u_kernelSamples[KERNEL_SAMPLES_SIZE];

const vec3 kernel[32] = // precalculated hemisphere kernel (low discrepancy noiser)
	{
		vec3(-0.668154, -0.084296, 0.219458),
		vec3(-0.092521,  0.141327, 0.505343),
		vec3(-0.041960,  0.700333, 0.365754),
		vec3( 0.722389, -0.015338, 0.084357),
		vec3(-0.815016,  0.253065, 0.465702),
		vec3( 0.018993, -0.397084, 0.136878),
		vec3( 0.617953, -0.234334, 0.513754),
		vec3(-0.281008, -0.697906, 0.240010),
		vec3( 0.303332, -0.443484, 0.588136),
		vec3(-0.477513,  0.559972, 0.310942),
		vec3( 0.307240,  0.076276, 0.324207),
		vec3(-0.404343, -0.615461, 0.098425),
		vec3( 0.152483, -0.326314, 0.399277),
		vec3( 0.435708,  0.630501, 0.169620),
		vec3( 0.878907,  0.179609, 0.266964),
		vec3(-0.049752, -0.232228, 0.264012),
		vec3( 0.537254, -0.047783, 0.693834),
		vec3( 0.001000,  0.177300, 0.096643),
		vec3( 0.626400,  0.524401, 0.492467),
		vec3(-0.708714, -0.223893, 0.182458),
		vec3(-0.106760,  0.020965, 0.451976),
		vec3(-0.285181, -0.388014, 0.241756),
		vec3( 0.241154, -0.174978, 0.574671),
		vec3(-0.405747,  0.080275, 0.055816),
		vec3( 0.079375,  0.289697, 0.348373),
		vec3( 0.298047, -0.309351, 0.114787),
		vec3(-0.616434, -0.117369, 0.475924),
		vec3(-0.035249,  0.134591, 0.840251),
		vec3( 0.175849,  0.971033, 0.211778),
		vec3( 0.024805,  0.348056, 0.240006),
		vec3(-0.267123,  0.204885, 0.688595),
		vec3(-0.077639, -0.753205, 0.070938)
	};


vec3 calcViewPos(vec2 coords)
{
    float depth = textureLod(depth_texture, coords, 0).r;

    vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_space = u_InverseProj * ndc;

    view_space.xyz = view_space.xyz / view_space.w;

    return view_space.xyz;
}
vec3 calcWorldPos(vec2 coords, float depth)
{   
    vec2 uv = coords * textureSize(depth_texture, 0).xy;

    vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, 1.0, 1.0) * 0.1;

    vec4 view_space = u_InverseProj * ndc;

    return view_space.xyz * depth;
}

float calcViewSpaceDepth(vec2 coords)
{
    float depth = textureLod(depth_texture, coords, 0).r;

    float ndc_depth = depth * 2.0 - 1.0;

    float w_component = 1.0;

    float viewSpaceDepth = u_InverseProj[2][2] * ndc_depth + u_InverseProj[3][2] * w_component;
    float viewSpaceW = u_InverseProj[2][3] * ndc_depth + u_InverseProj[3][3] * w_component;

    viewSpaceDepth /= viewSpaceW;
    
    return viewSpaceDepth;
}

vec3 normalFromViewPos(vec3 viewPos)
{
    return normalize(cross(dFdx(viewPos), dFdy(viewPos)));
}
vec3 accurateNormalFromDepth(vec2 coords)
{   
    float c0 = textureLod(depth_texture, coords, 0).r;
    float l2 = textureLod(depth_texture, coords - vec2(2,0), 0).r;
    float l1 = textureLod(depth_texture, coords - vec2(1,0), 0).r;
    float r1 = textureLod(depth_texture, coords + vec2(1,0), 0).r;
    float r2 = textureLod(depth_texture, coords + vec2(2,0), 0).r;
    float b2 = textureLod(depth_texture, coords - vec2(0,2), 0).r;
    float b1 = textureLod(depth_texture, coords - vec2(0,1), 0).r;
    float t1 = textureLod(depth_texture, coords + vec2(0,1), 0).r;
    float t2 = textureLod(depth_texture, coords + vec2(0,2), 0).r;

    float dl = abs(l1*l2/(2.0*l2-l1)-c0);
    float dr = abs(r1*r2/(2.0*r2-r1)-c0);
    float db = abs(b1*b2/(2.0*b2-b1)-c0);
    float dt = abs(t1*t2/(2.0*t2-t1)-c0);

    vec3 ce = calcWorldPos(coords, c0);

    vec3 dpdx = (dl < dr) ? ce - calcWorldPos(coords - vec2(1,0), l1) :
                           -ce + calcWorldPos(coords + vec2(1,0), r1);

    vec3 dpdy = (db < dt) ? ce - calcWorldPos(coords - vec2(0,1), b1) :
                           -ce + calcWorldPos(coords + vec2(0,1), t1);                         

   return normalize(cross(dpdx, dpdy));
}
vec3 viewSpacePosAtScreenUV(vec2 uv)
{
     float depth = textureLod(depth_texture, uv, 0).r;
    vec3 view_space = (u_InverseProj * (vec4(uv * 2.0 - 1.0, 1.0, 1.0))).xyz;

    return calcViewPos(uv);
}

vec3 viewSpacePosAtPixelPosition(vec2 vpos)
{
    vec2 uv = vpos * textureSize(depth_texture, 0);
    return viewSpacePosAtScreenUV(uv);
}

vec3 accurateNormalFromDepth2(vec2 coords)
{   
    vec2 depth_size = textureSize(depth_texture, 0) / 2;

    // screen uv from vpos
    vec2 uv = coords;

    // current pixel's depth
    float c = textureLod(depth_texture, uv, 0).r;

    // get current pixel's view space position
    vec3 viewSpacePos_c = viewSpacePosAtScreenUV(uv);

    // get view space position at 1 pixel offsets in each major direction
    vec3 viewSpacePos_l = viewSpacePosAtScreenUV(uv + vec2(-1.0, 0.0) * depth_size);
    vec3 viewSpacePos_r = viewSpacePosAtScreenUV(uv + vec2( 1.0, 0.0) * depth_size);
    vec3 viewSpacePos_d = viewSpacePosAtScreenUV(uv + vec2( 0.0,-1.0) * depth_size);
    vec3 viewSpacePos_u = viewSpacePosAtScreenUV(uv + vec2( 0.0, 1.0) * depth_size);

    // get the difference between the current and each offset position
    vec3 l = viewSpacePos_c - viewSpacePos_l;
    vec3 r = viewSpacePos_r - viewSpacePos_c;
    vec3 d = viewSpacePos_c - viewSpacePos_d;
    vec3 u = viewSpacePos_u - viewSpacePos_c;

    // get depth values at 1 & 2 pixels offsets from current along the horizontal axis
    vec4 H = vec4(
        textureLod(depth_texture, uv - vec2(1 / depth_size.x, 0), 0).r,
         textureLod(depth_texture, uv + vec2(1 / depth_size.x, 0), 0).r,
         textureLod(depth_texture, uv - vec2(2 / depth_size.x, 0), 0).r,
         textureLod(depth_texture, uv + vec2(2 / depth_size.x, 0), 0).r
    );

    // get depth values at 1 & 2 pixels offsets from current along the vertical axis
    vec4 V = vec4(
         textureLod(depth_texture, uv - vec2(0, 1 / depth_size.y), 0).r,
         textureLod(depth_texture, uv + vec2(0, 1 / depth_size.y), 0).r,
         textureLod(depth_texture, uv - vec2(0, 2 / depth_size.y), 0).r,
         textureLod(depth_texture, uv + vec2(0, 2 / depth_size.y), 0).r
    );

    // current pixel's depth difference from slope of offset depth samples
    // differs from original article because we're using non-linear depth values
    // see article's comments
    vec2 he = abs(H.xy * H.zw * (1 / (2 * H.zw * H.xy)) - 1/c);
    vec2 ve = abs(V.xy * V.zw * (1 / (2 * V.zw * V.xy)) - 1/c);

    // pick horizontal and vertical diff with the smallest depth difference from slopes
    vec3 hDeriv = he.x < he.y ? l : r;
    vec3 vDeriv = ve.x < ve.y ? d : u;

    // get view space normal from the cross product of the best derivatives
    vec3 viewNormal = normalize(cross(hDeriv, vDeriv));

    return viewNormal;
}

vec3 accurateNormalFromDepth3(vec2 vpos)
{
 // get current pixel's view space position
    vec3 viewSpacePos_c = viewSpacePosAtPixelPosition(vpos + vec2( 0.0, 0.0));

    // get view space position at 1 pixel offsets in each major direction
    vec3 viewSpacePos_r = viewSpacePosAtPixelPosition(vpos + vec2( 1.0, 0.0));
    vec3 viewSpacePos_u = viewSpacePosAtPixelPosition(vpos + vec2( 0.0, 1.0));

    // get the difference between the current and each offset position
    vec3 hDeriv = viewSpacePos_r - viewSpacePos_c;
    vec3 vDeriv = viewSpacePos_u - viewSpacePos_c;

    // get view space normal from the cross product of the diffs
    vec3 viewNormal = normalize(cross(hDeriv, vDeriv));

    return viewNormal;
}

float radius = 0.6;
float bias = 0.025;
const vec2 noiseScale = vec2(640.0/4.0, 360.0/4.0); 
void main()
{   
    vec3 fragPos = calcViewPos(v_texCoords).xyz;
    vec3 world_pos = (u_InverseView * vec4(fragPos, 1.0)).xyz;
    vec3 normal = texture(g_normal, v_texCoords).xyz * 2.0 - 1.0;
    vec3 randomVec = normalize(texture(noise_texture, v_texCoords * noiseScale).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < KERNEL_SAMPLES_SIZE; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * kernel[i]; // from tangent to view-space
        samplePos = world_pos + samplePos * radius; 
        samplePos = (u_view * vec4(samplePos, 1.0)).xyz;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = u_proj * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xy = offset.xy * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get sample depth
        float sampleDepth = calcViewSpaceDepth(offset.xy); // get depth value of kernel sample

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;        
    }
    occlusion = 1.0 - (occlusion / KERNEL_SAMPLES_SIZE);
    
   f_color = pow(occlusion, 2);
}   