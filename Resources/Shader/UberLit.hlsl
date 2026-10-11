#pragma pack_matrix(row_major)

// 기본값
#ifndef UNLIT
#define UNLIT 0
#endif
#ifndef LIGHTING_MODEL
#define LIGHTING_MODEL 0    // 0 Lambert, 1 Phong, 2 Blinn-Phong
#endif
#ifndef SHADING_MODEL
#define SHADING_MODEL 1     // 0 Gouraud, 1 Phong
#endif

// FViewConstants
cbuffer Viewconstants : register(b0)
{
    matrix VP;
    float3 CameraPosition;
    float ViewPadding;
};

cbuffer MaterialParams : register(b1)
{
    float4 BaseColor;
    float2 UVOffset;
    float bOpaque;
    float Shininess;
    float3 SpecularColor;
    float MaterialPadding;
};

// FPerObjectConstants
cbuffer Worldconstants : register(b2)
{
    matrix World;
    matrix NormalMatrix; // World의 역전치
};

// FLightingConstants (LightInfo.h)
#define NUM_POINT_LIGHT 4
#define NUM_SPOT_LIGHT 4

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

cbuffer Lighting : register(b3)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    FPointLightInfo PointLights[NUM_POINT_LIGHT];
    FSpotLightInfo SpotLights[NUM_SPOT_LIGHT];
    uint NumPointLights;
    uint NumSpotLights;
    float2 LightingPadding;
};

struct VS_INPUT
{
    float3 p : POSITION; // Input position from vertex buffer
    float3 n : NORMAL;
    float4 c : COLOR; // Input color from vertex buffer
    float2 t : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

Texture2D g_txColor : register(t0);
SamplerState g_Sample : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    output.worldPos = mul(float4(input.p, 1.0f), World).xyz;
    output.position = mul(mul(float4(input.p, 1.0f), World), VP);
    output.normal = mul(float4(input.n, 0.0f), NormalMatrix).xyz;
    output.color = input.c;
    output.uv = input.t;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 texColor = g_txColor.Sample(g_Sample, input.uv + UVOffset);
    float4 albedo = texColor * BaseColor;
    
    float alpha = bOpaque > 0.5f ? 1.0f : albedo.a;
    
#if UNLIT
    return float4(albedo.rgb, alpha);
#else
    // [임시] Phase 5 확인용. Phase 6에서 조명 계산으로 바꾼다.
    return float4(Ambient.Color, alpha);
#endif
}
