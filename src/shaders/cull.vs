#version 450 core


uniform sampler2D HiZBuffer;

uniform vec3 u_pointPos;
uniform mat4 u_mvp;
uniform vec3 u_extent;

uniform int u_index;

flat out int Visible;
out vec3 v_pointPos;

uniform vec3 u_rect_min;
uniform vec3 u_rect_max;

out vec2 v_bounding_rect_min;
out vec2 v_bounding_rect_max;

layout(std430, binding = 3) writeonly restrict buffer SSBO
{
    int data_SSBO[];
};

void main(void)
{
    vec4 BoundingBox[8];

    BoundingBox[0] = u_mvp * vec4( u_pointPos + vec3( u_extent.x, u_extent.y, u_extent.z), 1.0 );
    BoundingBox[1] = u_mvp * vec4( u_pointPos + vec3(-u_extent.x, u_extent.y, u_extent.z), 1.0 );
    BoundingBox[2] = u_mvp * vec4( u_pointPos + vec3( u_extent.x,-u_extent.y, u_extent.z), 1.0 );
    BoundingBox[3] = u_mvp * vec4( u_pointPos + vec3(-u_extent.x,-u_extent.y, u_extent.z), 1.0 );
    BoundingBox[4] = u_mvp * vec4( u_pointPos + vec3( u_extent.x, u_extent.y,-u_extent.z), 1.0 );
    BoundingBox[5] = u_mvp * vec4( u_pointPos + vec3(-u_extent.x, u_extent.y,-u_extent.z), 1.0 );
    BoundingBox[6] = u_mvp * vec4( u_pointPos + vec3( u_extent.x,-u_extent.y,-u_extent.z), 1.0 );
    BoundingBox[7] = u_mvp * vec4( u_pointPos + vec3(-u_extent.x,-u_extent.y,-u_extent.z), 1.0 );

    

  int outOfBound[6] = int[6]( 0, 0, 0, 0, 0, 0 );

    for (int i=0; i<8; i++)
    {
        if ( BoundingBox[i].x >  BoundingBox[i].w ) outOfBound[0]++;
        if ( BoundingBox[i].x < -BoundingBox[i].w ) outOfBound[1]++;
        if ( BoundingBox[i].y >  BoundingBox[i].w ) outOfBound[2]++;
        if ( BoundingBox[i].y < -BoundingBox[i].w ) outOfBound[3]++;
        if ( BoundingBox[i].z >  BoundingBox[i].w ) outOfBound[4]++;
        if ( BoundingBox[i].z < -BoundingBox[i].w ) outOfBound[5]++;
    }

	int inFrustum = 1;

    for (int i=0; i<6; i++)
        if ( outOfBound[i] == 8 ) inFrustum = 0;

inFrustum = 1;

if(inFrustum == 1)
{

    for (int i=0; i<8; i++)
        BoundingBox[i].xyz /= BoundingBox[i].w;

    vec2 BoundingRect[2];
    BoundingRect[0].x = min( min( min( BoundingBox[0].x, BoundingBox[1].x ),
                                    min( BoundingBox[2].x, BoundingBox[3].x ) ),
                                min( min( BoundingBox[4].x, BoundingBox[5].x ),
                                    min( BoundingBox[6].x, BoundingBox[7].x ) ) ) / 2.0 + 0.5;
    BoundingRect[0].y = min( min( min( BoundingBox[0].y, BoundingBox[1].y ),
                                    min( BoundingBox[2].y, BoundingBox[3].y ) ),
                                min( min( BoundingBox[4].y, BoundingBox[5].y ),
                                    min( BoundingBox[6].y, BoundingBox[7].y ) ) ) / 2.0 + 0.5;
    BoundingRect[1].x = max( max( max( BoundingBox[0].x, BoundingBox[1].x ),
                                    max( BoundingBox[2].x, BoundingBox[3].x ) ),
                                max( max( BoundingBox[4].x, BoundingBox[5].x ),
                                    max( BoundingBox[6].x, BoundingBox[7].x ) ) ) / 2.0 + 0.5;
    BoundingRect[1].y = max( max( max( BoundingBox[0].y, BoundingBox[1].y ),
                                    max( BoundingBox[2].y, BoundingBox[3].y ) ),
                                max( max( BoundingBox[4].y, BoundingBox[5].y ),
                                    max( BoundingBox[6].y, BoundingBox[7].y ) ) ) / 2.0 + 0.5;
    /* then the linear depth value of the front-most point */
    float InstanceDepth = min( min( min( BoundingBox[0].z, BoundingBox[1].z ),
                                    min( BoundingBox[2].z, BoundingBox[3].z ) ),
                                min( min( BoundingBox[4].z, BoundingBox[5].z ),
                                    min( BoundingBox[6].z, BoundingBox[7].z ) ) );

    float ViewSizeX = (BoundingRect[1].x-BoundingRect[0].x) * 800;
    float ViewSizeY = (BoundingRect[1].y-BoundingRect[0].y) * 600;

    /* now we calculate the texture LOD used for lookup in the depth buffer texture */
    float LOD = ceil( log2( max( ViewSizeX, ViewSizeY ) / 2.0 ) );

    vec4 Samples;
    Samples.x = textureLod( HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[0].y), LOD ).x;
    Samples.y = textureLod( HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[1].y), LOD ).x;
    Samples.z = textureLod( HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[1].y), LOD ).x;
    Samples.w = textureLod( HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[0].y), LOD ).x;
    float MaxDepth = max( max( Samples.x, Samples.y ), max( Samples.z, Samples.w ) );

    if(InstanceDepth > MaxDepth)
    {
        Visible = 0;
    }
    //else visible
    else
    {
        Visible = 1;
    }
    
    v_bounding_rect_min = BoundingRect[0];
    v_bounding_rect_max = BoundingRect[1];

   // v_bounding_rect_min = u_rect_min.xy;
    //v_bounding_rect_max = u_rect_max.xy;
}
else
{
    Visible = 0;
}

    Visible = 1;

    data_SSBO[u_index] = Visible;

    v_pointPos = u_pointPos;
}
