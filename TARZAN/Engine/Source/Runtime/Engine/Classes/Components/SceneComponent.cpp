#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Math/JungleMath.h"
#include "UObject/ObjectFactory.h"
#include "UTextUUID.h"
USceneComponent::USceneComponent() :RelativeLocation(FVector(0.f, 0.f, 0.f)), RelativeRotation(FVector(0.f, 0.f, 0.f)), RelativeScale3D(FVector(1.f, 1.f, 1.f))
{
}
USceneComponent::USceneComponent(const USceneComponent& Other)
    : UActorComponent(Other),
      AttachParent(nullptr), // 복제 시 복원
      RelativeLocation(Other.RelativeLocation),
      RelativeRotation(Other.RelativeRotation),
      QuatRotation(Other.QuatRotation),
      RelativeScale3D(Other.RelativeScale3D)
{
}
USceneComponent::~USceneComponent()
{
	
}
void USceneComponent::InitializeComponent()
{
    Super::InitializeComponent();

}

void USceneComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
}


int USceneComponent::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    int nIntersections = 0;
    return nIntersections;
}

FVector USceneComponent::GetForwardVector()
{
	FVector Forward = FVector(1.f, 0.f, 0.0f);
	Forward = JungleMath::FVectorRotate(Forward, QuatRotation);
	return Forward;
}

FVector USceneComponent::GetRightVector()
{
	FVector Right = FVector(0.f, 1.f, 0.0f);
	Right = JungleMath::FVectorRotate(Right, QuatRotation);
	return Right;
}

FVector USceneComponent::GetUpVector()
{
	FVector Up = FVector(0.f, 0.f, 1.0f);
	Up = JungleMath::FVectorRotate(Up, QuatRotation);
	return Up;
}


void USceneComponent::AddLocation(FVector _added)
{
	RelativeLocation = RelativeLocation + _added;

}

void USceneComponent::AddRotation(FVector _added)
{
	RelativeRotation = RelativeRotation + _added;
    
    QuatRotation = JungleMath::EulerToQuaternion(RelativeRotation);

}

void USceneComponent::AddScale(FVector _added)
{
	RelativeScale3D = RelativeScale3D + _added;

}

FVector USceneComponent::GetWorldRotation()
{
	if (AttachParent)
	{
		return FVector(AttachParent->GetLocalRotation() + GetLocalRotation());
	}
	else
		return GetLocalRotation();
}

FVector USceneComponent::GetWorldScale()
{
	if (AttachParent)
	{
		return FVector(AttachParent->GetWorldScale() + GetLocalScale());
	}
	else
		return GetLocalScale();
}

FVector USceneComponent::GetWorldLocation()
{
	if (AttachParent)
	{
		return FVector(AttachParent->GetWorldLocation() + GetLocalLocation());
	}
	else
		return GetLocalLocation();
}

FVector USceneComponent::GetLocalRotation()
{
	return JungleMath::QuaternionToEuler(QuatRotation);
}

void USceneComponent::SetRotation(FVector _newRot)
{
	RelativeRotation = _newRot;
	QuatRotation = JungleMath::EulerToQuaternion(_newRot);
}

void USceneComponent::SetupAttachment(USceneComponent* InParent)
{
    if (
        InParent != AttachParent                                  // 설정하려는 Parent가 기존의 Parent와 다르거나
        && InParent != this                                       // InParent가 본인이 아니고
        && InParent != nullptr                                    // InParent가 유효한 포인터 이며
        && (
            AttachParent == nullptr                               // AttachParent도 유효하며
            || !AttachParent->AttachChildren.Contains(this)  // 이미 AttachParent의 자식이 아닌 경우
        ) 
    ) {
        SetAttachParent(InParent);
        InParent->AttachChildren.AddUnique(this);
    }
}


USceneComponent* USceneComponent::GetAttachParent() const
{
    return AttachParent;
}

void USceneComponent::SetAttachParent(USceneComponent* InParent)
{
    AttachParent = InParent;
}

UObject* USceneComponent::Duplicate() const
{
    USceneComponent* NewComp = FObjectFactory::ConstructObjectFrom<USceneComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void USceneComponent::DuplicateSubObjects(const UObject* Source)
{
    UActorComponent::DuplicateSubObjects(Source);
    // AttachParent는 AActor::DuplicateSubObjects에서 복원
}

void USceneComponent::PostDuplicate() {}

void USceneComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    // --- 이제 자신의 속성을 동일한 OutProperties 맵에 추가 ---
    // 1. AttachParentID 저장
    USceneComponent* ParentComp = GetAttachParent();
    if (ParentComp != nullptr) {
        OutProperties.Add(TEXT("AttachParentID"), ParentComp->GetName());
    }
    else
    {
        OutProperties.Add(TEXT("AttachParentID"), "nullptr");
    }

    // 2. RelativeTransform 저장
    //FTransform RelativeTransform = GetRelativeTransform();
    // ... Location, Rotation, Scale 문자열로 변환 ...
    OutProperties.Add(TEXT("RelativeLocation"), RelativeLocation.ToString());
    OutProperties.Add(TEXT("RelativeRotation"), RelativeRotation.ToString());
    OutProperties.Add(TEXT("RelativeScale"), RelativeScale3D.ToString());
    OutProperties.Add(TEXT("QuatRotation"), QuatRotation.ToString());
    
    
    return;
}

void USceneComponent::SetProperties(const TMap<FString, FString>& Properties)
{
    Super::SetProperties(Properties);

    // const FString* TempStr = nullptr;
    //
    // // --- SceneComponent 자체의 상태 복원 ---
    //
    // // 예시: 가시성(Visibility) 상태 복원
    // // (GetProperties에서 "bVisible" 키로 저장했다고 가정)
    // TempStr = Properties.Find(TEXT("bVisible"));
    // if (TempStr)
    // {
    //     // SetVisibility 함수는 bVisible 멤버 변수 설정 및
    //     // 렌더링 상태 업데이트 등의 추가 작업을 수행할 수 있음.
    //     //SetVisibility(TempStr->ToBool()); // FString::ToBool() 사용 가정
    //     
    // }
}
