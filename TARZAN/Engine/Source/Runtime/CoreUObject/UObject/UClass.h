#pragma once
#include <concepts>
#include "Object.h"

/**
 * UObject의 RTTI를 가지고 있는 클래스
 */
class UClass : public UObject
{
public:
    UClass(const char* InClassName, uint32 InClassSize, uint32 InAlignment, UClass* InSuperClass);
    virtual ~UClass() override = default;

    // 복사 & 이동 생성자 제거
    UClass(const UClass&) = delete;
    UClass& operator=(const UClass&) = delete;
    UClass(UClass&&) = delete;
    UClass& operator=(UClass&&) = delete;

    uint32 GetClassSize() const { return ClassSize; }
    uint32 GetClassAlignment() const { return ClassAlignment; }

    typedef UObject* (*FObjectConstructorFunction)(UObject* Outer, FName Name);
    
    FObjectConstructorFunction ObjectConstructor = nullptr; // 생성자 함수 포인터

    /** SomeBase의 자식 클래스인지 확인합니다. */
    bool IsChildOf(const UClass* SomeBase) const;

    template <typename T>
        requires std::derived_from<T, UObject>
    bool IsChildOf() const
    {
        return IsChildOf(T::StaticClass());
    }

    /**
     * 부모의 UClass를 가져옵니다.
     *
     * @note AActor::StaticClass()->GetSuperClass() == UObject::StaticClass()
     */
    UClass* GetSuperClass() const
    {
        return SuperClass;
    }

    UObject* GetDefaultObject() const
    {
        if (!ClassDefaultObject)
        {
            const_cast<UClass*>(this)->CreateDefaultObject();
        }
        return ClassDefaultObject;
    }

protected:
    virtual UObject* CreateDefaultObject();

private:
    [[maybe_unused]]
    uint32 ClassSize;

    [[maybe_unused]]
    uint32 ClassAlignment;

    UClass* SuperClass = nullptr;

    UObject* ClassDefaultObject = nullptr;

};



using FGetClassFunction = UClass* (*)();

// GClassRegistry에 접근하는 함수 선언
TMap<FName, FGetClassFunction>& GetGlobalClassRegistry();

// 전역 클래스 레지스트리 (FName을 키로 사용)
//extern  TMap<FName, FGetClassFunction> GClassRegistry;


UClass* FindClassByName(FName ClassName);
UClass* FindClassByName(const FString& ClassName);
