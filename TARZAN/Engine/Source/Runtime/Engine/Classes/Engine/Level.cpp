#include "Level.h"
#include "GameFramework/Actor.h"
#include "Components/Light/LightComponent.h"
#include "Components/UBillboardComponent.h"
#include "Engine/Classes/Engine/World.h"

ULevel::ULevel()
{
    //SpawnDefaultActors();
}

ULevel::~ULevel()
{
}

UObject* ULevel::Duplicate() const
{
    ULevel* CloneLevel = FObjectFactory::ConstructObjectFrom<ULevel>(this);
    CloneLevel->DuplicateSubObjects(this);
    CloneLevel->PostDuplicate();
    return CloneLevel;
}

void ULevel::DuplicateSubObjects(const UObject* SourceObj)
{
    UObject::DuplicateSubObjects(SourceObj);
    for (AActor* Actor : Cast<ULevel>(SourceObj)->GetActors())
    {
        AActor* dupActor = static_cast<AActor*>(Actor->Duplicate());
        Actors.Add(dupActor);
    }
}

void ULevel::PostDuplicate()
{
    UObject::PostDuplicate();
}

void ULevel::SpawnDefaultActors()
{
    AActor* DefaultLight = OwnerWorld->SpawnActor<AActor>();
    DefaultLight->SetActorLabel(TEXT("DirectionalLight"));

    UDirectionalLightComponent* LightComp = DefaultLight->AddComponent<UDirectionalLightComponent>();
    DefaultLight->AddComponent<UAmbientLightComponent>();
    DefaultLight->SetActorLocation(FVector(0, 0, 10));
}
