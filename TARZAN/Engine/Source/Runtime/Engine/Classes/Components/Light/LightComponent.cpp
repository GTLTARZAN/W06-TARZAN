#include "LightComponent.h"
#include "Components/UBillboardComponent.h"
#include "Math/JungleMath.h"
#include "Renderer/Renderer.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
//#include "Engine/Source/Runtime/Engine/Classes/Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Source/Runtime/Launch/EditorEngine.h"

ULightComponentBase::ULightComponentBase()
    : Intensity(1.0f)
    , LightColor(FLinearColor::White())
    , bVisible(true)
    , Texture2D(nullptr)
{
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

// UAmbientLightComponent
UAmbientLightComponent::UAmbientLightComponent()
{
    FLinearColor AmbientColor = FLinearColor(0.1, 0.1, 0.1, 1);
    SetColor(AmbientColor);
}

UAmbientLightComponent::~UAmbientLightComponent()
{
}

void UAmbientLightComponent::InitializeComponent()
{
}

// UDirectionalLightComponent
UDirectionalLightComponent::UDirectionalLightComponent()
{
    SetRotation(FVector(0.f, 45.f, 0.f));
    Direction = GetForwardVector();
}

UDirectionalLightComponent::~UDirectionalLightComponent()
{
}

void UDirectionalLightComponent::InitializeComponent()
{
}

// UPointLightComponent
UPointLightComponent::UPointLightComponent()
    : Radius(10.0f)
    , LightFalloffExponent(2.0f)
{
}

UPointLightComponent::UPointLightComponent(const UPointLightComponent& Other)
{
    Radius = Other.Radius;
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

void UPointLightComponent::InitializeComponent()
{

}

void UPointLightComponent::SetRadius(float InRadius)
{
    Radius = InRadius;
}

float UPointLightComponent::GetRadius() const
{
    return Radius;
}

void UPointLightComponent::SetLightFalloffExponent(float Exponent)
{
    LightFalloffExponent = Exponent;
}

float UPointLightComponent::GetLightFalloffExponent() const
{
    return LightFalloffExponent;
}