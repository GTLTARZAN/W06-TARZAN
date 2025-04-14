Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer SubUVConstant : register(b1)
{
    float indexU;
    float indexV;
}

cbuffer UUIDConstant : register(b2)
{
    float4 UUID;
}

struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 Normal : SV_Target0;
    float4 Albedo : SV_Target1;
    //float4 uuid : SV_Target1;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float2 uv = input.texCoord + float2(indexU, indexV);
    //float4 col = gTexture.Sample(gSampler, input.texCoord);
    float4 col = gTexture.Sample(gSampler, uv);
    float threshold = 0.05; // 필요한 경우 임계값을 조정
    if (col.r < threshold && col.g < threshold && col.b < threshold)
        clip(-1); // 픽셀 버리기
    
    //output.Color = col;
    output.Normal = float4(col.xyz, 0.5f);
    output.Albedo = float4(col.xyz, 0.5f);
    //output.uuid = UUID;
    
    return output;
}
