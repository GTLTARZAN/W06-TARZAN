#ifndef LIGHTINGPASS_H
#define LIGHTINGPASS_H

#include "RenderPass.h"

class LightingPass : public RenderPass {
public:
    virtual void Setup(ID3D11DeviceContext* context) override;
    virtual void Execute(ID3D11DeviceContext* context) override;
    virtual void Cleanup(ID3D11DeviceContext* context) override;
};

inline void LightingPass::Setup(ID3D11DeviceContext* context) 
{
    // 예시: G-buffer의 SRV들을 셰이더에 바인딩하고, 조명 상수 버퍼 업데이트
    // context->PSSetShaderResources(...);
}

inline void LightingPass::Execute(ID3D11DeviceContext* context) 
{
    // 예시: 전체 화면 쿼드(draw full-screen quad)를 그려서, G-buffer 데이터를 활용해 조명 효과를 적용
    // context->Draw(...);
}

inline void LightingPass::Cleanup(ID3D11DeviceContext* context) 
{
    // 예시: 조명 패스에서 변경한 상태를 복원
}

#endif // LIGHTINGPASS_H
