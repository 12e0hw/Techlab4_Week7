#pragma pack_matrix(row_major)

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

cbuffer TileLightCullingConstants : register(b0)
{
    matrix InverseViewProjection;
    uint ScreenWidth;
    uint ScreenHeight;
    uint TileCountX;
    uint TileCountY;
    uint NumPointLights;
    uint NumSpotLights;
    uint PointTileStride;
    uint SpotTileStride;
    uint bVisualizeLightCount;
    uint3 TileCullingPadding;
};

StructuredBuffer<FPointLightInfo> PointLightBuffer : register(t0);
StructuredBuffer<FSpotLightInfo> SpotLightBuffer : register(t1);
RWStructuredBuffer<uint> PointTileListBuffer : register(u0);
RWStructuredBuffer<uint> SpotTileListBuffer : register(u1);

groupshared uint PointVisibleCount;
groupshared uint PointVisibleIndices[64];
groupshared uint SpotVisibleCount;
groupshared uint SpotVisibleIndices[64];

groupshared float4 TileFrustumPlanes[6];

float4 MakePlaneFacingPoint(float3 A, float3 B, float3 C, float3 InsidePoint)
{
    float3 Normal = normalize(cross(B - A, C - A));
    float4 Plane = float4(Normal, -dot(Normal, A));
    if (dot(Plane, float4(InsidePoint, 1.0f)) < 0.0f)
    {
        Plane = -Plane;
    }
    return Plane;
}

bool IntersectsSpotFrustum(FSpotLightInfo Light, float4 FrustumPlanes[6])
{
    float Height = Light.AttenuationRadius;

    if (Height <= 0.0f)
    {
        return false;
    }

    float3 Direction = normalize(Light.Direction);

    float CosOuter = max(Light.CosOuterCone, 0.0001f);
    float SinOuter = sqrt(saturate(1.0f - CosOuter * CosOuter));
    float BaseRadius = Height * SinOuter / CosOuter;

    float3 BaseCenter = Light.Position + Direction * Height;

    float3 SphereCenter;
    float SphereRadius;

    if (BaseRadius <= Height)
    {
        float CenterDistance = (Height * Height + BaseRadius * BaseRadius) / (2.0f * Height);
        SphereCenter = Light.Position + Direction * CenterDistance;
        SphereRadius = CenterDistance;
    }
    else
    {
        SphereCenter = BaseCenter;
        SphereRadius = BaseRadius;
    }

    // 1차: 원뿔을 감싸는 구 검사
    for (uint SpherePlaneIndex = 0; SpherePlaneIndex < 6; ++SpherePlaneIndex)
    {
        float4 Plane = FrustumPlanes[SpherePlaneIndex];

        float SphereDistance = dot(Plane.xyz, SphereCenter) + Plane.w;

        if (SphereDistance < -SphereRadius)
        {
            return false;
        }
    }

    // 2차: 원뿔 검사
    for (uint ConePlaneIndex = 0; ConePlaneIndex < 6; ++ConePlaneIndex)
    {
        float4 Plane = FrustumPlanes[ConePlaneIndex];

        float ApexDistance = dot(Plane.xyz, Light.Position) + Plane.w;
        
        float NormalAlongDirection = dot(Plane.xyz, Direction);
        float NormalPerpendicularLength = sqrt(saturate(1.0f - NormalAlongDirection * NormalAlongDirection));

        float BaseMaximumDistance = dot(Plane.xyz, BaseCenter) + Plane.w + BaseRadius * NormalPerpendicularLength;
        
        float ConeMaximumDistance = max(ApexDistance, BaseMaximumDistance);
        if (ConeMaximumDistance < 0.0f)
        {
            return false;
        }
    }

    return true;
}

[numthreads(64, 1, 1)]
void mainCS(uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    if (GroupID.x >= TileCountX || GroupID.y >= TileCountY)
    {
        return;
    }
    
    uint TileIndex = GroupID.y * TileCountX + GroupID.x;  // 2차원 배열 변환
    
    if (GroupIndex == 0)
    {        
        uint TileX = GroupID.x;
        uint TileY = GroupID.y;
    
        uint MinX = TileX * 16;
        uint MinY = TileY * 16;
        uint MaxX = min(MinX + 16, ScreenWidth);
        uint MaxY = min(MinY + 16, ScreenHeight);
    
        // -1~1 변환
        float LeftNdc = (float) MinX / (float) ScreenWidth * 2.0f - 1.0f;
        float RightNdc = (float) MaxX / (float) ScreenWidth * 2.0f - 1.0f;
        float TopNdc = 1.0f - (float) MinY / (float) ScreenHeight * 2.0f;
        float BottomNdc = 1.0f - (float) MaxY / (float) ScreenHeight * 2.0f;
    
        float2 NdcCorners[4] =
        {
            float2(LeftNdc, TopNdc),
            float2(RightNdc, TopNdc),
            float2(LeftNdc, BottomNdc),
            float2(RightNdc, BottomNdc)
        };
    
        float3 NearCorners[4];
        float3 FarCorners[4];
        for (uint Index = 0; Index < 4; ++Index)
        {
            float4 NearPosition = float4(NdcCorners[Index], 0.0f, 1.0f);
            float4 FarPosition = float4(NdcCorners[Index], 1.0f, 1.0f);
            float4 NearWorld = mul(NearPosition, InverseViewProjection);
            float4 FarWorld = mul(FarPosition, InverseViewProjection);
            NearCorners[Index] = NearWorld.xyz / NearWorld.w;
            FarCorners[Index] = FarWorld.xyz / FarWorld.w;
        }
        float3 FrustumCenter = 0.0f;
        for (uint CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
        {
            FrustumCenter += NearCorners[CornerIndex] + FarCorners[CornerIndex];
        }
        FrustumCenter *= 1.0f / 8.0f;
        TileFrustumPlanes[0] = MakePlaneFacingPoint(NearCorners[0], NearCorners[2], FarCorners[2], FrustumCenter);
        TileFrustumPlanes[1] = MakePlaneFacingPoint(NearCorners[3], NearCorners[1], FarCorners[1], FrustumCenter);
        TileFrustumPlanes[2] = MakePlaneFacingPoint(NearCorners[1], NearCorners[0], FarCorners[0], FrustumCenter);
        TileFrustumPlanes[3] = MakePlaneFacingPoint(NearCorners[2], NearCorners[3], FarCorners[3], FrustumCenter);
        TileFrustumPlanes[4] = MakePlaneFacingPoint(NearCorners[0], NearCorners[1], NearCorners[3], FrustumCenter);
        TileFrustumPlanes[5] = MakePlaneFacingPoint(FarCorners[1], FarCorners[0], FarCorners[2], FrustumCenter);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (GroupIndex == 0)
    {
        PointVisibleCount = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    for (uint PointLightIndex = GroupIndex; PointLightIndex < NumPointLights; PointLightIndex += 64)
    {
        FPointLightInfo Light = PointLightBuffer[PointLightIndex];

        bool bIntersects = true;

        for (uint PlaneIndex = 0; PlaneIndex < 6; ++PlaneIndex)
        {
            float4 Plane = TileFrustumPlanes[PlaneIndex];
            float SignedDistance = dot(Plane.xyz, Light.Position) + Plane.w;

            if (SignedDistance < -Light.AttenuationRadius)
            {
                bIntersects = false;
                break;
            }
        }

        if (bIntersects)
        {
            uint WriteIndex;
            InterlockedAdd(PointVisibleCount, 1, WriteIndex);

            if (WriteIndex < 64)
            {
                PointVisibleIndices[WriteIndex] = PointLightIndex;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();
    
    uint PointListStart = TileIndex * PointTileStride;
    uint StoredPointCount = min(PointVisibleCount, 64u);

    if (GroupIndex == 0)
    {
        PointTileListBuffer[PointListStart] = StoredPointCount;
    }

    if (GroupIndex < StoredPointCount)
    {
        PointTileListBuffer[PointListStart + 1 + GroupIndex] = PointVisibleIndices[GroupIndex];
    }
    
    if (GroupIndex == 0)
        SpotVisibleCount = 0;

    GroupMemoryBarrierWithGroupSync();

    for (uint SpotLightIndex = GroupIndex; SpotLightIndex < NumSpotLights; SpotLightIndex += 64)
    {
        FSpotLightInfo Light = SpotLightBuffer[SpotLightIndex];

        if (IntersectsSpotFrustum(Light, TileFrustumPlanes))
        {
            uint WriteIndex;
            InterlockedAdd(SpotVisibleCount, 1, WriteIndex);

            if (WriteIndex < 64)
            {
                SpotVisibleIndices[WriteIndex] = SpotLightIndex;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    uint SpotListStart = TileIndex * SpotTileStride;
    uint StoredSpotCount = min(SpotVisibleCount, 64u);

    if (GroupIndex == 0)
    {
        SpotTileListBuffer[SpotListStart] = StoredSpotCount;
    }
    
    if (GroupIndex < StoredSpotCount)
    {
        SpotTileListBuffer[SpotListStart + 1 + GroupIndex] = SpotVisibleIndices[GroupIndex];
    }
    
}