#include "ConstantBufferUpdater.h"
#include <Engine/Texture.h>
#include "EditorEngine.h"
#include "LevelEditor/SLevelEditor.h"
#include "ViewportClient.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Engine/Unrealclient.h"
#include "d3d11.h"
void FConstantBufferUpdater::Initialize(ID3D11DeviceContext* InDeviceContext)
{
    DeviceContext = InDeviceContext;
}

void FConstantBufferUpdater::UpdateConstant(ID3D11Buffer* ConstantBuffer, const FMatrix& MVP, const FMatrix& Model, const FMatrix& NormalMatrix, FVector4 UUIDColor, bool IsSelected) const
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        {
            FConstants* constants = static_cast<FConstants*>(ConstantBufferMSR.pData);
            constants->MVP = MVP;
            constants->ModelMatrix = Model;
            constants->ModelMatrixInverseTranspose = NormalMatrix;
            constants->UUIDColor = UUIDColor;
            constants->IsSelected = IsSelected;
            constants->ScreenSize = FVector2D(GEngine->graphicDevice.screenWidth, GEngine->graphicDevice.screenHeight);
            FViewport* viewport = GEngine->GetLevelEditor()->GetActiveViewportClient()->GetViewport();
            constants->ViewportSize = FVector2D(viewport->GetViewport().Width, viewport->GetViewport().Height); //Assuming a fixed viewport size for simplicity
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateConstantWithCamPos(ID3D11Buffer* ConstantBuffer, 
    const FMatrix& MVP, const FMatrix& Model, const FMatrix& NormalMatrix, FVector4 UUIDColor, bool IsSelected, FVector CamPos) const
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        {
            FConstants* constants = static_cast<FConstants*>(ConstantBufferMSR.pData);
            constants->MVP = MVP;
            constants->ModelMatrix = Model;
            constants->ModelMatrixInverseTranspose = NormalMatrix;
            constants->UUIDColor = UUIDColor;
            constants->IsSelected = IsSelected;
            constants->CameraPosition = CamPos;
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateMaterialConstant(ID3D11Buffer* MaterialConstantBuffer, const FObjMaterialInfo& MaterialInfo) const
{
    if (MaterialConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;

        DeviceContext->Map(MaterialConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        {
            FMaterialConstants* constants = static_cast<FMaterialConstants*>(ConstantBufferMSR.pData);
            constants->DiffuseColor = MaterialInfo.Diffuse;
            constants->TransparencyScalar = MaterialInfo.TransparencyScalar;
            constants->AmbientColor = MaterialInfo.Ambient;
            constants->DensityScalar = MaterialInfo.DensityScalar;
            constants->SpecularColor = MaterialInfo.Specular;
            constants->SpecularScalar = MaterialInfo.SpecularScalar;
            constants->EmmisiveColor = MaterialInfo.Emissive;
        }
        DeviceContext->Unmap(MaterialConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateLightConstant(ID3D11Buffer* LightingBuffer) const
{
    if (!LightingBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    DeviceContext->Map(LightingBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    {
        FLighting* constants = static_cast<FLighting*>(mappedResource.pData);
        constants->lightDirX = 1.0f;
        constants->lightDirY = 1.0f;
        constants->lightDirZ = 1.0f;
        constants->lightColorX = 1.0f;
        constants->lightColorY = 1.0f;
        constants->lightColorZ = 1.0f;
        constants->AmbientFactor = 0.06f;
    }
    DeviceContext->Unmap(LightingBuffer, 0);
}

void FConstantBufferUpdater::UpdateGlobalLightConstant(ID3D11Buffer* GlobalLightBuffer, FLightConstant GlobalLight) const
{
    if (!GlobalLightBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    DeviceContext->Map(GlobalLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    {
        FLightConstant* constants = static_cast<FLightConstant*>(mappedResource.pData);
        constants->Ambient = GlobalLight.Ambient;
        constants->Diffuse = GlobalLight.Diffuse;
        constants->Specular = GlobalLight.Specular;
        constants->Emissive = GlobalLight.Emissive;
        constants->Direction = GlobalLight.Direction;
        constants->CameraPosition = GlobalLight.CameraPosition;
        constants->Padding = GlobalLight.Padding;
    }
    DeviceContext->Unmap(GlobalLightBuffer, 0);
}


void FConstantBufferUpdater::UpdateLitUnlitConstant(ID3D11Buffer* FlagBuffer, int isLit) const
{
    if (FlagBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(FlagBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        auto constants = static_cast<FLitUnlitConstants*>(constantbufferMSR.pData);
        {
            constants->isLit = isLit;
        }
        DeviceContext->Unmap(FlagBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateSubMeshConstant(ID3D11Buffer* SubMeshConstantBuffer, bool isSelected) const
{
    if (SubMeshConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(SubMeshConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        FSubMeshConstants* constants = (FSubMeshConstants*)constantbufferMSR.pData;
        {
            constants->isSelectedSubMesh = isSelected;
        }
        DeviceContext->Unmap(SubMeshConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateTextureConstant(ID3D11Buffer* TextureConstantBufer, float UOffset, float VOffset)
{
    if (TextureConstantBufer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(TextureConstantBufer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        FTextureConstants* constants = (FTextureConstants*)constantbufferMSR.pData;
        {
            constants->UOffset = UOffset;
            constants->VOffset = VOffset;
        }
        DeviceContext->Unmap(TextureConstantBufer, 0);
    }
}

void FConstantBufferUpdater::UpdateSubUVConstant(ID3D11Buffer* SubUVConstantBuffer, float _indexU, float _indexV) const
{
    if (SubUVConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(SubUVConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        auto constants = static_cast<FSubUVConstant*>(constantbufferMSR.pData);
        {
            constants->indexU = _indexU;
            constants->indexV = _indexV;
        }
        DeviceContext->Unmap(SubUVConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateFireballConstant(ID3D11Buffer* FireballConstantBuffer, const FFireballArrayInfo FireballInfo) const
{
    if (FireballConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;

        DeviceContext->Map(FireballConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, &FireballInfo, sizeof(FFireballArrayInfo));
        DeviceContext->Unmap(FireballConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateFogConstant(ID3D11Buffer* FogConstantBuffer, const FFogConstants& FogConstants) const
{
    if (FogConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR;
        DeviceContext->Map(FogConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR); // update constant buffer every frame
        memcpy(ConstantBufferMSR.pData, &FogConstants, sizeof(FFogConstants));
        DeviceContext->Unmap(FogConstantBuffer, 0);
    }
}
void FConstantBufferUpdater::UpdateScreenConstant(ID3D11Buffer* ScreenConstantBuffer, std::shared_ptr<FEditorViewportClient>viewport) const
{
    if (ScreenConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(ScreenConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        auto constants = static_cast<FScreenConstants*>(constantbufferMSR.pData);
        D3D11_VIEWPORT ActiveViewport = viewport.get()->GetD3DViewport();
        constants->ViewportRatio = FVector2D(ActiveViewport.Width/ GEngine->graphicDevice.screenWidth, ActiveViewport.Height/ GEngine->graphicDevice.screenHeight);
        constants->ViewportPosition = FVector2D(ActiveViewport.TopLeftX / GEngine->graphicDevice.screenWidth, ActiveViewport.TopLeftY/GEngine->graphicDevice.screenHeight);
        DeviceContext->Unmap(ScreenConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateObjectMatrixConstants(ID3D11Buffer* ObjectMatrixConstantBuffer, const FObjectMatrixConstants& ObjectMatrix) const
{
    if (ObjectMatrixConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(ObjectMatrixConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        auto constants = static_cast<FObjectMatrixConstants*>(constantbufferMSR.pData);
        constants->World = ObjectMatrix.World;
        constants->View = ObjectMatrix.View;
        constants->Projection = ObjectMatrix.Projection;
        constants->NormalMatrix = ObjectMatrix.NormalMatrix;
        DeviceContext->Unmap(ObjectMatrixConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateCameraPositionConstants(ID3D11Buffer* CameraConstantBuffer, const FCameraConstant& CameraPosition) const
{
    if (CameraConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(CameraConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        auto constants = static_cast<FCameraConstant*>(constantbufferMSR.pData);
        constants->CameraWorldPos = CameraPosition.CameraWorldPos;
        DeviceContext->Unmap(CameraConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateLightConstants(ID3D11Buffer* LightConstantBuffer, const FLightConstants& Light) const
{
    if (LightConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(LightConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        auto constants = static_cast<FLightConstants*>(constantbufferMSR.pData);
        constants->Ambient = Light.Ambient;
        constants->Directional = Light.Directional;
        memcpy(constants->PointLights, Light.PointLights, sizeof(Light.PointLights));
        memcpy(constants->SpotLights, Light.SpotLights, sizeof(Light.SpotLights));
        DeviceContext->Unmap(LightConstantBuffer, 0);
    }
}

void FConstantBufferUpdater::UpdateMaterialConstants(ID3D11Buffer* MaterialConstantBuffer, const FMaterialConstants& Material) const
{
    if (MaterialConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
        DeviceContext->Map(MaterialConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
        auto constants = static_cast<FMaterialConstants*>(constantbufferMSR.pData);
        constants->DiffuseColor = Material.DiffuseColor;
        constants->TransparencyScalar = Material.TransparencyScalar;
        constants->AmbientColor = Material.AmbientColor;
        constants->DensityScalar = Material.DensityScalar;
        constants->SpecularColor = Material.SpecularColor;
        constants->SpecularScalar = Material.SpecularScalar;
        constants->EmmisiveColor = Material.EmmisiveColor;
        DeviceContext->Unmap(MaterialConstantBuffer, 0);
    }
}
