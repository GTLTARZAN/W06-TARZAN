#pragma once
#include <memory>
#include "Texture.h"
#include "Container/Map.h"

struct FQuadRenderData
{
    ID3D11Buffer* VertexTextureBuffer = nullptr;
    ID3D11Buffer* IndexTextureBuffer= nullptr;

    UINT numVertices = 0;
    UINT numIndices = 0;
};

class FRenderer;
class FGraphicsDevice;
class FResourceMgr
{

public:
    void Initialize(FRenderer* renderer, FGraphicsDevice* device);
    void Release(FRenderer* renderer);
    HRESULT LoadTextureFromFile(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);
    HRESULT LoadTextureFromDDS(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);

    const FQuadRenderData& GetQuadRenderData() const
    {
        return QuadRenderData;
    }
    
    std::shared_ptr<FTexture> GetTexture(const FWString& name) const;
    std::shared_ptr<FTexture> CreateTexture(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* Res, const FWString& name);
    std::shared_ptr<FTexture> CreateTexture(ID3D11Device* device, ID3D11DeviceContext* context, D3D11_TEXTURE2D_DESC textureDesc, const FWString& name);
private:
    std::shared_ptr<FTexture> CreateTextureView(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* texture2D, D3D11_TEXTURE2D_DESC textureDesc, const
                                                FWString& name);

    FQuadRenderData QuadRenderData;

    TMap<FWString, std::shared_ptr<FTexture>> textureMap;
};