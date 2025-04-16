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

// --- 추가: Material 상수 버퍼 (b3) ---
// FMaterialInfo 구조체 정의가 다른 곳에 있다고 가정
struct FMaterialInfo // TODO: 실제 구조체 정의 확인 및 필요시 여기에 복사
{
    float3 DiffuseColor;
    float TransparencyScalar;
    float3 AmbientColor;
    float DensityScalar;
    float3 SpecularColor;
    float SpecularScalar;
    float3 EmmisiveColor;
    float MaterialPad0;
};

cbuffer MaterialConstant : register(b3)
{
    FMaterialInfo Material;
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