#pragma once
#include <algorithm> // std::clamp
#include <cmath>     // std::sqrt

struct FLinearColor
{
    float R, G, B, A;

    FLinearColor() : R(0), G(0), B(0), A(1) {}
    FLinearColor(float InR, float InG, float InB, float InA = 1.0f)
        : R(InR), G(InG), B(InB), A(InA) {
    }

    static FLinearColor White() { return FLinearColor(1, 1, 1, 1); }
    static FLinearColor Black() { return FLinearColor(0, 0, 0, 1); }
    static FLinearColor Red() { return FLinearColor(1, 0, 0, 1); }
    static FLinearColor Green() { return FLinearColor(0, 1, 0, 1); }
    static FLinearColor Blue() { return FLinearColor(0, 0, 1, 1); }

    FLinearColor operator+(const FLinearColor& Other) const
    {
        return FLinearColor(R + Other.R, G + Other.G, B + Other.B, A + Other.A);
    }

    FLinearColor operator-(const FLinearColor& Other) const
    {
        return FLinearColor(R - Other.R, G - Other.G, B - Other.B, A - Other.A);
    }

    FLinearColor operator*(float Scalar) const
    {
        return FLinearColor(R * Scalar, G * Scalar, B * Scalar, A * Scalar);
    }

    FLinearColor operator/(float Scalar) const
    {
        float InvScalar = 1.0f / Scalar;
        return FLinearColor(R * InvScalar, G * InvScalar, B * InvScalar, A * InvScalar);
    }

    FLinearColor operator*(const FLinearColor& Other) const
    {
        return FLinearColor(R * Other.R, G * Other.G, B * Other.B, A * Other.A);
    }

    // 색상 보간
    static FLinearColor Lerp(const FLinearColor& A, const FLinearColor& B, float T)
    {
        return A + (B - A) * T;
    }

    // 밝기 (Luminance)
    float GetLuminance() const
    {
        return 0.2126f * R + 0.7152f * G + 0.0722f * B;
    }

    // Clamp (0~1 범위로)
    void Clamp()
    {
        R = std::clamp(R, 0.0f, 1.0f);
        G = std::clamp(G, 0.0f, 1.0f);
        B = std::clamp(B, 0.0f, 1.0f);
        A = std::clamp(A, 0.0f, 1.0f);
    }

    FLinearColor ToSRGB() const
    {
        auto GammaCorrect = [](float C) {
            return (C <= 0.0031308f) ? C * 12.92f : 1.055f * std::pow(C, 1.0f / 2.4f) - 0.055f;
            };
        return FLinearColor(GammaCorrect(R), GammaCorrect(G), GammaCorrect(B), A);
    }

    static FLinearColor FromSRGB(float R, float G, float B, float A = 1.0f)
    {
        auto Linearize = [](float C) {
            return (C <= 0.04045f) ? C / 12.92f : std::pow((C + 0.055f) / 1.055f, 2.4f);
            };
        return FLinearColor(Linearize(R), Linearize(G), Linearize(B), A);
    }

    FString ToString() const
    {
        return FString::Printf(TEXT("R=%f G=%f B=%f A=%f"), R, G, B, A);
    }

    bool InitFromString(const FString& SourceString)
    {
        // FString에서 C-스타일 문자열 포인터 얻기 (메서드 이름은 실제 FString 구현에 맞게 조정)
        const char* currentPos = *SourceString; // 또는 SourceString.c_str();
        if (currentPos == nullptr) return false; // 빈 문자열 또는 오류

        float parsedR, parsedG, parsedB, parsedA;
        char* endPtrR = nullptr;
        char* endPtrG = nullptr;
        char* endPtrB = nullptr;
        char* endPtrA = nullptr;

        // 1. "X=" 찾기 및 값 파싱
        const char* xMarker = strstr(currentPos, "R=");
        if (xMarker == nullptr) return false; // "X=" 마커 없음

        // "X=" 다음 위치로 이동
        const char* xValueStart = xMarker + 2; // "X=" 길이만큼 이동

        // 숫자 변환 시도 (strtof는 선행 공백 무시)
        parsedR = strtof(xValueStart, &endPtrR);
        // 변환 실패 확인 (숫자를 전혀 읽지 못함)
        if (endPtrR == xValueStart) return false;

        // 파싱 성공, 다음 검색 시작 위치 업데이트
        currentPos = endPtrR;

        // 2. "Y=" 찾기 및 값 파싱 (X 이후부터 검색)
        const char* yMarker = strstr(currentPos, "G=");
        if (yMarker == nullptr) return false; // "Y=" 마커 없음

        const char* yValueStart = yMarker + 2;
        parsedG = strtof(yValueStart, &endPtrG);
        if (endPtrG == yValueStart) return false; // 변환 실패

        // 다음 검색 시작 위치 업데이트
        currentPos = endPtrG;

        // 3. "Z=" 찾기 및 값 파싱 (Y 이후부터 검색)
        const char* zMarker = strstr(currentPos, "B=");
        if (zMarker == nullptr) return false; // "Z=" 마커 없음

        const char* zValueStart = zMarker + 2;
        parsedB = strtof(zValueStart, &endPtrB);
        if (endPtrB == zValueStart) return false; // 변환 실패


        // 다음 검색 시작 위치 업데이트
        currentPos = endPtrB;

        // 4. "A=" 찾기 및 값 파싱 (Z 이후부터 검색)
        const char* aMarker = strstr(currentPos, "A=");
        if (aMarker == nullptr) return false; // "Z=" 마커 없음

        const char* aValueStart = aMarker + 2;
        parsedA = strtof(aValueStart, &endPtrA);
        if (endPtrA == aValueStart) return false; // 변환 실패


        // 모든 파싱 성공 시, 멤버 변수 업데이트
        R = parsedR;
        G = parsedG;
        B = parsedB;
        A = parsedA;
        return true;
    }
};
