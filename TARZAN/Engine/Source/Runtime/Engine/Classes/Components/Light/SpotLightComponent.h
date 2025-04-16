#pragma once
#include "LightComponent.h"

struct FBoundingBox;
class USpotLightComponent : public UPointLightComponent
{
    DECLARE_CLASS(USpotLightComponent, UPointLightComponent)

public:
    USpotLightComponent();
    USpotLightComponent(const USpotLightComponent& Other);
    virtual ~USpotLightComponent() override;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;

    FVector GetDirection() { return GetForwardVector(); }
    float GetInnerConeAngle() const { return InnerConeAngle; }
    void SetInnerConeAngle(float angle) { InnerConeAngle = angle; }
    float GetOuterConeAngle() const { return OuterConeAngle; }
    void SetOuterConeAngle(float angle) { OuterConeAngle = angle; }

    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

    void GetProperties(TMap<FString, FString>& OutProperties) const override;
    void SetProperties(const TMap<FString, FString>& InProperties) override;

    void SetBoundingBox(const FBoundingBox& InAABB);
    FBoundingBox GetBoundingBox() { return AABB; }

protected:
    virtual const wchar_t* GetDefaultIconPath() const override { return L"Engine/Icon/SpotLight_64x.png"; }

protected:
    float InnerConeAngle;
    float OuterConeAngle;

    FBoundingBox AABB;
};