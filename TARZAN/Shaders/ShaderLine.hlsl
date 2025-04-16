cbuffer MatrixBuffer : register(b0)
{
    row_major float4x4 MVP;
    row_major float4x4 ModelMatrix;
    row_major float4x4 NormalMatrix;
    float4 CamPos;
};

cbuffer GridParametersData : register(b1)
{
    float GridSpacing;
    int GridCount; // 총 grid 라인 수
    float3 GridOrigin; // Grid의 중심
    float Padding;
};

cbuffer ScreenInfo : register(b2)
{
    float2 ViewPortRatio;
    float2 ViewPortPosition;
}
    
cbuffer PrimitiveCounts : register(b3)
{
    int BoundingBoxCount; // 렌더링할 AABB의 개수
    int pad;
    int ConeCount; // 렌더링할 cone의 개수
    int OBBCount;
    int CircleCount; // 렌더링할 Circle의 개수
};
struct FBoundingBoxData
{
    float3 bbMin;
    float padding0;
    float3 bbMax;
    float padding1;
};
struct FConeData
{
    float3 ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름
    
    float3 ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    float4 Color;
    
    int ConeSegmentCount; // 원뿔 밑면 분할 수
    float3 pad;
};

struct FCircleData
{
    float3 CircleApex; // Circle 상의 한 점
    float CircleRadius; // Circle의 반지름
    float3 CircleBaseCenter; // Circle의 중심점
    int CircleSegmentCount; // Circle의 분할 수
    float4 Color;
};

struct FOrientedBoxCornerData
{
    float3 corners[8]; // 회전/이동 된 월드 공간상의 8꼭짓점
};

struct FLineData
{
    float3 LineStart;
    float pad0;
    float3 LineEnd;
    float pad1;
    float4 Color;
};

StructuredBuffer<FBoundingBoxData> g_BoundingBoxes : register(t2);
StructuredBuffer<FConeData> g_ConeData : register(t3);
StructuredBuffer<FOrientedBoxCornerData> g_OrientedBoxes : register(t4);
StructuredBuffer<FCircleData> g_Circles : register(t5);
StructuredBuffer<FLineData> g_Lines : register(t6);

static const int BB_EdgeIndices[12][2] =
{
    { 0, 1 },
    { 1, 3 },
    { 3, 2 },
    { 2, 0 }, // 앞면
    { 4, 5 },
    { 5, 7 },
    { 7, 6 },
    { 6, 4 }, // 뒷면
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 } // 측면
};

struct VS_INPUT
{
    uint vertexID : SV_VertexID; // 0 또는 1: 각 라인의 시작과 끝
    uint instanceID : SV_InstanceID; // 인스턴스 ID로 grid, axis, bounding box를 구분
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float4 WorldPos : TEXCOORD0;
};

/////////////////////////////////////////////////////////////////////////
// Grid 위치 계산 함수
/////////////////////////////////////////////////////////////////////////
float3 ComputeGridPosition(uint instanceID, uint vertexID)
{
    int halfCount = GridCount / 2;
    float centerOffset = halfCount * 0.5; // grid 중심이 원점에 오도록

    float3 startPos;
    float3 endPos;
    
    if (instanceID < halfCount)
    {
        // 수직선: X 좌표 변화, Y는 -centerOffset ~ +centerOffset
        float x = GridOrigin.x + (instanceID - centerOffset) * GridSpacing;
        if (abs(x - GridOrigin.x) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(0, (GridOrigin.y - centerOffset * GridSpacing), 0);
        }
        else
        {
            startPos = float3(x, GridOrigin.y - centerOffset * GridSpacing, GridOrigin.z);
            endPos = float3(x, GridOrigin.y + centerOffset * GridSpacing, GridOrigin.z);
        }
    }
    else
    {
        // 수평선: Y 좌표 변화, X는 -centerOffset ~ +centerOffset
        int idx = instanceID - halfCount;
        float y = GridOrigin.y + (idx - centerOffset) * GridSpacing;
        if (abs(y - GridOrigin.y) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(-(GridOrigin.x + centerOffset * GridSpacing), 0, 0);
        }
        else
        {
            startPos = float3(GridOrigin.x - centerOffset * GridSpacing, y, GridOrigin.z);
            endPos = float3(GridOrigin.x + centerOffset * GridSpacing, y, GridOrigin.z);
        }

    }
    return (vertexID == 0) ? startPos : endPos;
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////
float3 ComputeAxisPosition(uint axisInstanceID, uint vertexID)
{
    float3 start = float3(0.0, 0.0, 0.0);
    float3 end;
    float zOffset = 0.f;
    if (axisInstanceID == 0)
    {
        // X 축: 빨간색
        end = float3(1000000.0, 0.0, zOffset);
    }
    else if (axisInstanceID == 1)
    {
        // Y 축: 초록색
        end = float3(0.0, 1000000.0, zOffset);
    }
    else if (axisInstanceID == 2)
    {
        // Z 축: 파란색
        end = float3(0.0, 0.0, 1000000.0 + zOffset);
    }
    else
    {
        end = start;
    }
    return (vertexID == 0) ? start : end;
}

/////////////////////////////////////////////////////////////////////////
// Bounding Box 위치 계산 함수
// bbInstanceID: StructuredBuffer에서 몇 번째 bounding box인지
// edgeIndex: 해당 bounding box의 12개 엣지 중 어느 것인지
/////////////////////////////////////////////////////////////////////////
float3 ComputeBoundingBoxPosition(uint bbInstanceID, uint edgeIndex, uint vertexID)
{
    FBoundingBoxData box = g_BoundingBoxes[bbInstanceID];
  
//    0: (bbMin.x, bbMin.y, bbMin.z)
//    1: (bbMax.x, bbMin.y, bbMin.z)
//    2: (bbMin.x, bbMax.y, bbMin.z)
//    3: (bbMax.x, bbMax.y, bbMin.z)
//    4: (bbMin.x, bbMin.y, bbMax.z)
//    5: (bbMax.x, bbMin.y, bbMax.z)
//    6: (bbMin.x, bbMax.y, bbMax.z)
//    7: (bbMax.x, bbMax.y, bbMax.z)
    int vertIndex = BB_EdgeIndices[edgeIndex][vertexID];
    float x = ((vertIndex & 1) == 0) ? box.bbMin.x : box.bbMax.x;
    float y = ((vertIndex & 2) == 0) ? box.bbMin.y : box.bbMax.y;
    float z = ((vertIndex & 4) == 0) ? box.bbMin.z : box.bbMax.z;
    return float3(x, y, z);
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////
// Cone 계산 함수
/////////////////////////////////////////////////
// Helper: 계산된 각도에 따른 밑면 꼭짓점 위치 계산

float3 ComputeConePosition(uint globalInstanceID, uint vertexID)
{
    // 모든 cone이 동일한 세그먼트 수를 가짐
    int N = 24;//g_ConeData[0].ConeSegmentCount;
    
    uint coneIndex = globalInstanceID / (2 * N + 10);
    uint lineIndex = globalInstanceID % (2 * N + 10);
    
    // cone 데이터 읽기
    FConeData cone = g_ConeData[coneIndex];
    
    // cone의 축 계산
    float3 axis = normalize(cone.ConeApex - cone.ConeBaseCenter);
    
    // axis에 수직인 두 벡터(u, v)를 생성
    float3 arbitrary = abs(dot(axis, float3(0, 0, 1))) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0);
    float3 u = normalize(cross(axis, arbitrary));
    float3 v = cross(axis, u);
    
    if (lineIndex < (uint) N)
    {
        // 측면 선분: cone의 꼭짓점과 밑면의 한 점을 잇는다.
        float angle = lineIndex * 6.28318530718 / N;
        float3 baseVertex = cone.ConeBaseCenter + (cos(angle) * u + sin(angle) * v) * cone.ConeRadius;
        return (vertexID == 0) ? cone.ConeApex : baseVertex;
    }
    else if (lineIndex < (uint) 2 * N)
    {
        // 밑면 둘레 선분: 밑면상의 인접한 두 점을 잇는다.
        uint idx = lineIndex - N;
        float angle0 = idx * 6.28318530718 / N;
        float angle1 = ((idx + 1) % N) * 6.28318530718 / N;
        float3 v0 = cone.ConeBaseCenter + (cos(angle0) * u + sin(angle0) * v) * cone.ConeRadius;
        float3 v1 = cone.ConeBaseCenter + (cos(angle1) * u + sin(angle1) * v) * cone.ConeRadius;
        return (vertexID == 0) ? v0 : v1;
    }
    else
    {
        uint idx = lineIndex - 2 * N;

        float radius = sqrt(cone.ConeRadius * cone.ConeRadius + cone.ConeHeight * cone.ConeHeight);
        
        // Cone의 각도 계산 (라디안)
        float coneAngle = atan2(cone.ConeRadius, cone.ConeHeight);
        
        // Forward 벡터 (Cone의 방향)
        float3 forward = normalize(cone.ConeApex - cone.ConeBaseCenter);
        
        // 임시 Up 벡터 (일반적으로 (0,1,0)을 사용)
        float3 tempUp = float3(0, 1, 0);
        
        // Forward와 Up이 거의 평행한 경우를 대비해 다른 Up 벡터 사용
        if (abs(dot(forward, tempUp)) > 0.99)
        {
            tempUp = float3(0, 0, 1);
        }
        
        // Right 벡터 계산 (Forward와 Up의 외적)
        float3 right = normalize(cross(forward, tempUp));
        
        // 최종 Up 벡터 계산 (Right와 Forward의 외적)
        float3 up = normalize(cross(right, forward));

        // Local Y축을 중심으로 하는 원 (Right와 Up 벡터 사용)
        if (idx < 5)
        {
            float angle0 = coneAngle - (idx * 2.0f * coneAngle / 5.0f) + 3.14159265359f;
            float angle1 = coneAngle - ((idx + 1) * 2.0f * coneAngle / 5.0f) + 3.14159265359f;
            
            float3 v0 = cone.ConeApex + (cos(angle0) * forward + sin(angle0) * up) * radius;
            float3 v1 = cone.ConeApex + (cos(angle1) * forward + sin(angle1) * up) * radius;
            return (vertexID == 0) ? v0 : v1;
        }
        // Local Z축을 중심으로 하는 원 (Forward와 Right 벡터 사용)
        else
        {
            idx -= 5;
            float angle0 = coneAngle - (idx * 2.0f * coneAngle / 5.0f) + 3.14159265359f;
            float angle1 = coneAngle - ((idx + 1) * 2.0f * coneAngle / 5.0f) + 3.14159265359f;
            
            float3 v0 = cone.ConeApex + (cos(angle0) * forward + sin(angle0) * right) * radius;
            float3 v1 = cone.ConeApex + (cos(angle1) * forward + sin(angle1) * right) * radius;
            return (vertexID == 0) ? v0 : v1;
        }
    }
}

/////////////////////////////////////////////////
// Circle 계산 함수
/////////////////////////////////////////////////
float3 ComputeCirclePosition(uint globalInstanceID, uint vertexID)
{
    int N = g_Circles[0].CircleSegmentCount;
    
    uint circleIndex = globalInstanceID / (3 * N);
    uint lineIndex = globalInstanceID % (3 * N);
    
    uint axisIndex = (globalInstanceID % (3 * N)) / N;
    
    FCircleData circle = g_Circles[circleIndex];
    
    float3 axis;
    if (axisIndex == 0)
    {
        axis = float3(1, 0, 0);
    }
    else if (axisIndex == 1)
    {
        axis = float3(0, 1, 0);
    }
    else
    {
        axis = float3(0, 0, 1);
    }
    
    float3 arbitrary = abs(dot(axis, float3(0, 0, 1))) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0);
    float3 u = normalize(cross(axis, arbitrary));
    float3 v = cross(axis, u);
    
    uint idx = lineIndex;
    float angle0 = idx * 6.28318530718 / N;
    float angle1 = ((idx + 1) % N) * 6.28318530718 / N;
    float3 v0 = circle.CircleBaseCenter + (cos(angle0) * u + sin(angle0) * v) * circle.CircleRadius;
    float3 v1 = circle.CircleBaseCenter + (cos(angle1) * u + sin(angle1) * v) * circle.CircleRadius;
    
    return (vertexID == 0) ? v0 : v1;
}


/////////////////////////////////////////////////////////////////////////
// OBB
/////////////////////////////////////////////////////////////////////////
float3 ComputeOrientedBoxPosition(uint obIndex, uint edgeIndex, uint vertexID)
{
    FOrientedBoxCornerData ob = g_OrientedBoxes[obIndex];
    int cornerID = BB_EdgeIndices[edgeIndex][vertexID];
    return ob.corners[cornerID];
}

float3 ComputePerpVector(float3 lineDir)
{
    // 먼저 world up 벡터를 사용
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 perp = cross(lineDir, up);
    // 만약 perp 벡터가 0에 가까우면, 다른 up 벡터를 사용
    if (length(perp) < 1e-5f)
    {
        up = float3(0.0f, 0.0f, 1.0f);
        perp = cross(lineDir, up);
    }
    return normalize(perp);
}

/////////////////////////////////////////////////////////////////////////
// 메인 버텍스 셰이더
/////////////////////////////////////////////////////////////////////////
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    float3 pos;
    float4 color;

    int ConeSegmentCount = 24;
    
    // Cone 하나당 (2 * SegmentCount) 선분.
    // ConeCount 개수만큼이므로 총 (2 * SegmentCount * ConeCount).
    uint coneInstCnt = ConeCount * (2 * ConeSegmentCount + 10);
    uint obbInstCnt = OBBCount * 12;
    uint circleInstCnt = CircleCount * (3 * g_Circles[0].CircleSegmentCount);

    // Grid / Axis / AABB 인스턴스 개수 계산
    uint gridLineCount = GridCount; // 그리드 라인
    uint axisCount = 3; // X, Y, Z 축 (월드 좌표축)
    uint aabbInstanceCount = 12 * BoundingBoxCount; // AABB 하나당 12개 엣지

    // 1) "콘 인스턴스 시작" 지점
    uint coneInstanceStart = gridLineCount + axisCount + aabbInstanceCount;
    // 2) 그 다음(=콘 구간의 끝)이 곧 OBB 시작 지점
    uint obbInstanceStart = coneInstanceStart + coneInstCnt;
    // 3) 그 다음(= obb 구간의 끝)이 곧 Circle 시작 지점
    uint circleInstanceStart = obbInstanceStart + obbInstCnt;
    
    uint lineInstanceStart = circleInstanceStart + circleInstCnt;
    
    // 이제 instanceID를 기준으로 분기
    if (input.instanceID < gridLineCount)
    {
        // 0 ~ (GridCount-1): 그리드
        pos = ComputeGridPosition(input.instanceID, input.vertexID);
        color = float4(0.1, 0.1, 0.1, 1.0);
    }
    else if (input.instanceID < gridLineCount + axisCount)
    {
        // 그 다음 (axisCount)개: 축(Axis)
        uint axisInstanceID = input.instanceID - gridLineCount;
        pos = ComputeAxisPosition(axisInstanceID, input.vertexID);

        // 축마다 색상
        if (axisInstanceID == 0)
            color = float4(1.0, 0.0, 0.0, 1.0); // X: 빨강
        else if (axisInstanceID == 1)
            color = float4(0.0, 1.0, 0.0, 1.0); // Y: 초록
        else
            color = float4(0.0, 0.0, 1.0, 1.0); // Z: 파랑
    }
    else if (input.instanceID < gridLineCount + axisCount + aabbInstanceCount)
    {
        // 그 다음 AABB 인스턴스 구간
        uint index = input.instanceID - (gridLineCount + axisCount);
        uint bbInstanceID = index / 12; // 12개가 1박스
        uint bbEdgeIndex = index % 12;
        
        pos = ComputeBoundingBoxPosition(bbInstanceID, bbEdgeIndex, input.vertexID);
        color = float4(1.0, 1.0, 0.0, 1.0); // 노란색
    }
    else if (input.instanceID < obbInstanceStart)
    {
        // 그 다음 콘(Cone) 구간
        uint coneInstanceID = input.instanceID - coneInstanceStart;
        pos = ComputeConePosition(coneInstanceID, input.vertexID);
        uint coneIndex = coneInstanceID / (2 * ConeSegmentCount + 10);
        
        color = g_ConeData[coneIndex].Color;
    }
    else if (input.instanceID < circleInstanceStart)
    {
        uint obbLocalID = input.instanceID - obbInstanceStart;
        uint obbIndex = obbLocalID / 12;
        uint edgeIndex = obbLocalID % 12;

        pos = ComputeOrientedBoxPosition(obbIndex, edgeIndex, input.vertexID);
        color = float4(0.4, 1.0, 0.4, 1.0); // 예시: 연두색
    }
    else if (input.instanceID < lineInstanceStart)
    {
        uint circleInstanceID = input.instanceID - circleInstanceStart;
        pos = ComputeCirclePosition(circleInstanceID, input.vertexID);
        uint circleIndex = circleInstanceID / (3 * g_Circles[0].CircleSegmentCount);
        
        color = g_Circles[circleIndex].Color;
    }
    else
    {
        uint lineInstanceID = input.instanceID - lineInstanceStart;
        uint lineIndex = lineInstanceID / 10;
        uint segmentID = (lineInstanceID % 10) / 2;
        
        float3 basePos = input.vertexID == 0 ? g_Lines[lineIndex].LineStart : g_Lines[lineIndex].LineEnd;
        float3 lineDir = normalize(g_Lines[lineIndex].LineEnd - g_Lines[lineIndex].LineStart);
        float3 perp = ComputePerpVector(lineDir);
        float offsetDistance = 0.3f;
        float3 finalPos = basePos;
        if (segmentID == 1)
        {
            finalPos += offsetDistance * perp;
            if (input.vertexID == 1)
            {
                finalPos += -lineDir * 0.5f;
            }
        }
        else if (segmentID == 2)
        {
            finalPos -= offsetDistance * perp;
            if (input.vertexID == 1)
            {
                finalPos += -lineDir * 0.5f;
            }
        }
        else if (segmentID == 3)
        {
            finalPos += 2 * offsetDistance * perp;
        }
        else if (segmentID == 4)
        {
            finalPos -= 2 * offsetDistance * perp;
        }
    
        pos = finalPos;
        color = g_Lines[lineIndex].Color;
    }

    // 출력 변환
    output.WorldPos = float4(pos, 1.0);
    output.Position = mul(float4(pos, 1.0), MVP);
    output.Color = color;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    float3 worldPos = input.WorldPos.xyz;
    float3 camPos = CamPos.xyz;

    camPos.z = 0;
    worldPos.z = 0;
    
    float dist = length(worldPos - camPos);

    // 지수 기반 페이드 아웃 - 더 자연스러운 안개 느낌
    float startDist = 80;
    float density = 0.02f; // 밀도 조절
    
    float fadeDist = max(0, dist - startDist);
    float fogFactor = saturate(1.0f - exp(-density * fadeDist));
    
    float alpha = 1.0f - fogFactor;
    
    float4 color = float4(input.Color.xyz, alpha);
    
    return color;
}
