cbuffer ObjectMatrixConstant : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Projection;
}

Texture2D g_Texture : register(t0);
SamplerState g_sampler : register(s0);

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    float4 WorldPos : SV_Target1;
};

PS_INPUT UberUnlit_VS(VS_INPUT Input)
{
    PS_INPUT Output;
    
    float4 worldPos = mul(float4(Input.Position, 1.0f), World);
    Output.Position = mul(worldPos, mul(View, Projection));
    Output.WorldPos = worldPos.xyz;
    Output.Color = float4(Input.Color.xyz, 1);
    Output.UV = Input.UV;
    
    return Output;
}

PS_OUTPUT UberUnlit_PS(PS_INPUT Input)
{
    PS_OUTPUT Output;
    
    float4 textureColor = g_Texture.Sample(g_sampler, Input.UV);
    bool isValidTexture = dot(textureColor, float4(1, 1, 1, 1)) > 1e-5f;
    
    if (isValidTexture)
    {
        Output.Color = textureColor;
    }
    else
    {
        Output.Color = float4(Input.Color.rgb, 1.f);
    }
    
    Output.WorldPos = float4(Input.WorldPos.xyz, 0.5f);
    
    return Output;
}
