#include "World.h"

#include "Actors/Player.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Camera/CameraComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Engine/FLoaderOBJ.h"
#include "Classes/Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/SkySphereComponent.h"
#include "UnrealEd/SceneMgr.h"
#include "UObject/UObjectIterator.h"
#include "Level.h"
#include "Components/Light/LightComponent.h"
#include "Components/UBillboardComponent.h"


UWorld::UWorld(const UWorld& Other): UObject(Other)
                                   , defaultMapName(Other.defaultMapName)
                                   , Level(Other.Level)
                                   , WorldType(Other.WorldType)
                                    , EditorPlayer(Other.EditorPlayer)
{
}

void UWorld::InitWorld()
{
    // TODO: Load Scene
    CreateBaseObject();
    Level = FObjectFactory::ConstructObject<ULevel>(this);
    Level->OwnerWorld = this;
    Level->SpawnDefaultActors();
}

void UWorld::CreateBaseObject()
{
    if (EditorPlayer == nullptr)
    {
        EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
    }
    
    if (LocalGizmo == nullptr)
    {
        LocalGizmo = FObjectFactory::ConstructObject<UTransformGizmo>(this);
    }
}

void UWorld::ReleaseBaseObject()
{
    if (LocalGizmo)
    {
        //delete LocalGizmo;  액터라 DestroyActor에서 처리됨
        LocalGizmo = nullptr;
    }
    
    if (EditorPlayer)
    {
        //delete EditorPlayer;
        EditorPlayer = nullptr;
    }

    SelectedActor = nullptr;
    pickingGizmo = nullptr;
    

}

void UWorld::Tick(ELevelTick tickType, float deltaSeconds)
{
    if (tickType == LEVELTICK_ViewportsOnly)
    {
        if (EditorPlayer)
            EditorPlayer->Tick(deltaSeconds);
        if (LocalGizmo)
            LocalGizmo->Tick(deltaSeconds);
    }
    // SpawnActor()에 의해 Actor가 생성된 경우, 여기서 BeginPlay 호출
    if (tickType == LEVELTICK_All)
    {
        for (AActor* Actor : Level->PendingBeginPlayActors)
        {
            Actor->BeginPlay();
        }
        Level->PendingBeginPlayActors.Empty();

        // 매 틱마다 Actor->Tick(...) 호출
        for (AActor* Actor : Level->GetActors())
        {
            Actor->Tick(deltaSeconds);
        }
    }
}

void UWorld::Release()
{
	for (AActor* Actor : Level->GetActors())
	{
		Actor->EndPlay(EEndPlayReason::WorldTransition);
        TSet<UActorComponent*> Components = Actor->GetComponents();
	    for (UActorComponent* Component : Components)
	    {
	        GUObjectArray.MarkRemoveObject(Component);
	    }
	    GUObjectArray.MarkRemoveObject(Actor);
	}
    Level->GetActors().Empty();

	pickingGizmo = nullptr;
	ReleaseBaseObject();

    GUObjectArray.ProcessPendingDestroyObjects();
}

void UWorld::ClearScene()
{
    // 1. 모든 Actor Destroy

    //TObjectRange<AActor>();
    
    for (AActor* actor : TObjectRange<AActor>())
    {
        DestroyActor(actor);
    }
    ReleaseBaseObject();
}

UObject* UWorld::Duplicate() const
{
    UWorld* CloneWorld = FObjectFactory::ConstructObjectFrom<UWorld>(this);
    CloneWorld->DuplicateSubObjects(this);
    CloneWorld->PostDuplicate();
    return CloneWorld;
}

void UWorld::DuplicateSubObjects(const UObject* SourceObj)
{
    UObject::DuplicateSubObjects(SourceObj);
    Level = Cast<ULevel>(Level->Duplicate());
    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
}

void UWorld::PostDuplicate()
{
    UObject::PostDuplicate();
}

void UWorld::ReloadScene(const FString& FileName)
{

    ClearScene(); // 기존 오브젝트 제거
    CreateBaseObject();
    
    GEngine->GetSceneManager()->LoadSceneFromFile(FileName, *this);

    // if (SceneOctree && SceneOctree->GetRoot())
    //     SceneOctree->GetRoot()->TickBuffers(GCurrentFrame, 0);


    //GEngine->GetSceneManager()->ParseSceneData(NewFile);
}

void UWorld::NewScene()
{
    ClearScene();
    CreateBaseObject();
}




void UWorld::SaveScene(const FString& FileName)
{
    //FSceneData SceneData = GEngine->GetSceneManager()->CreateSceneData(*this);

    GEngine->GetSceneManager()->SaveSceneToFile(FileName, *this);
    int a= 0;
}


AActor* UWorld::SpawnActor(UClass* ActorClass, FName InActorName)
{
    if (!ActorClass)
    {
        UE_LOG(LogLevel::Error, TEXT("SpawnActor failed: ActorClass is null."));
        return nullptr;
    }

    // 스폰하려는 클래스가 AActor에서 파생되었는지 확인
    if (!ActorClass->IsChildOf(AActor::StaticClass()))
    {
        UE_LOG(LogLevel::Error, TEXT("SpawnActor failed: Class '%s' is not derived from AActor."), *ActorClass->GetName());
        return nullptr;
    }

    // 액터 이름 결정 (SpawnParams 또는 자동 생성)
    FName ActorName = InActorName; // 우선 기본값
    // TODO: SpawnParams에서 이름 가져오거나, 필요시 여기서 자동 생성
    // if (SpawnParams.Name != NAME_None) ActorName = SpawnParams.Name;

    // FObjectFactory를 사용하여 객체 생성 시도 (Outer는 this 월드)
    UObject* NewUObject = FObjectFactory::ConstructObjectFromClass(ActorClass, this, ActorName);

    // 생성된 객체를 AActor*로 캐스팅
    AActor* Actor = Cast<AActor>(NewUObject); // Cast<T>(Obj) 함수 구현 필요

    if (Actor)
    {
        Level->GetActors().Add(Actor);
        Level->PendingBeginPlayActors.Add(Actor); // BeginPlay 호출 대기 목록에 추가
    }
    else
    {
        UE_LOG(LogLevel::Error, TEXT("SpawnActor failed: Constructed object '%s' of class '%s' is not an Actor. Destroying."), *NewUObject->GetName(), *ActorClass->GetName());
        return nullptr;
    }
    
    return Actor;
}


bool UWorld::DestroyActor(AActor* ThisActor)
{
    if (ThisActor->GetWorld() == nullptr)
    {
        return false;
    }

    if (ThisActor->IsActorBeingDestroyed())
    {
        return true;
    }

    // 액터의 Destroyed 호출
    ThisActor->Destroyed();

    if (ThisActor->GetOwner())
    {
        ThisActor->SetOwner(nullptr);
    }

    TSet<UActorComponent*> Components = ThisActor->GetComponents();
    for (UActorComponent* Component : Components)
    {
        Component->DestroyComponent();
    }

    // World에서 제거
    Level->GetActors().Remove(ThisActor);

    // 제거 대기열에 추가
    GUObjectArray.MarkRemoveObject(ThisActor);
    return true;
}

void UWorld::SetPickingGizmo(UObject* Object)
{
	pickingGizmo = Cast<USceneComponent>(Object);
}

/*************************임시******************************/
bool UWorld::IsPIEWorld() const
{
    return false;
}

void UWorld::BeginPlay()
{

}

UWorld* UWorld::DuplicateWorldForPIE(UWorld* world)
{
    return new UWorld();
}
/**********************************************************/
