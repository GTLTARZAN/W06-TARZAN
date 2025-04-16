#include "LightTreeNode.h"
#include "Engine/Classes/Components/Light/SpotLightComponent.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Casts.h"

//------------------------------
// LightTree 클래스 생성자
//------------------------------
LightTree::LightTree() {}

//------------------------------
// NextPowerOfTwo 함수 구현
//------------------------------
int LightTree::NextPowerOfTwo(int n) const {
    int power = 1;
    while (power < n)
        power *= 2;
    return power;
}

//------------------------------
// 리프 노드 생성 함수 (광원 정보를 이용)
//------------------------------
LightTreeNode LightTree::CreateLeafNode(ULightComponentBase* light) {
    LightTreeNode node;
    node.representative = light;
    node.totalIntensity = light->GetIntensity();
    node.bbox.min = light->GetWorldLocation();
    node.bbox.max = light->GetWorldLocation();
    node.leftIndex = -1;
    node.rightIndex = -1;
    node.isFake = false;
    return node;
}

LightTreeNode LightTree::CreateFakeLeafNode() const {
    LightTreeNode node;
    node.totalIntensity = 0.f;
    node.bbox.min = FVector(0.f, 0.f, 0.f);
    node.bbox.max = FVector(0.f, 0.f, 0.f);
    node.leftIndex = -1;
    node.rightIndex = -1;
    node.isFake = true;
    return node;
}

//------------------------------
// 두 노드 병합 함수: A와 B를 병합하여 내부 노드 생성
//------------------------------
LightTreeNode LightTree::MergeNodes(const LightTreeNode& A, const LightTreeNode& B) const {
    LightTreeNode parent;
    parent.totalIntensity = A.totalIntensity + B.totalIntensity;
    parent.bbox.min.x = std::min(A.bbox.min.x, B.bbox.min.x);
    parent.bbox.min.y = std::min(A.bbox.min.y, B.bbox.min.y);
    parent.bbox.min.z = std::min(A.bbox.min.z, B.bbox.min.z);
    parent.bbox.max.x = std::max(A.bbox.max.x, B.bbox.max.x);
    parent.bbox.max.y = std::max(A.bbox.max.y, B.bbox.max.y);
    parent.bbox.max.z = std::max(A.bbox.max.z, B.bbox.max.z);
    parent.representative = (A.totalIntensity >= B.totalIntensity) ? A.representative : B.representative;
    parent.leftIndex = -1;
    parent.rightIndex = -1;
    parent.isFake = false;
    return parent;
}

//------------------------------
// Build 함수: 외부에서 호출, 광원 목록을 받아 LightTree를 구축
//------------------------------
void LightTree::Build(const TArray<ULightComponentBase*>& lights) {
    if (lights.Num() == 0) {
        std::cerr << "광원 목록이 비어 있습니다.\n";
        return;
    }
    // 기존 노드 벡터 초기화
    nodes.Empty();
    BuildTreeGreedy(lights);
}

int LightTree::BuildTreeGreedy(const TArray<ULightComponentBase*>& lights) {
    // 초기 리프 노드 생성
    TArray<LightTreeNode> initialNodes;
    for (ULightComponentBase* light : lights) {
        initialNodes.Add(CreateLeafNode(light));
    }
    // 만약 개수가 2의 거듭제곱이 아니라면 가짜 노드 추가
    int n = initialNodes.Num();
    int nTotal = NextPowerOfTwo(n);
    while (initialNodes.Num() < static_cast<size_t>(nTotal))
        initialNodes.Add(CreateFakeLeafNode());

    // nodes 벡터에 먼저 리프 노드들을 추가하고, 인덱스를 저장
    nodes = initialNodes;
    TArray<int> clusterIndices;
    for (int i = 0; i < initialNodes.Num(); i++) {
        clusterIndices.Add(i);
    }

    // 그리디하게 클러스터 병합: 가장 유사한 두 클러스터를 선택하여 병합하고, 새로운 노드를 추가
    while (clusterIndices.Num() > 1) {
        float bestMetric = std::numeric_limits<float>::max();
        int bestI = -1, bestJ = -1;
        for (size_t i = 0; i < clusterIndices.Num(); i++) {
            for (size_t j = i + 1; j < clusterIndices.Num(); j++) {
                int idxA = clusterIndices[i];
                int idxB = clusterIndices[j];
                float metric = ComputeSimilarityMetric(nodes[idxA], nodes[idxB]);
                if (metric < bestMetric) {
                    bestMetric = metric;
                    bestI = static_cast<int>(i);
                    bestJ = static_cast<int>(j);
                }
            }
        }
        // 병합할 두 클러스터의 인덱스
        int indexA = clusterIndices[bestI];
        int indexB = clusterIndices[bestJ];
        // 새로운 부모 노드 생성
        LightTreeNode parent = MergeNodes(nodes[indexA], nodes[indexB]);
        // 부모 노드의 자식 인덱스 기록
        parent.leftIndex = indexA;
        parent.rightIndex = indexB;
        int newIndex = nodes.Num();
        nodes.Add(parent);
        // 기존 두 개 인덱스를 제거 (큰 인덱스를 먼저 제거)
        if (bestJ > bestI) {
            clusterIndices.RemoveAt(bestJ);
            clusterIndices.RemoveAt(bestI);
        }
        else {
            clusterIndices.RemoveAt(bestI);
            clusterIndices.RemoveAt(bestJ);
        }
        // 새로운 노드의 인덱스를 추가
        clusterIndices.Add(newIndex);
    }
    return clusterIndices[0]; // 최종 루트 노드 인덱스 반환
}

float LightTree::ComputeSimilarityMetric(const LightTreeNode& A, const LightTreeNode& B) const
{
    // 빈 노드(가짜 노드)나 총 강도가 0인 경우, 병합 우선순위에서 밀리도록 매우 큰 값을 반환합니다.
    if (A.isFake || B.isFake || A.totalIntensity <= 0.f || B.totalIntensity <= 0.f) {
        return std::numeric_limits<float>::max();
    }

    LightTreeNode merged = MergeNodes(A, B);
    // 클러스터 AABB의 대각선 길이 계산
    float dx = merged.bbox.max.x - merged.bbox.min.x;
    float dy = merged.bbox.max.y - merged.bbox.min.y;
    float dz = merged.bbox.max.z - merged.bbox.min.z;
    float diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 기본 metric은 총 강도와 대각선 제곱
    float intensity = A.totalIntensity + B.totalIntensity;
    float metric = intensity * (diagonal * diagonal);

    // 두 노드 모두 Spot 광원이면, 방향 유사도를 고려 (유사할수록 metric이 낮아야 함)
    if (A.representative->IsA<USpotLightComponent>() && B.representative->IsA<USpotLightComponent>()) {
        // Spot 광원은 대표적으로 GetDirection() 이용 (이미 정규화된 방향)
        USpotLightComponent* spotA = Cast<USpotLightComponent>(A.representative);
        USpotLightComponent* spotB = Cast<USpotLightComponent>(B.representative);
        if (spotA && spotB) {
            float dot = spotA->GetDirection().Dot(spotB->GetDirection());
            dot = std::clamp(dot, -1.0f, 1.0f);
            float diff = 1.0f - dot;  // 0이면 완전히 유사, 2이면 완전히 반대
            float directionTerm = diff * diff;
            // c 상수는 1.0으로 가정하여 metric에 더함
            metric += intensity * directionTerm;
        }
    }
    return metric;
}

//------------------------------
// ComputeSpotFactor 함수: Spot 광원의 경우, 영역 AABB와의 각도를 고려하여 보수적 Factor 계산
//------------------------------
float LightTree::ComputeSpotFactor(ULightComponentBase* light, const FVector& regionMin, const FVector& regionMax) const {
    if (light->IsA<UPointLightComponent>())
        return 1.0f;

    USpotLightComponent* spotLight = Cast<USpotLightComponent>(light);

    // 영역의 중심을 쉐이딩 영역의 대표 점으로 사용

    FVector regionCenter((regionMin.x + regionMax.x) * 0.5f,
        (regionMin.y + regionMax.y) * 0.5f,
        (regionMin.z + regionMax.z) * 0.5f);
    FVector toCenter(regionCenter.x - spotLight->GetWorldLocation().x,
        regionCenter.y - spotLight->GetWorldLocation().y,
        regionCenter.z - spotLight->GetWorldLocation().z);
    float dist = std::sqrt(toCenter.x * toCenter.x + toCenter.y * toCenter.y + toCenter.z * toCenter.z);
    if (dist < 1e-3f)
        dist = 1e-3f;
    FVector normVec(toCenter.x / dist, toCenter.y / dist, toCenter.z / dist);
    float cosTheta = normVec.Dot(spotLight->GetDirection());
    if (cosTheta < cos(spotLight->GetOuterConeAngle()))
        return 100.0f; // 범위 밖이면 큰 페널티
    else
        return 1.0f;
}

//------------------------------
// 두 AABB 사이의 최소 거리를 계산하는 함수
// 두 AABB (A와 B)의 최소 거리를 계산하여, 보수적 기하 bound 계산에 사용
//------------------------------
float DistanceBetweenAABBs(const FVector& aMin, const FVector& aMax, const FVector& bMin, const FVector& bMax) {
    float dx = 0.f, dy = 0.f, dz = 0.f;
    if (aMax.x < bMin.x) dx = bMin.x - aMax.x;
    else if (aMin.x > bMax.x) dx = aMin.x - bMax.x;

    if (aMax.y < bMin.y) dy = bMin.y - aMax.y;
    else if (aMin.y > bMax.y) dy = aMin.y - bMax.y;

    if (aMax.z < bMin.z) dz = bMin.z - aMax.z;
    else if (aMin.z > bMax.z) dz = aMin.z - bMax.z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

//------------------------------
// ComputeClusterError 함수: 영역 AABB를 사용하여 오차 상한 계산
// ε = I_C * (1/π) * (1/(d_min)^2)
// d_min은 쉐이딩 영역 AABB와 클러스터 AABB 사이의 최소 거리
// Spot 광원의 경우 spot factor를 곱함
//------------------------------
float LightTree::ComputeClusterError(const LightTreeNode& node, const FVector& regionMin, const FVector& regionMax) const {
    const float pi = 3.14159265f;
    float materialBound = 1.0f / pi;  // Lambertian 가정
    float dmin = DistanceBetweenAABBs(regionMin, regionMax, node.bbox.min, node.bbox.max);
    if (dmin < 1e-3f)
        dmin = 1e-3f;
    float geometricBound = 1.0f / (dmin * dmin);
    float visibilityBound = 1.0f;
    float spotFactor = 1.0f;
    if (node.representative->IsA<USpotLightComponent>())
        spotFactor = ComputeSpotFactor(node.representative, regionMin, regionMax);

    return node.totalIntensity * materialBound * geometricBound * visibilityBound * spotFactor;
}

//------------------------------
// ComputeLightCutRecursive 함수: 재귀적으로 영역 AABB에 대해 LightCut 선택
//------------------------------
void LightTree::ComputeLightCutRecursive(const int nodeIndex, float boundThreshold,
    const FVector& regionMin, const FVector& regionMax,
    TArray<int>& cut) const {
    const LightTreeNode& node = nodes[nodeIndex];
    // 리프 노드인 경우 바로 선택
    if (node.leftIndex == -1 && node.rightIndex == -1) {
        cut.Add(nodeIndex);
        return;
    }
    float error = ComputeClusterError(node, regionMin, regionMax);
    if (error < boundThreshold) {
        cut.Add(nodeIndex);
    }
    else {
        if (node.leftIndex != -1)
            ComputeLightCutRecursive(node.leftIndex, boundThreshold, regionMin, regionMax, cut);
        if (node.rightIndex != -1)
            ComputeLightCutRecursive(node.rightIndex, boundThreshold, regionMin, regionMax, cut);
    }
}

//------------------------------
// ComputeLightCut 함수: public 함수, 영역 AABB와 오차 한계를 받아 LightCut 노드 인덱스 반환
//------------------------------
TArray<int> LightTree::ComputeLightCut(const FVector& regionMin, const FVector& regionMax, float boundThreshold) const {
    TArray<int> cut;
    if (nodes.Num() == 0) return cut;
    ComputeLightCutRecursive(static_cast<int>(nodes.Num()) - 1, boundThreshold, regionMin, regionMax, cut);
    return cut;
}

void LightTree::ComputeLightCut(UStaticMeshComponent* Mesh)
{
    if (Mesh) 
    {
        // Mesh의 AABB를 가져옵니다.
        const FBoundingBox& meshAABB = Mesh->AABB;
        FVector regionMin = meshAABB.min;
        FVector regionMax = meshAABB.max;
        // LightCut을 계산합니다.
        TArray<int> cut = ComputeLightCut(regionMin, regionMax, 0.1f);
        AddOrUpdateLightCut(Mesh, cut);
    }
}

//------------------------------
// GetNodes 함수: 내부 노드 목록 반환
//------------------------------
const TArray<LightTreeNode>& LightTree::GetNodes() const {
    return nodes;
}

void LightTree::AddOrUpdateLightCut(UStaticMeshComponent* Mesh, const TArray<int>& LightCutList)
{
    if (Mesh)
    {
        // 이미 값이 있으면 덮어쓰고, 없으면 새로 추가합니다.
        cutMap.Emplace(Mesh, LightCutList);
    }
}

const TArray<int>* LightTree::GetLightCut(UStaticMeshComponent* Mesh) const
{
    return cutMap.Find(Mesh);
}

const TArray<ULightComponentBase*> LightTree::GetLightCutLights(UStaticMeshComponent* Mesh) const
{
    TArray<ULightComponentBase*> lightsArray;
    const TArray<int>* cutArray = GetLightCut(Mesh);

    for (int i = 0; i < cutArray->Num(); i++) {
        lightsArray.Add(nodes[(*cutArray)[i]].representative);
    }

    return lightsArray;
}

void LightTree::RemoveLightCut(UStaticMeshComponent* Mesh)
{
    cutMap.Remove(Mesh);
}

void LightTree::ClearLightCut()
{
    cutMap.Empty();
}

void LightTree::ClearLightTree()
{
    nodes.Empty();
}
