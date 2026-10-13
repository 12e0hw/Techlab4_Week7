#pragma pack_matrix(row_major)

cbuffer TileCullConstants : register(b0)
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

StructuredBuffer<uint> PointTileListSRV : register(t0);
StructuredBuffer<uint> SpotTileListSRV : register(t1);

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

VSOutput mainVS(uint VertexID : SV_VertexID)
{
    VSOutput Out;
    float2 UV = float2((VertexID << 1) & 2, VertexID & 2);
    Out.Position = float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    Out.UV = UV;
    return Out;
}

float4 GetHeatMapColor(uint count)
{
    if (count == 0)
        return float4(0.05f, 0.05f, 0.1f, 0.15f); // 0개: 은은한 틴트 (15% 알파)
    if (count == 1)
        return float4(0.0f, 0.4f, 1.0f, 0.55f);   // 1개: 파랑
    if (count == 2)
        return float4(0.0f, 1.0f, 0.3f, 0.60f);   // 2개: 초록
    if (count == 3)
        return float4(1.0f, 0.8f, 0.0f, 0.65f);   // 3개: 노랑
    if (count == 4)
        return float4(1.0f, 0.4f, 0.0f, 0.70f);   // 4개: 주황
    return float4(1.0f, 0.1f, 0.1f, 0.75f);       // 5개 이상: 빨강
}

float4 mainPS(VSOutput Input) : SV_TARGET
{
    uint2 PixelCoord = uint2(Input.Position.xy);
    uint2 TileCoord = PixelCoord / 16;

    if (TileCoord.x >= TileCountX || TileCoord.y >= TileCountY)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);

    uint TileIndex = TileCoord.y * TileCountX + TileCoord.x;
    uint TileOffset = TileIndex * 65;

    uint PointCount = PointTileListSRV[TileOffset];
    uint SpotCount = SpotTileListSRV[TileOffset];
    uint TotalCount = PointCount + SpotCount;

    bool bIsBorder = (PixelCoord.x % 16 == 0) || (PixelCoord.y % 16 == 0);

    if (bIsBorder)
    {
        // 16x16 격자 경계선
        return float4(0.5f, 0.5f, 0.5f, 0.7f);
    }

    return GetHeatMapColor(TotalCount);
}
