// Uber.hlsl

#define NUM_POINT_LIGHT 4
#define NUM_SPOT_LIGHT 4



struct FAmbientLightInfo
{
    float4 Color;
    float Intensity;
    float3 Pad;
};

struct FDirectionalLightInfo
{
    float4 Color;
    float3 Direction;
    float Intensity;
};

struct FPointLightInfo
{
    float4 Color;
    float3 Position;
    float Intensity;
    float AttenuationRadius;
    float LightFalloffExponent;
    float2 Pad;
};

struct FSpotLightInfo
{
    float4 Color;
    float3 Position;
    float3 Direction;
    float Intensity;
    float AttenuationRadius;
    float LightFalloffExponent;
    float InnerConeAngle;
    float OuterConeAngle;
    float Pad;
};

struct FMaterialInfo
{
    float3 DiffuseColor;
    float3 AmbientColor;
    float3 SpecularColor;
    float3 EmmisiveColor;
    float TransparencyScalar;
    float DensityScalar;
    float SpecularScalar;
    float MaterialPad0;
};

cbuffer ObjectMatrixConstant : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Projection;
};

cbuffer CameraConstant : register(b1)
{
    float3 CameraWorldPos;
    float Padding;
};

cbuffer LightConstant : register(b2)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    FPointLightInfo PointLights[NUM_POINT_LIGHT];
    FSpotLightInfo SpotLights[NUM_SPOT_LIGHT];
};

cbuffer MaterialConstant : register(b3)
{
    FMaterialInfo Material;
};

Texture2D g_Texture : register(t0);
Texture2D g_NormalMap : register(t1);
SamplerState g_Sampler : register(s0);

float4 CalculateAmbientLight(FAmbientLightInfo info)
{
    return info.Color * info.Intensity;
}

float4 CalculateDirectionalLight(FDirectionalLightInfo info, float3 normal, float3 viewDir)
{
    float3 lightDir = normalize(-info.Direction.xyz);
    float NdotL = max(0.0f, dot(normal, lightDir));
    return info.Color * info.Intensity * NdotL;
}

float4 CalculateDirectionalLightBlinnPhong(FDirectionalLightInfo info, float3 normal, float3 viewDir);

float4 CalculatePointLight(FPointLightInfo info, float3 worldPos, float3 normal, float3 viewDir)
{
    float3 lightDir = info.Position.xyz - worldPos;
    float distance = length(lightDir);
    
    // 거리가 Radius를 초과하면 빛의 영향을 주지 않음
    if (distance > info.AttenuationRadius)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
        
    lightDir = normalize(lightDir);
    
    // 거리에 따른 감쇠 계산 (Radius를 기준으로 정규화)
    float normalizedDistance = distance / info.AttenuationRadius;
    float attenuation = 1.0f / (1.0f + info.LightFalloffExponent * normalizedDistance * normalizedDistance);
    float NdotL = max(0.0f, dot(normal, lightDir));
    
    return info.Color * info.Intensity * NdotL * attenuation;
}

float4 CalculatePointLightBlinnPhong(FPointLightInfo info, float3 worldPos, float3 normal, float3 viewDir);

float4 CalculateSpotLight(FSpotLightInfo info, float3 worldPos, float3 normal, float3 viewDir)
{
    float3 lightDir = info.Position.xyz - worldPos;
    float distance = length(lightDir);
    
    // 거리가 Radius를 초과하면 빛의 영향을 주지 않음
    if (distance > info.AttenuationRadius)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
        
    lightDir = normalize(lightDir);
    
    // 거리에 따른 감쇠 계산 (Radius를 기준으로 정규화)
    float normalizedDistance = distance / info.AttenuationRadius;
    float attenuation = 1.0f / (1.0f + info.LightFalloffExponent * normalizedDistance * normalizedDistance);
    float NdotL = max(0.0f, dot(normal, lightDir));
    
    float3 spotDir = normalize(-info.Direction.xyz);
    float spotFactor = dot(lightDir, spotDir);
    float spotLightFactor = smoothstep(cos(info.OuterConeAngle), cos(info.InnerConeAngle), spotFactor);
    
    return info.Color * info.Intensity * NdotL * attenuation * spotLightFactor;
}

float4 CalculateSpotLightBlinnPhong(FSpotLightInfo info, float3 worldPos, float3 normal, float3 viewDir);

struct VS_IN
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
    float MaterialIndex : MATERIAL_INDEX;
};

struct VS_OUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
};

struct PS_OUT
{
    float4 Color : SV_Target0;
    float4 WorldPos : SV_Target1;
};

VS_OUT Uber_VS(VS_IN Input)
{
    VS_OUT output;
    float4 worldPos = mul(float4(Input.Position, 1.0f), World);
    output.Position = mul(worldPos, mul(View, Projection));
    output.WorldPos = worldPos.xyz;
    output.Normal = normalize(mul(Input.Normal, (float3x3)World));
    output.TexCoord = Input.TexCoord;
    
#if LIGHTING_MODEL_GOURAUD
    float3 viewDir = normalize(-worldPos.xyz);
    float4 finalColor = CalculateAmbientLight(Ambient);
    finalColor += CalculateDirectionalLight(Directional, output.Normal, viewDir);
    
    for (int i = 0; i < NUM_POINT_LIGHT; i++)
    {
        finalColor += CalculatePointLight(PointLights[i], output.WorldPos, output.Normal, viewDir);
    }
    
    for (int j = 0; j < NUM_SPOT_LIGHT; j++)
    {
        finalColor += CalculateSpotLight(SpotLights[j], output.WorldPos, output.Normal, viewDir);
    }
    
    output.Color = finalColor;
#elif NORMAL_VERTEX
    output.Color = Input.Color;
#endif
    
    return output;
}

PS_OUT Uber_PS(VS_OUT Input)
{
    PS_OUT Output;
    float4 finalPixel;
     
#if LIGHTING_MODEL_GOURAUD
    finalPixel = Input.Color;
#elif LIGHTING_MODEL_LAMBERT
    float3 viewDir = normalize(Input.WorldPos - CameraWorldPos);
    // AmbientLight
    float4 finalColor = CalculateAmbientLight(Ambient);
    // DirectionalLight
    finalColor += CalculateDirectionalLight(Directional, Input.Normal, viewDir);
    // PointLights
    for (int i = 0; i < NUM_POINT_LIGHT; i++)
    {
        finalColor += CalculatePointLight(PointLights[i], Input.WorldPos, Input.Normal, viewDir);
    }
    // SpotLights
    for (int j = 0; j < NUM_SPOT_LIGHT; j++)
    {
        finalColor += CalculateSpotLight(SpotLights[j], Input.WorldPos, Input.Normal, viewDir);
    }
    
    finalPixel = finalColor;
#elif LIGHTING_MODEL_PHONG
    float3 viewDir = normalize(Input.WorldPos - CameraWorldPos);
    float4 finalColor = CalculateAmbientLight(Ambient);
    
    // DirectionalLight
    finalColor += CalculateDirectionalLightBlinnPhong(Directional, Input.Normal, viewDir);
    
    // PointLight
    for (int i = 0; i < NUM_POINT_LIGHT; i++)
    {
        finalColor += CalculatePointLightBlinnPhong(PointLights[i], Input.WorldPos, Input.Normal, viewDir);
    }
    
    // SpotLight
    for (int j = 0; j < NUM_SPOT_LIGHT; j++)
    {
        finalColor += CalculateSpotLightBlinnPhong(SpotLights[j], Input.WorldPos, Input.Normal, viewDir);
    }
    
    finalPixel = finalColor;
#elif UNLIT
    float4 textureColor = g_Texture.Sample(g_Sampler, Input.TexCoord);
    bool isValidTexture = dot(textureColor, float4(1, 1, 1, 1)) > 1e-5f;
    
    if (isValidTexture)
    {
        Output.Color = textureColor;
    }
    else
    {
        Output.Color = float4(Input.Color.rgb, 1.f);
    }
#endif
    
    Output.Color = finalPixel;
    Output.WorldPos = float4(Input.WorldPos.xyz, 0.5f);
    
    return Output;
}

#if LIGHTING_MODEL_PHONG
// Caculate Ambient는 기존 CalculateAmbientLight 사용

float4 CalculateDirectionalLightBlinnPhong(FDirectionalLightInfo info, float3 normal, float3 viewDir)
{
    // Diffuse
    float diff = max(dot(normal, info.Direction.xyz), 0.0f);
    
    // Specular
    
    float3 halfDir = normalize(info.Direction.xyz + viewDir);
    
    // TODO: 32 값은 Roughness 값으로 변수화 필요
    float spec = pow(max(dot(normal, halfDir), 0.0f), 32);

    return info.Color * info.Intensity * (diff + spec);
}

float4 CalculatePointLightBlinnPhong(FPointLightInfo info, float3 worldPos, float3 normal, float3 viewDir)
{
    float3 lightDir = info.Position.xyz - worldPos;
    float distance = length(lightDir);
    // 거리가 Radius를 초과하면 빛의 영향을 주지 않음
    if (distance > info.AttenuationRadius)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    lightDir = normalize(lightDir);
    

    // 거리에 따른 감쇠 계산 (Radius를 기준으로 정규화)
    float normalizedDistance = distance / info.AttenuationRadius;
    float attenuation = 1.0f / (1.0f + info.LightFalloffExponent * normalizedDistance * normalizedDistance);
    float diff = max(0.0f, dot(normal, lightDir));
    
    float3 halfDir = normalize(lightDir + viewDir);
    // TODO: 32 값은 Roughness 값으로 변수화 필요
    float spec = pow(max(dot(normal, halfDir), 0.0f), 32);
    
    return info.Color * info.Intensity * attenuation * (diff + spec);
}

float4 CalculateSpotLightBlinnPhong(FSpotLightInfo info, float3 worldPos, float3 normal, float3 viewDir)
{
    float3 lightDir = info.Position.xyz - worldPos;
    float distance = length(lightDir);
    // 거리가 Radius를 초과하면 빛의 영향을 주지 않음
    if (distance > info.AttenuationRadius)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    lightDir = normalize(lightDir);
    
    // 거리에 따른 감쇠 계산 (Radius를 기준으로 정규화)
    float normalizedDistance = distance / info.AttenuationRadius;
    float attenuation = 1.0f / (1.0f + info.LightFalloffExponent * normalizedDistance * normalizedDistance);
    float diff = max(0.0f, dot(normal, lightDir));
    float4 diffuse = info.Color * info.Intensity * attenuation * diff;
    
    float3 halfDir = normalize(lightDir + viewDir);
    // TODO: 32 값은 Roughness 값으로 변수화 필요
    float spec = pow(max(dot(normal, halfDir), 0.0f), 32);
    float4 specular = info.Color * info.Intensity * attenuation * spec;
    
    float3 spotDir = normalize(-info.Direction.xyz);
    float spotFactor = dot(lightDir, spotDir);
    float spotLightFactor = smoothstep(cos(info.OuterConeAngle), cos(info.InnerConeAngle), spotFactor);
    
    return info.Color * info.Intensity * attenuation * spotLightFactor * (diff + spec);
}

#endif