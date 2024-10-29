#version 460 core

#define MAX_THICKNESS 0.001

out vec4 fragColor;

in vec2 v_texCoords;

uniform sampler2D depth_texture;
uniform sampler2D g_normal;
uniform sampler2D g_color;
uniform mat4 u_proj;
uniform mat4 u_InverseProj;
uniform mat4 u_view;

const ivec2 screen_size = ivec2(1280 / 2, 720 / 2);

 
vec3 calcViewPos(vec2 coords)
{
    float depth = textureLod(depth_texture, coords, 0).r;

    vec4 ndc = vec4(coords.x * 2.0 - 1.0, coords.y * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_space = u_InverseProj * ndc;

    view_space.xyz = view_space.xyz / view_space.w;

    return view_space.xyz;
}

void ComputePosAndReflection(vec2 coords, vec3 normalInVs, out vec3 outSamplePosInTS, out vec3 outReflDirInTS, out float outMaxDistance)
{
    float sample_depth = textureLod(depth_texture, coords, 0).r;
    vec4 sample_pos_in_cs =  vec4(coords * 2.0 - 1.0, sample_depth, 1);
    sample_pos_in_cs.y *= -1;

    vec4 sample_pos_in_vs = u_InverseProj * sample_pos_in_cs;
    sample_pos_in_vs /= sample_pos_in_vs.w;

    vec3 v_cam_sample_pos_in = normalize(sample_pos_in_vs.xyz);
    vec4 reflection_in_vs = vec4(reflect(v_cam_sample_pos_in.xyz, normalInVs.xyz), 0);

    vec4 reflection_end_pos_in_vs = sample_pos_in_vs + reflection_in_vs * 1000;
    reflection_end_pos_in_vs /= (reflection_end_pos_in_vs.z < 0 ? reflection_end_pos_in_vs.z : 1);
    vec4 reflection_end_pos_in_cs = u_proj * vec4(reflection_end_pos_in_vs.xyz, 1.0);
    reflection_end_pos_in_cs /= reflection_end_pos_in_cs.w;
    
    vec3 reflection_dir = normalize((reflection_end_pos_in_cs - sample_pos_in_cs).xyz);

    //transform to texture space
    sample_pos_in_cs.xy *= vec2(0.5, -0.5);
    sample_pos_in_cs.xy += vec2(0.5, 0.5);

    reflection_dir.xy *= vec2(0.5, -0.5);

    //out
    outSamplePosInTS = sample_pos_in_cs.xyz;
    outReflDirInTS = reflection_dir;

    outMaxDistance = outReflDirInTS.x>=0 ? (1 - outSamplePosInTS.x)/outReflDirInTS.x  : -outSamplePosInTS.x/outReflDirInTS.x;
    outMaxDistance = min(outMaxDistance, outReflDirInTS.y<0 ? (-outSamplePosInTS.y/outReflDirInTS.y)  : ((1-outSamplePosInTS.y)/outReflDirInTS.y));
    outMaxDistance = min(outMaxDistance, outReflDirInTS.z<0 ? (-outSamplePosInTS.z/outReflDirInTS.z) : ((1-outSamplePosInTS.z)/outReflDirInTS.z));
}

void ComputePosAndReflection2(vec2 coords, vec3 normalInVs, out vec3 outSamplePosInTS, out vec3 outReflDirInTS, out float outMaxDistance)
{
    float sampleDepth = textureLod(depth_texture, coords, 0).r;
    vec4 samplePosInCS =  vec4(coords * 2.0 - 1.0, sampleDepth * 2.0 - 1.0, 1);
    //samplePosInCS.y *= -1;

    vec4 samplePosInVS = u_InverseProj * samplePosInCS;
    samplePosInVS.xyz /= samplePosInVS.w;

    vec3 vCamToSampleInVS = normalize(samplePosInVS.xyz);
    vec4 vReflectionInVS = vec4(reflect(vCamToSampleInVS.xyz,  normalInVs.xyz),0);

    vec4 vReflectionEndPosInVS = samplePosInVS + vReflectionInVS * 1000;
	vReflectionEndPosInVS /= (vReflectionEndPosInVS.z < 0 ? vReflectionEndPosInVS.z : 1);
    vec4 vReflectionEndPosInCS = u_proj * vec4(vReflectionEndPosInVS.xyz, 1);
    vReflectionEndPosInCS /= vReflectionEndPosInCS.w;
    vec3 vReflectionDir = normalize((vReflectionEndPosInCS - samplePosInCS).xyz);

    // Transform to texture space
    samplePosInCS.xy *= 0.5 + 0.5;
    vReflectionDir.xy *= 0.5 + 0.5;
    
    outSamplePosInTS = samplePosInCS.xyz;
    outReflDirInTS = vReflectionDir;
    
	// Compute the maximum distance to trace before the ray goes outside of the visible area.
    outMaxDistance = outReflDirInTS.x>=0 ? (1 - outSamplePosInTS.x)/outReflDirInTS.x  : -outSamplePosInTS.x/outReflDirInTS.x;
    outMaxDistance = min(outMaxDistance, outReflDirInTS.y<0 ? (-outSamplePosInTS.y/outReflDirInTS.y) : ((1-outSamplePosInTS.y)/outReflDirInTS.y));
    outMaxDistance = min(outMaxDistance, outReflDirInTS.z<0 ? (-outSamplePosInTS.z/outReflDirInTS.z) : ((1-outSamplePosInTS.z)/outReflDirInTS.z));
}

float FindIntersection_Linear(vec3 samplePosInTS, vec3 vReflDirInTS, float maxTraceDistance, out vec3 outIntersection)
{
    vec3 vReflectionEndPosInTS = samplePosInTS + vReflDirInTS * maxTraceDistance;
    
    vec3 dp = vReflectionEndPosInTS.xyz - samplePosInTS.xyz;
    ivec2 sampleScreenPos = ivec2(samplePosInTS.xy * screen_size);
    ivec2 endPosScreenPos = ivec2(vReflectionEndPosInTS.xy * screen_size);
    ivec2 dp2 = endPosScreenPos - sampleScreenPos;
    const uint max_dist = max(abs(dp2.x), abs(dp2.y));
    dp /= max_dist;
    
    vec4 rayPosInTS = vec4(samplePosInTS.xyz + dp, 0);
    vec4 vRayDirInTS = vec4(dp.xyz, 0);
	vec4 rayStartPos = rayPosInTS;

    int hitIndex = -1;
    int max_iterations = 20;
    for(int i = 0;i<=max_dist && i< max_iterations;i += 4)
    {
        float depth0 = 0;
        float depth1 = 0;
        float depth2 = 0;
        float depth3 = 0;

        vec4 rayPosInTS0 = rayPosInTS+vRayDirInTS*0;
        vec4 rayPosInTS1 = rayPosInTS+vRayDirInTS*1;
        vec4 rayPosInTS2 = rayPosInTS+vRayDirInTS*2;
        vec4 rayPosInTS3 = rayPosInTS+vRayDirInTS*3;

        depth3 = textureLod(depth_texture, rayPosInTS3.xy, 0).x;
        depth2 = textureLod(depth_texture, rayPosInTS2.xy, 0).x;
        depth1 = textureLod(depth_texture, rayPosInTS1.xy, 0).x;
        depth0 = textureLod(depth_texture, rayPosInTS0.xy, 0).x;

        {
            float thickness = rayPosInTS0.z - depth0;
			if(thickness>=0 && thickness < MAX_THICKNESS)
			{
				hitIndex = i+0;
				break;
			}
        }
		{
			float thickness = rayPosInTS1.z - depth1;
			if(thickness>=0 && thickness < MAX_THICKNESS)
			{
				hitIndex = i+1;
				break;
			}
		}
		{
			float thickness = rayPosInTS2.z - depth2;
			if(thickness>=0 && thickness < MAX_THICKNESS)
			{
				hitIndex = i+2;
				break;
			}
		}
		{
			float thickness = rayPosInTS3.z - depth3;
			if(thickness>=0 && thickness < MAX_THICKNESS)
			{
				hitIndex = i+3;
				break;
			}
		}

        if(hitIndex != -1) break;

        rayPosInTS = rayPosInTS3 + vRayDirInTS;
    }

    bool intersected = hitIndex >= 0;
    outIntersection = rayStartPos.xyz + vRayDirInTS.xyz * hitIndex;
	
	float intensity = intersected ? 1 : 0;
	
    return intensity;
}

vec2 getCellCount(int mipLevel)
{
    return textureSize(depth_texture, mipLevel);
}

vec2 getCell(vec2 pos, vec2 cell_count)
{
    return vec2(floor(pos*cell_count));
}
vec3 intersectDepthPlane(vec3 o, vec3 d, float z)
{
	return o + d * z;
}

vec3 intersectCellBoundary(vec3 o, vec3 d, vec2 cell, vec2 cell_count, vec2 crossStep, vec2 crossOffset)
{
	vec3 intersection = vec3(0.0);
	
	vec2 index = cell + crossStep;
	vec2 boundary = index / cell_count;
	boundary += crossOffset;
	
	vec2 delta = boundary - o.xy;
	delta /= d.xy;
	float t = min(delta.x, delta.y);
	
	intersection = intersectDepthPlane(o, d, t);
	
	return intersection;
}
float getMinimumDepthPlane(vec2 p, int mipLevel)
{
  return textureLod(depth_texture, p, mipLevel).x;
}

bool crossedCellBoundary(vec2 old, vec2 new)
{
  return (old.x != new.x) || (old.y != new.y);
}

float FindIntersection_HiZ(vec3 samplePosInTS, vec3 vReflDirInTS, float maxTraceDistance, out vec3 outIntersection)
{
  const int max_level =  textureQueryLevels(depth_texture) - 1;

  vec2 crossStep = vec2(vReflDirInTS.x>=0 ? 1 : -1, vReflDirInTS.y>=0 ? 1 : -1);
  vec2 crossOffset = crossStep / screen_size / 128;
  crossStep = clamp(crossStep, 0.0, 1.0);

  vec3 ray = samplePosInTS.xyz;
  float minZ = ray.z;
  float maxZ = ray.z + vReflDirInTS.z * maxTraceDistance;
  float deltaZ = (maxZ - minZ);

  vec3 o = ray;
  vec3 d = vReflDirInTS * maxTraceDistance;

  int startLevel = 2;
  int stopLevel = 0;
  vec2 startCellCount = getCellCount(startLevel);

  vec2 rayCell = getCell(ray.xy, startCellCount);
  ray = intersectCellBoundary(o, d, rayCell, startCellCount, crossStep, crossOffset*64);
  
  int level = startLevel;
  uint iter = 0;
  bool isBackwardRay = vReflDirInTS.z<0;
  float rayDir = isBackwardRay ? -1 : 1;

  const int max_iteration = 300;
  while(level >= stopLevel && ray.z * rayDir <= maxZ * rayDir && iter < max_iteration)
  {
      const vec2 cellCount = getCellCount(level);
      const vec2 oldCellIdx = getCell(ray.xy, cellCount);

      float cell_minZ = getMinimumDepthPlane((oldCellIdx+0.5f)/cellCount, level);
      vec3 tmpRay = ((cell_minZ > ray.z) && !isBackwardRay) ? intersectDepthPlane(o, d, (cell_minZ - minZ)/deltaZ) : ray;
      
      const vec2 newCellIdx = getCell(tmpRay.xy, cellCount);

      float thickness = level == 0 ? (ray.z - cell_minZ) : 0;
      bool crossed = (isBackwardRay && (cell_minZ > ray.z)) || (thickness>(MAX_THICKNESS) || crossedCellBoundary(oldCellIdx, newCellIdx));
      ray = crossed ? intersectCellBoundary(o, d, oldCellIdx, cellCount, crossStep, crossOffset) : tmpRay;
      level = crossed ? min(max_level, level + 1) : level - 1;
      
      ++iter;
        
  }

  bool intersected = (level < stopLevel);
  outIntersection = ray;

  float intensity = intersected ? 1 : 0;
  
  return intensity;
}

vec4 ComputeReflectedColor(float intensity, vec3 intersection, vec4 skyColor)
{
    vec4 ssr_color = vec4(texture(g_color, intersection.xy));

    return mix(skyColor, ssr_color, 1);
}

void main()
{
  vec3 view_pos = calcViewPos(v_texCoords);
  vec4 normal_color = texture(g_normal, v_texCoords);
  vec3 normal =  normal_color.rgb * 2.0 - 1.0;
  vec4 color = texture(g_color, v_texCoords);
  vec4 sky_color = vec4(0, 0, 1, 1);

  vec4 reflection_color = vec4(0.0);

  int reflection_mask = 1;

  float intensity = 0;
 vec4 ssr_color = vec4(0.0);

  vec3 intersection = vec3(0.0);
  if(reflection_mask == 1)
  {
    reflection_color = sky_color;
    vec3 sample_pos_in_ts = vec3(0.0);
    vec3 refl_dir_in_ts = vec3(0.0);
    float max_trace_distance = 0;

    ComputePosAndReflection2(v_texCoords, normal, sample_pos_in_ts, refl_dir_in_ts, max_trace_distance);

    //intensity = FindIntersection_HiZ(sample_pos_in_ts, refl_dir_in_ts, max_trace_distance, intersection);
    intensity = FindIntersection_Linear(sample_pos_in_ts, refl_dir_in_ts, max_trace_distance, intersection);

    reflection_color = texture(g_color, intersection.xy);
    //reflection_color.rgb = sample_pos_in_ts;
  }
 
  fragColor = vec4(reflection_color.rgb, 1.0);
  
}