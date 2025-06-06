#include "Components/StaticMeshComponent.h"

#include "Engine/FLoaderOBJ.h"
#include "Engine/World.h"
#include "Launch/EditorEngine.h"
#include "UObject/ObjectFactory.h"
#include "UnrealEd/PrimitiveBatch.h"


UStaticMeshComponent::UStaticMeshComponent(const UStaticMeshComponent& Other)
    : UMeshComponent(Other),
      staticMesh(Other.staticMesh),
      selectedSubMeshIndex(Other.selectedSubMeshIndex)
{
}
uint32 UStaticMeshComponent::GetNumMaterials() const
{
    if (staticMesh == nullptr) return 0;

    return staticMesh->GetMaterials().Num();
}

UMaterial* UStaticMeshComponent::GetMaterial(uint32 ElementIndex) const
{
    if (staticMesh != nullptr)
    {
        if (OverrideMaterials[ElementIndex] != nullptr)
        {
            return OverrideMaterials[ElementIndex];
        }
    
        if (staticMesh->GetMaterials().IsValidIndex(ElementIndex))
        {
            return staticMesh->GetMaterials()[ElementIndex]->Material;
        }
    }
    return nullptr;
}

uint32 UStaticMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
    if (staticMesh == nullptr) return -1;

    return staticMesh->GetMaterialIndex(MaterialSlotName);
}

TArray<FName> UStaticMeshComponent::GetMaterialSlotNames() const
{
    TArray<FName> MaterialNames;
    if (staticMesh == nullptr) return MaterialNames;

    for (const FStaticMaterial* Material : staticMesh->GetMaterials())
    {
        MaterialNames.Emplace(Material->MaterialSlotName);
    }

    return MaterialNames;
}

void UStaticMeshComponent::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    if (staticMesh == nullptr) return;
    staticMesh->GetUsedMaterials(Out);
    for (int materialIndex = 0; materialIndex < GetNumMaterials(); materialIndex++)
    {
        if (OverrideMaterials[materialIndex] != nullptr)
        {
            Out[materialIndex] = OverrideMaterials[materialIndex];
        }
    }
}

int UStaticMeshComponent::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
    if (!AABB.Intersect(rayOrigin, rayDirection, pfNearHitDistance)) return 0;
    int nIntersections = 0;
    if (staticMesh == nullptr) return 0;

    OBJ::FStaticMeshRenderData* renderData = staticMesh->GetRenderData();

    FVertexSimple* vertices = renderData->Vertices.GetData();
    int vCount = renderData->Vertices.Num();
    UINT* indices = renderData->Indices.GetData();
    int iCount = renderData->Indices.Num();

    if (!vertices) return 0;
    BYTE* pbPositions = reinterpret_cast<BYTE*>(renderData->Vertices.GetData());

    int nPrimitives = (!indices) ? (vCount / 3) : (iCount / 3);
    float fNearHitDistance = FLT_MAX;
    for (int i = 0; i < nPrimitives; i++) {
        int idx0, idx1, idx2;
        if (!indices) {
            idx0 = i * 3;
            idx1 = i * 3 + 1;
            idx2 = i * 3 + 2;
        }
        else {
            idx0 = indices[i * 3];
            idx2 = indices[i * 3 + 1];
            idx1 = indices[i * 3 + 2];
        }

        // 각 삼각형의 버텍스 위치를 FVector로 불러옵니다.
        uint32 stride = sizeof(FVertexSimple);
        FVector v0 = *reinterpret_cast<FVector*>(pbPositions + idx0 * stride);
        FVector v1 = *reinterpret_cast<FVector*>(pbPositions + idx1 * stride);
        FVector v2 = *reinterpret_cast<FVector*>(pbPositions + idx2 * stride);

        float fHitDistance;
        if (IntersectRayTriangle(rayOrigin, rayDirection, v0, v1, v2, fHitDistance)) {
            if (fHitDistance < fNearHitDistance) {
                pfNearHitDistance = fNearHitDistance = fHitDistance;
            }
            nIntersections++;
        }

    }
    return nIntersections;
}

UObject* UStaticMeshComponent::Duplicate() const
{
    UStaticMeshComponent* NewComp = FObjectFactory::ConstructObjectFrom<UStaticMeshComponent>(this);
    NewComp->DuplicateSubObjects(this);
    NewComp->PostDuplicate();
    return NewComp;
}

void UStaticMeshComponent::DuplicateSubObjects(const UObject* Source)
{
    UMeshComponent::DuplicateSubObjects(Source);
    // staticMesh는 복사 생성자에서 복제됨
}

void UStaticMeshComponent::PostDuplicate() {}

void UStaticMeshComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    
    //StaticMesh 경로 저장
    UStaticMesh* CurrentMesh = GetStaticMesh(); 
    if (CurrentMesh != nullptr) {

        // 1. std::wstring 경로 얻기
        std::wstring PathWString = CurrentMesh->GetPathOjbectName(); // 이 함수가 std::wstring 반환 가정

        // 2. std::wstring을 FString으로 변환
        FString PathFString(PathWString.c_str()); // c_str()로 const wchar_t* 얻어서 FString 생성
        PathFString = CurrentMesh->ConvertToRelativePathFromAssets(PathFString);
        
        OutProperties.Add(TEXT("StaticMeshPath"), PathFString);
    } else
    {
        OutProperties.Add(TEXT("StaticMeshPath"), TEXT("None")); // 메시 없음 명시
    }
}

void UStaticMeshComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;

    // --- StaticMesh 설정 ---
    TempStr = InProperties.Find(TEXT("StaticMeshPath"));
    if (TempStr) // 키가 존재하는지 확인
    {
        if (*TempStr != TEXT("None")) // 값이 "None"이 아닌지 확인
        {
            // 경로 문자열로 UStaticMesh 에셋 로드 시도
            UStaticMesh* MeshToSet = FManagerOBJ::CreateStaticMesh(*TempStr);
            if (MeshToSet)
            {
                SetStaticMesh(MeshToSet); // 성공 시 메시 설정
                UE_LOG(LogLevel::Display, TEXT("Set StaticMesh '%s' for %s"), **TempStr, *GetName());
            }
            else
            {
                // 로드 실패 시 경고 로그
                UE_LOG(LogLevel::Warning, TEXT("Could not load StaticMesh '%s' for %s"), **TempStr, *GetName());
                SetStaticMesh(nullptr); // 안전하게 nullptr로 설정
            }
        }
        else // 값이 "None"이면
        {
            SetStaticMesh(nullptr); // 명시적으로 메시 없음 설정
            UE_LOG(LogLevel::Display, TEXT("Set StaticMesh to None for %s"), *GetName());
        }
    }
    else // 키 자체가 없으면
    {
        // 키가 없는 경우 어떻게 처리할지 결정 (기본값 유지? nullptr 설정?)
        // 여기서는 기본값을 유지하거나, 안전하게 nullptr로 설정할 수 있습니다.
        // SetStaticMesh(nullptr); // 또는 아무것도 안 함
        UE_LOG(LogLevel::Display, TEXT("StaticMeshPath key not found for %s, mesh unchanged."), *GetName());
    }

    
}

void UStaticMeshComponent::TickComponent(float DeltaTime)
{
    //Timer += DeltaTime * 0.005f;
    //SetLocation(GetWorldLocation()+ (FVector(1.0f,1.0f, 1.0f) * sin(Timer)));
}



UCylinderComponent::UCylinderComponent()
{
    SetType(StaticClass()->GetName());
}

UCylinderComponent::~UCylinderComponent()
{
}

void UCylinderComponent::InitializeComponent()
{
    Super::InitializeComponent();

    FManagerOBJ::CreateStaticMesh("Assets/Cylinder.obj");
    SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cylinder.obj"));
}

void UCylinderComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

UConeComponent::UConeComponent()
{
    SetType(StaticClass()->GetName());
}

UConeComponent::~UConeComponent()
{
}

void UConeComponent::InitializeComponent()
{
    Super::InitializeComponent();

    FManagerOBJ::CreateStaticMesh("Assets/Cone.obj");
    SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cone.obj"));
}

void UConeComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

UPlaneComponent::UPlaneComponent()
{
    SetType(StaticClass()->GetName());
}

UPlaneComponent::~UPlaneComponent()
{
}

void UPlaneComponent::InitializeComponent()
{
    Super::InitializeComponent();

    FManagerOBJ::CreateStaticMesh("Assets/Plane.obj");
    SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Plane.obj"));
}

void UPlaneComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

USphereComponent::USphereComponent()
{
    SetType(StaticClass()->GetName());
}

USphereComponent::~USphereComponent()
{
}

void USphereComponent::InitializeComponent()
{
    Super::InitializeComponent();

    FManagerOBJ::CreateStaticMesh("Assets/Sphere.obj");
    SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Sphere.obj"));
}

void USphereComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

UTorusComponent::UTorusComponent()
{
    SetType(StaticClass()->GetName());
}

UTorusComponent::~UTorusComponent()
{
}

void UTorusComponent::InitializeComponent()
{
    Super::InitializeComponent();

    FManagerOBJ::CreateStaticMesh("Assets/Torus.obj");
    SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Torus.obj"));
}

void UTorusComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}
