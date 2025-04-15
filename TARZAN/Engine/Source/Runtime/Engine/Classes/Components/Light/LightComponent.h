#pragma once
#include "Define.h"
#include "Components/SceneComponent.h"
class UBillboardComponent;

class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() override;

    virtual void TickComponent(float DeltaTime) override;
    virtual void InitializeLight();
    void SetColor(FLinearColor InColor);
    FLinearColor GetColor() const;
    void SetIntensity(float InIntensity);
    float GetIntensity() const;
    void SetVisible(bool bVisible);
    bool IsVisible() const;

    virtual void InitializeComponent() {};

protected:
    float Intensity;
    FLinearColor LightColor;
    bool bVisible;
    UBillboardComponent* Texture2D;

public:
    FLinearColor GetColor() {return LightColor;}
    UBillboardComponent* GetTexture2D() const {return Texture2D;}
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
};


class UDirectionalLightComponent : public ULightComponent
{
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)

public:
    UDirectionalLightComponent();
    virtual ~UDirectionalLightComponent() override;

private:
    virtual void InitializeComponent() override;

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

    virtual void InitializeComponent() override;

    void SetRadius(float InRadius);
    float GetRadius() const;
    void SetLightFalloffExponent(float Exponent);
    float GetLightFalloffExponent() const;

protected:
    float Radius;
    float LightFalloffExponent;
};
