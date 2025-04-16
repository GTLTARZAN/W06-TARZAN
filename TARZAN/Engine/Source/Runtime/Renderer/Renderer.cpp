#include "Renderer.h"
#include <d3dcompiler.h>
#include <functional>

#include "Engine/World.h"
#include "Actors/Player.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UBillboardComponent.h"
#include "Components/UParticleSubUVComp.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/FireballComp.h"
#include "Components/UHeightFogComponent.h"
#include "Components/SkySphereComponent.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Components/UText.h"
#include "Components/Material/Material.h"
#include "Launch/EditorEngine.h"
#include "Math/JungleMath.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "UObject/Casts.h"
#include "UObject/Object.h"
#include "PropertyEditor/ShowFlags.h"
#include "UObject/UObjectIterator.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Renderer/Pass/GBufferPass.h"
#include "Renderer/Pass/LightingPass.h"
#include "Renderer/Pass/PostProcessPass.h"
#include "Renderer/Pass/OverlayPass.h"
#include "Editor/LevelEditor/SLevelEditor.h"
#include "Runtime/Launch/ImGuiManager.h"
#include "UnrealEd/UnrealEd.h"
#include "UnrealEd/EditorViewportClient.h"

extern UEditorEngine* GEngine;

#pragma region Base
FRenderer::~FRenderer() {}

void FRenderer::Initialize(FGraphicsDevice* InGraphics)
{
    Graphics = InGraphics;
    RenderResourceManager.Initialize(Graphics->Device);
    ShaderManager.Initialize(Graphics->Device, Graphics->DeviceContext);
    ConstantBufferUpdater.Initialize(Graphics->DeviceContext);

    CreateShader();
    CreateConstantBuffer();
    ConstantBufferUpdater.UpdateLitUnlitConstant(FlagBuffer, 1);
    SetSampler();

    //UIMgr = new UImGuiManager;
    //UIMgr->Initialize(hWnd, graphicDevice.Device, graphicDevice.DeviceContext);
}

void FRenderer::Render()
{
    //DeprecatedRender();

    SLevelEditor* LevelEditor = GEngine->GetLevelEditor();
    const std::shared_ptr<FEditorViewportClient> CurrentViewport = LevelEditor->GetActiveViewportClient();
    World = GEngine->GetWorld();

    Graphics->Prepare();
    if (LevelEditor->IsMultiViewport())
    {
        for (int i = 0; i < 4; ++i)
        {
            LevelEditor->SetViewportClient(i);
            ActiveViewport = LevelEditor->GetActiveViewportClient();
            PrepareRender();
            RenderPass();
        }
        LevelEditor->SetViewportClient(CurrentViewport);
    }
    else
    {
        ActiveViewport = LevelEditor->GetActiveViewportClient();
        PrepareRender();
        RenderPass();
    }

    CheckGenerateLightTree();

    ClearRenderArr();
    RenderImGui();
    GEngine->graphicDevice.SwapBuffer();
}

void FRenderer::RenderPass()
{
    Graphics->DeviceContext->RSSetViewports(1, &ActiveViewport->GetD3DViewport());
    Graphics->ChangeRasterizer(ActiveViewport->GetViewMode());
    ChangeViewMode(ActiveViewport->GetViewMode());

#if USE_GBUFFER
    RenderGBuffer();

    RenderLightPass();
#else
    RenderUberPass();
#endif

    RenderPostProcessPass();
    RenderOverlayPass();
}

void FRenderer::RenderImGui()
{
    UImGuiManager* UIMgr = GEngine->GetUIManager();
    UnrealEd* UnrealEditor = GEngine->GetUnrealEditor();

    UIMgr->BeginFrame();

    UnrealEditor->Render();
    Console::GetInstance().Draw();

    UIMgr->EndFrame();
}

void FRenderer::Release()
{
    ReleaseShader();
    ReleaseConstantBuffer();
}
#pragma endregion Base

#pragma region Shader
void FRenderer::CreateShader()
{
    // FullScreen Shader
    D3D11_INPUT_ELEMENT_DESC fullscreenLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ShaderManager.CreateVertexShader(
        L"Shaders/FullScreenVertexShader.hlsl", "main", FullScreenVS,
        fullscreenLayout, ARRAYSIZE(fullscreenLayout), &FullScreenInputLayout, &FullScreenStride, sizeof(FVertexTexture));

#if USE_GBUFFER
    // GBuffer Shader
    D3D11_INPUT_ELEMENT_DESC GBufferLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"MATERIAL_INDEX", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    ShaderManager.CreateVertexShader(L"Shaders/MeshVertexShader.hlsl", "main",
        GBufferVS, GBufferLayout, ARRAYSIZE(GBufferLayout), &GBufferInputLayout, &Stride, sizeof(FVertexSimple));

    ShaderManager.CreatePixelShader(L"Shaders/MeshPixelShader.hlsl", "main", GBufferPS);

    ShaderManager.CreatePixelShader(L"Shaders/LightingPassPixelShader.hlsl", "main", LightingPassPS);
#else
    CreateUberShader();
#endif

    ShaderManager.CreatePixelShader(L"Shaders/GizmoPixelShader.hlsl", "main", GizmoPixelShader, nullptr);

    // Texture Shader
    D3D11_INPUT_ELEMENT_DESC textureLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    ShaderManager.CreateVertexShader(
        L"Shaders/VertexTextureShader.hlsl", "main",
        VertexTextureShader, textureLayout, ARRAYSIZE(textureLayout), &TextureInputLayout, &TextureStride, sizeof(FVertexTexture));
    ShaderManager.CreatePixelShader(
        L"Shaders/PixelTextureShader.hlsl", "main",
        PixelTextureShader, nullptr);

    // Line Shader
    LastLineWriteTime = std::filesystem::last_write_time("Shaders/ShaderLine.hlsl");

    ShaderManager.CreateVertexShader(
        L"Shaders/ShaderLine.hlsl", "mainVS",
        VertexLineShader[0], VertexLineShader[1], nullptr, 0); // 라인 셰이더는 Layout 안 쓰면 nullptr 전달
    ShaderManager.CreatePixelShader(
        L"Shaders/ShaderLine.hlsl", "mainPS",
        PixelLineShader[0], PixelLineShader[1], ELightingModel::None);

    // Fog Shader
    ShaderManager.CreatePixelShader(
        L"Shaders/PostProcessPixelShader.hlsl", "mainPS", PostProcessPassPS, nullptr);
}

void FRenderer::CreateUberShader()
{
    LastUberWriteTime = std::filesystem::last_write_time("Shaders/Uber.hlsl");

    D3D11_INPUT_ELEMENT_DESC UberLayout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"MATERIAL_INDEX", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    // Create NormalVertxShader
    ShaderManager.CreateVertexShader(L"Shaders/Uber.hlsl", "Uber_VS",
        NormalVS[0], NormalVS[1], UberLayout, ARRAYSIZE(UberLayout), &UberInputLayout[0], &UberInputLayout[1], 
        &Stride, sizeof(FVertexSimple), ELightingModel::Lambert);

    // Create GouraudVertexShader
    ShaderManager.CreateVertexShader(L"Shaders/Uber.hlsl", "Uber_VS",
        GouraudVS[0], GouraudVS[1], UberLayout, ARRAYSIZE(UberLayout), &UberInputLayout[0], &UberInputLayout[1], 
        &Stride, sizeof(FVertexSimple), ELightingModel::Gouraud);

    // Create UnlitPixelShader
    ShaderManager.CreatePixelShader(L"Shaders/Uber.hlsl", "Uber_PS", UnlitPS[0], UnlitPS[1], ELightingModel::Unlit);

    // Create GouraudPixelShader
    ShaderManager.CreatePixelShader(L"Shaders/Uber.hlsl", "Uber_PS", GouraudPS[0], GouraudPS[1], ELightingModel::Gouraud);

    // Create LambertPixelShader
    ShaderManager.CreatePixelShader(L"Shaders/Uber.hlsl", "Uber_PS", LambertPS[0], LambertPS[1], ELightingModel::Lambert);

    // Create BlinnPhongPixelShader
    ShaderManager.CreatePixelShader(L"Shaders/Uber.hlsl", "Uber_PS", BlinnPhongPS[0], BlinnPhongPS[1], ELightingModel::BlinnPhong);

    // Create NormalPixelShader
    ShaderManager.CreatePixelShader(L"Shaders/Uber.hlsl", "Uber_PS", NormalPS[0], NormalPS[1], ELightingModel::Normal);
}

void FRenderer::ReleaseShader()
{
    // UberShader 관련 부분들은 Hot Reload에 대한 대응이 필요하여 ReleaseUberShader에서 따로 처리

    // 중복 Release로 Crash 터짐, 중복 Release가 안 막아진다.
    ShaderManager.ReleaseShader(InputLayout, VertexShader, PixelShader);
    ShaderManager.ReleaseShader(TextureInputLayout, VertexTextureShader, PixelTextureShader);
}

void FRenderer::ReleaseHotReloadShader()
{
    // [0] 과 [1]이 다르다는 건 [1]이 옛날꺼고 [0]이 성공적으로 컴파일 되었다는것
    if (UberInputLayout[0] != UberInputLayout[1])
    {
        UberInputLayout[1]->Release();
    }

    if (GouraudVS[0] != GouraudVS[1])
    {
        GouraudVS[1]->Release();
    }

    if (GouraudPS[0] != GouraudPS[1])
    {
        GouraudPS[1]->Release();
    }

    if (NormalVS[0] != NormalVS[1])
    {
        NormalVS[1]->Release();
    }
    if (LambertPS[0] != LambertPS[1])
    {
        LambertPS[1]->Release();
    }

    if (BlinnPhongPS[0] != BlinnPhongPS[1]) 
    {
        BlinnPhongPS[1]->Release();
    }

    if (NormalPS[0] != NormalPS[1])
    {
        NormalPS[1]->Release();
    }

    if (UnlitPS[0] != UnlitPS[1]) 
    {
        UnlitPS[1]->Release();
    }

    if (VertexLineShader[0] != VertexLineShader[1]) 
    {
        VertexLineShader[1]->Release();
    }

    if (PixelLineShader[0] != PixelLineShader[1]) 
    {
        PixelLineShader[1]->Release();
    }
}

void FRenderer::PrepareUberShader() const
{
    Graphics->DeviceContext->IASetInputLayout(UberInputLayout[0]);

    switch (ActiveViewport->GetLighitingModel())
    {
    case ELightingModel::None:
        UE_LOG(LogLevel::Display, "No LightingModel");
        break;
    case ELightingModel::Gouraud:
        Graphics->DeviceContext->VSSetShader(GouraudVS[0], nullptr, 0);
        Graphics->DeviceContext->PSSetShader(GouraudPS[0], nullptr, 0);
        break;
    case ELightingModel::Lambert:
        Graphics->DeviceContext->VSSetShader(NormalVS[0], nullptr, 0);
        Graphics->DeviceContext->PSSetShader(LambertPS[0], nullptr, 0);
        break;
    case ELightingModel::BlinnPhong:
        Graphics->DeviceContext->VSSetShader(NormalVS[0], nullptr, 0);
        Graphics->DeviceContext->PSSetShader(BlinnPhongPS[0], nullptr, 0);
        break;
    case ELightingModel::Unlit:
        Graphics->DeviceContext->VSSetShader(NormalVS[0], nullptr, 0);
        Graphics->DeviceContext->PSSetShader(UnlitPS[0], nullptr, 0);
        break;
    case ELightingModel::Normal:
        Graphics->DeviceContext->VSSetShader(NormalVS[0], nullptr, 0);
        Graphics->DeviceContext->PSSetShader(NormalPS[0], nullptr, 0);
        break;
    }
    
    if (ObjectMatrixConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ObjectMatrixConstantBuffer);
        Graphics->DeviceContext->VSSetConstantBuffers(1, 1, &CameraConstantBuffer);
        Graphics->DeviceContext->VSSetConstantBuffers(2, 1, &LightConstantBuffer);
        Graphics->DeviceContext->VSSetConstantBuffers(3, 1, &MaterialConstantBuffer);

        Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &CameraConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(2, 1, &LightConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(3, 1, &MaterialConstantBuffer);
    }
}

// Prepare
void FRenderer::PrepareShader() const
{
    Graphics->DeviceContext->VSSetShader(GBufferVS, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(GBufferPS, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(GBufferInputLayout);

    if (ConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(0, 1, &GMaterialConstantBuffer);
    }
}

void FRenderer::PrepareLightShader() const
{
    Graphics->DeviceContext->VSSetShader(FullScreenVS, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(LightingPassPS, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(FullScreenInputLayout);

    if (LPLightConstantBuffer)
    {
        Graphics->DeviceContext->PSSetConstantBuffers(0, 1, &LPLightConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &FireballConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(2, 1, &ScreenConstantBuffer);
        //Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &LPMaterialConstantBuffer);
    }
}

void FRenderer::PreparePostProcessShader() const
{
    Graphics->DeviceContext->VSSetShader(FullScreenVS, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PostProcessPassPS, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(FullScreenInputLayout);

    if (FogConstantBuffer)
    {
        Graphics->DeviceContext->PSSetConstantBuffers(0, 1, &FogConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(2, 1, &ScreenConstantBuffer);
    }
}

void FRenderer::PrepareTextureShader() const
{
    Graphics->DeviceContext->VSSetShader(VertexTextureShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelTextureShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(TextureInputLayout);

    if (ConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);

        Graphics->DeviceContext->PSSetConstantBuffers(3, 1, &TextureMaterialConstants);
    }
}

void FRenderer::PrepareSubUVConstant() const
{
    if (SubUVConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(1, 1, &SubUVConstantBuffer);
        Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &SubUVConstantBuffer);
    }
}

void FRenderer::PrepareGizmoShader() const
{
#if USE_GBUFFER
    Graphics->DeviceContext->VSSetShader(GBufferVS[0], nullptr, 0);
#else
    Graphics->DeviceContext->VSSetShader(NormalVS[1], nullptr, 0);
#endif
    Graphics->DeviceContext->PSSetShader(GizmoPixelShader, nullptr, 0);

    if (ObjectMatrixConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ObjectMatrixConstantBuffer);

        Graphics->DeviceContext->PSSetConstantBuffers(0, 1, &MaterialConstantBuffer);
    }
}

void FRenderer::PrepareLineShader() const
{
    Graphics->DeviceContext->VSSetShader(VertexLineShader[0], nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelLineShader[0], nullptr, 0);

    if (ConstantBuffer && GridConstantBuffer)
    {
        Graphics->DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);     // MatrixBuffer (b0)
        Graphics->DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);

        Graphics->DeviceContext->VSSetConstantBuffers(1, 1, &GridConstantBuffer); // GridParameters (b1)
        Graphics->DeviceContext->PSSetConstantBuffers(1, 1, &GridConstantBuffer);
        Graphics->DeviceContext->VSSetConstantBuffers(3, 1, &LinePrimitiveBuffer);

        Graphics->DeviceContext->VSSetShaderResources(2, 1, &pBBSRV);
        Graphics->DeviceContext->VSSetShaderResources(3, 1, &pConeSRV);
        Graphics->DeviceContext->VSSetShaderResources(4, 1, &pOBBSRV);
        Graphics->DeviceContext->VSSetShaderResources(5, 1, &pCircleSRV);
        Graphics->DeviceContext->VSSetShaderResources(6, 1, &pLineSRV);
    }
}
#pragma endregion Shader

#pragma region ConstantBuffer
void FRenderer::CreateConstantBuffer()
{
    //ConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FConstants));
    SubUVConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FSubUVConstant));
    GridConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FGridParameters));

    LinePrimitiveBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FPrimitiveCounts));
    //MaterialConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FMaterialConstants));
    SubMeshConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FSubMeshConstants));
    TextureConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FTextureConstants));
    LightingBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FLighting));
    FlagBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FLitUnlitConstants));

    ConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FConstants));
#if USE_GBUFFER
    GMaterialConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FMaterialConstants));
#else
    ObjectMatrixConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FObjectMatrixConstants));
    CameraConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FCameraConstant));
    LightConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FLightConstants));
    MaterialConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FMaterialConstants));
#endif
    TextureMaterialConstants = RenderResourceManager.CreateConstantBuffer(sizeof(FTextureMaterialConstants));

    LPLightConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FLightConstant));
    FireballConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FFireballArrayInfo));

    FogConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FFogConstants));

    ScreenConstantBuffer = RenderResourceManager.CreateConstantBuffer(sizeof(FScreenConstants));
}

void FRenderer::ReleaseConstantBuffer()
{
    RenderResourceManager.ReleaseBuffer(ConstantBuffer);
    RenderResourceManager.ReleaseBuffer(SubUVConstantBuffer);
    RenderResourceManager.ReleaseBuffer(GridConstantBuffer);
    RenderResourceManager.ReleaseBuffer(LinePrimitiveBuffer);
    RenderResourceManager.ReleaseBuffer(GMaterialConstantBuffer);
    RenderResourceManager.ReleaseBuffer(SubMeshConstantBuffer);
    RenderResourceManager.ReleaseBuffer(TextureConstantBuffer);
    RenderResourceManager.ReleaseBuffer(LightingBuffer);
    RenderResourceManager.ReleaseBuffer(FlagBuffer);
    RenderResourceManager.ReleaseBuffer(FireballConstantBuffer);
    RenderResourceManager.ReleaseBuffer(LPLightConstantBuffer);
    RenderResourceManager.ReleaseBuffer(LPMaterialConstantBuffer);
    RenderResourceManager.ReleaseBuffer(FogConstantBuffer);
    RenderResourceManager.ReleaseBuffer(ScreenConstantBuffer);
    RenderResourceManager.ReleaseBuffer(TextureMaterialConstants);
}
#pragma endregion ConstantBuffer

#pragma region Sampler State
void FRenderer::SetSampler()
{
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
}
#pragma endregion

#pragma region Render
void FRenderer::ClearRenderArr()
{
    StaticMeshObjs.Empty();
    GizmoObjs.Empty();
    BillboardObjs.Empty();
    LightObjs.Empty();
    FireballObjs.Empty();
}

void FRenderer::PrepareRender()
{
    bool bHasFog = false;
    if (GEngine->GetWorld()->WorldType == EWorldType::Editor)
    {
        for (const auto iter : TObjectRange<USceneComponent>())
        {
            UE_LOG(LogLevel::Display, "%d", GUObjectArray.GetObjectItemArrayUnsafe().Num());
            if (UStaticMeshComponent* pStaticMeshComp = Cast<UStaticMeshComponent>(iter))
            {
                if (!Cast<UGizmoBaseComponent>(iter))
                    StaticMeshObjs.Add(pStaticMeshComp);
            }
            if (UGizmoBaseComponent* pGizmoComp = Cast<UGizmoBaseComponent>(iter))
            {
                GizmoObjs.Add(pGizmoComp);
            }
            if (UBillboardComponent* pBillboardComp = Cast<UBillboardComponent>(iter))
            {
                BillboardObjs.Add(pBillboardComp);
            }
            if (ULightComponentBase* pLightComp = Cast<ULightComponentBase>(iter))
            {
                LightObjs.Add(pLightComp);
            }
            if (UFireballComponent* pFireComp = Cast<UFireballComponent>(iter))
            {
                FireballObjs.Add(pFireComp);
            }
            if (UHeightFogComponent* HeightFog = Cast<UHeightFogComponent>(iter))
            {
                SubscribeToFogUpdates(HeightFog);
                bHasFog = true;
            }
        }
        if (!bHasFog)
        {
            ResetFogUpdates();
        }
    }
    else if (GEngine->GetWorld()->WorldType == EWorldType::PIE)
    {
        // UE_LOG(LogLevel::Display, "%d", GEngine->GetWorld()->GetActors().Num() );
        for (const auto iter : GEngine->GetWorld()->GetActors())
        {
            for (const auto iter2 : iter->GetComponents())
            {
                if (UStaticMeshComponent* pStaticMeshComp = Cast<UStaticMeshComponent>(iter2))
                {
                    if (!Cast<UGizmoBaseComponent>(iter2))
                        StaticMeshObjs.Add(pStaticMeshComp);
                }
                if (UBillboardComponent* pBillboardComp = Cast<UBillboardComponent>(iter2))
                {
                    BillboardObjs.Add(pBillboardComp);
                }
                if (ULightComponentBase* pLightComp = Cast<ULightComponentBase>(iter2))
                {
                    LightObjs.Add(pLightComp);
                }
                if (UFireballComponent* pFireComp = Cast<UFireballComponent>(iter2))
                {
                    FireballObjs.Add(pFireComp);
                }
                if (UHeightFogComponent* HeightFog = Cast<UHeightFogComponent>(iter2))
                {
                    SubscribeToFogUpdates(HeightFog);
                    bHasFog = true;
                }
            }
        }
        if (!bHasFog)
        {
            ResetFogUpdates();
        }
    }
}

void FRenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
    Graphics->DeviceContext->Draw(numVertices, 0);
}

void FRenderer::RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

void FRenderer::RenderPrimitive(OBJ::FStaticMeshRenderData* renderData, TArray<FStaticMaterial*> materials, TArray<UMaterial*> overrideMaterial, int selectedSubMeshIndex = -1) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &renderData->VertexBuffer, &Stride, &offset);
    if (renderData->IndexBuffer)
        Graphics->DeviceContext->IASetIndexBuffer(renderData->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // submesh X
    if (renderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(renderData->Indices.Num(), 0, 0);
    }

    // submesh O
    for (int subMeshIndex = 0; subMeshIndex < renderData->MaterialSubsets.Num(); subMeshIndex++)
    {
        int materialIndex = renderData->MaterialSubsets[subMeshIndex].MaterialIndex;

        subMeshIndex == selectedSubMeshIndex ?
            ConstantBufferUpdater.UpdateSubMeshConstant(SubMeshConstantBuffer, true)
            : ConstantBufferUpdater.UpdateSubMeshConstant(SubMeshConstantBuffer, false);

        overrideMaterial[materialIndex] != nullptr ?
            UpdateMaterial(overrideMaterial[materialIndex]->GetMaterialInfo())
            : UpdateMaterial(materials[materialIndex]->Material->GetMaterialInfo());

        //UpdateMaterial(materials[materialIndex]->Material->GetMaterialInfo());

        if (renderData->IndexBuffer)
        {
            // index draw
            uint64 startIndex = renderData->MaterialSubsets[subMeshIndex].IndexStart;
            uint64 indexCount = renderData->MaterialSubsets[subMeshIndex].IndexCount;
            Graphics->DeviceContext->DrawIndexed(indexCount, startIndex, 0);
        }
    }
}

void FRenderer::RenderTexturePrimitive(
    ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* _TextureSRV,
    ID3D11SamplerState* _SamplerState
) const
{
    if (!_TextureSRV || !_SamplerState)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "SRV, Sampler Error");
    }
    if (numIndices <= 0)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "numIndices Error");
    }
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &TextureStride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &_TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &_SamplerState);
    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

void FRenderer::RenderTextPrimitive(
    ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11ShaderResourceView* _TextureSRV, ID3D11SamplerState* _SamplerState
) const
{
    if (!_TextureSRV || !_SamplerState)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "SRV, Sampler Error");
    }
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &TextureStride, &offset);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &_TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &_SamplerState);
    Graphics->DeviceContext->Draw(numVertices, 0);
}

void FRenderer::RenderTexturedModelPrimitive(
    ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* InTextureSRV,
    ID3D11SamplerState* InSamplerState
) const
{
    if (!InTextureSRV || !InSamplerState)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "SRV, Sampler Error");
    }
    if (numIndices <= 0)
    {
        Console::GetInstance().AddLog(LogLevel::Warning, "numIndices Error");
    }
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    //Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &InTextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &InSamplerState);

    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

void FRenderer::RenderStaticMeshes()
{
#if USE_GBUFFER
    PrepareShader();
#else
    PrepareUberShader();
#endif

    for (UStaticMeshComponent* StaticMeshComp : StaticMeshObjs)
    {
        FMatrix Model = JungleMath::CreateModelMatrix(
            StaticMeshComp->GetWorldLocation(),
            StaticMeshComp->GetWorldRotation(),
            StaticMeshComp->GetWorldScale()
        );
        
        FObjectMatrixConstants MatrixConstant =
        {
            .World = Model,
            .View = ActiveViewport->GetViewMatrix(),
            .Projection = ActiveViewport->GetProjectionMatrix(),
            .NormalMatrix = FMatrix::Transpose(FMatrix::Inverse(Model))
        };
        ConstantBufferUpdater.UpdateObjectMatrixConstants(ObjectMatrixConstantBuffer, MatrixConstant);

        FCameraConstant CameraConstant =
        {
            .CameraWorldPos = ActiveViewport->GetCameraLocation()
        };
        ConstantBufferUpdater.UpdateCameraPositionConstants(CameraConstantBuffer, CameraConstant);

        // Point Light
        FAmbientLightInfo Ambient;
        FDirectionalLightInfo DirectionalLight;
        std::unique_ptr<FPointLightArrayInfo> PointLightArrayInfo = std::make_unique<FPointLightArrayInfo>();
        std::unique_ptr<FSpotLightArrayInfo> SpotLightArrayInfo = std::make_unique<FSpotLightArrayInfo>();
        
        if (PointLightTree.GetActive()) 
        {
            TArray<ULightComponentBase*> lightArray = PointLightTree.GetLightCutLights(StaticMeshComp);

            for (int i = 0; i < lightArray.Num(); i++) 
            {
                if (lightArray[i]->IsA<UPointLightComponent>())
                {
                    if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(lightArray[i]))
                    {
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].Color = PointLight->GetColor();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].Position = PointLight->GetWorldLocation();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].Intensity = PointLight->GetIntensity();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].AttenuationRadius = PointLight->GetRadius();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].LightFalloffExponent = PointLight->GetLightFalloffExponent();
                        PointLightArrayInfo->PointLightCount++;
                    }
                }
            }
        }


        if (LightObjs.Num() > 0)
        {
            for (int i = 0; i < LightObjs.Num(); i++)
            {
                if (LightObjs[i]->IsA<UAmbientLightComponent>())
                {
                    if (UAmbientLightComponent* AmbientLight = Cast<UAmbientLightComponent>(LightObjs[i]))
                    {
                        Ambient.Color = AmbientLight->GetColor();
                        Ambient.Intensity = AmbientLight->GetIntensity();
                    }
                }

                if (LightObjs[i]->IsA<UDirectionalLightComponent>())
                {
                    if (UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(LightObjs[i]))
                    {
                        DirectionalLight.Color = Light->GetColor();
                        DirectionalLight.Intensity = Light->GetIntensity();
                        DirectionalLight.Direction = Light->GetDirection();
                    }
                }

                if (LightObjs[i]->IsA<USpotLightComponent>())
                {
                    if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(LightObjs[i]))
                    {
                        SpotLightArrayInfo->SpotLightConstants[SpotLightArrayInfo->SpotLightCount].Color = SpotLight->GetColor();
                        SpotLightArrayInfo->SpotLightConstants[SpotLightArrayInfo->SpotLightCount].Position = SpotLight->GetWorldLocation();
                        SpotLightArrayInfo->SpotLightConstants[SpotLightArrayInfo->SpotLightCount].Direction = SpotLight->GetForwardVector();
                        SpotLightArrayInfo->SpotLightConstants[SpotLightArrayInfo->SpotLightCount].Intensity = SpotLight->GetIntensity();
                        SpotLightArrayInfo->SpotLightConstants[SpotLightArrayInfo->SpotLightCount].AttenuationRadius = SpotLight->GetRadius();
                        SpotLightArrayInfo->SpotLightConstants[SpotLightArrayInfo->SpotLightCount].InnerConeAngle = SpotLight->GetInnerConeAngle();
                        SpotLightArrayInfo->SpotLightConstants[SpotLightArrayInfo->SpotLightCount].OuterConeAngle = SpotLight->GetOuterConeAngle();
                        SpotLightArrayInfo->SpotLightCount++;
                    }

                    continue;
                }

                if (LightObjs[i]->IsA<UPointLightComponent>() && !PointLightTree.GetActive())  // PointLightTree가 켜져 있다면 그쪽에서 PointLight 처리를 해주므로
                {
                    if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(LightObjs[i]))
                    {
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].Color = PointLight->GetColor();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].Position = PointLight->GetWorldLocation();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].Intensity = PointLight->GetIntensity();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].AttenuationRadius = PointLight->GetRadius();
                        PointLightArrayInfo->PointLightConstants[PointLightArrayInfo->PointLightCount].LightFalloffExponent = PointLight->GetLightFalloffExponent();
                        PointLightArrayInfo->PointLightCount++;
                    }
                }

            }
        }

        std::unique_ptr<FFireballArrayInfo> fireballArrayInfo = std::make_unique<FFireballArrayInfo>();

        if (FireballObjs.Num() > 0)
        {
            fireballArrayInfo->FireballCount = 0;

            for (int i = 0; i < FireballObjs.Num(); i++)
            {
                if (FireballObjs[i] != nullptr)
                {
                    const FFireballInfo& fireballInfo = FireballObjs[i]->GetFireballInfo();
                    fireballArrayInfo->FireballConstants[i].Intensity = fireballInfo.Intensity;
                    fireballArrayInfo->FireballConstants[i].Radius = fireballInfo.Radius;
                    fireballArrayInfo->FireballConstants[i].Color = fireballInfo.Color;
                    fireballArrayInfo->FireballConstants[i].RadiusFallOff = fireballInfo.RadiusFallOff;
                    fireballArrayInfo->FireballConstants[i].Position = FireballObjs[i]->GetWorldLocation();
                    fireballArrayInfo->FireballConstants[i].LightType = fireballInfo.Type;
                    if (USpotLightComponent* spotLight = Cast<USpotLightComponent>(FireballObjs[i]))
                    {
                        fireballArrayInfo->FireballConstants[i].InnerAngle = spotLight->GetInnerConeAngle();
                        fireballArrayInfo->FireballConstants[i].OuterAngle = spotLight->GetOuterConeAngle();
                        fireballArrayInfo->FireballConstants[i].Direction = spotLight->GetForwardVector();
                    }
                    fireballArrayInfo->FireballCount++;
                }
            }
        }
        

        FPointLightInfo PointLight;
        FSpotLightInfo SpotLight;

        FLightConstants LightConstant =
        {
            .Ambient = Ambient,
            .Directional = DirectionalLight,
            .PointLights = PointLight,
            .SpotLights = SpotLight,
        };

        for (int i = 0; i < PointLightArrayInfo->PointLightCount; i++)
        {
            LightConstant.PointLights[i] = PointLightArrayInfo->PointLightConstants[i];
        }

        for (int i = 0; i < SpotLightArrayInfo->SpotLightCount; i++)
        {
            LightConstant.SpotLights[i] = SpotLightArrayInfo->SpotLightConstants[i];
        }

        ConstantBufferUpdater.UpdateLightConstants(LightConstantBuffer, LightConstant);

        //FMatrix MVP = Model * ActiveViewport->GetViewMatrix() * ActiveViewport->GetProjectionMatrix();
        //FMatrix NormalMatrix = FMatrix::Transpose(FMatrix::Inverse(Model));
        //FVector4 UUIDColor = StaticMeshComp->EncodeUUID() / 255.0f;
        //bool isSelected = World->GetSelectedActor() == StaticMeshComp->GetOwner();

        if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            UPrimitiveBatch::GetInstance().RenderAABB(
                StaticMeshComp->GetBoundingBox(),
                StaticMeshComp->GetWorldLocation(),
                Model
            );
        }

        if (!StaticMeshComp->GetStaticMesh()) continue;

        OBJ::FStaticMeshRenderData* renderData = StaticMeshComp->GetStaticMesh()->GetRenderData();
        if (renderData == nullptr) continue;

        RenderPrimitive(renderData, StaticMeshComp->GetStaticMesh()->GetMaterials(), StaticMeshComp->GetOverrideMaterials(), StaticMeshComp->GetselectedSubMeshIndex());
    }
}

void FRenderer::RenderGizmos()
{
    if (!World->GetSelectedActor())
    {
        return;
    }

    PrepareGizmoShader();

#pragma region GizmoDepth
    ID3D11DepthStencilState* DepthStateDisable = Graphics->DepthStateDisable;
    Graphics->DeviceContext->OMSetDepthStencilState(DepthStateDisable, 0);
#pragma endregion

    //  fill solid,  Wirframe 에서도 제대로 렌더링되기 위함
    Graphics->DeviceContext->RSSetState(UEditorEngine::graphicDevice.RasterizerStateSOLID);

    for (auto GizmoComp : GizmoObjs)
    {

        if ((GizmoComp->GetGizmoType() == UGizmoBaseComponent::ArrowX ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ArrowY ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
            && World->GetEditorPlayer()->GetControlMode() != CM_TRANSLATION)
            continue;
        else if ((GizmoComp->GetGizmoType() == UGizmoBaseComponent::ScaleX ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ScaleY ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::ScaleZ)
            && World->GetEditorPlayer()->GetControlMode() != CM_SCALE)
            continue;
        else if ((GizmoComp->GetGizmoType() == UGizmoBaseComponent::CircleX ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::CircleY ||
            GizmoComp->GetGizmoType() == UGizmoBaseComponent::CircleZ)
            && World->GetEditorPlayer()->GetControlMode() != CM_ROTATION)
            continue;
        FMatrix Model = JungleMath::CreateModelMatrix(GizmoComp->GetWorldLocation(),
            GizmoComp->GetWorldRotation(),
            GizmoComp->GetWorldScale()
        );
#if USE_GBUFFER
        FMatrix NormalMatrix = FMatrix::Transpose(FMatrix::Inverse(Model));
        FVector4 UUIDColor = GizmoComp->EncodeUUID() / 255.0f;

        FMatrix MVP = Model * ActiveViewport->GetViewMatrix() * ActiveViewport->GetProjectionMatrix();

        if (GizmoComp == World->GetPickingGizmo())
            ConstantBufferUpdater.UpdateConstant(ConstantBuffer, MVP, Model, NormalMatrix, UUIDColor, true);
        else
            ConstantBufferUpdater.UpdateConstant(ConstantBuffer, MVP, Model, NormalMatrix, UUIDColor, false);
#else
        FObjectMatrixConstants objMatrixConstatnts = {
            .World = Model,
            .View = ActiveViewport->GetViewMatrix(),
            .Projection = ActiveViewport->GetProjectionMatrix()
        };

        ConstantBufferUpdater.UpdateObjectMatrixConstants(ObjectMatrixConstantBuffer, objMatrixConstatnts);
#endif
        if (!GizmoComp->GetStaticMesh()) continue;

        OBJ::FStaticMeshRenderData* renderData = GizmoComp->GetStaticMesh()->GetRenderData();
        if (renderData == nullptr) continue;

        RenderPrimitive(renderData, GizmoComp->GetStaticMesh()->GetMaterials(), GizmoComp->GetOverrideMaterials());
    }

    Graphics->DeviceContext->RSSetState(Graphics->GetCurrentRasterizer());

#pragma region GizmoDepth
    ID3D11DepthStencilState* originalDepthState = Graphics->DepthStencilState;
    Graphics->DeviceContext->OMSetDepthStencilState(originalDepthState, 0);
#pragma endregion 
}

void FRenderer::RenderBillboards()
{
#if !USE_GBUFFER
    PrepareTextureShader();
    PrepareSubUVConstant();
#else
    PrepareUberUnlitShader();
#endif
    for (auto BillboardComp : BillboardObjs)
    {
        ConstantBufferUpdater.UpdateSubUVConstant(SubUVConstantBuffer, BillboardComp->finalIndexU, BillboardComp->finalIndexV);

        FMatrix Model = BillboardComp->CreateBillboardMatrix();

#if !USE_GBUFFER
        // 최종 MVP 행렬
        FMatrix MVP = Model * ActiveViewport->GetViewMatrix() * ActiveViewport->GetProjectionMatrix();
        FMatrix NormalMatrix = FMatrix::Transpose(FMatrix::Inverse(Model));
        FVector4 UUIDColor = BillboardComp->EncodeUUID() / 255.0f;
        if (BillboardComp == World->GetPickingGizmo())
            ConstantBufferUpdater.UpdateConstant(ConstantBuffer, MVP, Model, NormalMatrix, UUIDColor, true);
        else
            ConstantBufferUpdater.UpdateConstant(ConstantBuffer, MVP, Model, NormalMatrix, UUIDColor, false);
#else
        FObjectMatrixConstants MatrixConstant =
        {
            .World = Model,
            .View = ActiveViewport->GetViewMatrix(),
            .Projection = ActiveViewport->GetProjectionMatrix()
        };
        ConstantBufferUpdater.UpdateObjectMatrixConstants(ObjectMatrixConstantBuffer, MatrixConstant);
#endif

        FTextureMaterialConstants TextureMatConstant =
        {
            .TintColor = BillboardComp->GetTintColor(),
            .IsLightIcon = (float)BillboardComp->IsLightIcon()
        };

        ConstantBufferUpdater.UpdateTextureMaterialConstants(TextureMaterialConstants, TextureMatConstant);

        if (UParticleSubUVComp* SubUVParticle = Cast<UParticleSubUVComp>(BillboardComp))
        {
            const FQuadRenderData& QuadRenderData = UEditorEngine::resourceMgr.GetQuadRenderData();
            RenderTexturePrimitive(
                SubUVParticle->vertexSubUVBuffer, SubUVParticle->numTextVertices,
                QuadRenderData.IndexTextureBuffer, QuadRenderData.numIndices, SubUVParticle->Texture->TextureSRV, SubUVParticle->Texture->SamplerState
            );
        }
        else if (UText* Text = Cast<UText>(BillboardComp))
        {
            UEditorEngine::renderer.RenderTextPrimitive(
                Text->vertexTextBuffer, Text->numTextVertices,
                Text->Texture->TextureSRV, Text->Texture->SamplerState
            );
        }
        else
        {
            const FQuadRenderData& QuadRenderData = UEditorEngine::resourceMgr.GetQuadRenderData();

            RenderTexturePrimitive(
                QuadRenderData.VertexTextureBuffer, QuadRenderData.numVertices,
                QuadRenderData.IndexTextureBuffer, QuadRenderData.numIndices, BillboardComp->Texture->TextureSRV, BillboardComp->Texture->SamplerState
            );
        }
    }
#if USE_GBUFFER
    PrepareShader();
#else
    PrepareUberShader();
#endif
}

void FRenderer::RenderFullScreenQuad()
{
    const FQuadRenderData& Quad = UEditorEngine::resourceMgr.GetQuadRenderData();
    UINT stride = FullScreenStride;
    UINT offset = 0;

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &Quad.VertexTextureBuffer, &stride, &offset);
    Graphics->DeviceContext->IASetIndexBuffer(Quad.IndexTextureBuffer, DXGI_FORMAT_R32_UINT, 0);
    Graphics->DeviceContext->DrawIndexed(Quad.numIndices, 0, 0);
}

bool FRenderer::UberIsOutDate()
{
    auto CurrentUberWriteTime = std::filesystem::last_write_time("Shaders/Uber.hlsl");
    return CurrentUberWriteTime != LastUberWriteTime;
}

bool FRenderer::LineIsOutDate()
{
    auto CurrentLineWriteTime = std::filesystem::last_write_time("Shaders/ShaderLine.hlsl");
    return CurrentLineWriteTime != LastLineWriteTime;
}

void FRenderer::HotReloadUberShader()
{
    if (UberIsOutDate()) 
    {
        Release();
        CreateShader();
        CreateConstantBuffer();
    }

    if (LineIsOutDate()) 
    {
        Release();
        CreateShader();
        CreateConstantBuffer();
    }
}

void FRenderer::CheckGenerateLightTree()
{
    if (bGenerateLightTree) {
        bGenerateLightTree = false;
        GeneratePointLightCut();
    }
}

void FRenderer::GeneratePointLightCut()
{
    TArray<ULightComponentBase*> PointLightObjs;

    for (int i = 0; i < LightObjs.Num(); i++) {
        if (LightObjs[i]->IsA<UPointLightComponent>() && !LightObjs[i]->IsA<USpotLightComponent>()) {
            PointLightObjs.Add(LightObjs[i]);
        }
    }

    PointLightTree.Build(PointLightObjs);
    for (int i = 0; i < StaticMeshObjs.Num(); i++) {
        PointLightTree.ComputeLightCut(StaticMeshObjs[i]);
    }
    PointLightTree.SetActive(true);
}

void FRenderer::DeletePointLightCut()
{
    PointLightTree.ClearLightCut();
    PointLightTree.ClearLightTree();
    PointLightTree.SetActive(false);
}

void FRenderer::SubscribeToFogUpdates(UHeightFogComponent* HeightFog)
{

    FogData.FogDensity = HeightFog->GetFogDensity();
    FogData.FogHeightFalloff = HeightFog->GetFogHeightFalloff();
    FogData.FogStartDistance = HeightFog->GetStartDistance();
    FogData.FogCutoffDistance = HeightFog->GetFogCutoffDistance();
    FogData.FogMaxOpacity = HeightFog->GetFogMaxOpacity();
    FogData.FogInscatteringColor = HeightFog->GetColor();
    FogData.CameraPosition = ActiveViewport->GetCameraLocation();
    FogData.FogHeight = HeightFog->GetWorldLocation().z;
    FogData.InverseView = FMatrix::Inverse(ActiveViewport->GetViewMatrix());
    FogData.InverseProjection = FMatrix::Inverse(ActiveViewport->GetProjectionMatrix());
    FogData.DisableFog = HeightFog->GetDisableFog();
}

void FRenderer::ResetFogUpdates()
{
    FogData.FogDensity = 0.0f;
    FogData.FogHeightFalloff = 0.0f;
    FogData.FogStartDistance = 0.0f;
    FogData.FogCutoffDistance = 0.0f;
    FogData.FogMaxOpacity = 0.0f;
    FogData.FogInscatteringColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
    FogData.CameraPosition = FVector(0.0f, 0.0f, 0.0f);
    FogData.FogHeight = 0.0f;
}

void FRenderer::RenderLight()
{
    for (auto Light : LightObjs)
    {
        // 일단 여기에 Light들 중에 SelectActor의 Component인 경우만
        if (Light->GetOwner() != World->GetSelectedActor()) continue;
        if (Light->IsA<UPointLightComponent>() && !Light->IsA<USpotLightComponent>())
        {
            // PointLight를 상속 받은 경우에 대해
            UPointLightComponent* PointLight = Cast<UPointLightComponent>(Light);
            if (GEngine->GetWorld()->WorldType == EWorldType::PIE) continue;
            FMatrix Model = JungleMath::CreateModelMatrix(PointLight->GetWorldLocation(), PointLight->GetWorldRotation(), { 1, 1, 1 });
            UPrimitiveBatch::GetInstance().AddCircle(PointLight->GetWorldLocation(), PointLight->GetRadius(), 90, PointLight->GetColor(), Model);
        }
        if (Light->IsA<USpotLightComponent>())
        {
            USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Light);
            if (SpotLight)
            {
                if (GEngine->GetWorld()->WorldType == EWorldType::PIE) continue;
                FMatrix Model = JungleMath::CreateModelMatrix(SpotLight->GetWorldLocation(), SpotLight->GetWorldRotation(), { 1, 1, 1 });
                UPrimitiveBatch::GetInstance().AddCone(SpotLight->GetWorldLocation(), SpotLight->GetRadius(), SpotLight->GetOuterConeAngle(), SpotLight->GetColor(), Model);
                UPrimitiveBatch::GetInstance().AddCone(SpotLight->GetWorldLocation(), SpotLight->GetRadius(), SpotLight->GetInnerConeAngle(), SpotLight->GetColor() / 2, Model);
                UPrimitiveBatch::GetInstance().RenderOBB(SpotLight->GetBoundingBox(), Light->GetWorldLocation(), Model);
            }
        }
        if (Light->IsA<UDirectionalLightComponent>())
        {
            UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(Light);
            if (DirectionalLight) 
            {
                if (GEngine->GetWorld()->WorldType == EWorldType::PIE) continue;
                FMatrix Model = JungleMath::CreateModelMatrix(DirectionalLight->GetWorldLocation(), DirectionalLight->GetWorldRotation(), { 1, 1, 1 });
                UPrimitiveBatch::GetInstance().AddLine(DirectionalLight->GetWorldLocation(), DirectionalLight->GetWorldLocation() + DirectionalLight->GetDirection() * 3.0f, DirectionalLight->GetColor());
            }
        }
    }
}

void FRenderer::RenderBatch(
    const FGridParameters& gridParam, ID3D11Buffer* pVertexBuffer, int boundingBoxCount, int coneCount, int coneSegmentCount, int obbCount, int circleCount, int circleSegmentCount, int lineCount
) const
{
    UINT stride = sizeof(FSimpleVertex);
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    UINT vertexCountPerInstance = 2;
    UINT instanceCount = gridParam.numGridLines + 3 + (boundingBoxCount * 12) + (coneCount * (2 * coneSegmentCount + 10)) + (12 * obbCount) + (3 * circleCount * (circleSegmentCount)) + 10 * lineCount;
    Graphics->DeviceContext->DrawInstanced(vertexCountPerInstance, instanceCount, 0, 0);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
#pragma endregion Render

#pragma region MultiPass
void FRenderer::RenderUberPass()
{
    // StaticMesh
    if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Primitives)) 
    {
        RenderStaticMeshes();
    }

    // Billboard
    if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_BillboardText)) 
    {
        RenderBillboards();
    }
}

void FRenderer::RenderGBuffer()
{
    //Graphics->DeviceContext->OMSetRenderTargets(4, Graphics->GBufferRTVs, Graphics->DepthStencilView);

    // StaticMesh
    if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Primitives))
        RenderStaticMeshes();

    // Billboard
    if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_BillboardText))
        RenderBillboards();
}

void FRenderer::RenderLightPass()
{  
    PrepareLightShader();

    //ID3D11RenderTargetView* rtv = Graphics->FrameBufferRTV;
    //Graphics->DeviceContext->OMSetRenderTargets(1, &rtv, Graphics->DepthStencilView);
    int mode = ActiveViewport->GetViewMode();

    // Directional Light
    FLightConstant GlobalLight = {
        .Ambient = FVector4(0.1f, 0.1f, 0.1f, 1.f),
        .Diffuse = FVector4(1.0f, 1.0f, 1.0f, 1.0f),
        .Specular = FVector4(1.0f, 1.0f, 1.0f, 1.0f),
        .Emissive = FVector(1.0f, 1.0f, 1.0f),
        .Padding1 = 0,
        .Direction = FVector(1.0f, -1.0f, -1.0f),
        .Padding2 = 0,
        .CameraPosition = ActiveViewport->GetCameraLocation(),
        .Padding = (float)ActiveViewport->GetViewMode(),
    };
    ConstantBufferUpdater.UpdateGlobalLightConstant(LPLightConstantBuffer, GlobalLight);

    RenderLight();

    // Point Light
    std::unique_ptr<FFireballArrayInfo> fireballArrayInfo = std::make_unique<FFireballArrayInfo>();

    if (FireballObjs.Num() > 0)
    {
        fireballArrayInfo->FireballCount = 0;

        for (int i = 0; i < FireballObjs.Num(); i++)
        {
            if (FireballObjs[i] != nullptr)
            {
                const FFireballInfo& fireballInfo = FireballObjs[i]->GetFireballInfo();
                fireballArrayInfo->FireballConstants[i].Intensity = fireballInfo.Intensity;
                fireballArrayInfo->FireballConstants[i].Radius = fireballInfo.Radius;
                fireballArrayInfo->FireballConstants[i].Color = fireballInfo.Color;
                fireballArrayInfo->FireballConstants[i].RadiusFallOff = fireballInfo.RadiusFallOff;
                fireballArrayInfo->FireballConstants[i].Position = FireballObjs[i]->GetWorldLocation();
                fireballArrayInfo->FireballConstants[i].LightType = fireballInfo.Type;
                if (USpotLightComponent* spotLight = Cast<USpotLightComponent>(FireballObjs[i]))
                {
                    fireballArrayInfo->FireballConstants[i].InnerAngle = spotLight->GetInnerConeAngle();
                    fireballArrayInfo->FireballConstants[i].OuterAngle = spotLight->GetOuterConeAngle();
                    fireballArrayInfo->FireballConstants[i].Direction = spotLight->GetForwardVector();
                }
                fireballArrayInfo->FireballCount++;
            }
        }
    }
    ConstantBufferUpdater.UpdateFireballConstant(FireballConstantBuffer, *fireballArrayInfo);
    ConstantBufferUpdater.UpdateScreenConstant(ScreenConstantBuffer, ActiveViewport);
    // Spot Light

    // 1. RenderTarget 설정 (Color, Position)
    Graphics->DeviceContext->OMSetRenderTargets(2, Graphics->LightPassRTVs, nullptr);

    // 2. GBuffer에서 SRV 연결 (Normal, Albedo, Position)
    ID3D11ShaderResourceView* SRVs[] = {
        Graphics->GBufferSRV_Normal,
        Graphics->GBufferSRV_Albedo,
        Graphics->GBufferSRV_Ambient,
        Graphics->GBufferSRV_Position
    };
    Graphics->DeviceContext->PSSetShaderResources(0, 4, SRVs);

    // 3. Set Sampler
    Graphics->Device->CreateSamplerState(&SamplerDesc, &SamplerState);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &SamplerState);

    // 4. Fullscreen Quad 렌더링
    RenderFullScreenQuad();

    // 5. SRV 언바인딩 (다음 Pass에서 충돌 방지)
    ID3D11ShaderResourceView* nullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 4, nullSRVs);
}

void FRenderer::RenderPostProcessPass()
{
    // 1. Prepare Shader
    PreparePostProcessShader();

    // 2. Update Constant
    ConstantBufferUpdater.UpdateFogConstant(FogConstantBuffer, FogData);
    ConstantBufferUpdater.UpdateScreenConstant(ScreenConstantBuffer, ActiveViewport);

    // 3. Set RTV
    ID3D11RenderTargetView* FrameBufferRTV = Graphics->FrameBufferRTV;
    Graphics->DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);

    // 4. Set SRV
    ID3D11ShaderResourceView* SRVs[] = {
#if USE_GBUFFER
        Graphics->LightPassSRV_Color,
        Graphics->LightPassSRV_Position
#else
        Graphics->UberSRV_Color,
        Graphics->UberSRV_Position
#endif
    };
    Graphics->DeviceContext->PSSetShaderResources(0, 2, SRVs);

    // 5. Set Sampler
    Graphics->Device->CreateSamplerState(&SamplerDesc, &SamplerState);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &SamplerState);

    // 6. FullScreen Quad
    RenderFullScreenQuad();

    // 7. Unbinding SRV
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 2, nullSRVs);
}

void FRenderer::RenderOverlayPass()
{
    // Enable Depth Test
    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->FrameBufferRTV, Graphics->DepthStencilView);

    // Line
    FVector CamPos = ActiveViewport->GetCameraLocation();
    FVector4 CamPos4 = FVector4(CamPos.x, CamPos.y, CamPos.z, 1.f);
    float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    Graphics->DeviceContext->OMSetBlendState(Graphics->LineBlendState, blendFactor, 0xffffffff);
    // LightPass 가 비활성화되었기에
    // 기존 Light의 Wireframe을 호출해주던 함수를 여기에 위치시킴
#if USE_GBUFFER
#else
    RenderLight();
#endif
    UPrimitiveBatch::GetInstance().RenderBatch(ConstantBuffer, ActiveViewport->GetViewMatrix(), ActiveViewport->GetProjectionMatrix(), CamPos4);
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    // Gizmo
    RenderGizmos();
}
#pragma endregion MultiPass

void FRenderer::ChangeViewMode(EViewModeIndex evi) const
{
    switch (evi)
    {
    case EViewModeIndex::VMI_Lit:
        ConstantBufferUpdater.UpdateLitUnlitConstant(FlagBuffer, 1);
        break;
    case EViewModeIndex::VMI_Wireframe:
    case EViewModeIndex::VMI_Unlit:
        ConstantBufferUpdater.UpdateLitUnlitConstant(FlagBuffer, 0);
        break;
    }
}

void FRenderer::UpdateMaterial(const FObjMaterialInfo& MaterialInfo) const
{
#if USE_GBUFFER
    ConstantBufferUpdater.UpdateMaterialConstant(GMaterialConstantBuffer, MaterialInfo);
#else
    ConstantBufferUpdater.UpdateMaterialConstant(MaterialConstantBuffer, MaterialInfo);
#endif

    bool isHasTexture = MaterialInfo.bHasTexture || MaterialInfo.bHasNormalMap;
    if (isHasTexture)
    {
        if (MaterialInfo.bHasTexture == true)
        {
            std::shared_ptr<FTexture> texture = UEditorEngine::resourceMgr.GetTexture(MaterialInfo.DiffuseTexturePath);
            Graphics->DeviceContext->PSSetShaderResources(0, 1, &texture->TextureSRV);
            Graphics->DeviceContext->PSSetSamplers(0, 1, &texture->SamplerState);
        }

        if (MaterialInfo.bHasNormalMap)
        {
            std::shared_ptr<FTexture> normalMap = UEditorEngine::resourceMgr.GetTexture(MaterialInfo.BumpTexturePath);
            Graphics->DeviceContext->PSSetShaderResources(1, 1, &normalMap->TextureSRV);
            //Graphics->DeviceContext->PSSetSamplers(0, 1, &normalMap->SamplerState);
        }
    }
    else
    {
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        ID3D11SamplerState* nullSampler[1] = { nullptr };

        Graphics->DeviceContext->PSSetShaderResources(0, 1, nullSRV);
        Graphics->DeviceContext->PSSetSamplers(0, 1, nullSampler);
    }
}

ID3D11ShaderResourceView* FRenderer::CreateBoundingBoxSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numBoundingBoxes;


    Graphics->Device->CreateShaderResourceView(pBoundingBoxBuffer, &srvDesc, &pBBSRV);
    return pBBSRV;
}

ID3D11ShaderResourceView* FRenderer::CreateOBBSRV(ID3D11Buffer* pBoundingBoxBuffer, UINT numBoundingBoxes)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numBoundingBoxes;
    Graphics->Device->CreateShaderResourceView(pBoundingBoxBuffer, &srvDesc, &pOBBSRV);
    return pOBBSRV;
}

ID3D11ShaderResourceView* FRenderer::CreateConeSRV(ID3D11Buffer* pConeBuffer, UINT numCones)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numCones;


    Graphics->Device->CreateShaderResourceView(pConeBuffer, &srvDesc, &pConeSRV);
    return pConeSRV;
}

ID3D11ShaderResourceView* FRenderer::CreateCircleSRV(ID3D11Buffer* pCircleBuffer, UINT numCircles)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numCircles;


    Graphics->Device->CreateShaderResourceView(pCircleBuffer, &srvDesc, &pCircleSRV);
    return pCircleSRV;
}

ID3D11ShaderResourceView* FRenderer::CreateLineSRV(ID3D11Buffer* pLineBuffer, UINT numLines)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements = numLines;

    Graphics->Device->CreateShaderResourceView(pLineBuffer, &srvDesc, &pLineSRV);
    return pLineSRV;
}

void FRenderer::UpdateBoundingBoxBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FBoundingBox>& BoundingBoxes, int numBoundingBoxes) const
{
    if (!pBoundingBoxBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pBoundingBoxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FBoundingBox*>(mappedResource.pData);
    for (int i = 0; i < BoundingBoxes.Num(); ++i)
    {
        pData[i] = BoundingBoxes[i];
    }
    Graphics->DeviceContext->Unmap(pBoundingBoxBuffer, 0);
}

void FRenderer::UpdateOBBBuffer(ID3D11Buffer* pBoundingBoxBuffer, const TArray<FOBB>& BoundingBoxes, int numBoundingBoxes) const
{
    if (!pBoundingBoxBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pBoundingBoxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FOBB*>(mappedResource.pData);
    for (int i = 0; i < BoundingBoxes.Num(); ++i)
    {
        pData[i] = BoundingBoxes[i];
    }
    Graphics->DeviceContext->Unmap(pBoundingBoxBuffer, 0);
}

void FRenderer::UpdateConesBuffer(ID3D11Buffer* pConeBuffer, const TArray<FCone>& Cones, int numCones) const
{
    if (!pConeBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pConeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FCone*>(mappedResource.pData);
    for (int i = 0; i < Cones.Num(); ++i)
    {
        pData[i] = Cones[i];
    }
    Graphics->DeviceContext->Unmap(pConeBuffer, 0);
}

void FRenderer::UpdateCirclesBuffer(ID3D11Buffer* pCircleBuffer, const TArray<FCircle>& Circles, int numCircles) const
{
    if (!pCircleBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pCircleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FCircle*>(mappedResource.pData);
    for (int i = 0; i < Circles.Num(); ++i)
    {
        pData[i] = Circles[i];
    }
    Graphics->DeviceContext->Unmap(pCircleBuffer, 0);
}

void FRenderer::UpdateLinesBuffer(ID3D11Buffer* pLineBuffer, const TArray<FLine>& Lines, int numLines) const
{
    if (!pLineBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Graphics->DeviceContext->Map(pLineBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = reinterpret_cast<FLine*>(mappedResource.pData);
    for (int i = 0; i < Lines.Num(); ++i)
    {
        pData[i] = Lines[i];
    }
    Graphics->DeviceContext->Unmap(pLineBuffer, 0);
}

void FRenderer::UpdateGridConstantBuffer(const FGridParameters& gridParams) const
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Graphics->DeviceContext->Map(GridConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &gridParams, sizeof(FGridParameters));
        Graphics->DeviceContext->Unmap(GridConstantBuffer, 0);
    }
    else
    {
        UE_LOG(LogLevel::Warning, "gridParams");
    }
}

void FRenderer::UpdateLinePrimitveCountBuffer(int numBoundingBoxes, int numCones, int numOBBs, int numCircles) const
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Graphics->DeviceContext->Map(LinePrimitiveBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    auto pData = static_cast<FPrimitiveCounts*>(mappedResource.pData);
    pData->BoundingBoxCount = numBoundingBoxes;
    pData->ConeCount = numCones;
    pData->OBBCount = numOBBs;
    pData->CircleCount = numCircles;
    Graphics->DeviceContext->Unmap(LinePrimitiveBuffer, 0);
}

// Fog 데이터 업데이트 이벤트를 구독하는 함수 (한번만 설정하면 됨)
