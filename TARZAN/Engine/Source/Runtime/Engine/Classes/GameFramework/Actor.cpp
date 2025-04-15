#include "Actor.h"

#include "Engine/World.h"

AActor::AActor(const AActor& Other)
    : UObject(Other),
      RootComponent(nullptr),
      bTickInEditor(Other.bTickInEditor),
      bActorIsBeingDestroyed(Other.bActorIsBeingDestroyed),
      ActorLabel(Other.ActorLabel),
    OwnedComponents(Other.OwnedComponents)
{
    OwnedComponents.Empty();
}
void AActor::BeginPlay()
{
    // TODO: 나중에 삭제를 Pending으로 하던가 해서 복사비용 줄이기
    const auto CopyComponents = OwnedComponents;
    for (UActorComponent* Comp : CopyComponents)
    {
        Comp->BeginPlay();
    }
}

void AActor::Tick(float DeltaTime)
{
    if (!RootComponent) return;
    // TODO: 임시로 Actor에서 Tick 돌리기
    // TODO: 나중에 삭제를 Pending으로 하던가 해서 복사비용 줄이기
    const auto CopyComponents = OwnedComponents;
    for (UActorComponent* Comp : CopyComponents)
    {
        if (Comp && Comp->IsComponentTickEnabled())
            Comp->TickComponent(DeltaTime);
    }
    // SetActorLocation(GetActorLocation() + FVector(1.0f, 0.0f, 0.0f));
}

void AActor::Destroyed()
{
    // Actor가 제거되었을 때 호출하는 EndPlay
    EndPlay(EEndPlayReason::Destroyed);
}

void AActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 본인이 소유하고 있는 모든 컴포넌트의 EndPlay 호출
    for (UActorComponent* Component : GetComponents())
    {
        if (Component->HasBegunPlay())
        {
            Component->EndPlay(EndPlayReason);
        }
    }
    UninitializeComponents();
}


bool AActor::Destroy()
{
    if (!IsActorBeingDestroyed())
    {
        if (UWorld* World = GetWorld())
        {
            World->DestroyActor(this);
            bActorIsBeingDestroyed = true;
        }
    }

    return IsActorBeingDestroyed();
}

void AActor::RemoveOwnedComponent(UActorComponent* Component)
{
    OwnedComponents.Remove(Component);
}

void AActor::InitializeComponents()
{
    TSet<UActorComponent*> Components = GetComponents();
    for (UActorComponent* ActorComp : Components)
    {
        // 먼저 컴포넌트를 등록 처리
        if (!ActorComp->IsRegistered())
        {
            ActorComp->RegisterComponent();
        }

        if (ActorComp->bAutoActive && !ActorComp->IsActive())
        {
            ActorComp->Activate();
        }

        if (!ActorComp->HasBeenInitialized())
        {
            ActorComp->InitializeComponent();
        }
    }
}

void AActor::UninitializeComponents()
{
    TSet<UActorComponent*> Components = GetComponents();
    for (UActorComponent* ActorComp : Components)
    {
        if (ActorComp->HasBeenInitialized())
        {
            ActorComp->UninitializeComponent();
        }
        if (ActorComp->IsRegistered())
        {
            ActorComp->UnregisterComponent();
        }
    }
}

bool AActor::SetRootComponent(USceneComponent* NewRootComponent)
{
    if (NewRootComponent == nullptr || NewRootComponent->GetOwner() == this)
    {
        if (RootComponent != NewRootComponent)
        {
            USceneComponent* OldRootComponent = RootComponent;
            RootComponent = NewRootComponent;


            if (OldRootComponent != nullptr) // OldRootComponent가 유효한 경우에만 처리
            {
                // SetupAttachment는 보통 내부적으로 필요시 Detach를 처리함.
                OldRootComponent->SetupAttachment(RootComponent);
            }

        }
        return true;
    }
    return false;
}

bool AActor::SetActorLocation(const FVector& NewLocation)
{
    if (RootComponent)
    {
        RootComponent->SetLocation(NewLocation);
        return true;
    }
    return false;
}

bool AActor::SetActorRotation(const FVector& NewRotation)
{
    if (RootComponent)
    {
        RootComponent->SetRotation(NewRotation);
        return true;
    }
    return false;
}

bool AActor::SetActorScale(const FVector& NewScale)
{
    if (RootComponent)
    {
        RootComponent->SetScale(NewScale);
        return true;
    }
    return false;
}

UActorComponent* AActor::AddComponent(UClass* ComponentClass, FName Name)
{

    if (!ComponentClass)
    {
        UE_LOG(LogLevel::Error, TEXT("UActorComponent failed: ComponentClass is null."));
        return nullptr;
    }

    // 스폰하려는 클래스가 AActor에서 파생되었는지 확인
    if (!ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        UE_LOG(LogLevel::Error, TEXT("UActorComponent failed: Class '%s' is not derived from AActor."), *ComponentClass->GetName());
        return nullptr;
    }

    // 액터 이름 결정 (SpawnParams 또는 자동 생성)
    FName ActorName = Name;
    // TODO: SpawnParams에서 이름 가져오거나, 필요시 여기서 자동 생성
    // if (SpawnParams.Name != NAME_None) ActorName = SpawnParams.Name;

    // FObjectFactory를 사용하여 객체 생성 시도 
    UObject* NewUObject = FObjectFactory::ConstructObjectFromClass(ComponentClass,this, Name);

    // 생성된 객체를 AActor*로 캐스팅
    UActorComponent* Component = Cast<UActorComponent>(NewUObject); // Cast<T>(Obj) 함수 구현 필요

    if (!Component)
    {
        UE_LOG(LogLevel::Error, TEXT("UActorComponent failed: Class '%s' is not derived from AActor."), *ComponentClass->GetName());
        return nullptr;
    }
    
    OwnedComponents.Add(Component);
    Component->Owner = this;

    // 만약 SceneComponent를 상속 받았다면
    if (USceneComponent* NewSceneComp = Cast<USceneComponent>(Component))
    {
        if (RootComponent == nullptr)
        {
            RootComponent = NewSceneComp;
        }
        else
        {
            NewSceneComp->SetupAttachment(RootComponent);
        }
    }

    // TODO: RegisterComponent() 생기면 제거
    Component->InitializeComponent();

    return Component;
}

// AActor.cpp
void AActor::AddComponent(UActorComponent* Component)
{
    OwnedComponents.Add(Component);
    Component->Owner = this;

    if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
    {
        if (RootComponent == nullptr)
        {
            RootComponent = SceneComp;
        }
        else
        {
            SceneComp->SetupAttachment(RootComponent);
        }
    }

    Component->InitializeComponent();
}


UObject* AActor::Duplicate() const
{
    AActor* ClonedActor = FObjectFactory::ConstructObjectFrom<AActor>(this);
    ClonedActor->DuplicateSubObjects(this);
    ClonedActor->PostDuplicate();
    return ClonedActor;
}
void AActor::DuplicateSubObjects(const UObject* SourceObj)
{
    UObject::DuplicateSubObjects(SourceObj);

    const AActor* Source = Cast<AActor>(SourceObj);
    if (!Source) return;

    TMap<const USceneComponent*, USceneComponent*> SceneCloneMap;

    for (UActorComponent* Component : Source->OwnedComponents)
    {
        UActorComponent* dupComponent = static_cast<UActorComponent*>(Component->Duplicate());
        dupComponent->bHasBeenInitialized = false;
        dupComponent->Owner = this;
        OwnedComponents.Add(dupComponent);
        RootComponent = Cast<USceneComponent>(dupComponent);
        if (const USceneComponent* OldScene = Cast<USceneComponent>(Component))
        {
            if (USceneComponent* NewScene = Cast<USceneComponent>(dupComponent))
            {
                SceneCloneMap.Add(OldScene, NewScene);
            }
        }
    }

    for (const auto& Pair : SceneCloneMap)
    {
        const USceneComponent* Old = Pair.Key;
        USceneComponent* New = Pair.Value;
        if (const USceneComponent* OldParent = Old->GetAttachParent())
        {
            if (USceneComponent** Found = SceneCloneMap.Find(OldParent))
            {
                USceneComponent* NewParent = *Found;
                New->SetupAttachment(NewParent);
            }
        }
    }

    if (Source->RootComponent)
    {
        if (USceneComponent** Found = SceneCloneMap.Find(Source->RootComponent))
        {
            SetRootComponent(*Found);
        }
    }
}

void AActor::PostDuplicate()
{
    // Override in subclasses if needed
}