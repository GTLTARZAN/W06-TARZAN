#pragma once
#include "PrimitiveComponent.h"
#include "UTexture.h"

struct FTexture;


class UBillboardComponent : public UPrimitiveComponent
{
    DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)
    
public:
    UBillboardComponent();
    virtual ~UBillboardComponent() override;
    UBillboardComponent(const UBillboardComponent& other);

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(
        FVector& rayOrigin,
        FVector& rayDirection, float& pfNearHitDistance
    ) override;

    void SetTexture(const FWString& _fileName);
    void SetUUIDParent(USceneComponent* _parent);
    FMatrix CreateBillboardMatrix();
    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;

    void GetProperties(TMap<FString, FString>& OutProperties) const override;
    
    void SetProperties(const TMap<FString, FString>& InProperties) override;

    float finalIndexU = 0.0f;
    float finalIndexV = 0.0f;
    std::shared_ptr<FTexture> Texture;
    FWString TexturePath;
    
    void SetTintColor(FLinearColor InColor) { TintColor = InColor; }
    void SetIsLightIcon(bool IsLIcon) { bIsLightIcon = IsLIcon; }

    FLinearColor GetTintColor() const { return TintColor; }
    bool IsLightIcon() const { return bIsLightIcon; }

protected:
    bool CheckPickingOnNDC(const TArray<FVector>& checkQuad, float& hitDistance);

private:
    FLinearColor TintColor = FLinearColor::White();
    bool bIsLightIcon = false;
};
