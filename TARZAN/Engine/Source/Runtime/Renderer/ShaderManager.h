#pragma once
#include <HAL/PlatformType.h>
#include <d3d11.h>
#include <string>
#include <Container/String.h>

enum class ELightingModel 
{
    None,
    Gouraud,
    Lambert,
    BlinnPhong,
    Normal, // Lighting Model은 아니지만 Normal을 보여주는 용도
    Unlit,
};

class FShaderManager
{
public:
    void Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);
    void Release();

    // 함수 오버로딩을 활용한 Hot Reload 지원 CreateVertexShader
    bool CreateVertexShader(
        const FWString& vsPath,
        const FString& vsEntry,
        ID3D11VertexShader*& outVS,
        ID3D11VertexShader*& BackUpVS,
        const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
        UINT numElements,
        ID3D11InputLayout** outInputLayout = nullptr,
        ID3D11InputLayout** backUpInputLayout = nullptr,
        UINT* outStride = nullptr,
        UINT vertexSize = 0,
        ELightingModel lightingModel = ELightingModel::None);

    // 기존 CreateVertexShader
    bool CreateVertexShader(
        const FWString& vsPath,
        const FString& vsEntry,
        ID3D11VertexShader*& outVS,
        const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
        UINT numElements,
        ID3D11InputLayout** outInputLayout = nullptr,
        UINT* outStride = nullptr,
        UINT vertexSize = 0,
        ELightingModel lightingModel = ELightingModel::None);

    // 함수 오버로딩을 활용한 Hot Reload 지원 CreatePixelShader
    bool CreatePixelShader(
        const FWString& psPath,
        const FString& psEntry,
        ID3D11PixelShader*& outPS,
        ID3D11PixelShader* backUpPS,
        ELightingModel lightingModel = ELightingModel::None);

    // 매크로를 직접 전달받는 CreatePixelShader 오버로드 추가
    bool CreatePixelShader(
        const FWString& psPath,
        const FString& psEntry,
        ID3D11PixelShader*& outPS,
        const D3D_SHADER_MACRO* pDefines); // 매크로 인자 추가

    // 기존 CreatePixelShader
    bool CreatePixelShader(
        const FWString& psPath,
        const FString& psEntry,
        ID3D11PixelShader*& outPS,
        ELightingModel lightingModel = ELightingModel::None);

    // 컴퓨트 셰이더 생성 함수 추가
    bool CreateComputeShader(
        const FWString& csPath,
        const FString& csEntry,
        ID3D11ComputeShader*& outCS);

    void ReleaseShader(ID3D11InputLayout* layout, ID3D11VertexShader* vs, ID3D11PixelShader* ps);

    //void SetVertexShader(ID3D11VertexShader* VertexShader, const FWString& filename, const FString& funcname, const FString& version);
    //void SetPixelShader(ID3D11PixelShader* PixelShader, const FWString& filename, const FString& funcname, const FString& version);
    //void ClearVertexShader(ID3D11VertexShader* VertexShader);
    //void ClearPixelShader(ID3D11PixelShader* PixelShader);

private:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;

    ID3D11VertexShader* CurrentVS = nullptr;
    ID3D11PixelShader* CurrentPS = nullptr;
};

