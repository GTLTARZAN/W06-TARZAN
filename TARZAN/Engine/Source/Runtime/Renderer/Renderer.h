#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>
#include "EngineBaseTypes.h"
#include "Define.h"
#include "Container/Set.h"
#include "RenderResourceManager.h"
#include "ShaderManager.h"
#include "ConstantBufferUpdater.h"
#include "Pass/RenderPass.h"

class ULightComponentBase;
class UFireballComponent;
class UWorld;
class FGraphicsDevice;
class UMaterial;
struct FStaticMaterial;
class UObject;
class FEditorViewportClient;
class UBillboardComponent;
class UStaticMeshComponent;
class UGizmoBaseComponent;
class FRenderResourceManager;
class UHeightFogComponent;

class FRenderer 
{
public:
    ~FRenderer();
    void Render();
private:
    void RenderPass();
    void RenderImGui();
private:
    float litFlag = 0;
public:
    FGraphicsDevice* Graphics;

    // Full Screen Vertex Shader
    ID3D11VertexShader* FullScreenVS = nullptr;
    ID3D11InputLayout* FullScreenInputLayout = nullptr;
    
    // GBuffer Shader
    ID3D11VertexShader* GBufferVS = nullptr;
    ID3D11PixelShader* GBufferPS = nullptr;
    ID3D11InputLayout* GBufferInputLayout = nullptr;
    
    // Lighting Shader
    ID3D11PixelShader* LightingPassPS = nullptr;

    // PostProcess Shader
    ID3D11PixelShader* PostProcessPassPS = nullptr;

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;

    // Constant Buffer
    ID3D11Buffer* ConstantBuffer = nullptr;
    ID3D11Buffer* LightingBuffer = nullptr;
    ID3D11Buffer* FlagBuffer = nullptr;
    ID3D11Buffer* MaterialConstantBuffer = nullptr;
    ID3D11Buffer* SubMeshConstantBuffer = nullptr;
    ID3D11Buffer* TextureConstantBuffer = nullptr;
    ID3D11Buffer* FireballConstantBuffer = nullptr;
    ID3D11Buffer* LPLightConstantBuffer = nullptr;
    ID3D11Buffer* LPMaterialConstantBuffer = nullptr;
    ID3D11Buffer* FogConstantBuffer = nullptr;
    ID3D11Buffer* ScreenConstantBuffer = nullptr;

    // Data
    FLighting LightingData;
    FFogConstants FogData;

    // Stride
    uint32 FullScreenStride;
    uint32 Stride;
    uint32 Stride2;

    ID3D11SamplerState* LPSamplerState;

public:
    void Initialize(FGraphicsDevice* graphics);
   
    //Render
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const;
    void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const;
    void RenderPrimitive(OBJ::FStaticMeshRenderData* renderData, TArray<FStaticMaterial*> materials, TArray<UMaterial*> overrideMaterial, int selectedSubMeshIndex) const;
   
    void RenderTexturedModelPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* InTextureSRV, ID3D11SamplerState* InSamplerState) const;
    
    //Release
    void Release();
    void ReleaseShader();
    void ReleaseConstantBuffer();

    // Shader
    void CreateShader();
    void PrepareShader() const;
    void PrepareLightShader() const;
    void PreparePostProcessShader() const;
    void PrepareTextureShader() const;
    void PrepareSubUVConstant() const;

    // Sampler State
    void SetSampler();

    void ChangeViewMode(EViewModeIndex evi) const;
    
    // CreateBuffer
    void CreateConstantBuffer();

    // Material
    void UpdateMaterial(const FObjMaterialInfo& MaterialInfo) const;

private:
    // Render Pass
    void RenderGBuffer();
    void RenderLightPass();
    void RenderPostProcessPass();
    void RenderOverlayPass();

public:
    ID3D11VertexShader* VertexTextureShader = nullptr;
    ID3D11PixelShader* PixelTextureShader = nullptr;
    ID3D11InputLayout* TextureInputLayout = nullptr;

    uint32 TextureStride;
    struct FSubUVConstant
    {
        float indexU;
        float indexV;
    };
    ID3D11Buffer* SubUVConstantBuffer = nullptr;

public:
    void RenderTexturePrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices,
        ID3D11Buffer* pIndexBuffer, UINT numIndices,
        ID3D11ShaderResourceView* _TextureSRV,
        ID3D11SamplerState* _SamplerState) const;
    void RenderTextPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices,
        ID3D11ShaderResourceView* _TextureSRV,
        ID3D11SamplerState* _SamplerState) const;

public:
    // line
    void PrepareLineShader() const;
    void RenderBatch(const FGridParameters& gridParam, ID3D11Buffer* pVertexBuffer, int boundingBoxCount, int coneCount, int coneSegmentCount, int obbCount) const;
    void UpdateGridConstantBuffer(const FGridParameters& gridParams) const;
    void UpdateLinePrimitveCountBuffer(int numBoundingBoxes, int numCones) const;

    ID3D11ShaderResourceView* CreateBoundingBoxSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes);
    ID3D11ShaderResourceView* CreateOBBSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes);
    ID3D11ShaderResourceView* CreateConeSRV(ID3D11Buffer* pConeBuffer, UINT numCones);

    void UpdateBoundingBoxBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FBoundingBox>& BoundingBoxes, int numBoundingBoxes) const;
    void UpdateOBBBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FOBB>& BoundingBoxes, int numBoundingBoxes) const;
    void UpdateConesBuffer(ID3D11Buffer* pConeBuffer, const TArray<FCone>& Cones, int numCones) const;

    //Render Pass Demo
    void PrepareRender();
    void ClearRenderArr();
    void RenderStaticMeshes(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void RenderGizmos(const UWorld* World, const std::shared_ptr<FEditorViewportClient>& ActiveViewport);
    void RenderLight(UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void RenderBillboards(UWorld* World,std::shared_ptr<FEditorViewportClient> ActiveViewport);
    void RenderFullScreenQuad();
private:
    // GBuffer
    TArray<UStaticMeshComponent*> StaticMeshObjs;
    TArray<UBillboardComponent*> BillboardObjs;

    // Lighting
    TArray<ULightComponentBase*> LightObjs;
    TArray<UFireballComponent*> FireballObjs;

    // Overaly
    TArray<UGizmoBaseComponent*> GizmoObjs;

public:
    ID3D11VertexShader* VertexLineShader = nullptr;
    ID3D11PixelShader* PixelLineShader = nullptr;
    ID3D11Buffer* GridConstantBuffer = nullptr;
    ID3D11Buffer* LinePrimitiveBuffer = nullptr;
    ID3D11ShaderResourceView* pBBSRV = nullptr;
    ID3D11ShaderResourceView* pConeSRV = nullptr;
    ID3D11ShaderResourceView* pOBBSRV = nullptr;

public:
    FRenderResourceManager& GetResourceManager() { return RenderResourceManager; }
    FShaderManager& GetShaderManager() { return ShaderManager; }
    FConstantBufferUpdater& GetConstantBufferUpdater() { return ConstantBufferUpdater; }

private:
    FRenderResourceManager RenderResourceManager;
    FShaderManager ShaderManager;
    FConstantBufferUpdater ConstantBufferUpdater;

    //UImGuiManager* UIMgr;
    // TODO: 리펙토링 할 때 이거 EditorEngine에서 다 옮겨와야함.
    //UnrealEd* UnrealEditor;
    //FGraphicsDevice* graphicDevice;
    //FResourceMgr* reourceMgr;
    
private:
    ID3D11DeviceContext* Context;

    ID3D11SamplerState* SamplerState;
    D3D11_SAMPLER_DESC SamplerDesc = {};

    std::shared_ptr<FEditorViewportClient> ActiveViewport;
    UWorld* World;

    void SubscribeToFogUpdates(UHeightFogComponent* HeightFog);
};

