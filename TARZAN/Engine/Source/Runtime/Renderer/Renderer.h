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
#include <filesystem>

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
    
#pragma region UberShader
    // Uber.hlsl이 마지막으로 수정된 시각
    std::filesystem::file_time_type LastUberWriteTime;

    // Shading Model은 ViewPort마다 지정

    // [0]에는 현재 사용되는 버전, [1]에는 가장 최근에 컴파일 성공한 버전을 넣어서
    // 컴파일 실패한 경우에 대해 대처할 수 있도록 작업
    
    // Uber Input Layout
    ID3D11InputLayout* UberInputLayout[2] = { nullptr, nullptr };
    
    // Normal VertexShader (Not Use Gouraud Shading)
    // Used by Unlit, Lambert, Blinn-Phong
    ID3D11VertexShader* NormalVS[2] = { nullptr, nullptr };

    // Gouraud VertexShader
    // Used by Gouraud
    ID3D11VertexShader* GouraudVS[2] = { nullptr, nullptr };

    // Gouruad PixelShader
    // Unlit과 내부 코드가 달라서 PixelShader 분리
    ID3D11PixelShader* GouraudPS[2] = { nullptr, nullptr };

    // Lambert PixelShader
    // Used by Lambert
    ID3D11PixelShader* LambertPS[2] = { nullptr, nullptr };

    // BlinnPhong PixelShader
    // Used by BlinnPhong
    ID3D11PixelShader* BlinnPhongPS[2] = { nullptr, nullptr };

    // Normal PixelShader
    ID3D11PixelShader* NormalPS[2] = { nullptr, nullptr };

    // Unlit PixelShader
    ID3D11PixelShader* UnlitPS[2] = { nullptr, nullptr };

#pragma endregion

    // GBuffer Shader
    ID3D11VertexShader* GBufferVS = nullptr;
    ID3D11PixelShader* GBufferPS = nullptr;
    ID3D11InputLayout* GBufferInputLayout = nullptr;

    // Lighting Shader
    ID3D11PixelShader* LightingPassPS = nullptr;

    // PostProcess Shader
    ID3D11PixelShader* PostProcessPassPS = nullptr;

    // Overlay Shader
    ID3D11PixelShader* GizmoPixelShader = nullptr;

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;

    // Constant Buffer
    ID3D11Buffer* ConstantBuffer = nullptr;
    ID3D11Buffer* ObjectMatrixConstantBuffer = nullptr;
    ID3D11Buffer* CameraConstantBuffer = nullptr;
    ID3D11Buffer* LightConstantBuffer = nullptr;
    ID3D11Buffer* MaterialConstantBuffer = nullptr;

    ID3D11Buffer* LightingBuffer = nullptr;
    ID3D11Buffer* FlagBuffer = nullptr;
    ID3D11Buffer* GMaterialConstantBuffer = nullptr;
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
    void Initialize(FGraphicsDevice* InGraphics);
   
    //Render
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const;
    void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const;
    void RenderPrimitive(OBJ::FStaticMeshRenderData* renderData, TArray<FStaticMaterial*> materials, TArray<UMaterial*> overrideMaterial, int selectedSubMeshIndex) const;
   
    void RenderTexturedModelPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* InTextureSRV, ID3D11SamplerState* InSamplerState) const;
    
    //Release
    void Release();
    void ReleaseShader();
    void ReleaseHotReloadShader();
    void ReleaseConstantBuffer();

    // Shader
    void CreateShader();
    void CreateUberShader(); //UberShader는 Hot Reload를 지원하므로 파트 분리 하긴 했지만.. 전체 다 하는 것이 안전할 것 같다.
    void PrepareUberShader() const;
    void PrepareShader() const;
    void PrepareLightShader() const;
    void PreparePostProcessShader() const;
    void PrepareTextureShader() const;
    void PrepareSubUVConstant() const;
    void PrepareGizmoShader() const;

    // Sampler State
    void SetSampler();

    void ChangeViewMode(EViewModeIndex evi) const;
    
    // CreateBuffer
    void CreateConstantBuffer();

    // Material
    void UpdateMaterial(const FObjMaterialInfo& MaterialInfo) const;

private:
    void RenderUberPass();
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
    void RenderBatch(const FGridParameters& gridParam, ID3D11Buffer* pVertexBuffer, int boundingBoxCount, int coneCount, int coneSegmentCount, int obbCount, int circleCount, int circleSegmentCount) const;
    void UpdateGridConstantBuffer(const FGridParameters& gridParams) const;
    void UpdateLinePrimitveCountBuffer(int numBoundingBoxes, int numCones, int numCircles) const;

    ID3D11ShaderResourceView* CreateBoundingBoxSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes);
    ID3D11ShaderResourceView* CreateOBBSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes);
    ID3D11ShaderResourceView* CreateConeSRV(ID3D11Buffer* pConeBuffer, UINT numCones);
    ID3D11ShaderResourceView* CreateCircleSRV(ID3D11Buffer* pCircleBuffer, UINT numCircles);


    void UpdateBoundingBoxBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FBoundingBox>& BoundingBoxes, int numBoundingBoxes) const;
    void UpdateOBBBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FOBB>& BoundingBoxes, int numBoundingBoxes) const;
    void UpdateConesBuffer(ID3D11Buffer* pConeBuffer, const TArray<FCone>& Cones, int numCones) const;
    void UpdateCirclesBuffer(ID3D11Buffer* pCircleBuffer, const TArray<FCircle>& Circles, int numCircles) const;

    //Render Pass Demo
    void PrepareRender();
    void ClearRenderArr();
    void RenderStaticMeshes();
    void RenderGizmos();
    void RenderLight();
    void RenderBillboards();
    void RenderFullScreenQuad();

    // Hot Reload
    bool UberIsOutDate();
    bool LineIsOutDate();
    void HotReloadUberShader();
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
    std::filesystem::file_time_type LastLineWriteTime;

    ID3D11VertexShader* VertexLineShader[2] = { nullptr, nullptr };
    ID3D11PixelShader* PixelLineShader[2] = { nullptr, nullptr };
    ID3D11Buffer* GridConstantBuffer = nullptr;
    ID3D11Buffer* LinePrimitiveBuffer = nullptr;
    ID3D11ShaderResourceView* pBBSRV = nullptr;
    ID3D11ShaderResourceView* pConeSRV = nullptr;
    ID3D11ShaderResourceView* pOBBSRV = nullptr;
    ID3D11ShaderResourceView* pCircleSRV = nullptr;

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
    void ResetFogUpdates();
};

