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
    //delete Texture2D;
}

void ULightComponentBase::InitializeLight()
{
    Texture2D = new UBillboardComponent();
    Texture2D->SetTexture(L"Assets/Texture/spotLight.png");
    Texture2D->InitializeComponent();
}

void ULightComponentBase::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    // 자신의 속성을 동일한 OutProperties에 추가

    USceneComponent* ParentComp = GetAttachParent();
    if (ParentComp != nullptr) {
        OutProperties.Add(TEXT("AttachParentID"), ParentComp->GetName());
    }
    else
    {
        OutProperties.Add(TEXT("AttachParentID"), "nullptr");
    }

    OutProperties.Add(TEXT("Intensity"), FString::Printf(TEXT("%f"), Intensity));
    OutProperties.Add(TEXT("LightColor"), LightColor.ToString());
    OutProperties.Add(TEXT("bVisible"), bVisible ? TEXT("true") : TEXT("false"));
    //if (Texture2D && Texture2D->TexturePath.size() > 0)
    //{
    //    OutProperties.Add(TEXT("IconPath"), Texture2D->TexturePath.c_str());
    //}
}

void ULightComponentBase::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("Intensity"));
    if (TempStr)
    {
        Intensity = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("LightColor"));
    if (TempStr)
    {
        LightColor.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("bVisible"));
    if (TempStr)
    {
        bVisible = TempStr->ToBool();
    }
    //TempStr = InProperties.Find(TEXT("IconPath"));
    //if (TempStr && GetOwner())
    //{
    //    if (!Texture2D)
    //    {
    //        UBillboardComponent* Billboard = GetOwner()->AddComponent<UBillboardComponent>();
    //    }
    //    else
    //    {
    //        Texture2D->SetTexture(TempStr->ToWideString());
    //    }
    //}
}

void ULightComponentBase::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    Texture2D->TickComponent(DeltaTime);
    Texture2D->SetLocation(GetWorldLocation());
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
    Super::InitializeComponent();
    AActor* OwnerActor = GetOwner();

    if (!Texture2D && OwnerActor)
    {
        UBillboardComponent* Billboard = OwnerActor->AddComponent<UBillboardComponent>("Icon");
        
        Billboard->SetTexture(GetDefaultIconPath());
        Billboard->SetIsLightIcon(true);

        SetTexture2D(Billboard);
    }
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

void UPointLightComponent::InitializeComponent()
{
    Super::InitializeComponent();
    AActor* OwnerActor = GetOwner();

    if (!Texture2D && OwnerActor)
    {
        UBillboardComponent* Billboard = OwnerActor->AddComponent<UBillboardComponent>("Icon");
        Billboard->SetTexture(GetDefaultIconPath());
        Billboard->SetIsLightIcon(true);
        SetTexture2D(Billboard);
    }
}

void UPointLightComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("Radius"), FString::Printf(TEXT("%f"), Radius));
    OutProperties.Add(TEXT("LightFalloffExponent"), FString::Printf(TEXT("%f"), LightFalloffExponent));
}

void UPointLightComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("Radius"));
    if (TempStr) Radius = FString::ToFloat(*TempStr);

    TempStr = InProperties.Find(TEXT("LightFalloffExponent"));
    if (TempStr) LightFalloffExponent = FString::ToFloat(*TempStr);
}
