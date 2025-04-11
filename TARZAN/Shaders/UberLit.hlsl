// UberLit.hlsl

#define NUM_POINT_LIGHT 4
#define NUM_SPOT_LIGHT 4

struct FAmbientLightInfo
{
    float4 Color;
    float Intensity;
};

struct FDirectionalLightInfo
{
    float4 Color;
    float4 Direction;
    float Intensity;
};

struct FPointLightInfo
{
    float4 Color;
    float4 Position;
    float Intensity;
    float AttenuationRadius;
    float LightFalloffExponent;
};

struct FSpotLightInfo
{
    float4 Color;
    float4 Position;
    float4 Direction;
    float Intensity;
    float AttenuationRadius;
    float LightFalloffExponent;
    float InnerConeAngle;
    float OuterConeAngle;
};

cbuffer PerObject : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Projection;
};

cbuffer Lighting : register(b1)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    FPointLightInfo PointLights[NUM_POINT_LIGHT];
    FSpotLightInfo SpotLights[NUM_SPOT_LIGHT];
};

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

struct VS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
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
#elif LIGHTING_MODEL_LAMBERT
    output.Color = float4(1.0f, 1.0f, 1.0f, 1.0f);
#elif LIGHTING_MODEL_PHONG
    output.Color = float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif
    
    return output;
}

float4 Uber_PS(VS_OUT Input) : SV_TARGET
{
    float4 finalPixel;
    
#if LIGHTING_MODEL_GOURAUD
    finalPixel = Input.Color;
#elif LIGHTING_MODEL_LAMBERT
    float3 viewDir = normalize(-Input.WorldPos);
    float4 finalColor = CalculateAmbientLight(Ambient);
    finalColor += CalculateDirectionalLight(Directional, Input.Normal, viewDir);
    
    for (int i = 0; i < NUM_POINT_LIGHT; i++)
    {
        finalColor += CalculatePointLight(PointLights[i], Input.WorldPos, Input.Normal, viewDir);
    }
    
    for (int j = 0; j < NUM_SPOT_LIGHT; j++)
    {
        finalColor += CalculateSpotLight(SpotLights[j], Input.WorldPos, Input.Normal, viewDir);
    }
    
    finalPixel = finalColor;
#elif LIGHTING_MODEL_PHONG
    float3 viewDir = normalize(-Input.WorldPos);
    float4 finalColor = CalculateAmbientLight(Ambient);
    
    // Directional Light
    float3 lightDir = normalize(-Directional.Direction.xyz);
    float NdotL = max(0.0f, dot(Input.Normal, lightDir));
    float3 reflectDir = reflect(-lightDir, Input.Normal);
    float spec = pow(max(0.0f, dot(viewDir, reflectDir)), 32.0f);
    finalColor += Directional.Color * Directional.Intensity * (NdotL + spec);
    
    // Point Lights
    for (int i = 0; i < NUM_POINT_LIGHT; i++)
    {
        float3 pointLightDir = PointLights[i].Position.xyz - Input.WorldPos;
        float distance = length(pointLightDir);
        pointLightDir = normalize(pointLightDir);

        float attenuation = 1.0f / (1.0f + PointLights[i].Attenuation * distance * distance);
        NdotL = max(0.0f, dot(Input.Normal, pointLightDir));
        reflectDir = reflect(-pointLightDir, Input.Normal);
        spec = pow(max(0.0f, dot(viewDir, reflectDir)), 32.0f);
        
        finalColor += PointLights[i].Color * PointLights[i].Intensity * (NdotL + spec) * attenuation;
    }
    
    // Spot Lights
    for (int j = 0; j < NUM_SPOT_LIGHT; j++)
    {
        float3 spotLightDir = SpotLights[j].Position.xyz - Input.WorldPos;
        float distance = length(spotLightDir);
        spotLightDir = normalize(spotLightDir);
        
        float attenuation = 1.0f / (1.0f + SpotLights[j].Attenuation * distance * distance);
        NdotL = max(0.0f, dot(Input.Normal, spotLightDir));
        reflectDir = reflect(-spotLightDir, Input.Normal);
        spec = pow(max(0.0f, dot(viewDir, reflectDir)), 32.0f);
        
        float3 spotDir = normalize(-SpotLights[j].Direction.xyz);
        float spotFactor = dot(spotLightDir, spotDir);
        float spotLightFactor = smoothstep(cos(SpotLights[j].OuterAngle), cos(SpotLights[j].InnerAngle), spotFactor);
        
        finalColor += SpotLights[j].Color * SpotLights[j].Intensity * (NdotL + spec) * attenuation * spotLightFactor;
    }
    
    finalPixel = finalColor;
#endif
    
    return finalPixel;
}