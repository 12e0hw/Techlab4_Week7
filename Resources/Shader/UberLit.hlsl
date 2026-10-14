#pragma pack_matrix(row_major)

// EViewMode 순서와 일치해야 한다.
#define VIEWMODE_LIT          0
#define VIEWMODE_UNLIT        1
#define VIEWMODE_WORLDNORMAL  3

// ELightingModel 순서와 일치해야 한다.
#define LIGHTING_MODEL_LAMBERT      0
#define LIGHTING_MODEL_PHONG        1
#define LIGHTING_MODEL_BLINNPHONG   2

// EShadingModel 순서와 일치해야 한다.
#define SHADING_MODEL_GOURAUD  0
#define SHADING_MODEL_PHONG    1

// 기본값: Lit, Blinn-Phong, Phong
#ifndef VIEWMODE
#define VIEWMODE VIEWMODE_LIT
#endif

#ifndef LIGHTING_MODEL
#define LIGHTING_MODEL LIGHTING_MODEL_BLINNPHONG
#endif

#ifndef SHADING_MODEL
#define SHADING_MODEL SHADING_MODEL_PHONG
#endif

#ifndef HAS_NORMAL_MAP
#define HAS_NORMAL_MAP 0
#endif
#define USE_NORMAL_MAP (SHADING_MODEL == SHADING_MODEL_PHONG && HAS_NORMAL_MAP)

#define GOURAUD (VIEWMODE == VIEWMODE_LIT && SHADING_MODEL == SHADING_MODEL_GOURAUD)

#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 4

struct FAmbientLightInfo
{
    float3 Color;
    float Padding;
};

struct FDirectionalLightInfo
{
    float3 Color;
    float Padding0;
    float3 Direction;
    float Padding1;
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

struct FVSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float4 Color : COLOR; // 지금은 쓰지 않지만 나중에 쓰게 될 수 있음
    float2 UV : TEXCOORD;
};

struct FPSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 WorldPosition : TEXCOORD1;
#if USE_NORMAL_MAP
    float3 Tangent : TANGENT;
    int BitangentSign : BITANGENT_SIGN;
#endif
#if GOURAUD
    float3 DiffuseLight : TEXCOORD2;
    float3 SpecularLight : TEXCOORD3;
#endif
};

cbuffer ViewConstants : register(b0)
{
    matrix ViewProjection;
    float3 CameraPosition;
    float ViewPadding;
};

cbuffer StaticMeshMaterialParams : register(b1)
{
    float4 BaseColor;
    float2 UVScrollOffset;
    uint bOpaque;
    float Shininess;
    float3 SpecularColor;
    uint bFlipNormalGreen;
};

cbuffer PerObjectConstants : register(b2)
{
    matrix World;
    matrix NormalMatrix; // World의 역전치
};

cbuffer LightingConstants : register(b3)
{
    FAmbientLightInfo AmbientLight;
    FDirectionalLightInfo DirectionalLight;
    FPointLightInfo PointLights[MAX_POINT_LIGHTS];
    FSpotLightInfo SpotLights[MAX_SPOT_LIGHTS];
    uint NumPointLights;
    uint NumSpotLights;
    float2 LightingPadding;
};

cbuffer TileLightConstants : register(b4)
{
    matrix TileInverseViewProjection;
    uint TileScreenWidth;
    uint TileScreenHeight;
    uint TileCountX;
    uint TileCountY;
    uint StructuredPointLightCount;
    uint StructuredSpotLightCount;
    uint PointTileStride;
    uint SpotTileStride;
    uint bVisualizeLightCount;
    uint3 TileCullingPadding;
};

StructuredBuffer<FPointLightInfo> StructuredPointLights : register(t8);
StructuredBuffer<FSpotLightInfo> StructuredSpotLights : register(t9);
StructuredBuffer<uint> PointTileList : register(t10);
StructuredBuffer<uint> SpotTileList : register(t11);

Texture2D AlbedoTexture : register(t0);

#if USE_NORMAL_MAP
Texture2D NormalTexture : register(t1);
#endif

SamplerState AlbedoSampler : register(s0);

float3 DiffuseTerm(float3 Il, float3 N, float3 L)
{
    return Il * max(dot(N, L), 0.0f);
}

// Phong: A(R), B(V) Blinn-Phong: A(N), B(H)
float3 SpecularTerm(float3 Il, float3 A, float3 B, float n)
{
    return Il * pow(max(dot(A, B), 0.0f), n);
}

float ComputeDistanceAttenuation(float Distance, float Radius)
{
    return pow(saturate(1.0f - pow(Distance / Radius, 4)), 2);
}

void AccumulateLight(float3 Il, float3 N, float3 L, float3 V, float n, inout float3 DiffuseLight, inout float3 SpecularLight)
{
    float NdotL = max(dot(N, L), 0.0f);
    DiffuseLight += Il * NdotL; // Lambert
    if (NdotL > 0.0f)
    {
#if LIGHTING_MODEL == LIGHTING_MODEL_PHONG
        float3 R = 2.0f * dot(N, L) * N - L;
        // ★ NdotL을 곱해 빛이 스치는 어두운 곳에서는 스펙큘러가 0으로 부드럽게 사라지게 함
        SpecularLight += SpecularTerm(Il, R, V, n) * NdotL;
#elif LIGHTING_MODEL == LIGHTING_MODEL_BLINNPHONG
        float3 H = normalize(L + V);
        SpecularLight += SpecularTerm(Il, N, H, n) * NdotL;
#endif
    }
}

float3 GetLightCountColor(uint LightCount)
{
    if (LightCount == 0u)
        return float3(0.0f, 0.0f, 0.0f);
    if (LightCount <= 4u)
        return float3(0.0f, 0.0f, 1.0f);
    if (LightCount <= 16u)
        return float3(0.0f, 1.0f, 0.0f);
    if (LightCount <= 32u)
        return float3(1.0f, 1.0f, 0.0f);
    if (LightCount <= 64u)
        return float3(1.0f, 0.5f, 0.0f);
    return float3(1.0f, 0.0f, 0.0f);
}

// 일단 GOURAUD 전용
void ComputeLighting(float3 N, float3 WorldPosition, float3 V, float n, out float3 DiffuseLight, out float3 SpecularLight)
{
    DiffuseLight = 0.0f;
    SpecularLight = 0.0f;

    // Directional Light
    {
        float3 Il = DirectionalLight.Color;
        float3 L = -DirectionalLight.Direction;
        float GeoFactor = 1.0f;
        AccumulateLight(Il * GeoFactor, N, L, V, n, DiffuseLight, SpecularLight);
    }

    // Point Light
    {
        for (uint Index = 0; Index < NumPointLights; ++Index)
        {
            FPointLightInfo Light = PointLights[Index];
            float3 ToLight = Light.Position - WorldPosition;
            float Distance = length(ToLight);
            float Radius = Light.AttenuationRadius;
            if (Distance >= Radius)
                continue;

            float3 L = ToLight / Distance;
            float GeoFactor = 1.0f;

            float DistanceAttenuation = ComputeDistanceAttenuation(Distance, Radius);
            float3 Il = Light.Color * DistanceAttenuation * GeoFactor;
            AccumulateLight(Il, N, L, V, n, DiffuseLight, SpecularLight);
        }
    }

    // Spot Light
    {
        for (uint Index = 0; Index < NumSpotLights; ++Index)
        {
            FSpotLightInfo Light = SpotLights[Index];
            float3 ToLight = Light.Position - WorldPosition;
            float Distance = length(ToLight);
            float Radius = Light.AttenuationRadius;
            if (Distance >= Radius)
                continue;
            float3 L = ToLight / Distance;
            float GeoFactor = 1.0f;

            float DistanceAttenuation = ComputeDistanceAttenuation(Distance, Radius);
            float CosAngle = dot(-L, Light.Direction);
            float SpotAttenuation = Light.CosInnerCone == Light.CosOuterCone
            ? (CosAngle > Light.CosOuterCone ? 1.0f : 0.0f)
            : saturate((CosAngle - Light.CosOuterCone) / (Light.CosInnerCone - Light.CosOuterCone));
            
            float3 Il = Light.Color * DistanceAttenuation * SpotAttenuation * GeoFactor;
            AccumulateLight(Il, N, L, V, n, DiffuseLight, SpecularLight);
        }
    }
}


void ComputeTiledLighting(float3 N,
#if USE_NORMAL_MAP
    float3 GeoNormal,
#endif
float3 WorldPosition, float3 V, float n, float2 PixelPosition, out float3 DiffuseLight, out float3 SpecularLight)
{
    DiffuseLight = 0.0f;
    SpecularLight = 0.0f;

    // 화면 전체에 영향을 주는 Directional Light는 타일 목록에 넣지 않음
    {
        float3 Il = DirectionalLight.Color;
        float3 L = -DirectionalLight.Direction;
#if USE_NORMAL_MAP
        float GeoFactor = saturate(dot(GeoNormal, L) * 4.0f);
#else
        float GeoFactor = 1.0f;
#endif
        AccumulateLight(Il * GeoFactor, N, L, V, n, DiffuseLight, SpecularLight);
    }

    uint2 TileCoord = min(uint2(PixelPosition) / 16u, uint2(TileCountX - 1u, TileCountY - 1u));
    uint TileIndex = TileCoord.y * TileCountX + TileCoord.x;

    // Point Light
    uint PointListStart = TileIndex * PointTileStride;
    uint PointCount = min(PointTileList[PointListStart], 64u);
    for (uint ListIndex = 0; ListIndex < PointCount; ++ListIndex)
    {
        uint LightIndex = PointTileList[PointListStart + 1u + ListIndex];
        if (LightIndex >= StructuredPointLightCount)
            continue;

        FPointLightInfo Light = StructuredPointLights[LightIndex];
        float3 ToLight = Light.Position - WorldPosition;
        float Distance = length(ToLight);
        float Radius = Light.AttenuationRadius;
        if (Distance <= 0.0f || Distance >= Radius)
            continue;

        float3 L = ToLight / Distance;
#if USE_NORMAL_MAP
        float GeoFactor = saturate(dot(GeoNormal, L) * 4.0f);
        if (GeoFactor <= 0.0f)
            continue;
#else
        float GeoFactor = 1.0f;
#endif

        float DistanceAttenuation = ComputeDistanceAttenuation(Distance, Radius);
        float3 Il = Light.Color * DistanceAttenuation * GeoFactor;
        AccumulateLight(Il, N, L, V, n, DiffuseLight, SpecularLight);
    }

    // Spot Light
    uint SpotListStart = TileIndex * SpotTileStride;
    uint SpotCount = min(SpotTileList[SpotListStart], 64u);
    for (uint SpotListIndex = 0; SpotListIndex < SpotCount; ++SpotListIndex)
    {
        uint LightIndex = SpotTileList[SpotListStart + 1u + SpotListIndex];
        if (LightIndex >= StructuredSpotLightCount)
            continue;

        FSpotLightInfo Light = StructuredSpotLights[LightIndex];
        float3 ToLight = Light.Position - WorldPosition;
        float Distance = length(ToLight);
        float Radius = Light.AttenuationRadius;
        if (Distance <= 0.0f || Distance >= Radius)
            continue;

        float3 L = ToLight / Distance;
#if USE_NORMAL_MAP
        float GeoFactor = saturate(dot(GeoNormal, L) * 4.0f);
        if (GeoFactor <= 0.0f)
            continue;
#else
        float GeoFactor = 1.0f;
#endif

        float DistanceAttenuation = ComputeDistanceAttenuation(Distance, Radius);
        float CosAngle = dot(-L, normalize(Light.Direction));
        float SpotAttenuation = Light.CosInnerCone == Light.CosOuterCone
            ? (CosAngle > Light.CosOuterCone ? 1.0f : 0.0f)
            : saturate((CosAngle - Light.CosOuterCone) / (Light.CosInnerCone - Light.CosOuterCone));

        float3 Il = Light.Color * DistanceAttenuation * SpotAttenuation * GeoFactor;
        AccumulateLight(Il, N, L, V, n, DiffuseLight, SpecularLight);
    }
}

FPSInput mainVS(FVSInput Input)
{
    FPSInput Output;

    float4 WorldPosition = mul(float4(Input.Position, 1.0f), World);

    Output.Position = mul(WorldPosition, ViewProjection);
    Output.WorldPosition = WorldPosition.xyz;
    Output.Normal = mul(Input.Normal, (float3x3) NormalMatrix);
#if USE_NORMAL_MAP
    Output.Tangent = mul(Input.Tangent.xyz, (float3x3) World);
    Output.BitangentSign = Input.Tangent.w;
#endif
    Output.UV = Input.UV;
#if GOURAUD
    float3 N = normalize(Output.Normal);
    float3 V = normalize(CameraPosition - Output.WorldPosition);
    // TODO
    ComputeLighting(N, Output.WorldPosition, V, Shininess, Output.DiffuseLight, Output.SpecularLight);
    
#endif
    return Output;
}

float4 mainPS(FPSInput Input) : SV_TARGET
{
    float4 FinalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

    float4 TextureColor = AlbedoTexture.Sample(AlbedoSampler, Input.UV + UVScrollOffset);
    float4 SurfaceColor = TextureColor * BaseColor;

    float3 Albedo = SurfaceColor.rgb;
    float Alpha = bOpaque ? 1.0f : SurfaceColor.a;

    float3 Kd = Albedo;
    float3 Ka = Kd; // 나중에 Ka 를 개별로 설정할 수 있음
    float3 Ks = SpecularColor;
    
    float3 Normal = normalize(Input.Normal);
    
#if USE_NORMAL_MAP
    // 그람 슈미트 직교화
    float3 Tangent = normalize(Input.Tangent - dot(Input.Tangent, Normal) * Normal);
    float3 BiTangent = cross(Normal, Tangent) * Input.BitangentSign;
    float3x3 TBN = float3x3(Tangent, BiTangent, Normal);
    // Texture Normal 매핑
    float3 NormalSample = NormalTexture.Sample(AlbedoSampler, Input.UV + UVScrollOffset).xyz;
    float3 TextureNormal = normalize(NormalSample * 2.0f - 1.0f);
    TextureNormal.y = bFlipNormalGreen ? -TextureNormal.y : TextureNormal.y;
    // TBN 행렬을 이용하여 월드 공간으로 변환
    float3 WorldNormal = mul(TextureNormal, TBN);
#else
    float3 WorldNormal = Normal;
#endif
   
#if VIEWMODE == VIEWMODE_UNLIT
    FinalColor = float4(Albedo, Alpha);
#elif VIEWMODE == VIEWMODE_WORLDNORMAL
    // 색이 아니라 데이터이므로 Gamma Encode Pass에서 인코딩하지 않는다.
    FinalColor = float4(normalize(WorldNormal) * 0.5f + 0.5f, 1.0f);
#else // Lit
    if (bVisualizeLightCount != 0u)
    {
        uint2 PixelCoord = uint2(Input.Position.xy);
        uint2 TileCoord = min(PixelCoord / 16u, uint2(TileCountX - 1u, TileCountY - 1u));
        uint TileIndex = TileCoord.y * TileCountX + TileCoord.x;
        
        uint PointListStart = TileIndex * PointTileStride;
        uint SpotListStart = TileIndex * SpotTileStride;
        
        uint PointCount = min(PointTileList[PointListStart], 64u);
        uint SpotCount = min(SpotTileList[SpotListStart], 64u);
        
        float3 Color = GetLightCountColor(PointCount + SpotCount);
        
        uint2 LocalCoord = PixelCoord % 16u;

        if (LocalCoord.x == 0u || LocalCoord.y == 0u)
        {
            Color *= 0.2f;
        }

        return float4(Color, 1.0f);
    }
    
    float3 DiffuseLight;
    float3 SpecularLight;
#if SHADING_MODEL == SHADING_MODEL_GOURAUD
    DiffuseLight = Input.DiffuseLight;
    SpecularLight = Input.SpecularLight;
#else // Phong
    float3 N = normalize(WorldNormal);
    float3 V = normalize(CameraPosition - Input.WorldPosition);
    
#if USE_NORMAL_MAP
    ComputeTiledLighting(N, Normal, Input.WorldPosition, V, Shininess, Input.Position.xy, DiffuseLight, SpecularLight);
#else
    ComputeTiledLighting(N, Input.WorldPosition, V, Shininess, Input.Position.xy, DiffuseLight, SpecularLight);
#endif
    
#endif
    FinalColor = float4(Ka * AmbientLight.Color + Kd * DiffuseLight + Ks * SpecularLight, Alpha);
#endif
    return FinalColor;
}
