#pragma once
#include "Define.h"
#include <d3d11.h>
class UPrimitiveBatch
{
public:
    UPrimitiveBatch();
    ~UPrimitiveBatch();
    static UPrimitiveBatch& GetInstance() {
        static UPrimitiveBatch instance;
        return instance;
    }

public:
    void Release();
    void ClearGrid() {};
    float GetSpacing() { return GridParam.gridSpacing; }
    void GenerateGrid(float spacing, int gridCount);
    void RenderBatch(ID3D11Buffer* ConstantBuffer, const FMatrix& View, const FMatrix& Projection, const FVector4 CamPos);
    void InitializeVertexBuffer();
    void UpdateBoundingBoxResources();
    void ReleaseBoundingBoxResources();
    void UpdateConeResources();
    void ReleaseConeResources();
    void UpdateOBBResources();
    void ReleaseOBBResources();
    void UpdateCircleResources();
    void ReleaseCircleResources();
    void UpdateLineResources();
    void ReleaseLineResources();
    void RenderAABB(const FBoundingBox& localAABB, const FVector& center, const FMatrix& modelMatrix);
    void RenderOBB(const FBoundingBox& localAABB, const FVector& center, const FMatrix& modelMatrix);
	
    void AddCone(const FVector& Center, float Radius, float Angle, const FLinearColor& Color, const FMatrix& ModelMatrix);
    void AddCircle(const FVector& center, float radius, int segments, const FLinearColor& color, const FMatrix& modelMatrix);
    void AddLine(const FVector& start, const FVector& end, const FLinearColor& color);

    // 복사 생성자 및 대입 연산자 삭제
    UPrimitiveBatch(const UPrimitiveBatch&) = delete;
    UPrimitiveBatch& operator=(const UPrimitiveBatch&) = delete;
private:
    ID3D11Buffer* pVertexBuffer;
    ID3D11Buffer* pBoundingBoxBuffer;
    ID3D11ShaderResourceView* pBoundingBoxSRV;
    ID3D11Buffer* pConesBuffer;
    ID3D11Buffer* pCircleBuffer;
    ID3D11Buffer* pOBBBuffer;
    ID3D11Buffer* pLineBuffer;
    ID3D11ShaderResourceView* pConesSRV;
    ID3D11ShaderResourceView* pOBBSRV;
    ID3D11ShaderResourceView* pCircleSRV;
    ID3D11ShaderResourceView* pLineSRV;

    size_t allocatedBoundingBoxCapacity;
    size_t allocatedConeCapacity;
    size_t allocatedOBBCapacity;
    size_t allocatedCircleCapacity;
    size_t allocatedLineCapacity;
    TArray<FBoundingBox> BoundingBoxes;
    TArray<FOBB> OrientedBoundingBoxes;
    TArray<FCone> Cones;
    TArray<FCircle> Circles;
    TArray<FLine> Lines;

    FGridParameters GridParam;
    int ConeSegmentCount = 0;
    int CircleSegmentCount = 0;

};