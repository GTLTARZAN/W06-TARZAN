#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Container/Array.h"
#include "Engine/Source/Runtime/Launch/Define.h"
#include "Engine/Classes/Components/Light/LightComponent.h"

struct LightTreeNode {
    ULightComponentBase* representative;   // 클러스터의 대표 광원 (Point 또는 Spot)
    float totalIntensity;   // 클러스터 내 광원의 총 강도
    FBoundingBox bbox;        // 클러스터의 바운딩 박스
    int leftIndex;          // 왼쪽 자식 인덱스 (-1이면 리프)
    int rightIndex;         // 오른쪽 자식 인덱스 (-1이면 리프)
    bool isFake;            // 가짜 노드 여부 (실제 광원 없음)

    LightTreeNode()
        : representative(nullptr), totalIntensity(0.f), leftIndex(-1), rightIndex(-1), isFake(false),
        bbox(FVector(std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()),
            FVector(std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest())
        )
    { }
};

class LightTree {
public:
    LightTree();
    // lights 벡터를 입력받아 LightTree를 구축
    void Build(std::vector<ULightComponentBase*> lights);

    // 영역 AABB (보수적 영역)와 오류 한계를 받아 LightCut 노드 인덱스 목록을 계산
    std::vector<int> ComputeLightCut(const FVector& regionMin, const FVector& regionMax, float boundThreshold) const;

    // 내부 노드 목록 반환 (디버깅용)
    const std::vector<LightTreeNode>& GetNodes() const;

private:
    // LightTree의 모든 노드를 보관하는 벡터
    std::vector<LightTreeNode> nodes;

    // 내부 Helper 함수들
    int NextPowerOfTwo(int n) const;
    LightTreeNode CreateLeafNode(ULightComponentBase* light);
    LightTreeNode CreateFakeLeafNode() const;
    LightTreeNode MergeNodes(const LightTreeNode& A, const LightTreeNode& B) const;
    int BuildTree(const std::vector<ULightComponentBase*> lights);
    float ComputeSpotFactor(ULightComponentBase* light, const FVector& regionMin, const FVector& regionMax) const;
    float ComputeClusterError(const LightTreeNode& node, const FVector& regionMin, const FVector& regionMax) const;
    void ComputeLightCutRecursive(int nodeIndex, float boundThreshold,
        const FVector& regionMin, const FVector& regionMax,
        std::vector<int>& cut) const;
};