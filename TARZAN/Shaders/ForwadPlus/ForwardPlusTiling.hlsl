//--------------------------------------------------------------------------------------
// File: ForwardPlus11.hlsl
//
// HLSL file for the ForwardPlus11 sample. Tiled light culling.
//--------------------------------------------------------------------------------------


#include "ForwardPlus11Common.hlsl"

#define FLT_MAX         3.402823466e+38F

//-----------------------------------------------------------------------------------------
// Textures and Buffers
//-----------------------------------------------------------------------------------------
Buffer<float4> PointLightBufferCenterAndRadius : register(t0);
Buffer<float4> SpotLightBufferCenterAndRadius : register(t1);

#if ( USE_DEPTH_BOUNDS == 1 )   // non-MSAA
Texture2D<float> DepthTexture : register(t2);
#elif ( USE_DEPTH_BOUNDS == 2 ) // MSAA
Texture2DMS<float> DepthTexture : register(t2);
#endif

RWBuffer<uint> PerTileLightIndexBufferOut : register(u0);

//-----------------------------------------------------------------------------------------
// Group Shared Memory (aka local data share, or LDS)
//-----------------------------------------------------------------------------------------
#if ( USE_DEPTH_BOUNDS == 1 || USE_DEPTH_BOUNDS == 2 )
groupshared uint ldsZMax;
groupshared uint ldsZMin;
#endif

groupshared uint ldsLightIdxCounter;
groupshared uint ldsLightIdx[MAX_NUM_LIGHTS_PER_TILE];

//-----------------------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------------------

// this creates the standard Hessian-normal-form plane equation from three points, 
// except it is simplified for the case where the first point is the origin
float3 CreatePlaneEquation(float3 b, float3 c)
{
    // normalize(cross( b-a, c-a )), except we know "a" is the origin
    // also, typically there would be a fourth term of the plane equation, 
    // -(n dot a), except we know "a" is the origin
    return normalize(cross(b, c));;
}

// point-plane distance, simplified for the case where 
// the plane passes through the origin
float GetSignedDistanceFromPlane(float3 p, float3 eqn)
{
    // dot(eqn.xyz,p) + eqn.w, , except we know eqn.w is zero 
    // (see CreatePlaneEquation above)
    return dot(eqn, p);
}

bool TestFrustumSides(float3 c, float r, float3 plane0, float3 plane1, float3 plane2, float3 plane3)
{
    bool intersectingOrInside0 = GetSignedDistanceFromPlane(c, plane0) < r;
    bool intersectingOrInside1 = GetSignedDistanceFromPlane(c, plane1) < r;
    bool intersectingOrInside2 = GetSignedDistanceFromPlane(c, plane2) < r;
    bool intersectingOrInside3 = GetSignedDistanceFromPlane(c, plane3) < r;

    return (intersectingOrInside0 && intersectingOrInside1 &&
            intersectingOrInside2 && intersectingOrInside3);
}

// calculate the number of tiles in the horizontal direction
uint GetNumTilesX()
{
    // '+ TILE_RES - 1' <- 올림 처리를 위한 트릭
    return (uint)((uWindowWidth + TILE_RES - 1) / (float)TILE_RES);
}

// calculate the number of tiles in the vertical direction
uint GetNumTilesY()
{
    return (uint)((uWindowHeight + TILE_RES - 1) / (float)TILE_RES);
}

// convert a point from post-projection space into view space
float4 ConvertProjToView(float4 p)
{
    p = mul(p, ProjectionInv);
    p /= p.w;
    return p;
}

// convert a depth value from post-projection space into view space
float ConvertProjDepthToView(float z)
{
    z = 1.f / (z * ProjectionInv._34 + ProjectionInv._44);
    return z;
}

#if ( USE_DEPTH_BOUNDS == 1 )   // non-MSAA
void CalculateMinMaxDepthInLds(uint3 globalThreadIdx)
{
    float depth = DepthTexture.Load(uint3(globalThreadIdx.x, globalThreadIdx.y, 0)).x;
    float viewPosZ = ConvertProjDepthToView(depth);
    uint z = asuint(viewPosZ);
    if (depth != 0.f)
    {
        InterlockedMax(ldsZMax, z);
        InterlockedMin(ldsZMin, z);
    }
}
#endif

#if ( USE_DEPTH_BOUNDS == 2 ) // MSAA
void CalculateMinMaxDepthInLdsMSAA(uint3 globalThreadIdx, uint depthBufferNumSamples)
{
    float minZForThisPixel = FLT_MAX;
    float maxZForThisPixel = 0.f;

    float depth0 = DepthTexture.Load(uint2(globalThreadIdx.x, globalThreadIdx.y), 0).x;
    float viewPosZ0 = ConvertProjDepthToView(depth0);
    if (depth0 != 0.f)
    {
        maxZForThisPixel = max(maxZForThisPixel, viewPosZ0);
        minZForThisPixel = min(minZForThisPixel, viewPosZ0);
    }

    for (uint sampleIdx = 1; sampleIdx < depthBufferNumSamples; sampleIdx++)
    {
        float depth = DepthTexture.Load(uint2(globalThreadIdx.x, globalThreadIdx.y), sampleIdx).x;
        float viewPosZ = ConvertProjDepthToView(depth);
        if (depth != 0.f)
        {
            maxZForThisPixel = max(maxZForThisPixel, viewPosZ);
            minZForThisPixel = min(minZForThisPixel, viewPosZ);
        }
    }

    uint zMaxForThisPixel = asuint(maxZForThisPixel);
    uint zMinForThisPixel = asuint(minZForThisPixel);
    InterlockedMax(ldsZMax, zMaxForThisPixel);
    InterlockedMin(ldsZMin, zMinForThisPixel);
}
#endif

//-----------------------------------------------------------------------------------------
// Parameters for the light culling shader
//-----------------------------------------------------------------------------------------
#define NUM_THREADS_X TILE_RES
#define NUM_THREADS_Y TILE_RES
#define NUM_THREADS_PER_TILE (NUM_THREADS_X*NUM_THREADS_Y)

//-----------------------------------------------------------------------------------------
// Light culling shader
//-----------------------------------------------------------------------------------------
[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void CullLightsCS(uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
    // 2D Array to 1D Array
    uint localIdxFlattened = localIdx.x + localIdx.y * NUM_THREADS_X;

    if (localIdxFlattened == 0)
    {
#if ( USE_DEPTH_BOUNDS == 1 || USE_DEPTH_BOUNDS == 2 )
        ldsZMin = 0x7f7fffff;  // FLT_MAX as a uint
        ldsZMax = 0;
#endif
        ldsLightIdxCounter = 0;
    }

    float3 frustumEqn0, frustumEqn1, frustumEqn2, frustumEqn3;
    {   // construct frustum for this tile
        uint pxl = TILE_RES * groupIdx.x;
        uint pyt = TILE_RES * groupIdx.y;
        uint pxr = TILE_RES * (groupIdx.x + 1);
        uint pyb = TILE_RES * (groupIdx.y + 1);

        uint uWindowWidthEvenlyDivisibleByTileRes = TILE_RES * GetNumTilesX();
        uint uWindowHeightEvenlyDivisibleByTileRes = TILE_RES * GetNumTilesY();

        // four corners of the tile, clockwise from top-left
        float3 frustum0 = ConvertProjToView(float4(pxl / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.f - 1.f, (uWindowHeightEvenlyDivisibleByTileRes - pyt) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.f - 1.f, 1.f, 1.f)).xyz;
        float3 frustum1 = ConvertProjToView(float4(pxr / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.f - 1.f, (uWindowHeightEvenlyDivisibleByTileRes - pyt) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.f - 1.f, 1.f, 1.f)).xyz;
        float3 frustum2 = ConvertProjToView(float4(pxr / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.f - 1.f, (uWindowHeightEvenlyDivisibleByTileRes - pyb) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.f - 1.f, 1.f, 1.f)).xyz;
        float3 frustum3 = ConvertProjToView(float4(pxl / (float)uWindowWidthEvenlyDivisibleByTileRes * 2.f - 1.f, (uWindowHeightEvenlyDivisibleByTileRes - pyb) / (float)uWindowHeightEvenlyDivisibleByTileRes * 2.f - 1.f, 1.f, 1.f)).xyz;

        // create plane equations for the four sides of the frustum, 
        // with the positive half-space outside the frustum (and remember, 
        // view space is left handed, so use the left-hand rule to determine 
        // cross product direction)
        frustumEqn0 = CreatePlaneEquation(frustum0, frustum1);
        frustumEqn1 = CreatePlaneEquation(frustum1, frustum2);
        frustumEqn2 = CreatePlaneEquation(frustum2, frustum3);
        frustumEqn3 = CreatePlaneEquation(frustum3, frustum0);
    }

    GroupMemoryBarrierWithGroupSync();

    // calculate the min and max depth for this tile, 
    // to form the front and back of the frustum

#if ( USE_DEPTH_BOUNDS == 1 || USE_DEPTH_BOUNDS == 2 )
    float minZ = FLT_MAX;
    float maxZ = 0.f;

#if ( USE_DEPTH_BOUNDS == 1 )   // non-MSAA
    CalculateMinMaxDepthInLds(globalIdx);
#elif ( USE_DEPTH_BOUNDS == 2 ) // MSAA
    uint depthBufferWidth, depthBufferHeight, depthBufferNumSamples;
    DepthTexture.GetDimensions(depthBufferWidth, depthBufferHeight, depthBufferNumSamples);
    CalculateMinMaxDepthInLdsMSAA(globalIdx, depthBufferNumSamples);
#endif

    GroupMemoryBarrierWithGroupSync();
    maxZ = asfloat(ldsZMax);
    minZ = asfloat(ldsZMin);
#endif

    // loop over the lights and do a sphere vs. frustum intersection test
    uint uNumPointLights = uNumLights & 0xFFFFu;
    for (uint i = localIdxFlattened; i < uNumPointLights; i += NUM_THREADS_PER_TILE)
    {
        float4 center = PointLightBufferCenterAndRadius[i];
        float r = center.w;
        center.xyz = mul(float4(center.xyz, 1), WorldView).xyz;

        // test if sphere is intersecting or inside frustum
        if (TestFrustumSides(center.xyz, r, frustumEqn0, frustumEqn1, frustumEqn2, frustumEqn3))
        {
#if ( USE_DEPTH_BOUNDS != 0 )
            if (-center.z + minZ < r && center.z - maxZ < r)
#else
            if (-center.z < r)
#endif
            {
                // do a thread-safe increment of the list counter 
                // and put the index of this light into the list
                uint dstIdx = 0;
                InterlockedAdd(ldsLightIdxCounter, 1, dstIdx);
                ldsLightIdx[dstIdx] = i;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // and again for spot lights
    uint uNumPointLightsInThisTile = ldsLightIdxCounter;
    uint uNumSpotLights = (uNumLights & 0xFFFF0000u) >> 16;
    for (uint j = localIdxFlattened; j < uNumSpotLights; j += NUM_THREADS_PER_TILE)
    {
        float4 center = SpotLightBufferCenterAndRadius[j];
        float r = center.w;
        center.xyz = mul(float4(center.xyz, 1), WorldView).xyz;

        // test if sphere is intersecting or inside frustum
        if (TestFrustumSides(center.xyz, r, frustumEqn0, frustumEqn1, frustumEqn2, frustumEqn3))
        {
#if ( USE_DEPTH_BOUNDS != 0 )
            if (-center.z + minZ < r && center.z - maxZ < r)
#else
            if (-center.z < r)
#endif
            {
                // do a thread-safe increment of the list counter 
                // and put the index of this light into the list
                uint dstIdx = 0;
                InterlockedAdd(ldsLightIdxCounter, 1, dstIdx);
                ldsLightIdx[dstIdx] = j;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    {   // write back
        uint tileIdxFlattened = groupIdx.x + groupIdx.y * GetNumTilesX();
        uint startOffset = uMaxNumLightsPerTile * tileIdxFlattened;

        for (uint i = localIdxFlattened; i < uNumPointLightsInThisTile; i += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            PerTileLightIndexBufferOut[startOffset + i] = ldsLightIdx[i];
        }

        for (uint j = (localIdxFlattened + uNumPointLightsInThisTile); j < ldsLightIdxCounter; j += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            PerTileLightIndexBufferOut[startOffset + j + 1] = ldsLightIdx[j];
        }

        if (localIdxFlattened == 0)
        {
            // mark the end of each per-tile list with a sentinel (point lights)
            PerTileLightIndexBufferOut[startOffset + uNumPointLightsInThisTile] = LIGHT_INDEX_BUFFER_SENTINEL;

            // mark the end of each per-tile list with a sentinel (spot lights)
            PerTileLightIndexBufferOut[startOffset + ldsLightIdxCounter + 1] = LIGHT_INDEX_BUFFER_SENTINEL;
        }
    }
}