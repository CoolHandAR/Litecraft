#version 450 core


uniform sampler2D HiZBuffer;

uniform vec4 u_sphere;

uniform int u_index;

flat out int Visible;
out vec3 v_pointPos;

uniform vec3 u_rect_min;
uniform vec3 u_rect_max;

out vec2 v_bounding_rect_min;
out vec2 v_bounding_rect_max;

out vec2 v_corner1;
out vec2 v_corner2;
out vec2 v_corner3;
out vec2 v_corner4;

//camera stuff
uniform vec3 u_viewDir;
uniform vec3 u_viewUp;
uniform vec3 u_viewFront;
uniform mat4 u_viewProj;
uniform mat4 u_view;
uniform mat4 u_proj;

layout(std430, binding = 3) writeonly restrict buffer SSBO
{
    int data_SSBO[];
};

void main(void)
{   
    vec3 view_eye = -vec3(u_view[0].w, u_view[1].w, u_view[2].w);

    vec4 Bounds = u_sphere;

    float CameraSphereDistance = distance( view_eye, Bounds.xyz );
 
    vec3 viewEyeSphereDirection = view_eye - Bounds.xyz;
    
    vec3 viewUp = vec3(u_view[0].y, u_view[1].y, u_view[2].y);
    vec3 viewDirection = vec3(u_view[0].z, u_view[1].z, u_view[2].z);
    vec3 viewRight = normalize(cross(viewEyeSphereDirection, viewUp));

    float fRadius = CameraSphereDistance * tan(asin(Bounds.w / CameraSphereDistance));

    // Compute the offsets for the points around the sphere
    vec3 vUpRadius = viewUp * fRadius;
    vec3 vRightRadius = viewRight * fRadius;

        // Generate the 4 corners of the sphere in world space.
    vec4 vCorner0WS = vec4( Bounds.xyz + vUpRadius - vRightRadius, 1 ); // Top-Left
    vec4 vCorner1WS = vec4( Bounds.xyz + vUpRadius + vRightRadius, 1 ); // Top-Right
    vec4 vCorner2WS = vec4( Bounds.xyz - vUpRadius - vRightRadius, 1 ); // Bottom-Left
    vec4 vCorner3WS = vec4( Bounds.xyz - vUpRadius + vRightRadius, 1 ); // Bottom-Right

    // Project the 4 corners of the sphere into clip space
    vec4 vCorner0CS = u_viewProj * vCorner0WS;
    vec4 vCorner1CS = u_viewProj * vCorner1WS;
    vec4 vCorner2CS = u_viewProj * vCorner2WS;
    vec4 vCorner3CS = u_viewProj * vCorner3WS;


    // Convert the corner points from clip space to normalized device coordinates
    vec2 vCorner0NDC = vCorner0CS.xy / vCorner0CS.w;
    vec2 vCorner1NDC = vCorner1CS.xy / vCorner1CS.w;
    vec2 vCorner2NDC = vCorner2CS.xy / vCorner2CS.w;
    vec2 vCorner3NDC = vCorner3CS.xy / vCorner3CS.w;
    vCorner0NDC = vec2( 0.5, -0.5 ) * vCorner0NDC + vec2( 0.5, 0.5 );
    vCorner1NDC = vec2( 0.5, -0.5 ) * vCorner1NDC + vec2( 0.5, 0.5 );
    vCorner2NDC = vec2( 0.5, -0.5 ) * vCorner2NDC + vec2( 0.5, 0.5 );
    vCorner3NDC = vec2( 0.5, -0.5 ) * vCorner3NDC + vec2( 0.5, 0.5 );

    // In order to have the sphere covering at most 4 texels, we need to use
    // the entire width of the rectangle, instead of only the radius of the rectangle,
    // which was the original implementation in the ATI paper, it had some edge case
    // failures I observed from being overly conservative.
    float fSphereWidthNDC = distance( vCorner0NDC, vCorner1NDC );

    // Compute the center of the bounding sphere in screen space
    vec3 Cv = (u_view * vec4( Bounds.xyz, 1 )).xyz;

    // compute nearest point to camera on sphere, and project it
    vec3 Pv = Cv - normalize( Cv ) * Bounds.w;
    vec4 ClosestSpherePoint = u_proj * vec4( Pv, 1 );
    
      // Choose a MIP level in the HiZ map.
        // The original assumed viewport width > height, however I've changed it
        // to determine the greater of the two.
        //
        // This will result in a mip level where the object takes up at most
        // 2x2 texels such that the 4 sampled points have depths to compare
        // against.
    float W = fSphereWidthNDC * max(800, 600);
    float fLOD = ceil(log2( W ));

      // fetch depth samples at the corners of the square to compare against
        vec4 vSamples;
        vSamples.x = textureLod( HiZBuffer, vCorner0NDC, fLOD ).x;
        vSamples.y = textureLod( HiZBuffer, vCorner1NDC, fLOD ).x;
        vSamples.z = textureLod( HiZBuffer, vCorner2NDC, fLOD ).x;
        vSamples.w = textureLod( HiZBuffer, vCorner3NDC, fLOD ).x;


   float fMaxSampledDepth = max( max( vSamples.x, vSamples.y ), max( vSamples.z, vSamples.w ) );
   float fSphereDepth = (ClosestSpherePoint.z / ClosestSpherePoint.w);

    if(fSphereDepth > fMaxSampledDepth)
    {
        Visible = 0;
    }
    else
    {
        Visible = 1;
    }

    //Visible = 1;

    data_SSBO[u_index] = Visible;

    v_bounding_rect_min = vCorner0NDC;
    v_bounding_rect_max = vCorner3NDC;

    v_corner1 = vCorner0NDC;
    v_corner2 = vCorner1NDC;
    v_corner3 = vCorner2NDC;
    v_corner4 = vCorner3NDC;
}
