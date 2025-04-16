#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Container/Array.h"
#include "Engine/Source/Runtime/Launch/Define.h"
#include "Engine/Classes/Components/Light/LightComponent.h"
#include "Engine/Classes/Components/StaticMeshComponent.h"

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
    void Build(const TArray<ULightComponentBase*>& lights);

    // 영역 AABB (보수적 영역)와 오류 한계를 받아 LightCut 노드 인덱스 목록을 계산
    TArray<int> ComputeLightCut(const FVector& regionMin, const FVector& regionMax, float boundThreshold) const;
    void ComputeLightCut(UStaticMeshComponent* Mesh);

    // 내부 노드 목록 반환 (디버깅용)
    const TArray<LightTreeNode>& GetNodes() const;

    // 메시의 LightCut 목록을 추가하거나 업데이트합니다.
    void AddOrUpdateLightCut(UStaticMeshComponent* Mesh, const TArray<int>& LightCutList);

    // 지정된 메시의 LightCut 목록을 반환합니다. 없는 경우 nullptr.
    const TArray<int>* GetLightCut(UStaticMeshComponent* Mesh) const;

    const TArray<ULightComponentBase*> GetLightCutLights(UStaticMeshComponent* Mesh) const;

    // 지정된 메시의 LightCut 목록을 제거합니다.
    void RemoveLightCut(UStaticMeshComponent* Mesh);

    // 전체 LightCut 맵을 비웁니다.
    void ClearLightCut();

    void SetActive(bool active) { isActive = active; }
    bool GetActive() const { return isActive; }

    void ClearLightTree();

private:
    // LightTree의 모든 노드를 보관하는 벡터
    TArray<LightTreeNode> nodes;

    TMap<UStaticMeshComponent*, TArray<int>> cutMap;

    bool isActive = false; // LightTree 활성화 여부

    // 내부 Helper 함수들
    int NextPowerOfTwo(int n) const;
    LightTreeNode CreateLeafNode(ULightComponentBase* light);
    LightTreeNode CreateFakeLeafNode() const;
    LightTreeNode MergeNodes(const LightTreeNode& A, const LightTreeNode& B) const;
    // 그리디 클러스터링을 위해, 전체 노드를 병합하는 함수 (입력: 개별 리프 노드들)
    int BuildTreeGreedy(const TArray<ULightComponentBase*>& lights);
    // 두 노드를 병합할 때의 유사도 측정을 위해, 낮을수록 더 유사한 것으로 판단 (낮은 값일수록 병합 우선)
    float ComputeSimilarityMetric(const LightTreeNode& A, const LightTreeNode& B) const;

    float ComputeSpotFactor(ULightComponentBase* light, const FVector& regionMin, const FVector& regionMax) const;
    float ComputeClusterError(const LightTreeNode& node, const FVector& regionMin, const FVector& regionMax) const;
    void ComputeLightCutRecursive(int nodeIndex, float boundThreshold,
        const FVector& regionMin, const FVector& regionMax,
        TArray<int>& cut) const;
};