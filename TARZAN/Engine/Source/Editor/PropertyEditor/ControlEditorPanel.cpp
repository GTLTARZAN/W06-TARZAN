#include "ControlEditorPanel.h"

#include "Engine/World.h"
#include "Actors/Player.h"
#include "Components/Light/LightComponent.h"
#include "Components/SphereComp.h"
#include "Components/UParticleSubUVComp.h"
#include "Components/UText.h"
#include "Engine/FLoaderOBJ.h"
#include "Engine/StaticMeshActor.h"
#include "LevelEditor/SLevelEditor.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include "UnrealEd/EditorViewportClient.h"
#include "PropertyEditor/ShowFlags.h"
#include "UnrealEd/SceneMgr.h"
#include "Components/UHeightFogComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/FireballComp.h"
#include "UEditorStateManager.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Source/Runtime/Renderer/Renderer.h"


void ControlEditorPanel::Render()
{
    /* Pre Setup */
    ImGuiIO& io = ImGui::GetIO();
    ImFont* IconFont = io.Fonts->Fonts[FEATHER_FONT];
    ImVec2 IconSize = ImVec2(32, 32);

    float PanelWidth = (Width) * 0.8f;
    float PanelHeight = 45.0f;

    float PanelPosX = 1.0f;
    float PanelPosY = 1.0f;

    ImVec2 MinSize(300, 50);
    ImVec2 MaxSize(FLT_MAX, 50);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;

    /* Render Start */
    ImGui::Begin("Control Panel", nullptr, PanelFlags);

    CreateMenuButton(IconSize, IconFont);

    ImGui::SameLine();

    CreateFlagButton();

    ImGui::SameLine();

    CreateLightCutButton();

    ImGui::SameLine();

    CreateModifyButton(IconSize, IconFont);

    ImGui::SameLine();

    ImVec2 PIEIconSize = ImVec2(IconSize.x + 8, IconSize.y);
    ImGui::PushFont(IconFont);
    CreatePIEButton(PIEIconSize);
    ImGui::PopFont();

    ImGui::SameLine();

    /* Get Window Content Region */
    float ContentWidth = ImGui::GetWindowContentRegionMax().x;

    /* Move Cursor X Position */
    ImGui::SetCursorPosX(ContentWidth - (IconSize.x * 3.0f + 16.0f));

    ImGui::PushFont(IconFont);
    CreateSRTButton(IconSize);
    ImGui::PopFont();

    ImGui::End();
}

void ControlEditorPanel::CreateMenuButton(ImVec2 ButtonSize, ImFont* IconFont)
{
    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9ad", ButtonSize)) // Menu
    {
        bOpenMenu = !bOpenMenu;
    }
    ImGui::PopFont();

    if (bOpenMenu)
    {
        //std::unique_ptr<FSceneMgr> SceneMgr = std::make_unique<FSceneMgr>();
        FSceneMgr* SceneMgr = GEngine->GetSceneManager();
        ImGui::SetNextWindowPos(ImVec2(10, 55), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(135, 170), ImGuiCond_Always);

        ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (ImGui::MenuItem("New Scene"))
        {
            GEngine->GetWorld()->NewScene();
            // TODO: New Scene
        }

        if (ImGui::MenuItem("Load Scene"))
        {
            char const* lFilterPatterns[1] = { "*.scene" };
            const char* FileName = tinyfd_openFileDialog("Open Scene File", "", 1, lFilterPatterns, "Scene(.scene) file", 0);

            if (FileName == nullptr)
            {
                tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
                ImGui::End();
                return;
            }
            GEngine->GetWorld()->ReloadScene(FileName);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save Scene"))
        {
            char const* lFilterPatterns[1] = { "*.scene" };
            const char* FileName = tinyfd_saveFileDialog("Save Scene File", "", 1, lFilterPatterns, "Scene(.scene) file");

            if (FileName == nullptr)
            {
                ImGui::End();
                return;
            }

            GEngine->GetWorld()->SaveScene(FileName);

            tinyfd_messageBox("알림",

                "저장되었습니다.", "ok", "info", 1);
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Import"))
        {
            if (ImGui::MenuItem("Wavefront (.obj)"))
            {
                char const* lFilterPatterns[1] = { "*.obj" };
                const char* FileName = tinyfd_openFileDialog("Open OBJ File", "", 1, lFilterPatterns, "Wavefront(.obj) file", 0);

                if (FileName != nullptr)
                {
                    std::cout << FileName << std::endl;

                    if (FManagerOBJ::CreateStaticMesh(FileName) == nullptr)
                    {
                        tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
                    }
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quit"))
        {
            ImGui::OpenPopup("프로그램 종료");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("프로그램 종료", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("정말 프로그램을 종료하시겠습니까?");
            ImGui::Separator();

            float ContentWidth = ImGui::GetWindowContentRegionMax().x;

            /* Move Cursor X Position */
            ImGui::SetCursorPosX(ContentWidth - (160.f + 10.0f));

            if (ImGui::Button("OK", ImVec2(80, 0))) { PostQuitMessage(0); }

            ImGui::SameLine();

            ImGui::SetItemDefaultFocus();
            ImGui::PushID("CancelButtonWithQuitWindow");
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 1.0f, 1.0f));
            if (ImGui::Button("Cancel", ImVec2(80, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::PopStyleColor(3);
            ImGui::PopID();

            ImGui::EndPopup();
        }

        ImGui::End();
    }
}

void ControlEditorPanel::CreateModifyButton(ImVec2 ButtonSize, ImFont* IconFont)
{
    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9c4", ButtonSize)) // Slider
    {
        ImGui::OpenPopup("SliderControl");
    }
    ImGui::PopFont();

    if (ImGui::BeginPopup("SliderControl"))
    {
        ImGui::Text("Grid Scale");
        GridScale = GEngine->GetLevelEditor()->GetActiveViewportClient()->GetGridSize();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Grid Scale", &GridScale, 0.1f, 1.0f, 20.0f, "%.1f"))
        {
            GEngine->GetLevelEditor()->GetActiveViewportClient()->SetGridSize(GridScale);
        }
        ImGui::Separator();

        ImGui::Text("Camera FOV");
        FOV = &GEngine->GetLevelEditor()->GetActiveViewportClient()->ViewFOV;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Fov", FOV, 0.1f, 30.0f, 120.0f, "%.1f"))
        {
            //GEngineLoop.GetWorld()->GetCamera()->SetFOV(FOV);

        }
        ImGui::Spacing();

        ImGui::Text("Camera Speed");
        CameraSpeed = GEngine->GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##CamSpeed", &CameraSpeed, 0.1f, 0.198f, 192.0f, "%.1f"))
        {
            GEngine->GetLevelEditor()->GetActiveViewportClient()->SetCameraSpeedScalar(CameraSpeed);
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9c8", ButtonSize))
    {
        ImGui::OpenPopup("PrimitiveControl");
    }
    ImGui::PopFont();

    if (ImGui::BeginPopup("PrimitiveControl"))
    {
        struct Primitive {
            const char* label;
            int obj;
        };

        static const Primitive primitives[] = {

            {.label = "EmptyActor",         .obj = OBJ_StaticMesh},
            {.label = "Cube",               .obj = OBJ_CUBE },
            {.label = "Sphere",             .obj = OBJ_SPHERE },
            {.label = "Cylinder",           .obj = OBJ_Cylinder },
            {.label = "Plane",              .obj = OBJ_Plane },
            {.label = "Cone",               .obj = OBJ_Cone },
            {.label = "Torus",              .obj = OBJ_Torus },
            {.label = "SpotLight",          .obj = OBJ_SpotLight },
            {.label = "PointLight",         .obj = OBJ_PointLight },
            {.label = "DirectionalLight",   .obj = OBJ_DirectionalLight },
            {.label = "Particle",           .obj = OBJ_Particle },
            {.label = "Text",               .obj = OBJ_Text },
            {.label = "Fog",                .obj = OBJ_Fog},

        };

        for (const auto& primitive : primitives)
        {
            if (ImGui::Selectable(primitive.label))
            {
                // GEngineLoop.GetWorld()->SpawnObject(static_cast<OBJECTS>(primitive.obj));
                UWorld* World = GEngine->GetWorld();
                AActor* SpawnedActor = nullptr;
                AStaticMeshActor* TempActor = nullptr;
                switch (static_cast<OBJECTS>(primitive.obj))
                {
                case OBJ_StaticMesh:
                {
                    TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_EmptyActor"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    break;
                }
                case OBJ_CUBE:
                {
                    TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_CUBE"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Cube.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cube.obj"));
                    break;
                }
                case OBJ_SPHERE:
                {
                    TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_SPHERE"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Sphere.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Sphere.obj"));
                    break;
                }
                case OBJ_Cylinder:
                {
                    TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_Cylinder"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Cylinder.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cylinder.obj"));
                    break;
                }
                case OBJ_Plane:
                {
                    TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_Plane"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Plane.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Plane.obj"));
                    break;
                }
                case OBJ_Cone:
                {
                    TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_Cone"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Cone.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cone.obj"));
                    break;
                }
                case OBJ_Torus:
                {
                    TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLabel(TEXT("OBJ_Torus"));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/Torus.obj");
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Torus.obj"));
                    break;
                }
                case OBJ_SpotLight:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_SpotLight"));
                    SpawnedActor->AddComponent<USpotLightComponent>();
                    //UBillboardComponent* BillboardComponent = SpawnedActor->AddComponent<UBillboardComponent>();
                    //BillboardComponent->SetTexture(L"Engine/Icon/SpotLight_64x.png");
                    //LightComp->SetTexture2D(BillboardComponent);
                    break;
                }
                case OBJ_PointLight:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_PointLight"));
                    SpawnedActor->AddComponent<UPointLightComponent>();
                    //UBillboardComponent* BillboardComponent = SpawnedActor->AddComponent<UBillboardComponent>();
                    //BillboardComponent->SetTexture(L"Engine/Icon/PointLight_64x.png");
                    //LightComp->SetTexture2D(BillboardComponent);
                    break;
                }
                case OBJ_DirectionalLight:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_DirectionalLight"));
                    UDirectionalLightComponent* LightComp = SpawnedActor->AddComponent<UDirectionalLightComponent>();
                    SpawnedActor->AddComponent<UAmbientLightComponent>();
                    //UBillboardComponent* BillboardComponent = SpawnedActor->AddComponent<UBillboardComponent>();
                    //BillboardComponent->SetTexture(L"Engine/Icon/DirectionalLight_64x.png");
                    //LightComp->SetTexture2D(BillboardComponent);
                    break;
                }
                case OBJ_Particle:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_PARTICLE"));
                    UParticleSubUVComp* ParticleComponent = SpawnedActor->AddComponent<UParticleSubUVComp>();
                    ParticleComponent->SetTexture(L"Assets/Texture/T_Explosion_SubUV.png");
                    ParticleComponent->SetRowColumnCount(6, 6);
                    ParticleComponent->SetScale(FVector(10.0f, 10.0f, 1.0f));
                    ParticleComponent->Activate();
                    break;
                }
                case OBJ_Text:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_Text"));
                    UText* TextComponent = SpawnedActor->AddComponent<UText>();
                    TextComponent->SetTexture(L"Assets/Texture/font.png");
                    TextComponent->SetRowColumnCount(106, 106);
                    TextComponent->SetText(L"안녕하세요 Jungle 1");
                    break;
                }
                case OBJ_Fog:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_Fog"));
                    SpawnedActor->AddComponent<UHeightFogComponent>();
                    break;
                }
                //case OBJ_Fireball:
                //{
                //    TempActor = World->SpawnActor<AStaticMeshActor>();
                //    TempActor->SetActorLabel(TEXT("OBJ_Fireball"));
                //    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                //    FManagerOBJ::CreateStaticMesh("Assets/FireBall.obj");
                //    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"FireBall.obj"));
                //    TempActor->AddComponent<UFireballComponent>();
                //    break;
                //}
                case OBJ_TRIANGLE:
                case OBJ_CAMERA:
                case OBJ_PLAYER:
                case OBJ_END:
                    break;
                }

                if (TempActor)
                {
                    SpawnedActor = TempActor;
                }

                if (SpawnedActor)
                {
                    World->SetPickedActor(SpawnedActor);
                }
            }
        }
        ImGui::EndPopup();
    }
}

void ControlEditorPanel::CreateFlagButton() const
{
    auto ActiveViewport = GEngine->GetLevelEditor()->GetActiveViewportClient();

    const char* ViewTypeNames[] = { "Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back" };
    ELevelViewportType ActiveViewType = ActiveViewport->GetViewportType();
    FString TextViewType = ViewTypeNames[ActiveViewType];

    if (ImGui::Button(GetData(TextViewType), ImVec2(120, 32)))
    {
        // toggleViewState = !toggleViewState;
        ImGui::OpenPopup("ViewControl");
    }

    if (ImGui::BeginPopup("ViewControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewTypeNames); i++)
        {
            bool bIsSelected = ((int)ActiveViewport->GetViewportType() == i);
            if (ImGui::Selectable(ViewTypeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewportType((ELevelViewportType)i);
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    const char* ViewModeNames[] = { "Lit", "Unlit", "Wireframe", "Base Color", "Normal", "Depth", "World Position" };
    FString SelectLightControl = ViewModeNames[(int)ActiveViewport->GetViewMode()];
    ImVec2 LightTextSize = ImGui::CalcTextSize(GetData(SelectLightControl));

    if (ImGui::Button(GetData(SelectLightControl), ImVec2(30 + LightTextSize.x, 32)))
    {
        ImGui::OpenPopup("LightControl");
    }

    if (ImGui::BeginPopup("LightControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewModeNames); i++)
        {
            bool bIsSelected = ((int)ActiveViewport->GetViewMode() == i);
            if (ImGui::Selectable(ViewModeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewMode((EViewModeIndex)i);
                UEditorEngine::graphicDevice.ChangeRasterizer(ActiveViewport->GetViewMode());
                UEditorEngine::renderer.ChangeViewMode(ActiveViewport->GetViewMode());
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    const char* ShadeModeNames[] = { "Gouraud", "Lambert", "Blinn-Phong", "Normal"};
    if (ActiveViewport->GetViewMode() == EViewModeIndex::VMI_Lit)
    {
        // Lit을 선택했을 때만 Shade를 선택할 수 있도록
        FString SelectShaderControl = ShadeModeNames[(int)ActiveViewport->GetLighitingModel() - 1];
        ImVec2 ShaderTextSize = ImGui::CalcTextSize(GetData(SelectShaderControl));

        if (ImGui::Button(GetData(SelectShaderControl), ImVec2(30 + ShaderTextSize.x, 32)))
        {
            ImGui::OpenPopup("ShaderControl");
        }

        if (ImGui::BeginPopup("ShaderControl"))
        {
            for (int i = 0; i < IM_ARRAYSIZE(ShadeModeNames); i++)
            {
                bool bIsSelected = ((int)ActiveViewport->GetLighitingModel() - 1 == i);
                if (ImGui::Selectable(ShadeModeNames[i], bIsSelected))
                {
                    ActiveViewport->SetLightingModel((ELightingModel)(i + 1));
                }

                if (bIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();
    }

    if (ImGui::Button("Show", ImVec2(60, 32)))
    {
        ImGui::OpenPopup("ShowControl");
    }

    const char* items[] = { "AABB", "Primitive", "BillBoard", "UUID", "Fog", "LightWireFrame"};
    uint64 ActiveViewportFlags = ActiveViewport->GetShowFlag();

    if (ImGui::BeginPopup("ShowControl"))
    {
        bool selected[IM_ARRAYSIZE(items)] =
        {
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_AABB)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_Primitives)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_BillboardText)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_UUIDText)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_Fog)) != 0,
            (ActiveViewportFlags & static_cast<uint64>(EEngineShowFlags::SF_LightWireframe)) != 0
        };

        for (int i = 0; i < IM_ARRAYSIZE(items); i++)
        {
            ImGui::Checkbox(items[i], &selected[i]);
        }
        ActiveViewport->SetShowFlag(ConvertSelectionToFlags(selected));

        for (const auto iter : TObjectRange<USceneComponent>())
        {
            if (UHeightFogComponent* HeightFog = Cast<UHeightFogComponent>(iter))
            {
                bool bFogEnabled = (selected[4] == true);
                HeightFog->SetDisableFog(!bFogEnabled);
            }
        }

        ImGui::EndPopup();
    }
}

void ControlEditorPanel::CreateLightCutButton() const
{
    if (ImGui::Button("LightCut", ImVec2(90, 32)))
    {
        ImGui::OpenPopup("LightCutPopup");
    }

    // 팝업에 표시할 항목들
    const char* items[] = { "Generate LightCut", "Remove LightCut" };

    if (ImGui::BeginPopup("LightCutPopup"))
    {
        if (ImGui::Selectable(items[0]))
        {
            GEngine->renderer.SetGenerateLightTree(true);
        }
        if (ImGui::Selectable(items[1]))
        {
            GEngine->renderer.DeletePointLightCut();
        }
        ImGui::EndPopup();
    }
}
void ControlEditorPanel::CreatePIEButton(ImVec2 ButtonSize) const
{
    float TotalWidth = ButtonSize.x * 3.0f + 16.0f;
    float ContentWidth = ImGui::GetWindowContentRegionMax().x;

    // 중앙 정렬을 위한 커서 위치 설정
    float CursorPosX = (ContentWidth - TotalWidth) * 0.5f;
    ImGui::SetCursorPosX(CursorPosX);

    if (ImGui::Button("\ue9a8", ButtonSize)) // Play
    {
        UEditorStateManager::Get().SetState(EEditorState::PreparingPlay);
        // TODO: PIE 시작
        // if (!UEditorStateManager::Get().IsPIERunning())
        //     UEditorStateManager::Get().SetState(EEditorState::PreparingPlay);
        // else
        //     UEditorStateManager::Get().SetState(EEditorState::Resuming);
    }

    ImGui::SameLine();

    if (ImGui::Button("\ue99c", ButtonSize)) // Pause
    {
        // TODO: PIE 일시정지
        UEditorStateManager::Get().SetState(EEditorState::Paused);
    }

    ImGui::SameLine();

    if (ImGui::Button("\ue9e4", ButtonSize)) // Stop
    {
        // TODO: PIE 정지
        UEditorStateManager::Get().SetState(EEditorState::Stopped);
    }
}

// code is so dirty / Please refactor
void ControlEditorPanel::CreateSRTButton(ImVec2 ButtonSize) const
{
    AEditorPlayer* Player = GEngine->GetWorld()->GetEditorPlayer();

    ImVec4 ActiveColor = ImVec4(0.00f, 0.00f, 0.85f, 1.0f);

    ControlMode ControlMode = Player->GetControlMode();

    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9bc", ButtonSize)) // Move
    {
        Player->SetMode(CM_TRANSLATION);
    }
    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    if (ControlMode == CM_ROTATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9d3", ButtonSize)) // Rotate
    {
        Player->SetMode(CM_ROTATION);
    }
    if (ControlMode == CM_ROTATION)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    if (ControlMode == CM_SCALE)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9ab", ButtonSize)) // Scale
    {
        Player->SetMode(CM_SCALE);
    }
    if (ControlMode == CM_SCALE)
    {
        ImGui::PopStyleColor();
    }
}

uint64 ControlEditorPanel::ConvertSelectionToFlags(const bool selected[]) const
{
    uint64 flags = static_cast<uint64>(EEngineShowFlags::None);

    if (selected[0])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_AABB);
    if (selected[1])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_Primitives);
    if (selected[2])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
    if (selected[3])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_UUIDText);
    if (selected[4])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_Fog);
    if (selected[5])
        flags |= static_cast<uint64>(EEngineShowFlags::SF_LightWireframe);
    return flags;
}


void ControlEditorPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
}
