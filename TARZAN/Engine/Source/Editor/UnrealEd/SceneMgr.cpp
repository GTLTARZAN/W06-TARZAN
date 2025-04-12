#include "UnrealEd/SceneMgr.h"
#include "JSON/json.hpp"
#include "UObject/Object.h"
#include "Components/SphereComp.h"
#include "Components/CubeComp.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "UObject/ObjectFactory.h"
#include <fstream>

#include "EditorViewportClient.h"
#include "Components/UBillboardComponent.h"
#include "Components/Light/LightComponent.h"
#include "Components/SkySphereComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/Casts.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/FLoaderOBJ.h"
#include "LevelEditor/SLevelEditor.h"
#include "UObject/UObjectGlobals.h"
#include "Sound/SoundManager.h"
using json = nlohmann::json;

/**
 * JSON 객체로부터 FSceneData 구조체를 파싱하여 채웁니다.
 * @param j 파싱할 nlohmann::json 객체.
 * @param OutSceneData 파싱된 데이터를 저장할 FSceneData 참조.
 * @return 파싱 성공 여부 (간단한 버전에서는 bool 반환, 상세 오류 처리는 예외나 로그 사용).
 */

bool FSceneMgr::ParseSceneData(const json& j , FSceneData& OutSceneData)
{
try // JSON 파싱 및 데이터 접근 시 예외 발생 가능성 처리
    {
        // 기존 데이터 클리어
        OutSceneData.Actors.Empty();

        // Version 파싱
        if (j.contains("Version") && j["Version"].is_number_integer()) {
            OutSceneData.Version = j["Version"].get<int32>();
        } else {
            // 필수 필드가 없으면 오류 처리 또는 기본값 사용
            UE_LOG(LogLevel::Warning, TEXT("JSON does not contain a valid 'Version' field."));
            // return false; // 또는 기본값 0 등으로 진행
            OutSceneData.Version = 0;
        }

        // NextUUID 파싱 (선택적 필드 처리 예시)
        OutSceneData.NextUUID = j.value("NextUUID", 0); // 없으면 기본값 0 사용

        // --- Actors 배열 파싱 ---
        if (j.contains("Actors") && j["Actors"].is_array())
        {
            const json& actorsArrayJson = j["Actors"];
            OutSceneData.Actors.Reserve(actorsArrayJson.size()); // 미리 메모리 할당

            for (const auto& actorJson : actorsArrayJson) // 각 액터 JSON 객체 순회
            {
                if (!actorJson.is_object()) {
                    UE_LOG(LogLevel::Warning, TEXT("Item in 'Actors' array is not a valid JSON object. Skipping."));
                    continue; // 유효하지 않은 항목 건너뛰기
                }

                FActorSaveData actorData; // 현재 액터 데이터 생성

                // 액터 기본 정보 파싱 (문자열 변환 포함)
                if (actorJson.contains("ActorID") && actorJson["ActorID"].is_string()) {
                    actorData.ActorID = FString((actorJson["ActorID"].get<std::string>().c_str()));
                } else { /* 오류 처리 또는 기본값 */ }

                if (actorJson.contains("ActorClass") && actorJson["ActorClass"].is_string()) {
                    actorData.ActorClass = FString((actorJson["ActorClass"].get<std::string>().c_str()));
                } else { /* 필수 필드 오류 처리 */ UE_LOG(LogLevel::Warning,  TEXT("Actor JSON missing 'ActorClass'.")); return false; }

                // ActorLabel은 선택적일 수 있음
                actorData.ActorLabel = FString((actorJson.value("ActorLabel", "").c_str())); // 없으면 빈 문자열

                if (actorJson.contains("RootComponentID") && actorJson["RootComponentID"].is_string()) {
                     actorData.RootComponentID = FString((actorJson["RootComponentID"].get<std::string>().c_str()));
                } else { /* 루트 컴포넌트가 없는 경우 ""가 될 수 있으므로 오류는 아닐 수 있음 */ actorData.RootComponentID = TEXT("");}


                // --- Components 배열 파싱 ---
                if (actorJson.contains("Components") && actorJson["Components"].is_array())
                {
                    const json& componentsArrayJson = actorJson["Components"];
                    actorData.Components.Reserve(componentsArrayJson.size());

                    for (const auto& componentJson : componentsArrayJson) // 각 컴포넌트 JSON 객체 순회
                    {
                         if (!componentJson.is_object()) {
                            UE_LOG(LogLevel::Warning, TEXT("Item in 'Components' array is not a valid JSON object. Skipping."));
                            continue;
                         }

                         FComponentSaveData componentData; // 현재 컴포넌트 데이터 생성

                         // 컴포넌트 기본 정보 파싱
                        if (componentJson.contains("ComponentID") && componentJson["ComponentID"].is_string()) {
                            componentData.ComponentID = FString((componentJson["ComponentID"].get<std::string>().c_str()));
                        } else { /* 오류 처리 */ }

                        if (componentJson.contains("ComponentClass") && componentJson["ComponentClass"].is_string()) {
                            componentData.ComponentClass = FString((componentJson["ComponentClass"].get<std::string>().c_str()));
                        } else { /* 필수 필드 오류 처리 */ UE_LOG(LogLevel::Warning, TEXT("Component JSON missing 'ComponentClass'.")); return false; }

                        // --- Properties 객체 파싱 ---
                        if (componentJson.contains("Properties") && componentJson["Properties"].is_object())
                        {
                            const json& propertiesJson = componentJson["Properties"];
                            // Properties는 TMap이므로 Reserve 불필요

                            // JSON 객체의 모든 키-값 쌍 순회
                            for (auto it = propertiesJson.begin(); it != propertiesJson.end(); ++it)
                            {
                                // it.key()는 속성 이름(std::string), it.value()는 속성 값(json 문자열)
                                if (it.value().is_string()) // 값이 문자열인지 확인
                                {
                                    FString Key = FString((it.key().c_str()));
                                    FString Value = FString((it.value().get<std::string>().c_str()));
                                    componentData.Properties.Add(Key, Value); // TMap에 추가
                                }
                                else {
                                    // 값이 문자열이 아닌 경우 (예상치 못한 데이터) 경고
                                     UE_LOG(LogLevel::Warning,  TEXT("Property '%s' in component '%s' has non-string value. Skipping."),
                                         *FString((it.key().c_str())), *componentData.ComponentID);
                                }
                            }
                        } // Properties 파싱 끝

                        actorData.Components.Add(componentData); // 완성된 컴포넌트 데이터를 액터에 추가
                    } // 컴포넌트 루프 끝
                } // Components 배열 파싱 끝

                OutSceneData.Actors.Add(actorData); // 완성된 액터 데이터를 씬 데이터에 추가
            } // 액터 루프 끝
        } // Actors 배열 파싱 끝
        else {
             UE_LOG(LogLevel::Warning, TEXT("JSON does not contain a valid 'Actors' array. Scene might be empty."));
             // 액터가 없는 빈 씬일 수 있으므로 오류는 아님
        }

        return true; // 파싱 성공
    }
    catch (const json::parse_error& e)
    {
        // JSON 파싱 자체에서 오류 발생 시
        UE_LOG(LogLevel::Warning, TEXT("JSON parse error: %hs at byte %d"), e.what(), e.byte);
        return false;
    }
    catch (const json::type_error& e)
    {
        // JSON 타입 불일치 (예: 배열을 기대했는데 객체가 옴)
        UE_LOG(LogLevel::Warning, TEXT("JSON type error: %hs"), e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        // 기타 예외 처리
        UE_LOG(LogLevel::Warning, TEXT("An unexpected error occurred during JSON parsing: %hs"), e.what());
        return false;
    }
    catch (...)
    {
        // 알 수 없는 예외
        UE_LOG(LogLevel::Warning, TEXT("An unknown error occurred during JSON parsing."));
        return false;
    }
}


void FSceneMgr::LoadSceneFromFile(const FString& filename, UWorld& world)
{
    std::string NewFileName = *filename;
    std::ifstream inFile(NewFileName);
    if (!inFile) {
        UE_LOG(LogLevel::Error, "Failed to open file for reading: %s", *filename);
        return ;
    }

    json j;
    try {
        inFile >> j; // JSON 파일 읽기
    }
    catch (const std::exception& e) {
        UE_LOG(LogLevel::Error, "Error parsing JSON: %s", e.what());
        return;
    }

    inFile.close();

    FSceneData SceneData;
    bool Result = ParseSceneData(j,SceneData);
    if (!Result)
    {
        UE_LOG(LogLevel::Error, "Failed to parse scene data from file: %s", *filename);
        return ;
    }

    LoadSceneFromData(SceneData, &world);
    
    
    int a = 0;

    

    return ;
}

bool FSceneMgr::LoadSceneFromData(const FSceneData& sceneData, UWorld* targetWorld)
{
    if (targetWorld == nullptr)
    {
        UE_LOG(LogLevel::Error, TEXT("LoadSceneFromData: Target World is null!"));
        return false;
    }

    // 임시 맵: 저장된 ID와 새로 생성된 객체 포인터를 매핑
    TMap<FString, AActor*> SpawnedActorsMap;
    //TMap<FString, UActorComponent*> SpawnedComponentsMap;


    

    // --- 1단계: 액터 및 컴포넌트 생성 ---
    UE_LOG(LogLevel::Display, TEXT("Loading Scene Data: Phase 1 - Spawning Actors and Components..."));
    for (const FActorSaveData& actorData : sceneData.Actors)
    {
        // 1.1. 액터 클래스 찾기
        
        UClass* classAActor = FindClassByName(FName(actorData.ActorClass));
        
        AActor* SpawnedActor = targetWorld->SpawnActor(classAActor, FName(actorData.ActorID));

        // if (actorData.ActorClass == AActor::StaticClass()->GetName())
        // {
        //     SpawnedActor = targetWorld->SpawnActor<AActor>();
        // }
        // if (actorData.ActorClass == AStaticMeshActor::StaticClass()->GetName())
        // {
        //     SpawnedActor = targetWorld->SpawnActor<AStaticMeshActor>();
        // }
        // // 또는 특정 경로에서 클래스 로드: UClass* ActorClass = LoadClass<AActor>(nullptr, *actorData.ActorClass);
        if (SpawnedActor == nullptr)
        {
            UE_LOG(LogLevel::Error, TEXT("LoadSceneFromData: Could not find Actor Class '%s'. Skipping Actor '%s'."),
                   *actorData.ActorClass, *actorData.ActorID);
            continue;
        }

        
        // 액터 클래스가 AActor의 자식인지 확인

        // 1.2. 액터 스폰 (기본 위치/회전 사용, 나중에 루트 컴포넌트가 설정)
        //FActorSpawnParameters SpawnParams;
        //SpawnParams.Name = FName(*actorData.ActorID); // 저장된 ID를 이름으로 사용 시도 (Unique해야 함)
        //SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested; // 이름 충돌 시 엔진이 처리하도록 할 수도 있음
        //AActor* SpawnedActor = targetWorld->SpawnActor<AActor>(ActorClass, FVector::ZeroVector);

        if (SpawnedActor == nullptr)
        {
            UE_LOG(LogLevel::Error, TEXT("LoadSceneFromData: Failed to spawn Actor '%s' of class '%s'."),
                   *actorData.ActorID, *actorData.ActorClass);
            continue;
        }

        SpawnedActor->SetActorLabel(actorData.ActorLabel); // 액터 레이블 설정
        SpawnedActorsMap.Add(actorData.ActorID, SpawnedActor); // 맵에 추가

        // 액터별 로컬 컴포넌트 맵: ComponentID -> 생성/재사용된 컴포넌트 포인터
        TMap<FString, UActorComponent*> ActorComponentsMap;

        // 1.3. 컴포넌트 생성 및 속성 설정 (아직 부착 안 함)
        for (const FComponentSaveData& componentData : actorData.Components)
        {
            UClass* ComponentClass = FindClassByName(FName(componentData.ComponentClass));

            if (componentData.ComponentID == "USpotLightComponent_156")
            {
                int a= 0;
            }
            // 컴포넌트 생성 (액터를 Outer로 지정, 저장된 ID를 이름으로)
            UActorComponent* TargetComponent = nullptr; // 최종적으로 사용할 컴포넌트 포인터

            // *** 핵심 변경: 저장된 ID(이름)로 액터에서 기존 컴포넌트를 먼저 찾아본다 ***
            FName ComponentFName(*componentData.ComponentID);
            TargetComponent = FindObject<UActorComponent>(SpawnedActor, ComponentFName); // Outer를 SpawnedActor로 지정하여 검색

            // 클래스 일치 확인
            if (TargetComponent && TargetComponent->GetClass()->GetName() != componentData.ComponentClass) {
                UE_LOG(LogLevel::Warning, TEXT("Component '%s' class mismatch. Recreating."), *componentData.ComponentID);
                // TODO: 기존 컴포넌트를 제거해야 할 수도 있음? 아니면 그냥 새것으로 덮어쓰나? 정책 필요.
                TargetComponent = nullptr; // 새로 생성하도록 리셋
            }

            // 기존 컴포넌트가 없으면 새로 생성
            if (TargetComponent == nullptr)
            {
                TargetComponent = SpawnedActor->AddComponent(ComponentClass, FName(componentData.ComponentID));

                
                if (!actorData.RootComponentID.IsEmpty())
                {
                    if (componentData.ComponentID != actorData.RootComponentID)
                    {
                        // 임시로 RootComponent 가 아니면 떼어줌
                        USceneComponent* SceneComp = Cast<USceneComponent>(TargetComponent);
                        if (SceneComp)
                        {
                            SpawnedActor->SetRootComponent(nullptr);
                        }
                    }
                }
                
                // if (componentData.ComponentClass == UStaticMesh::StaticClass()->GetName())
                // {
                //     TargetComponent = SpawnedActor->AddComponent<UStaticMeshComponent>();
                // }
                // else if (componentData.ComponentClass == UCubeComp::StaticClass()->GetName())
                // {
                //     TargetComponent = SpawnedActor->AddComponent<UCubeComp>();
                // }
                // else
                // {
                //     TargetComponent = SpawnedActor->AddComponent<UActorComponent>();
                // }
                
                // !!! 중요: 컴포넌트 등록 !!!
                //NewComponent->RegisterComponent();
            }
            
            if (TargetComponent == nullptr)
            {
                 UE_LOG(LogLevel::Error, TEXT("LoadSceneFromData: Failed to create Component '%s' of class '%s' for Actor '%s'."),
                       *componentData.ComponentID, *componentData.ComponentClass, *actorData.ActorID);
                continue;
            }

            if (componentData.ComponentID == "USpotLightComponent_156")
            {
                int a= 0;
            }

            // --- 이제 TargetComponent는 유효한 기존 컴포넌트 또는 새로 생성된 컴포넌트 ---
            if (TargetComponent)
            {
                // 1.4. 컴포넌트 속성 설정 (공통 로직)
                //ApplyComponentProperties(TargetComponent, componentData.Properties);
                TargetComponent->SetProperties( componentData.Properties); // 태그 설정 (ID로 사용)

                // 1.5. *** 수정: 복합 키를 사용하여 컴포넌트 맵에 추가 ***
                //FString CompositeKey = actorData.ActorID + TEXT("::") + componentData.ComponentID; // 예: "MyActor1::MeshComponent"
                ActorComponentsMap.Add(componentData.ComponentID, TargetComponent);
            }
        }

        // 루트 컴포넌트 설정
        if (!actorData.RootComponentID.IsEmpty())
        {
            UActorComponent** FoundRootCompPtr = ActorComponentsMap.Find(actorData.RootComponentID);
            if (FoundRootCompPtr && *FoundRootCompPtr)
            {
                USceneComponent* RootSceneComp = Cast<USceneComponent>(*FoundRootCompPtr);
                if (RootSceneComp) {
                    SpawnedActor->SetRootComponent(RootSceneComp);
                    UE_LOG(LogLevel::Display, TEXT("Set RootComponent '%s' for Actor '%s'"), *actorData.RootComponentID, *actorData.ActorID);
                }
                else { /* 루트가 SceneComponent 아님 경고 */ }
            }
            else { /* 루트 컴포넌트 못 찾음 경고 */ }
        }

        // 컴포넌트 부착 및 상대 트랜스폼 설정
        for (const FComponentSaveData& componentData : actorData.Components) // 다시 컴포넌트 데이터 순회
        {
            UActorComponent** FoundCompPtr = ActorComponentsMap.Find(componentData.ComponentID);
            if (componentData.ComponentID == "USpotLightComponent_156")
            {
                int a= 0;
            }
            
            if (FoundCompPtr == nullptr || *FoundCompPtr == nullptr) continue; // 위에서 생성/찾기 실패한 경우

            USceneComponent* CurrentSceneComp = Cast<USceneComponent>(*FoundCompPtr);
            if (CurrentSceneComp == nullptr) continue; // SceneComponent만 부착/트랜스폼 가능

            // 부착 정보 찾기 (Properties 맵 사용)
            const FString* ParentIDPtr = componentData.Properties.Find(TEXT("AttachParentID"));
            if (ParentIDPtr && !ParentIDPtr->IsEmpty() && *ParentIDPtr != TEXT("nullptr"))
            {
                // !!! 부모 검색 범위를 ActorComponentsMap (현재 액터의 컴포넌트)으로 한정 !!!
                UActorComponent** FoundParentCompPtr = ActorComponentsMap.Find(*ParentIDPtr);
                if (FoundParentCompPtr && *FoundParentCompPtr)
                {
                    USceneComponent* ParentSceneComp = Cast<USceneComponent>(*FoundParentCompPtr);
                    if (ParentSceneComp) {
                        // 부착 실행 (SetupAttachment 대신 AttachToComponent 권장 - 규칙 명시 가능)
                        CurrentSceneComp->SetupAttachment(ParentSceneComp);
                        UE_LOG(LogLevel::Display, TEXT("Attached Component '%s' to Parent '%s' in Actor '%s'"), *componentData.ComponentID, *(*ParentIDPtr), *actorData.ActorID);
                    }
                    else { /* 부모가 SceneComponent 아님 경고 */ }
                }
                else {
                    // 부모 컴포넌트를 이 액터 내에서 찾지 못함 (오류 가능성 높음)
                    UE_LOG(LogLevel::Warning, TEXT("Could not find Parent component '%s' within Actor '%s' for '%s'."), *(*ParentIDPtr), *actorData.ActorID, *componentData.ComponentID);
                }
            }

            FVector RelativeLocation = FVector::ZeroVector;
            const FString* LocStr = componentData.Properties.Find(TEXT("RelativeLocation"));
            if (LocStr) RelativeLocation.InitFromString(*LocStr); // 또는 직접 파싱

            FVector RelativeRotation;
            const FString* RotatStr = componentData.Properties.Find(TEXT("RelativeRotation")); // 쿼터니언 저장/로드 권장
            if (RotatStr) RelativeRotation.InitFromString(*RotatStr);

            FVector RelativeScale3D = FVector::OneVector;
            const FString* ScaleStr = componentData.Properties.Find(TEXT("RelativeScale")); // 스케일 키 이름 확인! (GetProperties와 일치해야 함)
            if (ScaleStr) RelativeScale3D.InitFromString(*ScaleStr);

            CurrentSceneComp->SetLocation(RelativeLocation);
            CurrentSceneComp->SetRotation(RelativeRotation);
            CurrentSceneComp->SetScale(RelativeScale3D);
        }

    }
    UE_LOG(LogLevel::Display, TEXT("Loading Scene Data: Phase 1 Complete. Spawned %d actors."), SpawnedActorsMap.Num());

    //// --- 2단계: 루트 컴포넌트 설정 및 부착 ---
    //UE_LOG(LogLevel::Display, TEXT("Loading Scene Data: Phase 2 - Setting Root Components and Attachments..."));
    //for (const FActorSaveData& actorData : sceneData.Actors)
    //{
    //    AActor** FoundActorPtr = SpawnedActorsMap.Find(actorData.ActorID);
    //    if (FoundActorPtr == nullptr || *FoundActorPtr == nullptr) continue; // 1단계에서 스폰 실패한 경우

    //    AActor* CurrentActor = *FoundActorPtr;

    //    // 2.1. 루트 컴포넌트 설정
    //    if (!actorData.RootComponentID.IsEmpty())
    //    {
    //        UActorComponent** FoundCompPtr = SpawnedComponentsMap.Find(actorData.RootComponentID);
    //        if (FoundCompPtr && *FoundCompPtr)
    //        {
    //            USceneComponent* RootSceneComp = Cast<USceneComponent>(*FoundCompPtr);
    //            if (RootSceneComp) {
    //                CurrentActor->SetRootComponent(RootSceneComp);
    //                 UE_LOG(LogLevel::Display, TEXT("Set RootComponent '%s' for Actor '%s'"), *actorData.RootComponentID, *actorData.ActorID);
    //            } else {
    //                UE_LOG(LogLevel::Warning, TEXT("Component '%s' designated as root for Actor '%s' is not a SceneComponent."),
    //                       *actorData.RootComponentID, *actorData.ActorID);
    //            }
    //        } else {
    //             UE_LOG(LogLevel::Warning, TEXT("Could not find component '%s' designated as root for Actor '%s'."),
    //                       *actorData.RootComponentID, *actorData.ActorID);
    //        }
    //    }

    //    // 2.2. 컴포넌트 부착 및 상대 트랜스폼 설정
    //    for (const FComponentSaveData& componentData : actorData.Components)
    //    {
    //        UActorComponent** FoundCompPtr = SpawnedComponentsMap.Find(componentData.ComponentID);
    //        if (FoundCompPtr == nullptr || *FoundCompPtr == nullptr) continue; // 1단계에서 생성 실패한 경우

    //        USceneComponent* CurrentSceneComp = Cast<USceneComponent>(*FoundCompPtr);
    //        if (CurrentSceneComp == nullptr) continue; // SceneComponent만 부착 가능

    //        // 부착 정보 찾기
    //        const FString* ParentIDPtr = componentData.Properties.Find(TEXT("AttachParentID"));
    //        if (ParentIDPtr && !ParentIDPtr->IsEmpty() && *ParentIDPtr != TEXT("nullptr")) // "nullptr" 문자열 체크 추가
    //        {
    //            UActorComponent** FoundParentCompPtr = SpawnedComponentsMap.Find(*ParentIDPtr);
    //            if (FoundParentCompPtr && *FoundParentCompPtr)
    //            {
    //                USceneComponent* ParentSceneComp = Cast<USceneComponent>(*FoundParentCompPtr);
    //                if (ParentSceneComp) {
    //                    // !!! 부착 실행 !!!
    //                    CurrentSceneComp->SetupAttachment(ParentSceneComp);
    //                     UE_LOG(LogLevel::Display, TEXT("Attached Component '%s' to Parent '%s'"), *componentData.ComponentID, *(*ParentIDPtr));
    //                } else {
    //                     UE_LOG(LogLevel::Warning, TEXT("Parent component '%s' for '%s' is not a SceneComponent."), *(*ParentIDPtr), *componentData.ComponentID);
    //                }
    //            } else {
    //                 UE_LOG(LogLevel::Warning, TEXT("Could not find Parent component '%s' for '%s'."), *(*ParentIDPtr), *componentData.ComponentID);
    //            }
    //        }

    //        // FVector RelativeLocation;
    //        // FVector RelativeRotation;
    //        // FVector RelativeScale3D;
    //        // FQuat RelativeQuat;
    //        //
    //        // RelativeLocation.InitFromString(*componentData.Properties[TEXT("RelativeLocation")]);
    //        // RelativeRotation.InitFromString(*componentData.Properties[TEXT("RelativeRotation")]);
    //        // RelativeScale3D.InitFromString(*componentData.Properties[TEXT("RelativeScale3D")]);
    //        // RelativeQuat.InitFromString(*componentData.Properties[TEXT("RelativeQuat")]);


    //        
    //        
    //        //CurrentSceneComp->SetRelativeTransform(RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics); 
    //    }
    //}
    UE_LOG(LogLevel::Display, TEXT("Scene loading complete."));

    // 임시 맵 정리 (선택적)
    SpawnedActorsMap.Empty();
    //SpawnedComponentsMap.Empty();

    // 필요하다면 추가적인 월드 초기화 로직 (예: 네비게이션 재빌드 요청)
    // ...

    UE_LOG(LogLevel::Display, TEXT("Scene loading complete."));
    return true;
}

std::string FSceneMgr::SerializeSceneData(const FSceneData& sceneData)
{
    json j; // 최종 JSON 객체

    // Version과 NextUUID 저장
    j["Version"] = sceneData.Version;
    // NextUUID는 이제 씬 에셋 ID 관리 등에 사용될 수 있으나,
    // 액터/컴포넌트 고유 ID는 각 데이터 내에 있으므로 그 자체의 의미는 줄어들 수 있음.
    j["NextUUID"] = sceneData.NextUUID; // 필요하다면 유지

    // --- 새로운 구조 기반 직렬화 ---

    json actorsArrayJson = json::array(); // 액터 목록을 담을 JSON 배열

    // FSceneData의 Actors 배열 순회
    for (const FActorSaveData& actorData : sceneData.Actors)
    {
        json actorJson = json::object(); // 개별 액터 정보를 담을 JSON 객체

        // 액터 기본 정보 저장 (FString -> std::string 변환 필요 시 TCHAR_TO_UTF8 사용)
        actorJson["ActorID"] = (*actorData.ActorID);
        actorJson["ActorClass"] = (*actorData.ActorClass);
        actorJson["ActorLabel"] = (*actorData.ActorLabel);
        actorJson["RootComponentID"] = (*actorData.RootComponentID);

        json componentsArrayJson = json::array(); // 컴포넌트 목록을 담을 JSON 배열

        // 액터의 Components 배열 순회
        for (const FComponentSaveData& componentData : actorData.Components)
        {
            json componentJson = json::object(); // 개별 컴포넌트 정보를 담을 JSON 객체

            // 컴포넌트 기본 정보 저장
            componentJson["ComponentID"] = (*componentData.ComponentID);
            componentJson["ComponentClass"] = (*componentData.ComponentClass);

            // Properties 맵 저장 (TMap<FString, FString> -> JSON object)
            json propertiesJson = json::object();
            for (const auto& Pair : componentData.Properties) // TMap 순회
            {
                const FString& Key = Pair.Key;
                const FString& Value = Pair.Value;
                // JSON 키와 값 모두 문자열로 저장
                propertiesJson[(*Key)] = (*Value);
            }
            componentJson["Properties"] = propertiesJson; // 프로퍼티 객체를 컴포넌트 객체에 추가

            componentsArrayJson.push_back(componentJson); // 완성된 컴포넌트 JSON을 배열에 추가
        }

        actorJson["Components"] = componentsArrayJson; // 컴포넌트 배열을 액터 객체에 추가
        actorsArrayJson.push_back(actorJson); // 완성된 액터 JSON을 배열에 추가
    }

    j["Actors"] = actorsArrayJson; // 액터 배열을 최종 JSON 객체에 추가
    
    
    return j.dump(4); // 4는 들여쓰기 수준

}

bool FSceneMgr::SaveSceneToFile(const FString& filename, const UWorld& world)
{
    FSceneData sceneData  = CreateSceneData(world);
    
    std::ofstream outFile(*filename);
    if (!outFile) {
        FString errorMessage = "Failed to open file for writing: ";
        MessageBoxA(NULL, *errorMessage, "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::string jsonData = SerializeSceneData(sceneData);
    outFile << jsonData;
    outFile.close();

    return true;
}

FSceneData FSceneMgr::CreateSceneData(const UWorld& world)
{
    FSceneData sceneData;
    sceneData.Version = 1;

    const TSet<AActor*>& Actors =  world.GetActors();

    sceneData.Actors.Reserve(Actors.Num());

    for (const auto& Actor : Actors)
    {
        FActorSaveData actorData;

        actorData.ActorID = Actor->GetName();
        actorData.ActorClass = Actor->GetClass()->GetName();
        actorData.ActorLabel = Actor->GetActorLabel();

        USceneComponent* RootComp = Actor->GetRootComponent();
        actorData.RootComponentID = (RootComp != nullptr) ? RootComp->GetName() : TEXT(""); // 루트 없으면 빈 문자열
        
        for (const auto& Component : Actor->GetComponents())
        {
            FComponentSaveData componentData;
            componentData.ComponentID = Component->GetName();
            componentData.ComponentClass = Component->GetClass()->GetName();
            
            //TMap<FString, FString> InProperties;
            Component->GetProperties(componentData.Properties);
            
            // 컴포넌트의 속성들을 JSON으로 변환하여 저장
            // for (const auto& Property : InProperties)
            // {
            //    FString Value = Property.Value;
            //     
            //     componentData.Properties[Property.Key] = Value;
            // }

            actorData.Components.Add(componentData);
        }
        sceneData.Actors.Add(actorData);
    }
    return sceneData;
}

