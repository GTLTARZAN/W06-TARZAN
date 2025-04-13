#include "ShaderManager.h"
#include <d3dcompiler.h>

void FShaderManager::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
    Device = InDevice;
    DeviceContext = InDeviceContext;
}

void FShaderManager::Release()
{
    Device = nullptr;
}

bool FShaderManager::CreateVertexShader(
    const FWString& vsPath,
    const FString& vsEntry,
    ID3D11VertexShader*& outVS,
    const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
    UINT numElements,
    ID3D11InputLayout** outInputLayout,
    UINT* outStride,
    UINT vertexSize,
    LightingModel lightingModel)
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    // Vertex Shader Macro
    D3D_SHADER_MACRO defines_Normal[] = 
    {
        { "NORMAL_VERTEX", "1" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_Gouraud[] =
    {
        { "NORMAL_VERTEX", "0" },
        { "LIGHTING_MODEL_GOURAUD", "1" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO* defines_Vertex = nullptr;

    switch (lightingModel)
    {
    case LightingModel::Gouraud:
        defines_Vertex = defines_Gouraud;
    case LightingModel::Unlit:
    case LightingModel::Lambert:
    case LightingModel::BlinnPhong:
        break;
    default:
        defines_Vertex = nullptr;
        break;
    }

    ID3DBlob* vsBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(vsPath.c_str(), defines_Vertex, nullptr, *vsEntry, "vs_5_0", shaderFlags, 0, &vsBlob, nullptr);
    if (FAILED(hr)) return false;

     hr = Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &outVS);
    if (FAILED(hr)) { vsBlob->Release(); return false; }

    if (outInputLayout)
    {
        hr = Device->CreateInputLayout(inputLayoutDesc, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), outInputLayout);
        if (FAILED(hr)) { vsBlob->Release(); return false; }
    }
    if (outStride)
        *outStride = vertexSize;

    vsBlob->Release();
    return true;
}

bool FShaderManager::CreatePixelShader(
    const FWString& psPath,
    const FString& psEntry,
    ID3D11PixelShader*& outPS,
    LightingModel lightingModel)
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    // Pixel Shader Macro
    D3D_SHADER_MACRO defines_Unlit[] =
    {
        { "UNLIT", "1" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_Gouraud[] =
    {
        { "UNLIT", "0" },
        { "LIGHTING_MODEL_GOURAUD", "1" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_Lambert[] =
    {
        { "UNLIT", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "1" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_BlinnPhong[] =
    {
        { "UNLIT", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "1" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO* defines_Pixel = nullptr;

    switch (lightingModel)
    {
    case LightingModel::Unlit:
        defines_Pixel = defines_Unlit;
        break;
    case LightingModel::Gouraud:
        defines_Pixel = defines_Gouraud;
        break;
    case LightingModel::Lambert:
        defines_Pixel = defines_Lambert;
        break;
    case LightingModel::BlinnPhong:
        defines_Pixel = defines_BlinnPhong;
        break;
    default:
        defines_Pixel = nullptr;
        break;
    }

    ID3DBlob* psBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(psPath.c_str(), defines_Pixel, nullptr, *psEntry, "ps_5_0", shaderFlags, 0, &psBlob, nullptr);

    hr = Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &outPS);
    psBlob->Release();
    return SUCCEEDED(hr);
}

void FShaderManager::ReleaseShader(ID3D11InputLayout* layout, ID3D11VertexShader* vs, ID3D11PixelShader* ps)
{
    if (layout != nullptr)
    {
        layout->Release();
        layout = nullptr;
    }
    if (vs != nullptr)
    {
        vs->Release();
        vs = nullptr;
    }
    if (ps != nullptr)
    {
        ps->Release();
        ps = nullptr;
    }
}

//void FShaderManager::ClearVertexShader(ID3D11VertexShader* VertexShader)
//{
//    if (VertexShader)
//    {
//        DeviceContext->VSSetShader(nullptr, nullptr, 0);
//        VertexShader->Release();
//        VertexShader = nullptr;
//    }
//}
//
//void FShaderManager::ClearPixelShader(ID3D11PixelShader* PixelShader)
//{
//    if (PixelShader)
//    {
//        DeviceContext->PSSetShader(nullptr, nullptr, 0);
//        PixelShader->Release();
//        PixelShader = nullptr;
//    }
//}
//
//void FShaderManager::SetVertexShader(ID3D11VertexShader* VertexShader, const FWString& filename, const FString& funcname, const FString& version)
//{
//    if (VertexShader != nullptr)
//        ClearVertexShader(VertexShader);
//
//    ID3DBlob* vertexshaderCSO;
//
//    D3DCompileFromFile(filename.c_str(), nullptr, nullptr, *funcname, *version, 0, 0, &vertexshaderCSO, nullptr);
//    Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &VertexShader);
//    vertexshaderCSO->Release();
//}
//
//void FShaderManager::SetPixelShader(ID3D11PixelShader* PixelShader, const FWString& filename, const FString& funcname, const FString& version)
//{
//    if (PixelShader != nullptr)
//        ClearPixelShader(PixelShader);
//
//    ID3DBlob* pixelshaderCSO;
//    D3DCompileFromFile(filename.c_str(), nullptr, nullptr, *funcname, *version, 0, 0, &pixelshaderCSO, nullptr);
//    Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &PixelShader);
//
//    pixelshaderCSO->Release();
//}
//
