#pragma once
#include <cmath>
#include <algorithm>
#include "Core/Container/String.h"
#include "Core/Container/Array.h"
#include "UObject/NameTypes.h"
#include "Color.h"

// 수학 관련
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Matrix.h"


#define UE_LOG Console::GetInstance().AddLog

#define _TCHAR_DEFINED

#define Max_Fireball 300
#define NUM_POINT_LIGHT 16
#define NUM_SPOT_LIGHT 16

#include <d3d11.h>

#include "UserInterface/Console.h"

struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
    float tx, ty, tz; // Tangent
    float nx, ny, nz; // Normal
    float u, v;       // UV
    uint32 MaterialIndex;
};

struct FVertexUnlit
{
    //FVector Position;
    //FLinearColor Color;
    //FVector2D UV;
    float x, y, z;
    float r, g, b, a;
    float u = 0, v = 0;
};

// Material Subset
struct FMaterialSubset
{
    uint32 IndexStart; // Index Buffer Start pos
    uint32 IndexCount; // Index Count
    uint32 MaterialIndex; // Material Index
    FString MaterialName; // Material Name
};

struct FStaticMaterial
{
    class UMaterial* Material;
    FName MaterialSlotName;
    //FMeshUVChannelInfo UVChannelData;
};

// OBJ File Raw Data
struct FObjInfo
{
    FWString ObjectName; // OBJ File Name
    FWString PathName; // OBJ File Paths
    FString DisplayName; // Display Name
    FString MatName; // OBJ MTL File Name
    
    // Group
    uint32 NumOfGroup = 0; // token 'g' or 'o'
    TArray<FString> GroupName;
    
    // Vertex, UV, Normal List
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    
    // Faces
    TArray<int32> Faces;

    // Index
    TArray<uint32> VertexIndices;
    TArray<uint32> NormalIndices;
    TArray<uint32> TextureIndices;
    
    // Material
    TArray<FMaterialSubset> MaterialSubsets;
};

struct FObjMaterialInfo
{
    FString MTLName;  // newmtl : Material Name.

    bool bHasTexture = false;  // Has Texture?
    bool bHasNormalMap = false;
    bool bTransparent = false; // Has alpha channel?

    FVector Diffuse;  // Kd : Diffuse (Vector4)
    FVector Specular;  // Ks : Specular (Vector) 
    FVector Ambient;   // Ka : Ambient (Vector)
    FVector Emissive;  // Ke : Emissive (Vector)

    float SpecularScalar = 1.0f; // Ns : Specular Power (Float)
    float DensityScalar;  // Ni : Optical Density (Float)
    float TransparencyScalar; // d or Tr  : Transparency of surface (Float)

    uint32 IlluminanceModel; // illum: illumination Model between 0 and 10. (UINT)

    /* Texture */
    FString DiffuseTextureName;  // map_Kd : Diffuse texture
    FWString DiffuseTexturePath;
    
    FString AmbientTextureName;  // map_Ka : Ambient texture
    FWString AmbientTexturePath;
    
    FString SpecularTextureName; // map_Ks : Specular texture
    FWString SpecularTexturePath;
    
    FString BumpTextureName;     // map_Bump : Bump texture
    FWString BumpTexturePath;
    
    FString AlphaTextureName;    // map_d : Alpha texture
    FWString AlphaTexturePath;
};

// Cooked Data
namespace OBJ
{
    struct FStaticMeshRenderData
    {
        FWString ObjectName;
        FWString PathName;
        FString DisplayName;
        
        TArray<FVertexSimple> Vertices;
        TArray<UINT> Indices;

        ID3D11Buffer* VertexBuffer;
        ID3D11Buffer* IndexBuffer;
        
        TArray<FObjMaterialInfo> Materials;
        TArray<FMaterialSubset> MaterialSubsets;

        FVector BoundingBoxMin;
        FVector BoundingBoxMax;
    };
}

struct FVertexTexture
{
	float x, y, z;    // Position
	float u, v; // Texture
};
struct FGridParameters
{
	float gridSpacing;
	int   numGridLines;
	FVector gridOrigin;
	float pad;
};
struct FSimpleVertex
{
	float dummy; // 내용은 사용되지 않음
    float padding[11];
};
struct FOBB {
    FVector corners[8];
};
struct FRect
{
    FRect() : leftTopX(0), leftTopY(0), width(0), height(0) {}
    FRect(float x, float y, float w, float h) : leftTopX(x), leftTopY(y), width(w), height(h) {}
    float leftTopX, leftTopY, width, height;
};
struct FPoint
{
    FPoint() : x(0), y(0) {}
    FPoint(float _x, float _y) : x(_x), y(_y) {}
    FPoint(long _x, long _y) : x(_x), y(_y) {}
    FPoint(int _x, int _y) : x(_x), y(_y) {}

    float x, y;
};
struct FBoundingBox
{
    FBoundingBox(){}
    FBoundingBox(FVector _min, FVector _max) : min(_min), max(_max) {}
	FVector min; // Minimum extents
	float pad;
	FVector max; // Maximum extents
	float pad1;
    bool Intersect(const FVector& rayOrigin, const FVector& rayDir, float& outDistance)
    {
        float tmin = -FLT_MAX;
        float tmax = FLT_MAX;
        const float epsilon = 1e-6f;

        // X축 처리
        if (fabs(rayDir.x) < epsilon)
        {
            // 레이가 X축 방향으로 거의 평행한 경우,
            // 원점의 x가 박스 [min.x, max.x] 범위 밖이면 교차 없음
            if (rayOrigin.x < min.x || rayOrigin.x > max.x)
                return false;
        }
        else
        {
            float t1 = (min.x - rayOrigin.x) / rayDir.x;
            float t2 = (max.x - rayOrigin.x) / rayDir.x;
            if (t1 > t2)  std::swap(t1, t2);

            // tmin은 "현재까지의 교차 구간 중 가장 큰 min"
            tmin = (t1 > tmin) ? t1 : tmin;
            // tmax는 "현재까지의 교차 구간 중 가장 작은 max"
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Y축 처리
        if (fabs(rayDir.y) < epsilon)
        {
            if (rayOrigin.y < min.y || rayOrigin.y > max.y)
                return false;
        }
        else
        {
            float t1 = (min.y - rayOrigin.y) / rayDir.y;
            float t2 = (max.y - rayOrigin.y) / rayDir.y;
            if (t1 > t2)  std::swap(t1, t2);

            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // Z축 처리
        if (fabs(rayDir.z) < epsilon)
        {
            if (rayOrigin.z < min.z || rayOrigin.z > max.z)
                return false;
        }
        else
        {
            float t1 = (min.z - rayOrigin.z) / rayDir.z;
            float t2 = (max.z - rayOrigin.z) / rayDir.z;
            if (t1 > t2)  std::swap(t1, t2);

            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
            if (tmin > tmax)
                return false;
        }

        // 여기까지 왔으면 교차 구간 [tmin, tmax]가 유효하다.
        // tmax < 0 이면, 레이가 박스 뒤쪽에서 교차하므로 화면상 보기엔 교차 안 한다고 볼 수 있음
        if (tmax < 0.0f)
            return false;

        // outDistance = tmin이 0보다 크면 그게 레이가 처음으로 박스를 만나는 지점
        // 만약 tmin < 0 이면, 레이의 시작점이 박스 내부에 있다는 의미이므로, 거리를 0으로 처리해도 됨.
        outDistance = (tmin >= 0.0f) ? tmin : 0.0f;

        return true;
    }

};
struct FCone
{
    FVector ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름

    FVector ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    FLinearColor Color;

    int ConeSegmentCount; // 원뿔 밑면 분할 수
    float pad[3];

};

struct FCircle 
{
    FVector CircleApex; // Circle 상의 한 점
    float CircleRadius; // Circle의 반지름
    FVector CircleBaseCenter;   // Circle의 중심점
    int CircleSegmentCount;    // Circle의 분할 수
    FLinearColor Color;
};

struct FLine 
{
    FVector LineStart; // 시작점
    float pad0;
    FVector LineEnd;   // 끝점
    float pad1;
    FLinearColor LineColor; // 색상
};

struct FPrimitiveCounts 
{
	int BoundingBoxCount;
	int pad;
	int ConeCount; 
    int OBBCount;
    int CircleCount;
};
struct FLighting
{
	float lightDirX, lightDirY, lightDirZ; // 조명 방향
	float pad1;                      // 16바이트 정렬용 패딩
	float lightColorX, lightColorY, lightColorZ;    // 조명 색상
	float pad2;                      // 16바이트 정렬용 패딩
	float AmbientFactor;             // ambient 계수
	float pad3; // 16바이트 정렬 맞춤 추가 패딩
	float pad4; // 16바이트 정렬 맞춤 추가 패딩
	float pad5; // 16바이트 정렬 맞춤 추가 패딩
};

struct FMaterialConstants {
    FVector DiffuseColor;
    float TransparencyScalar;
    FVector AmbientColor;
    float DensityScalar;
    FVector SpecularColor;
    float SpecularScalar;
    FVector EmmisiveColor;
    float MaterialPad0;
};

struct FConstants {
    FMatrix MVP;      // 모델
    FMatrix ModelMatrix; // 모델 행렬
    FMatrix ModelMatrixInverseTranspose; // normal 변환을 위한 행렬
    FVector4 UUIDColor;
    bool IsSelected;
    FVector CameraPosition;
    FVector2D ScreenSize;
    FVector2D ViewportSize;
};

#pragma region Uber
struct FObjectMatrixConstants 
{
    FMatrix World;
    FMatrix View;
    FMatrix Projection;
    FMatrix NormalMatrix;
};

struct FCameraConstant 
{
    FVector CameraWorldPos;
    float Padding;
};

struct FAmbientLightInfo
{
    FLinearColor Color;
    float Intensity;
    FVector Pad;
};

struct FDirectionalLightInfo
{
    FLinearColor Color;
    FVector Direction;
    float Intensity;
};

struct FPointLightInfo
{
    FLinearColor Color;
    FVector Position;
    float Intensity;
    float AttenuationRadius;
    float LightFalloffExponent;
    FVector2D Pad;
};

struct FSpotLightInfo
{
    FLinearColor Color;
    FVector Position;
    float Intensity;
    FVector Direction;
    float AttenuationRadius;
    float LightFalloffExponent;
    float InnerConeAngle;
    float OuterConeAngle;
    float Pad;
};

struct FLightConstants 
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    FPointLightInfo PointLights[NUM_POINT_LIGHT];
    FSpotLightInfo SpotLights[NUM_SPOT_LIGHT];
};
#pragma endregion

struct FLitUnlitConstants {
    int isLit; // 1 = Lit, 0 = Unlit 
    FVector pad;
};

struct FSubMeshConstants {
    float isSelectedSubMesh;
    FVector pad;
};

struct FTextureConstants {
    float UOffset;
    float VOffset;
    float pad0;
    float pad1;
};

struct FTextureMaterialConstants
{
    FLinearColor TintColor;
    float IsLightIcon;
};

struct FSubUVConstant
{
    float indexU;
    float indexV;
};

#pragma region Lighting Pass Constants
struct FLightConstant
{
    FVector4 Ambient;
    FVector4 Diffuse;
    FVector4 Specular;
    FVector Emissive;
    float Padding1;
    FVector Direction;
    float Padding2;
    FVector CameraPosition;
    float Padding;
};

struct FMaterial
{
    FVector4 Diffuse;
    FVector4 Ambient;
    FVector4 Specular;
    FVector Emissive;
    float Roughness;
};

//struct FViewModeConstatnt
//{
//    int ViewMode;
//    FVector Padding;
//};
#pragma endregion

struct FFireballConstant
{
    FVector Position = (0,0,0);
    float Intensity = 2;
    float Radius = 2;
    float RadiusFallOff = 2;
    float InnerAngle = 15;
    float OuterAngle = 45;
    FLinearColor Color = FLinearColor::Red();
    FVector Direction = (0, 0, 0);
    int LightType = 0; 
};

enum LightType {
    PointLight = 0,
    SpotLight = 1,
    DirectionalLight = 2
};

struct FFireballInfo
{
    float Intensity = 2;
    float Radius = 5;
    float RadiusFallOff = 2;
    FLinearColor Color = FLinearColor::Red();
    LightType Type = LightType::PointLight;
};

struct FFireballArrayInfo
{
    FFireballConstant FireballConstants[Max_Fireball];
    int FireballCount = 0;
    FVector padding;
};

struct FPointLightArrayInfo
{
    FPointLightInfo PointLightConstants[NUM_POINT_LIGHT];
    int PointLightCount = 0;
    FVector padding;
};

struct FSpotLightArrayInfo 
{
    FSpotLightInfo SpotLightConstants[NUM_SPOT_LIGHT];
    int SpotLightCount = 0;
    FVector padding;
};

struct FFogConstants    
{
    float FogDensity = 0.5f;
    float FogHeightFalloff = 0.5f;
    float FogStartDistance = 0.0f;
    float FogCutoffDistance = 1000.0f;
    float FogMaxOpacity = 1.0f;
    FVector padding;
    FLinearColor FogInscatteringColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    FVector CameraPosition;
    float FogHeight;
    FMatrix InverseView;
    FMatrix InverseProjection;
    float DisableFog;
    FVector padding1;
};

struct FScreenConstants
{
    FVector2D ViewportRatio;
    FVector2D ViewportPosition;
};