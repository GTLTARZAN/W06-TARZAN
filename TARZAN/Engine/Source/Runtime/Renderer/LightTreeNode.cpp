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
// BuildTree: 입력 광원 목록으로 리프 노드를 생성하고 완벽한 이진 트리를 구축
//------------------------------
int LightTree::BuildTree(const std::vector<ULightComponentBase*> lights) {
    std::vector<LightTreeNode> leafNodes;
    for (const auto& light : lights) {
        leafNodes.push_back(CreateLeafNode(light));
    }
    int n = leafNodes.size();
    int nTotal = NextPowerOfTwo(n);
    while (leafNodes.size() < static_cast<size_t>(nTotal))
        leafNodes.push_back(CreateFakeLeafNode());

    // 노드들을 초기 LightTree 노드 목록에 복사
    nodes = leafNodes;

    int offset = 0;
    int levelCount = nTotal;
    while (levelCount > 1) {
        int nextLevelCount = levelCount / 2;
        for (int i = 0; i < levelCount; i += 2) {
            LightTreeNode leftNode = nodes[offset + i];
            LightTreeNode rightNode = nodes[offset + i + 1];
            LightTreeNode parent = MergeNodes(leftNode, rightNode);
            nodes.push_back(parent);
        }
        offset = nodes.size() - nextLevelCount;
        levelCount = nextLevelCount;
    }
    return nodes.size() - 1; // 루트 노드의 인덱스
}

//------------------------------
// Build 함수: 외부에서 호출, 광원 목록을 받아 LightTree를 구축
//------------------------------
void LightTree::Build(const std::vector<ULightComponentBase*> lights) {
    if (lights.empty()) {
        std::cerr << "광원 목록이 비어 있습니다.\n";
        return;
    }
    BuildTree(lights); // nodes 벡터 내부에 트리 구축
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
    std::vector<int>& cut) const {
    const LightTreeNode& node = nodes[nodeIndex];
    // 리프 노드인 경우 바로 선택
    if (node.leftIndex == -1 && node.rightIndex == -1) {
        cut.push_back(nodeIndex);
        return;
    }
    float error = ComputeClusterError(node, regionMin, regionMax);
    if (error < boundThreshold) {
        cut.push_back(nodeIndex);
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
std::vector<int> LightTree::ComputeLightCut(const FVector& regionMin, const FVector& regionMax, float boundThreshold) const {
    std::vector<int> cut;
    if (nodes.empty()) return cut;
    ComputeLightCutRecursive(static_cast<int>(nodes.size()) - 1, boundThreshold, regionMin, regionMax, cut);
    return cut;
}

//------------------------------
// GetNodes 함수: 내부 노드 목록 반환
//------------------------------
const std::vector<LightTreeNode>& LightTree::GetNodes() const {
    return nodes;
}