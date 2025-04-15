#include "ShaderManager.h"
#include <d3dcompiler.h>
#include "tinyfiledialogs/tinyfiledialogs.h"

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
    ID3D11VertexShader*& backUpVS,
    const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
    UINT numElements,
    ID3D11InputLayout** outInputLayout,
    ID3D11InputLayout** backUpInputLayout,
    UINT* outStride,
    UINT vertexSize,
    ELightingModel lightingModel)
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
    case ELightingModel::Gouraud:
        defines_Vertex = defines_Gouraud;
    case ELightingModel::Unlit:
    case ELightingModel::Lambert:
    case ELightingModel::BlinnPhong:
        break;
    default:
        defines_Vertex = nullptr;
        break;
    }

    ID3DBlob* vsBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(vsPath.c_str(), defines_Vertex, nullptr, *vsEntry, "vs_5_0", shaderFlags, 0, &vsBlob, nullptr);
    if (FAILED(hr)) 
    {
        if (backUpVS != nullptr) 
        {
            tinyfd_messageBox("Error", "핫 리 로 드 실패, BackUp Shader 적용.", "ok", "error", 1);
            outVS = backUpVS;
        }
        return false;
    }

     hr = Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &outVS);
    if (FAILED(hr)) 
    { 
        if (backUpVS != nullptr)
        {
            tinyfd_messageBox("Error", "핫 리 로 드 실패, BackUp Shader 적용.", "ok", "error", 1);
            outVS = backUpVS;
        }
        vsBlob->Release(); 
        return false;
    }

    if (backUpVS == nullptr) // backUpVS가 비었으면 넣어주기
    {
        backUpVS = outVS;
    }

    if (outInputLayout)
    {
        hr = Device->CreateInputLayout(inputLayoutDesc, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), outInputLayout);
        if (FAILED(hr)) 
        { 
            tinyfd_messageBox("Error", "핫 리 로 드 실패, BackUp InputLayout 적용.", "ok", "error", 1);
            outInputLayout = backUpInputLayout;
            vsBlob->Release(); 
            return false; 
        }
    }
    if (outStride)
        *outStride = vertexSize;

    if (backUpInputLayout == nullptr) // backUpInputLayout 비었으면 넣어주기
    {
        backUpInputLayout = outInputLayout;
    }

    vsBlob->Release();
    return true;
}



bool FShaderManager::CreateVertexShader(const FWString& vsPath, const FString& vsEntry, ID3D11VertexShader*& outVS, const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc, UINT numElements, ID3D11InputLayout** outInputLayout, UINT* outStride, UINT vertexSize, ELightingModel lightingModel)
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* vsBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(vsPath.c_str(), nullptr, nullptr, *vsEntry, "vs_5_0", shaderFlags, 0, &vsBlob, nullptr);
    if (FAILED(hr))
    {
        return false;
    }

    hr = Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &outVS);
    if (FAILED(hr))
    {
        vsBlob->Release();
        return false;
    }

    if (outInputLayout)
    {
        hr = Device->CreateInputLayout(inputLayoutDesc, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), outInputLayout);
        if (FAILED(hr))
        {
            vsBlob->Release();
            return false;
        }
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
    ID3D11PixelShader* backUpPS,
    ELightingModel lightingModel)
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
        { "SHOW_NORMAL", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_Gouraud[] =
    {
        { "UNLIT", "0" },
        { "LIGHTING_MODEL_GOURAUD", "1" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { "SHOW_NORMAL", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_Lambert[] =
    {
        { "UNLIT", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "1" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { "SHOW_NORMAL", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_BlinnPhong[] =
    {
        { "UNLIT", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "1" },
        { "SHOW_NORMAL", "0" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO defines_Normal[] =
    {
        { "UNLIT", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        { "SHOW_NORMAL", "1" },
        { nullptr, nullptr } // 반드시 마지막은 null로 종료
    };

    D3D_SHADER_MACRO* defines_Pixel = nullptr;

    switch (lightingModel)
    {
    case ELightingModel::Unlit:
        defines_Pixel = defines_Unlit;
        break;
    case ELightingModel::Gouraud:
        defines_Pixel = defines_Gouraud;
        break;
    case ELightingModel::Lambert:
        defines_Pixel = defines_Lambert;
        break;
    case ELightingModel::BlinnPhong:
        defines_Pixel = defines_BlinnPhong;
        break;
    case ELightingModel::Normal:
        defines_Pixel = defines_Normal;
        break;
    default:
        defines_Pixel = nullptr;
        break;
    }

    ID3DBlob* psBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(psPath.c_str(), defines_Pixel, nullptr, *psEntry, "ps_5_0", shaderFlags, 0, &psBlob, nullptr);

    if (FAILED(hr))
    {
        if (backUpPS != nullptr) 
        {
            tinyfd_messageBox("Error", "핫 리 로 드 실패, BackUp Shader 적용.", "ok", "error", 1);
            outPS = backUpPS;
        }
        return false;
    }
    hr = Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &outPS);
    psBlob->Release();
    return SUCCEEDED(hr);
}

bool FShaderManager::CreatePixelShader(const FWString& psPath, const FString& psEntry, ID3D11PixelShader*& outPS, ELightingModel lightingModel)
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* psBlob = nullptr;
    HRESULT hr = D3DCompileFromFile(psPath.c_str(), nullptr, nullptr, *psEntry, "ps_5_0", shaderFlags, 0, &psBlob, nullptr);

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
