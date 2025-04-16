#include "UBillboardComponent.h"
#include "Actors/Player.h"
#include "QuadTexture.h"
#include "Define.h"
#include <DirectXMath.h>

#include "Components/Light/LightComponent.h"
#include "Engine/World.h"
#include "Math/MathUtility.h"
#include "UnrealEd/EditorViewportClient.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ShowFlags.h"


UBillboardComponent::UBillboardComponent()
{
    SetType(StaticClass()->GetName());
}

UBillboardComponent::~UBillboardComponent()
{
	// if (vertexTextureBuffer)
	// {
	// 	vertexTextureBuffer->Release();
	// 	vertexTextureBuffer = nullptr;
	// }
	// if (indexTextureBuffer)
	// {
	// 	indexTextureBuffer->Release();
	// 	indexTextureBuffer = nullptr;
	// }
}

UBillboardComponent::UBillboardComponent(const UBillboardComponent& other) 
    : UPrimitiveComponent(other)
    , finalIndexU(other.finalIndexU), finalIndexV(other.finalIndexV), Texture(other.Texture)
    , TintColor(other.TintColor), bIsLightIcon(other.bIsLightIcon)
{
}

void UBillboardComponent::InitializeComponent()
{
    Super::InitializeComponent();
	//CreateQuadTextureVertexBuffer();
}

void UBillboardComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

int UBillboardComponent::CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance)
{
	TArray<FVector> quad;
	for (int i = 0; i < 4; i++)
	{
		quad.Add(FVector(quadTextureVertices[i].x, 
			quadTextureVertices[i].y, quadTextureVertices[i].z));
	}
	return CheckPickingOnNDC(quad,pfNearHitDistance);
}


void UBillboardComponent::SetTexture(const FWString&  _fileName)
{
    TexturePath = _fileName;
	Texture = UEditorEngine::resourceMgr.GetTexture(TexturePath);
}

void UBillboardComponent::SetUUIDParent(USceneComponent* _parent)
{
}


FMatrix UBillboardComponent::CreateBillboardMatrix()
{
	FMatrix CameraView = GetEngine()->GetLevelEditor()->GetActiveViewportClient()->GetViewMatrix();

	CameraView.M[0][3] = 0.0f;
	CameraView.M[1][3] = 0.0f;
	CameraView.M[2][3] = 0.0f;


	CameraView.M[3][0] = 0.0f;
	CameraView.M[3][1] = 0.0f;
	CameraView.M[3][2] = 0.0f;
	CameraView.M[3][3] = 1.0f;


	CameraView.M[0][2] = -CameraView.M[0][2];
	CameraView.M[1][2] = -CameraView.M[1][2];
	CameraView.M[2][2] = -CameraView.M[2][2];
	FMatrix LookAtCamera = FMatrix::Transpose(CameraView);
	
	FVector worldLocation = GetWorldLocation();
	FVector worldScale = RelativeScale3D;
	FMatrix S = FMatrix::CreateScale(worldScale.x, worldScale.y, worldScale.z);
	FMatrix R = LookAtCamera;
	FMatrix T = FMatrix::CreateTranslationMatrix(worldLocation);
	FMatrix M = S * R * T;

	return M;
}

UObject* UBillboardComponent::Duplicate() const
{
    UBillboardComponent* ClonedActor = FObjectFactory::ConstructObjectFrom<UBillboardComponent>(this);
    ClonedActor->DuplicateSubObjects(this);
    ClonedActor->PostDuplicate();
    return ClonedActor;
}

void UBillboardComponent::DuplicateSubObjects(const UObject* Source)
{
    Super::DuplicateSubObjects(Source);
}

void UBillboardComponent::PostDuplicate()
{
    Super::PostDuplicate();
}

void UBillboardComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);

    OutProperties.Add(TEXT("FinalIndexU"), FString::Printf(TEXT("%f"), finalIndexU));
    OutProperties.Add(TEXT("FinalIndexV"), FString::Printf(TEXT("%f"), finalIndexV));
    OutProperties.Add(TEXT("TexturePath"), FString(TexturePath.c_str()));
    OutProperties.Add(TEXT("TintColor"), TintColor.ToString());
    OutProperties.Add(TEXT("bIsLightIcon"), bIsLightIcon ? TEXT("true") : TEXT("false"));
}

void UBillboardComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("FinalIndexU"));
    if (TempStr)
    {
        finalIndexU = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("FinalIndexV"));
    if (TempStr)
    {
        finalIndexV = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("TexturePath"));
    if (TempStr)
    {
        TexturePath = TempStr->ToWideString();
        Texture = UEditorEngine::resourceMgr.GetTexture(TexturePath);
    }
    TempStr = InProperties.Find(TEXT("TintColor"));
    if (TempStr)
    {
        TintColor.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("bIsLightIcon"));
    if (TempStr)
    {
        bIsLightIcon = TempStr->ToBool();
        if (bIsLightIcon)
        {
            if (GetAttachParent()->IsA<ULightComponentBase>())
            {
                ULightComponentBase* LightComp = dynamic_cast<ULightComponentBase*>(GetAttachParent());
                LightComp->SetTexture2D(this);
            }

            //if (AActor* OwnerActor = GetOwner())
            //{
            //    // LightComponent를 찾아서 자동으로 연결해준다
            //    for (UActorComponent* Comp : OwnerActor->GetComponents())
            //    {
            //        if (ULightComponentBase* LightComp = dynamic_cast<ULightComponentBase*>(Comp))
            //        {
            //            LightComp->SetTexture2D(this);
            //            break; // 한 개만 연결되면 충분
            //        }
            //    }
            //}
        }
    }
}

bool UBillboardComponent::CheckPickingOnNDC(const TArray<FVector>& checkQuad, float& hitDistance)
{
	bool result = false;
	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(GEngine->hWnd, &mousePos);

	D3D11_VIEWPORT viewport;
	UINT numViewports = 1;
	UEditorEngine::graphicDevice.DeviceContext->RSGetViewports(&numViewports, &viewport);
	float screenWidth = viewport.Width;
	float screenHeight = viewport.Height;

	FVector pickPosition;
	int screenX = mousePos.x;
	int screenY = mousePos.y;
    FMatrix projectionMatrix = GetEngine()->GetLevelEditor()->GetActiveViewportClient()->GetProjectionMatrix();
	pickPosition.x = ((2.0f * static_cast<float>(screenX) / viewport.Width) - 1);
	pickPosition.y = -((2.0f * static_cast<float>(screenY) / viewport.Height) - 1);
	pickPosition.z = 1.0f; // Near Plane

	FMatrix M = CreateBillboardMatrix();
    FMatrix V = GEngine->GetLevelEditor()->GetActiveViewportClient()->GetViewMatrix();;
	FMatrix P = projectionMatrix;
	FMatrix MVP = M * V * P;

	float minX = FLT_MAX;
	float maxX = FLT_MIN;
	float minY = FLT_MAX;
	float maxY = FLT_MIN;
	float avgZ = 0.0f;
	for (int i = 0; i < checkQuad.Num(); i++)
	{
		FVector4 v = FVector4(checkQuad[i].x, checkQuad[i].y, checkQuad[i].z, 1.0f);
		FVector4 clipPos = FMatrix::TransformVector(v, MVP);
		
		if (clipPos.a != 0)	clipPos = clipPos/clipPos.a;

		minX = FMath::Min(minX, clipPos.x);
		maxX = FMath::Max(maxX, clipPos.x);
		minY = FMath::Min(minY, clipPos.y);
		maxY = FMath::Max(maxY, clipPos.y);
		avgZ += clipPos.z;
	}

	avgZ /= checkQuad.Num();

	if (pickPosition.x >= minX && pickPosition.x <= maxX &&
		pickPosition.y >= minY && pickPosition.y <= maxY)
	{
		float A = P.M[2][2];  // Projection Matrix의 A값 (Z 변환 계수)
		float B = P.M[3][2];  // Projection Matrix의 B값 (Z 변환 계수)

		float z_view_pick = (pickPosition.z - B) / A; // 마우스 클릭 View 공간 Z
		float z_view_billboard = (avgZ - B) / A; // Billboard View 공간 Z

		hitDistance = 1000.0f;
		result = true;
	}

	return result;
}
