//--------------------------------------------------------------------------------------
// File: ForwardPlus11Common.hlsl
//
// HLSL file for the ForwardPlus11 Sample. Common code.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register(b0)
{
    matrix  WorldViewProjection         : packoffset(c0);
    matrix  WorldView                   : packoffset(c4);
    matrix  World                       : packoffset(c8);
    float4  MaterialAmbientColorUp      : packoffset(c12);
    float4  MaterialAmbientColorDown    : packoffset(c13);
}

cbuffer cbPerFrame : register(b1)
{
    matrix  Projection              : packoffset(c0);
    matrix  ProjectionInv           : packoffset(c4);
    float3  CameraPos               : packoffset(c8);
    float   fAlphaTest              : packoffset(c8.w);
    uint    uNumLights              : packoffset(c9);
    uint    uWindowWidth            : packoffset(c9.y);
    uint    uWindowHeight           : packoffset(c9.z);
    uint    uMaxNumLightsPerTile    : packoffset(c9.w);
};

//--------------------------------------------------------------------------------------
// Miscellaneous constants
//--------------------------------------------------------------------------------------
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

//--------------------------------------------------------------------------------------
// Light culling constants.
// These must match their counterparts in ForwardPlusUtil.h
//--------------------------------------------------------------------------------------
#define TILE_RES 16
#define MAX_NUM_LIGHTS_PER_TILE 544

//-----------------------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------------------
uint GetTileIndex(float2 ScreenPos)
{
    float fTileRes = (float)TILE_RES;
    uint nNumCellsX = (uWindowWidth + TILE_RES - 1) / TILE_RES;
    uint nTileIdx = floor(ScreenPos.x / fTileRes) + floor(ScreenPos.y / fTileRes) * nNumCellsX;
    return nTileIdx;
}