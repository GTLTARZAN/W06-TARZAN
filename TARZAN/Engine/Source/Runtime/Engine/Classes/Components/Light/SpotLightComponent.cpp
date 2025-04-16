#include "SpotLightComponent.h"
#include "UObject/ObjectFactory.h"
#include "Components/UBillboardComponent.h"
#include "GameFramework/Actor.h"
#include "Define.h"

USpotLightComponent::USpotLightComponent()
{
    InnerConeAngle = 15.0f;
    OuterConeAngle = 30.0f;
}

USpotLightComponent::USpotLightComponent(const USpotLightComponent& Other)
    : UPointLightComponent(Other)
{
    InnerConeAngle = Other.InnerConeAngle;
    OuterConeAngle = Other.OuterConeAngle;
}

USpotLightComponent::~USpotLightComponent()
{
}

void USpotLightComponent::InitializeComponent()
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

void USpotLightComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

void USpotLightComponent::PostDuplicate()
{
    //Super::PostDuplicate();
}

void USpotLightComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);

    OutProperties.Add(TEXT("InnerConeAngle"), FString::Printf(TEXT("%f"), InnerConeAngle));
    OutProperties.Add(TEXT("OuterConeAngle"), FString::Printf(TEXT("%f"), OuterConeAngle));
}

void USpotLightComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);

    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("InnerConeAngle"));
    if (TempStr)
    {
        InnerConeAngle = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("OuterConeAngle"));
    if (TempStr)
    {
        OuterConeAngle = FString::ToFloat(*TempStr);
    }
}

void USpotLightComponent::SetBoundingBox(const FBoundingBox& InAABB)
{
    AABB = InAABB;
}

void USpotLightComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

UObject* USpotLightComponent::Duplicate() const
{
    USpotLightComponent* NewComp = FObjectFactory::ConstructObjectFrom<USpotLightComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}