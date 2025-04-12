#include "LightComponent.h"
#include "Components/UBillboardComponent.h"
#include "Math/JungleMath.h"
#include "Renderer/Renderer.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

ULightComponentBase::ULightComponentBase()
    : Intensity(1.0f)
    , LightColor(FLinearColor::White())
    , bVisible(true)
    , Texture2D(nullptr)
{
    // FString name = "SpotLight";
    // SetName(name);
    InitializeLight();
}

ULightComponentBase::~ULightComponentBase()
{
    delete Texture2D;
}

void ULightComponentBase::SetColor(FLinearColor InColor)
{
    LightColor = InColor;
}

FLinearColor ULightComponentBase::GetColor() const
{
    return LightColor;
}

void ULightComponentBase::InitializeLight()
{
    Texture2D = new UBillboardComponent();
    Texture2D->SetTexture(L"Assets/Texture/spotLight.png");
    Texture2D->InitializeComponent();
    LightColor = { 1,1,1,1 };
}

void ULightComponentBase::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    Texture2D->TickComponent(DeltaTime);
    Texture2D->SetLocation(GetWorldLocation());
}

void ULightComponentBase::SetIntensity(float InIntensity)
{
    Intensity = InIntensity;
}

float ULightComponentBase::GetIntensity() const
{
    return Intensity;
}

void ULightComponentBase::SetVisible(bool InbVisible)
{
    bVisible = InbVisible;
}

bool ULightComponentBase::IsVisible() const
{
    return bVisible;
}

// ULightComponent
ULightComponent::ULightComponent()
{
}

ULightComponent::~ULightComponent()
{
}

// UAmbientLightComponent
UAmbientLightComponent::UAmbientLightComponent()
{
}

UAmbientLightComponent::~UAmbientLightComponent()
{
}

// UDirectionalLightComponent
UDirectionalLightComponent::UDirectionalLightComponent()
{
}

UDirectionalLightComponent::~UDirectionalLightComponent()
{
}

// UPointLightComponent
UPointLightComponent::UPointLightComponent()
    : AttenuationRadius(1000.0f)
    , LightFalloffExponent(2.0f)
{
}

UPointLightComponent::UPointLightComponent(const UPointLightComponent& Other)
{
    AttenuationRadius = Other.AttenuationRadius;
    LightFalloffExponent = Other.LightFalloffExponent;
    LightColor = Other.LightColor;
    Intensity = Other.Intensity;
    bVisible = Other.bVisible;
    Texture2D = new UBillboardComponent(*Other.Texture2D);
    Texture2D->SetTexture(L"Assets/Texture/pointLight.png");
    Texture2D->InitializeComponent();
}

UPointLightComponent::~UPointLightComponent()
{
}

void UPointLightComponent::SetAttenuationRadius(float Radius)
{
    AttenuationRadius = Radius;
}

float UPointLightComponent::GetAttenuationRadius() const
{
    return AttenuationRadius;
}

void UPointLightComponent::SetLightFalloffExponent(float Exponent)
{
    LightFalloffExponent = Exponent;
}

float UPointLightComponent::GetLightFalloffExponent() const
{
    return LightFalloffExponent;
}