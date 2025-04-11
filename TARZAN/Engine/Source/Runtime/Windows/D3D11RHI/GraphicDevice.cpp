#include "GraphicDevice.h"
#include <wchar.h>

#include "Engine/ResourceMgr.h"

void FGraphicsDevice::Initialize(HWND hWindow) {
    CreateDeviceAndSwapChain(hWindow);
    CreateFrameBuffer();
#if USE_GBUFFER
    CreateGBuffer();
#else
    CreateUber();
#endif
    CreateLightPassBuffer();
    CreateDepthStencilBuffer(hWindow);
    CreateDepthStencilState();
    CreateRasterizerState();
    CreateBlendState();
    CurrentRasterizer = RasterizerStateSOLID;
}

void FGraphicsDevice::CreateDeviceAndSwapChain(HWND hWindow) {
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // 스왑 체인 설정 구조체 초기화
    SwapchainDesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 색상 포맷
    SwapchainDesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
    SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
    SwapchainDesc.BufferCount = 2; // 더블 버퍼링
    SwapchainDesc.OutputWindow = hWindow; // 렌더링할 창 핸들
    SwapchainDesc.Windowed = TRUE; // 창 모드
    SwapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

    // 디바이스와 스왑 체인 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
        &SwapchainDesc, &SwapChain, &Device, nullptr, &DeviceContext);

    if (FAILED(hr)) {
        MessageBox(hWindow, L"CreateDeviceAndSwapChain failed!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // 스왑 체인 정보 가져오기 (이후에 사용을 위해)
    SwapChain->GetDesc(&SwapchainDesc);
    screenWidth = SwapchainDesc.BufferDesc.Width;
    screenHeight = SwapchainDesc.BufferDesc.Height;
}

void FGraphicsDevice::CreateDepthStencilBuffer(HWND hWindow) 
{
    RECT clientRect;
    GetClientRect(hWindow, &clientRect);
    UINT width = clientRect.right - clientRect.left;
    UINT height = clientRect.bottom - clientRect.top;

    // 1. DepthStencilBuffer
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width; // 텍스처 너비 설정
    descDepth.Height = height; // 텍스처 높이 설정
    descDepth.MipLevels = 1; // 미맵 레벨 수 (1로 설정하여 미맵 없음)
    descDepth.ArraySize = 1; // 텍스처 배열의 크기 (1로 단일 텍스처)
    descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS; // typeless 포맷 (DSV + SRV 겸용 사용 위함)
    descDepth.SampleDesc.Count = 1; // 멀티샘플링 설정 (1로 단일 샘플)
    descDepth.SampleDesc.Quality = 0; // 샘플 퀄리티 설정
    descDepth.Usage = D3D11_USAGE_DEFAULT; // 텍스처 사용 방식
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0; // CPU 접근 방식 설정
    descDepth.MiscFlags = 0; // 기타 플래그 설정

    HRESULT hr = Device->CreateTexture2D(&descDepth, NULL, &DepthStencilBuffer);

    if (FAILED(hr)) {
        MessageBox(hWindow, L"Failed to create depth stencilBuffer!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // 2. DepthStencilView
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 깊이 스텐실 포맷
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // 뷰 타입 설정 (2D 텍스처)
    descDSV.Texture2D.MipSlice = 0; // 사용할 미맵 슬라이스 설정

    hr = Device->CreateDepthStencilView(DepthStencilBuffer, // Depth stencil texture
        &descDSV, // Depth stencil desc
        &DepthStencilView);  // [out] Depth stencil view

    if (FAILED(hr)) {
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to create depth stencil view! HRESULT: 0x%08X", hr);
        MessageBox(hWindow, errorMsg, L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // 3. DepthStencilSRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = Device->CreateShaderResourceView(DepthStencilBuffer, &srvDesc, &DepthStencilSRV);
    if (FAILED(hr))
    {
        MessageBox(hWindow, L"Failed to create depth SRV!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }
}

void FGraphicsDevice::CreateDepthStencilState()
{
    // DepthStencil 상태 설명 설정
    D3D11_DEPTH_STENCIL_DESC dsDesc;

    // Depth test parameters
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    dsDesc.StencilEnable = true;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // DepthStencil 상태 생성
    Device->CreateDepthStencilState(&dsDesc, &DepthStencilState);

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = FALSE;  // 깊이 테스트 유지
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;  // 깊이 버퍼에 쓰지 않음
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;  // 깊이 비교를 항상 통과
    Device->CreateDepthStencilState(&depthStencilDesc, &DepthStateDisable);
}

void FGraphicsDevice::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID;
    rasterizerdesc.CullMode = D3D11_CULL_BACK;
    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerStateSOLID);

    rasterizerdesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerdesc.CullMode = D3D11_CULL_BACK;
    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerStateWIREFRAME);
}

void FGraphicsDevice::CreateUber()
{
    D3D11_TEXTURE2D_DESC UberTexDesc = {};
    {
        UberTexDesc.Width = screenWidth;
        UberTexDesc.Height = screenHeight;
        UberTexDesc.MipLevels = 1;
        UberTexDesc.ArraySize = 1;
        UberTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        UberTexDesc.SampleDesc.Count = 1;
        UberTexDesc.Usage = D3D11_USAGE_DEFAULT;
        UberTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    }

    D3D11_RENDER_TARGET_VIEW_DESC UberRTVDesc = {};
    {
        UberRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;      // 색상 포맷
        UberRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC UberSRVDesc = {};
    {
        UberSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        UberSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        UberSRVDesc.Texture2D.MipLevels = 1;
        UberSRVDesc.Texture2D.MostDetailedMip = 0;
    }

    Device->CreateTexture2D(&UberTexDesc, nullptr, &UberTexture_Color);
    Device->CreateTexture2D(&UberTexDesc, nullptr, &UberTexture_Position);

    Device->CreateRenderTargetView(UberTexture_Color, &UberRTVDesc, &UberRTV_Color);
    Device->CreateRenderTargetView(UberTexture_Position, &UberRTVDesc, &UberRTV_Position);

    Device->CreateShaderResourceView(UberTexture_Color, &UberSRVDesc, &UberSRV_Color);
    Device->CreateShaderResourceView(UberTexture_Position, &UberSRVDesc, &UberSRV_Position);

    UberRTVs[0] = UberRTV_Color;
    UberRTVs[1] = UberRTV_Position;
}

void FGraphicsDevice::CreateGBuffer()
{
    //SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&GBufferTexture_Albedo);

    D3D11_TEXTURE2D_DESC GBufferTexDesc = {};
    {
        GBufferTexDesc.Width = screenWidth;
        GBufferTexDesc.Height = screenHeight;
        GBufferTexDesc.MipLevels = 1;
        GBufferTexDesc.ArraySize = 1;
        GBufferTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        GBufferTexDesc.SampleDesc.Count = 1;
        GBufferTexDesc.Usage = D3D11_USAGE_DEFAULT;
        GBufferTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    }
    
    D3D11_RENDER_TARGET_VIEW_DESC GBufferRTVDesc = {};
    {
        GBufferRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;      // 색상 포맷
        GBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC GBufferSRVDesc = {};
    {
        GBufferSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        GBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        GBufferSRVDesc.Texture2D.MipLevels = 1;
        GBufferSRVDesc.Texture2D.MostDetailedMip = 0;
    }

    Device->CreateTexture2D(&GBufferTexDesc, nullptr, &GBufferTexture_Normal);
    Device->CreateTexture2D(&GBufferTexDesc, nullptr, &GBufferTexture_Albedo);
    Device->CreateTexture2D(&GBufferTexDesc, nullptr, &GBufferTexture_Ambient);
    Device->CreateTexture2D(&GBufferTexDesc, nullptr, &GBufferTexture_Position);

    Device->CreateRenderTargetView(GBufferTexture_Normal, &GBufferRTVDesc, &GBufferRTV_Normal);
    Device->CreateRenderTargetView(GBufferTexture_Albedo, &GBufferRTVDesc, &GBufferRTV_Albedo);
    Device->CreateRenderTargetView(GBufferTexture_Ambient, &GBufferRTVDesc, &GBufferRTV_Ambient);
    Device->CreateRenderTargetView(GBufferTexture_Position, &GBufferRTVDesc, &GBufferRTV_Position);

    Device->CreateShaderResourceView(GBufferTexture_Normal, &GBufferSRVDesc, &GBufferSRV_Normal);
    Device->CreateShaderResourceView(GBufferTexture_Albedo, &GBufferSRVDesc, &GBufferSRV_Albedo);
    Device->CreateShaderResourceView(GBufferTexture_Ambient, &GBufferSRVDesc, &GBufferSRV_Ambient);
    Device->CreateShaderResourceView(GBufferTexture_Position, &GBufferSRVDesc, &GBufferSRV_Position);

    GBufferRTVs[0] = GBufferRTV_Normal;
    GBufferRTVs[1] = GBufferRTV_Albedo;
    GBufferRTVs[2] = GBufferRTV_Ambient;
    GBufferRTVs[3] = GBufferRTV_Position;
}

void FGraphicsDevice::CreateFrameBuffer()
{
    // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 색상 포맷
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
    
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = screenWidth;
    textureDesc.Height = screenHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    Device->CreateTexture2D(&textureDesc, nullptr, &UUIDFrameBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC UUIDFrameBufferRTVDesc = {};
    UUIDFrameBufferRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;      // 색상 포맷
    UUIDFrameBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(UUIDFrameBuffer, &UUIDFrameBufferRTVDesc, &UUIDFrameBufferRTV);

    RTVs[0] = FrameBufferRTV;
    RTVs[1] = UUIDFrameBufferRTV;
}

void FGraphicsDevice::CreateLightPassBuffer()
{
    D3D11_TEXTURE2D_DESC LightPassTexDesc = {};
    {
        LightPassTexDesc.Width = screenWidth;
        LightPassTexDesc.Height = screenHeight;
        LightPassTexDesc.MipLevels = 1;
        LightPassTexDesc.ArraySize = 1;
        LightPassTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        LightPassTexDesc.SampleDesc.Count = 1;
        LightPassTexDesc.Usage = D3D11_USAGE_DEFAULT;
        LightPassTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    }

    D3D11_RENDER_TARGET_VIEW_DESC LightPassRTVDesc = {};
    {
        LightPassRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;      
        LightPassRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; 
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC LightPassSRVDesc = {};
    {
        LightPassSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        LightPassSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        LightPassSRVDesc.Texture2D.MipLevels = 1;
        LightPassSRVDesc.Texture2D.MostDetailedMip = 0;
    }

    Device->CreateTexture2D(&LightPassTexDesc, nullptr, &LightPassTexture_Color);
    Device->CreateTexture2D(&LightPassTexDesc, nullptr, &LightPassTexture_Position);

    Device->CreateRenderTargetView(LightPassTexture_Color, &LightPassRTVDesc, &LightPassRTV_Color);
    Device->CreateRenderTargetView(LightPassTexture_Position, &LightPassRTVDesc, &LightPassRTV_Position);

    Device->CreateShaderResourceView(LightPassTexture_Color, &LightPassSRVDesc, &LightPassSRV_Color);
    Device->CreateShaderResourceView(LightPassTexture_Position, &LightPassSRVDesc, &LightPassSRV_Position);

    LightPassRTVs[0] = LightPassRTV_Color;
    LightPassRTVs[1] = LightPassRTV_Position;
}

void FGraphicsDevice::CreateBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Device->CreateBlendState(&blendDesc, &LineBlendState);
}

void FGraphicsDevice::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // 남아있는 GPU 명령 실행
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
}

void FGraphicsDevice::ReleaseUber()
{
    if (UberTexture_Color) { UberTexture_Color->Release(); UberTexture_Color = nullptr; }
    if (UberRTV_Color) { UberRTV_Color->Release();     UberRTV_Color = nullptr; }
    if (UberSRV_Color) { UberSRV_Color->Release();     UberSRV_Color = nullptr; }

    if (UberTexture_Position) { UberTexture_Position->Release(); UberTexture_Position = nullptr; }
    if (UberRTV_Position) { UberRTV_Position->Release();     UberRTV_Position = nullptr; }
    if (UberSRV_Position) { UberSRV_Position->Release();     UberSRV_Position = nullptr; }
}

void FGraphicsDevice::ReleaseGBuffer()
{
    if (GBufferTexture_Normal) { GBufferTexture_Normal->Release(); GBufferTexture_Normal = nullptr; }
    if (GBufferRTV_Normal)     { GBufferRTV_Normal->Release();     GBufferRTV_Normal = nullptr; }
    if (GBufferSRV_Normal)     { GBufferSRV_Normal->Release();     GBufferSRV_Normal = nullptr; }

    if (GBufferTexture_Albedo) { GBufferTexture_Albedo->Release(); GBufferTexture_Albedo = nullptr; }
    if (GBufferRTV_Albedo)     { GBufferRTV_Albedo->Release();     GBufferRTV_Albedo = nullptr; }
    if (GBufferSRV_Albedo)     { GBufferSRV_Albedo->Release();     GBufferSRV_Albedo = nullptr; }

    if (GBufferTexture_Position) { GBufferTexture_Position->Release(); GBufferTexture_Position = nullptr; }
    if (GBufferRTV_Position)     { GBufferRTV_Position->Release();     GBufferRTV_Position = nullptr; }
    if (GBufferSRV_Position)     { GBufferSRV_Position->Release();     GBufferSRV_Position = nullptr; }
}

void FGraphicsDevice::ReleaseFrameBuffer()
{
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    if (FrameBufferRTV)
    {
        FrameBufferRTV->Release();
        FrameBufferRTV = nullptr;
    }

    if (UUIDFrameBuffer)
    {
        UUIDFrameBuffer->Release();
        UUIDFrameBuffer = nullptr;
    }

    if (UUIDFrameBufferRTV)
    {
        UUIDFrameBufferRTV->Release();
        UUIDFrameBufferRTV = nullptr;
    }
}

void FGraphicsDevice::ReleaseLightPassBuffer()
{
    if (LightPassTexture_Color) { LightPassTexture_Color->Release(); LightPassTexture_Color = nullptr; }
    if (LightPassRTV_Color) { LightPassRTV_Color->Release();     LightPassRTV_Color = nullptr; }
    if (LightPassSRV_Color) { LightPassSRV_Color->Release();     LightPassSRV_Color = nullptr; }

    if (LightPassTexture_Position) { LightPassTexture_Position->Release(); LightPassTexture_Position = nullptr; }
    if (LightPassRTV_Position) { LightPassRTV_Position->Release();     LightPassRTV_Position = nullptr; }
    if (LightPassSRV_Position) { LightPassSRV_Position->Release();     LightPassSRV_Position = nullptr; }
}

void FGraphicsDevice::ReleaseRasterizerState()
{
    if (RasterizerStateSOLID)
    {
        RasterizerStateSOLID->Release();
        RasterizerStateSOLID = nullptr;
    }
    if (RasterizerStateWIREFRAME)
    {
        RasterizerStateWIREFRAME->Release();
        RasterizerStateWIREFRAME = nullptr;
    }
}

void FGraphicsDevice::ReleaseDepthStencilResources()
{
    if (DepthStencilView) {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }

    if (DepthStencilBuffer) {
        DepthStencilBuffer->Release();
        DepthStencilBuffer = nullptr;
    }

    if (DepthStencilState) {
        DepthStencilState->Release();
        DepthStencilState = nullptr;
    }

    if (DepthStateDisable) {
        DepthStateDisable->Release();
        DepthStateDisable = nullptr;
    }
}

void FGraphicsDevice::ReleaseBlendState()
{
    if (LineBlendState)
    {
        LineBlendState->Release();
        LineBlendState = nullptr;
    }
}

void FGraphicsDevice::Release() 
{
    ReleaseRasterizerState();
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

#if USE_GBUFFER
    ReleaseGBuffer();
#else
    ReleaseUber();
#endif
    ReleaseFrameBuffer();
    ReleaseLightPassBuffer();
    ReleaseDepthStencilResources();
    ReleaseDeviceAndSwapChain();
    ReleaseBlendState();
}

void FGraphicsDevice::SwapBuffer() {
    SwapChain->Present(1, 0);
}

void FGraphicsDevice::Prepare()
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor); // 렌더 타겟 뷰에 저장된 이전 프레임 데이터를 삭제
    DeviceContext->ClearRenderTargetView(UUIDFrameBufferRTV, ClearColor); 
  
#if USE_GBUFFER
    DeviceContext->ClearRenderTargetView(GBufferRTV_Normal, ClearColor); 
    DeviceContext->ClearRenderTargetView(GBufferRTV_Albedo, ClearColor); 
    DeviceContext->ClearRenderTargetView(GBufferRTV_Ambient, ClearColor); 
    DeviceContext->ClearRenderTargetView(GBufferRTV_Position, ClearColor);
#else
    DeviceContext->ClearRenderTargetView(UberRTV_Color, ClearColor);
    DeviceContext->ClearRenderTargetView(UberRTV_Position, ClearColor);
#endif

    DeviceContext->ClearRenderTargetView(LightPassRTV_Color, ClearColor);
    DeviceContext->ClearRenderTargetView(LightPassRTV_Position, ClearColor);

    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // 깊이 버퍼 초기화 추가

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정정 연결 방식 설정

    //DeviceContext->RSSetViewports(1, &ViewportInfo); // GPU가 화면을 렌더링할 영역 설정
    DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정

    DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);

#if USE_GBUFFER
    DeviceContext->OMSetRenderTargets(4, GBufferRTVs, DepthStencilView);  // 렌더 타겟 설정(백버퍼를 가르킴)
#else
    DeviceContext->OMSetRenderTargets(2, UberRTVs, DepthStencilView);
#endif

    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // 블렌뎅 상태 설정, 기본블렌딩 상태임
}

void FGraphicsDevice::OnResize(HWND hWindow)
{
    // 먼저 기존 GBuffer 관련 리소스를 해제합니다.
    ReleaseFrameBuffer();
    ReleaseGBuffer();
    ReleaseLightPassBuffer();

    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }

    // 스왑 체인의 버퍼 크기를 재조정합니다.
    HRESULT hr = SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
    if (FAILED(hr))
    {
        MessageBox(hWindow, L"ResizeBuffers 실패!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // 새로 조정된 크기를 가져옵니다.
    SwapChain->GetDesc(&SwapchainDesc);
    screenWidth = SwapchainDesc.BufferDesc.Width;
    screenHeight = SwapchainDesc.BufferDesc.Height;

    // Buffer 재생성
    CreateFrameBuffer();
    CreateGBuffer();
    CreateLightPassBuffer();
    CreateDepthStencilBuffer(hWindow);
}

void FGraphicsDevice::ChangeRasterizer(EViewModeIndex evi)
{
    switch (evi)
    {
    case EViewModeIndex::VMI_Wireframe:
        CurrentRasterizer = RasterizerStateWIREFRAME;
        break;
    case EViewModeIndex::VMI_Lit:
    case EViewModeIndex::VMI_Unlit:
        CurrentRasterizer = RasterizerStateSOLID;
        break;
    }
    DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정
}

void FGraphicsDevice::ChangeDepthStencilState(ID3D11DepthStencilState* newDetptStencil)
{
    DeviceContext->OMSetDepthStencilState(newDetptStencil, 0);
}

uint32 FGraphicsDevice::GetPixelUUID(POINT pt)
{
    //// pt.x 값 제한하기
    //if (pt.x < 0) {
    //    pt.x = 0;
    //}
    //else if (pt.x > screenWidth) {
    //    pt.x = screenWidth;
    //}

    //// pt.y 값 제한하기
    //if (pt.y < 0) {
    //    pt.y = 0;
    //}
    //else if (pt.y > screenHeight) {
    //    pt.y = screenHeight;
    //}

    //// 1. Staging 텍스처 생성 (1x1 픽셀)
    //D3D11_TEXTURE2D_DESC stagingDesc = {};
    //stagingDesc.Width = 1; // 픽셀 1개만 복사
    //stagingDesc.Height = 1;
    //stagingDesc.MipLevels = 1;
    //stagingDesc.ArraySize = 1;
    //stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 원본 텍스처 포맷과 동일
    //stagingDesc.SampleDesc.Count = 1;
    //stagingDesc.Usage = D3D11_USAGE_STAGING;
    //stagingDesc.BindFlags = 0;
    //stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    //ID3D11Texture2D* stagingTexture = nullptr;
    //Device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);

    //// 2. 복사할 영역 정의 (D3D11_BOX)
    //D3D11_BOX srcBox = {};
    //srcBox.left = static_cast<UINT>(pt.x);
    //srcBox.right = srcBox.left + 1; // 1픽셀 너비
    //srcBox.top = static_cast<UINT>(pt.y);
    //srcBox.bottom = srcBox.top + 1; // 1픽셀 높이
    //srcBox.front = 0;
    //srcBox.back = 1;
    //FVector4 UUIDColor{ 1, 1, 1, 1 }; 

    //if (stagingTexture == nullptr)
    //    return DecodeUUIDColor(UUIDColor);

    //// 3. 특정 좌표만 복사
    //DeviceContext->CopySubresourceRegion(
    //    stagingTexture, // 대상 텍스처
    //    0,              // 대상 서브리소스
    //    0, 0, 0,        // 대상 좌표 (x, y, z)
    //    UUIDFrameBuffer, // 원본 텍스처
    //    0,              // 원본 서브리소스
    //    &srcBox         // 복사 영역
    //);

    //// 4. 데이터 매핑
    //D3D11_MAPPED_SUBRESOURCE mapped = {};
    //DeviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);

    //// 5. 픽셀 데이터 추출 (1x1 텍스처이므로 offset = 0)
    //const BYTE* pixelData = static_cast<const BYTE*>(mapped.pData);

    //if (pixelData)
    //{
    //    UUIDColor.x = static_cast<float>(pixelData[0]); // R
    //    UUIDColor.y = static_cast<float>(pixelData[1]); // G
    //    UUIDColor.z = static_cast<float>(pixelData[2]) ; // B
    //    UUIDColor.a = static_cast<float>(pixelData[3]); // A
    //}

    //// 6. 매핑 해제 및 정리
    //DeviceContext->Unmap(stagingTexture, 0);
    //if (stagingTexture) stagingTexture->Release(); stagingTexture = nullptr;

    //return DecodeUUIDColor(UUIDColor);
    return 0;
}

uint32 FGraphicsDevice::DecodeUUIDColor(FVector4 UUIDColor) {
    //uint32_t W = static_cast<uint32_t>(UUIDColor.a) << 24;
    //uint32_t Z = static_cast<uint32_t>(UUIDColor.z) << 16;
    //uint32_t Y = static_cast<uint32_t>(UUIDColor.y) << 8;
    //uint32_t X = static_cast<uint32_t>(UUIDColor.x);

    //return W | Z | Y | X;

    return 0;
}