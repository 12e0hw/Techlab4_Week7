#pragma pack_matrix(row_major)

#define THREADS_PER_TILE 256
#define MAX_POINT_LIGHTS_PER_TILE 64
#define MAX_SPOT_LIGHTS_PER_TILE 64

cbuffer CullConstants : register(b0)
{
    matrix InverseViewProjection;
    float3 CameraPosition;
    float Padding0;
    float2 ScreenSize;
    uint TileCountX;
    uint TileCountY;
    uint NumPointLights;
    uint NumSpotLights;
    float2 Padding1;
};

struct FPointLightInfo
{
    float3 Color;
    float AttenuationRadius;
    float3 Position;
    float Padding;
};

struct FSpotLightInfo
{
    float3 Color;
    float AttenuationRadius;
    float3 Position;
    float CosInnerCone;
    float3 Direction;
    float CosOuterCone;
};

StructuredBuffer<FPointLightInfo> PointLights : register(t0);
StructuredBuffer<FSpotLightInfo> SpotLights : register(t1);

RWStructuredBuffer<uint> PointTileListUAV : register(u0);
RWStructuredBuffer<uint> SpotTileListUAV : register(u1);

groupshared uint s_PointLightCount;
groupshared uint s_PointLightIndices[MAX_POINT_LIGHTS_PER_TILE];

groupshared uint s_SpotLightCount;
groupshared uint s_SpotLightIndices[MAX_SPOT_LIGHTS_PER_TILE];

groupshared float4 s_FrustumPlanes[4];

// NDC -> World Space 역투영
float3 ScreenToWorld(float4 screenPos)
{
    float4 worldPos = mul(screenPos, InverseViewProjection);
    return worldPos.xyz / worldPos.w;
}

[numthreads(16, 16, 1)]
void CullLightsCS(uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    if (GroupIndex == 0)
    {
        s_PointLightCount = 0;
        s_SpotLightCount = 0;
        
        // 1. 현재 타일의 스크린 픽셀 범위 구하기 (16x16 크기)
        float2 tileMin = float2(GroupID.xy * 16);
        float2 tileMax = tileMin + float2(16.0f, 16.0f);
        
        // 2. 픽셀 좌표 -> NDC 좌표 (-1 ~ 1)로 변환
        float2 ndcMin = float2(tileMin.x / ScreenSize.x * 2.0f - 1.0f, 1.0f - tileMax.y / ScreenSize.y * 2.0f);
        float2 ndcMax = float2(tileMax.x / ScreenSize.x * 2.0f - 1.0f, 1.0f - tileMin.y / ScreenSize.y * 2.0f);
        
        // 3. 타일의 네 모서리를 월드 공간 점으로 역투영
        float3 frustumCorner[4];
        frustumCorner[0] = ScreenToWorld(float4(ndcMin.x, ndcMin.y, 1.0f, 1.0f)); // 좌하단
        frustumCorner[1] = ScreenToWorld(float4(ndcMax.x, ndcMin.y, 1.0f, 1.0f)); // 우하단
        frustumCorner[2] = ScreenToWorld(float4(ndcMin.x, ndcMax.y, 1.0f, 1.0f)); // 좌상단
        frustumCorner[3] = ScreenToWorld(float4(ndcMax.x, ndcMax.y, 1.0f, 1.0f)); // 우상단
        
        // 4. 카메라 위치에서 모서리로 뻗어나가는 방향 벡터
        float3 dir0 = frustumCorner[0] - CameraPosition;
        float3 dir1 = frustumCorner[1] - CameraPosition;
        float3 dir2 = frustumCorner[2] - CameraPosition;
        float3 dir3 = frustumCorner[3] - CameraPosition;

        // 5. 월드 공간 타일 4개 평면 법선(Normal) 구하기 (타일 안쪽을 향하도록)
        s_FrustumPlanes[0] = float4(normalize(cross(dir2, dir0)), 0.0f); // Left
        s_FrustumPlanes[1] = float4(normalize(cross(dir1, dir3)), 0.0f); // Right
        s_FrustumPlanes[2] = float4(normalize(cross(dir3, dir2)), 0.0f); // Top
        s_FrustumPlanes[3] = float4(normalize(cross(dir0, dir1)), 0.0f); // Bottom
    }
    GroupMemoryBarrierWithGroupSync();

    for (uint i = GroupIndex; i < NumPointLights; i += THREADS_PER_TILE)
    {
        FPointLightInfo light = PointLights[i];
        float3 toLight = light.Position - CameraPosition;
        float radius = light.AttenuationRadius;

        bool bInFrustum = true;
        [unroll]
        for (int p = 0; p < 4; ++p)
        {
            if (dot(s_FrustumPlanes[p].xyz, toLight) < -radius)
            {
                bInFrustum = false;
                break;
            }
        }
        if (bInFrustum)
        {
            uint slot;
            InterlockedAdd(s_PointLightCount, 1, slot);
            if (slot < MAX_POINT_LIGHTS_PER_TILE)
            {
                s_PointLightIndices[slot] = i;
            }
        }
    }

    for (uint j = GroupIndex; j < NumSpotLights; j += THREADS_PER_TILE)
    {
        uint Slot;
        InterlockedAdd(s_SpotLightCount, 1, Slot);
        if (Slot < MAX_SPOT_LIGHTS_PER_TILE)
        {
            s_SpotLightIndices[Slot] = j;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // 최종 결과를 전역 버퍼(UAV)에 기록
    uint TileIndex = GroupID.y * TileCountX + GroupID.x;

    uint PointTileOffset = TileIndex * (MAX_POINT_LIGHTS_PER_TILE + 1);
    if (GroupIndex == 0)
    {
        // 첫 번째 칸(Index 0)에 최종 조명 개수 기록
        PointTileListUAV[PointTileOffset] = min(s_PointLightCount, MAX_POINT_LIGHTS_PER_TILE);
    }
    // 그 뒤 칸들에 조명 인덱스들 복사
    for (uint k = GroupIndex; k < s_PointLightCount && k < MAX_POINT_LIGHTS_PER_TILE; k += THREADS_PER_TILE)
    {
        PointTileListUAV[PointTileOffset + 1 + k] = s_PointLightIndices[k];
    }

    uint SpotTileOffset = TileIndex * (MAX_SPOT_LIGHTS_PER_TILE + 1);
    if (GroupIndex == 0)
    {
        SpotTileListUAV[SpotTileOffset] = min(s_SpotLightCount, MAX_SPOT_LIGHTS_PER_TILE);
    }
    for (uint m = GroupIndex; m < s_SpotLightCount && m < MAX_SPOT_LIGHTS_PER_TILE; m += THREADS_PER_TILE)
    {
        SpotTileListUAV[SpotTileOffset + 1 + m] = s_SpotLightIndices[m];
    }
}