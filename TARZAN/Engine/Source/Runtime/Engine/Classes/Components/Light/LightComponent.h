#pragma once
#include "Define.h"
#include "Components/SceneComponent.h"
#include "Components/UBillboardComponent.h"

class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() override;

    virtual void TickComponent(float DeltaTime) override;
    virtual void InitializeLight();
    void SetColor(FLinearColor InColor) 
    {
        LightColor = InColor;
        if (Texture2D)
        {
            Texture2D->SetTintColor(LightColor);
        }
    }
    FLinearColor GetColor() const { return LightColor; }
    void SetIntensity(float InIntensity) { Intensity = InIntensity; }
    float GetIntensity() const { return Intensity; }
    void SetVisible(bool InbVisible) { bVisible = InbVisible; }
    bool IsVisible() const { return bVisible; }
    void SetTexture2D(UBillboardComponent* InBillComp) 
    {
        Texture2D = InBillComp; 
        Texture2D->SetIsLightIcon(true);
    }
    UBillboardComponent* GetTexture2D() const { return Texture2D; }

    virtual void InitializeComponent() {};
    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

protected:
    virtual const wchar_t* GetDefaultIconPath() const { return L"Engine/Icon/DefaultLight_64x.png"; }

protected:
    float Intensity;
    FLinearColor LightColor;
    bool bVisible;
    UBillboardComponent* Texture2D;
};

class ULightComponent : public ULightComponentBase
{
    DECLARE_CLASS(ULightComponent, ULightComponentBase)

public:
    ULightComponent() {};
    virtual ~ULightComponent() override {};

    virtual void InitializeComponent() override {};
};

/*********************** Light Component *****************************/

class UAmbientLightComponent : public ULightComponent
{
    DECLARE_CLASS(UAmbientLightComponent, ULightComponent)

public:
    UAmbientLightComponent();
    virtual ~UAmbientLightComponent() override;

    virtual void InitializeComponent() override;

protected:
    virtual const wchar_t* GetDefaultIconPath() const override { return L"Engine/Icon/AmbientLight_64x.png"; }
};


class UDirectionalLightComponent : public ULightComponent
{
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)

public:
    UDirectionalLightComponent();
    virtual ~UDirectionalLightComponent() override;

    virtual void InitializeComponent() override;
    FVector GetDirection() { return GetForwardVector(); }

protected:
    virtual const wchar_t* GetDefaultIconPath() const override { return L"Engine/Icon/DirectionalLight_64x.png"; }

protected:
    FVector Direction;
};


// @todo 파일 분리하기
class UPointLightComponent : public ULightComponent 
{
    DECLARE_CLASS(UPointLightComponent, ULightComponent)

public:
    UPointLightComponent();
    UPointLightComponent(const UPointLightComponent& Other);
    virtual ~UPointLightComponent() override;

    void SetRadius(float InRadius);
    float GetRadius() const;
    void SetLightFalloffExponent(float Exponent);
    float GetLightFalloffExponent() const;

    virtual void InitializeComponent() override;

    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

protected:
    virtual const wchar_t* GetDefaultIconPath() const override { return L"Engine/Icon/PointLight_64x.png"; } 

protected:
    float Radius;
    float LightFalloffExponent;
};
