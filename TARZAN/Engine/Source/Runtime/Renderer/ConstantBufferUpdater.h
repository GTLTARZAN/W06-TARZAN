#pragma once

#define _TCHAR_DEFINED
#include <d3d11.h>
#include "Define.h"

class FEditorViewportClient;

class FConstantBufferUpdater
{
public:
    void Initialize(ID3D11DeviceContext* InDeviceContext);

    void UpdateConstant(ID3D11Buffer* ConstantBuffer, const FMatrix& MVP, const FMatrix& Model, const FMatrix& NormalMatrix, FVector4 UUIDColor, bool IsSelected) const;
    void UpdateConstantWithCamPos(ID3D11Buffer* ConstantBuffer, const FMatrix& MVP, const FMatrix& Model, const FMatrix& NormalMatrix, FVector4 UUIDColor, bool IsSelected, FVector CamPos) const;
    void UpdateMaterialConstant(ID3D11Buffer* MaterialConstantBuffer, const FObjMaterialInfo& MaterialInfo) const;
    void UpdateLightConstant(ID3D11Buffer* LightingBuffer) const;
    void UpdateGlobalLightConstant(ID3D11Buffer* GlobalLightBuffer, FLightConstant GlobalLight) const;
    void UpdateLitUnlitConstant(ID3D11Buffer* FlagBuffer, int isLit) const;
    void UpdateSubMeshConstant(ID3D11Buffer* SubMeshConstantBuffer, bool isSelected) const;
    void UpdateTextureConstant(ID3D11Buffer* TextureConstantBufer, float UOffset, float VOffset);
    void UpdateSubUVConstant(ID3D11Buffer* SubUVConstantBuffer, float _indexU, float _indexV) const;
    void UpdateFireballConstant(ID3D11Buffer* FireballConstantBuffer, const FFireballArrayInfo FireballInfo) const;
    void UpdateFogConstant(ID3D11Buffer* FogConstantBuffer, const FFogConstants& FogConstants) const;
    void UpdateScreenConstant(ID3D11Buffer* ScreenConstantBuffer, std::shared_ptr<FEditorViewportClient>viewport) const;

    void UpdateObjectMatrixConstants(ID3D11Buffer* ObjectMatrixConstantBuffer, const FObjectMatrixConstants& ObjectMatrix) const;
    void UpdateCameraPositionConstants(ID3D11Buffer* CameraConstantBuffer, const FCameraConstant& CameraPosition) const;
    void UpdateLightConstants(ID3D11Buffer* LightConstantBuffer, const FLightConstants& Light) const;
    void UpdateMaterialConstants(ID3D11Buffer* MaterialConstantBuffer, const FMaterialConstants& Material) const; 
    void UpdateTextureMaterialConstants(ID3D11Buffer* TextureMaterialBuffer, const FTextureMaterialConstants& TexMat) const;
private:
    ID3D11DeviceContext* DeviceContext = nullptr;
};

