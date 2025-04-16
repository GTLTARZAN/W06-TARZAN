#define Max_Fireball 300

Texture2D g_GBufferNormal : register(t0);
Texture2D g_GBufferAlbedo : register(t1);
Texture2D g_GBufferAmbient : register(t2);
Texture2D g_GBufferPosition : register(t3);

SamplerState g_sampler : register(s0);

struct FLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Emissive;
    float Padding1;
    float3 Direction;
    float Padding2;
};

struct FireballConstants
{
    float3 FireballPosition;
    float FireballIndensity;
    float radius;
    float RadiusFallOff;
    float InnerAngle;
    float OuterAngle;
    float4 FireballColor; 
    float3 Direction;
    int LightType; // 0: Point, 1: Spot
};

//struct FMaterial
//{
//    float4 Diffuse;
//    float4 Ambient;
//    float4 Specular;
//    float3 Emissive;
//    float Roughness;
//};

cbuffer GlobalLightConstants : register(b0)
{
    FLight GlobalLight;
    float3 CameraPosition;
    float ViewMode;
};

cbuffer FireballBuffer : register(b1)
{
    FireballConstants Fireball[Max_Fireball];
    int FierballCount;
    float3 padding;
}

cbuffer ScreenInfo : register(b2)
{
    float2 ViewPortRatio;
    float2 ViewPortPosition;
}

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    float4 WorldPos : SV_Target1;
};

float4 ComputeDirectionalLight(float3 normal, float3 worldPosition, float3 albedo, float3 ambient)
{
    // 각 광원 요소에 대한 색상을 초기화
    float4 ambientColor = 0;
    float4 diffuseColor = 0;
    float4 specularColor = 0;
    float4 emissiveColor = 0;
    
    float3 lightDir = normalize(-GlobalLight.Direction);
    normal = normalize(normal);
    
    // === Ambient ===
    ambientColor = float4(albedo, 1.0f) * GlobalLight.Ambient * float4(ambient, 1.0f);
    
    // === Diffuse ===
    float NdotL = saturate(dot(normal, lightDir));
    //float NdotL = dot(normal, lightDir);
    //float diffuseBoost = lerp(1.0f, 1.3f, 1.0f - Material.Roughness);
    diffuseColor = float4(albedo, 1.0f) * GlobalLight.Diffuse * NdotL /** diffuseBoost*/;
    
    return ambientColor + diffuseColor;
    
    //// === Specular ===
    //float3 viewDir = normalize(CameraPosition - worldPosition);
    //float3 reflectDir = reflect(-lightDir, normal);
    //float specFactor = saturate(dot(reflectDir, viewDir));
    ////float shininess = lerp(64.0f, 1.0f, 1.0f - saturate(Material.Roughness));
    //float shininess = lerp(64.0f, 1.0f, 1.0f - Material.Roughness);
    //float specular = pow(specFactor, shininess);
    //specular *= lerp(1.0f, 0.1f, pow(1.0f - Material.Roughness, 2.0f));
    //specularColor = GlobalLight.Specular * Material.Specular * specular;
    
    //// === Emissive ===
    //float viewDot = saturate(dot(viewDir, normal));
    //float emissive = pow(smoothstep(0.0f, 1.0f, 1.0f - viewDot), 2.0f);
    //emissiveColor = float4(GlobalLight.Emissive * Material.Emissive * emissive, 1.0f);

    //return ambientColor + diffuseColor + specularColor + emissiveColor;
}

float3 ComputeFireballLighting(float4 worldPos, float3 normal)
{
    float3 N = normalize(normal);
    float3 V = float3(0, 0, 1);
    float3 totalLighting = float3(0, 0, 0);

    for (int i = 0; i < FierballCount; ++i)
    {
        float3 L = normalize(Fireball[i].FireballPosition - worldPos.xyz);
        float3 H = normalize(L + V);

        float dist = length(Fireball[i].FireballPosition - worldPos.xyz);
        float attenuation = 1.0 / (1.0 + Fireball[i].RadiusFallOff * (dist / Fireball[i].radius) * (dist / Fireball[i].radius));
        attenuation *= Fireball[i].FireballIndensity;


        float spotAttenuation = 1.0f;
        if (Fireball[i].LightType == 1) // Spot light
        {
            float3 spotDir = normalize(Fireball[i].Direction);
            float spotCos = dot(-L, spotDir);
            float innerCos = cos(radians(Fireball[i].InnerAngle/2));
            float outerCos = cos(radians(Fireball[i].OuterAngle/2));
            spotAttenuation = saturate((spotCos - outerCos) / (innerCos - outerCos));
        }

        float finalAttenuation = attenuation * spotAttenuation;

        float diffuse = saturate(dot(N, L));
        float3 diffuseColor = diffuse * Fireball[i].FireballColor.rgb * finalAttenuation;

        totalLighting += diffuseColor;
    }

    return totalLighting;
}


PS_OUTPUT main(PS_Input input)
{
    PS_OUTPUT output;
    
    float2 uv = input.TexCoord * ViewPortRatio + ViewPortPosition;
    //output.Color = float4(uv.x, uv.y, 0, 1);
    //return output;
    
    float4 normalTex = g_GBufferNormal.Sample(g_sampler, uv);
    float4 albedoTex = g_GBufferAlbedo.Sample(g_sampler, uv);
    float3 ambient = g_GBufferAmbient.Sample(g_sampler, uv).rgb;
    float4 worldPosTex = g_GBufferPosition.Sample(g_sampler, uv);
    
    float3 albedo = albedoTex.rgb;
    float isValidObject = albedoTex.a;
    
    float3 worldPos = worldPosTex.xyz;
    float depth = worldPosTex.w;
        
    float3 normal = (normalTex.xyz - 0.5f) * 2.0f;
    
    float4 DirectionLightColor;
    float3 PointLightColor;
    
    if (isValidObject == 0.5f) // 배경 구분
    {
        output.WorldPos = float4(worldPos.xyz, 0.5f);
        switch (ViewMode)
        {
            case 0 : // Lit 모드: 일반 조명 계산 적용
                DirectionLightColor = ComputeDirectionalLight(normal, worldPos, albedo, ambient);
                PointLightColor = ComputeFireballLighting(float4(worldPos, 1), normal);
                output.Color = DirectionLightColor + float4(PointLightColor.xyz, 1);
                break;
            case 1:
            case 2: // Unlit 모드, WireFrame 모드: 조명 계산 생략, 재질 기본 색상 사용
                output.Color = float4(albedo.xyz, 1.f); // 혹은 필요한 처리를 적용
                break;
            case 3: // Base Color 모드: 알베도 텍스처 또는 재질의 베이스 컬러 출력
                output.Color = float4(albedo.xyz, 1.f);
                break;
            case 4: // Normal 모드
                output.Color = float4(normal, 1.f);
                break;
            case 5: // Depth 모드
                output.Color = float4(depth, depth, depth, 1.f);
                break;
            case 6: // World Position 모드
                output.Color = float4(normalize(worldPos), 1.f);
                break;
            default: // 정의되지 않은 모드의 경우, 기본값 처리
                output.Color = float4(1, 0, 1, 1); // 에러나 예외 처리를 위한 마젠타 색상 사용
                break;
        }
    }
    else if (isValidObject == 0.7f)
    {
        output.Color = float4(albedo.xyz, 1.f);
        output.WorldPos = float4(worldPos.xyz, 0.5);

    }
    else
    {
        output.Color = float4(albedo.xyz, 1.f);
        output.WorldPos = float4(worldPos.xyz, 1.f);
    }
       
    return output;
}
