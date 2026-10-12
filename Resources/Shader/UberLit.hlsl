#pragma pack_matrix(row_major)

// 기본값: Lit, Blinn-Phong, Phong
#ifndef UNLIT
#define UNLIT 0             // 0 Lit, 1 Unlit
#endif

#ifndef LIGHTING_MODEL
#define LIGHTING_MODEL 2    // 0 Lambert, 1 Phong, 2 Blinn-Phong
#endif

#ifndef SHADING_MODEL
#define SHADING_MODEL 1     // 0 Gouraud, 1 Phong
#endif

#define GOURAUD (!UNLIT && SHADING_MODEL == 0)

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
    float3 Tangent : TANGENT;
    float4 Color : COLOR; // 지금은 쓰지 않지만 나중에 쓰게 될 수 있음
    float2 UV : TEXCOORD;
};

struct FPSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV : TEXCOORD0;
    float3 WorldPosition : TEXCOORD1;
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
    float MaterialPadding;
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

Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);

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
    DiffuseLight += DiffuseTerm(Il, N, L); // Lambert

    if (dot(N, L) > 0.0f)
    {
#if LIGHTING_MODEL == 1 // Phong
        float3 R = 2.0f * dot(N, L) * N - L;
        SpecularLight += SpecularTerm(Il, R, V, n);
#elif LIGHTING_MODEL == 2 // Blinn-Phong
        float3 H = normalize(L + V);
        SpecularLight += SpecularTerm(Il, N, H, n);
#endif
    }
}

void ComputeLighting(float3 N, float3 WorldPosition, float3 V, float n, out float3 DiffuseLight, out float3 SpecularLight)
{
    DiffuseLight = 0.0f;
    SpecularLight = 0.0f;

    // Directional
    {
        float3 Il = DirectionalLight.Color;
        float3 L = -DirectionalLight.Direction;
        AccumulateLight(Il, N, L, V, n, DiffuseLight, SpecularLight);
    }

    // Point
    {
        for (uint Index = 0; Index < NumPointLights; ++Index)
        {
            FPointLightInfo Light = PointLights[Index];

            float3 ToLight = Light.Position - WorldPosition;
            float Distance = length(ToLight);
            float Radius = Light.AttenuationRadius;

            if (Distance >= Radius)
            {
                continue;
            }

            float3 L = ToLight / Distance;
            float DistanceAttenuation = ComputeDistanceAttenuation(Distance, Radius);
            float3 Il = Light.Color * DistanceAttenuation;

            AccumulateLight(Il, N, L, V, n, DiffuseLight, SpecularLight);
        }
    }

    // Spot
    {
        for (uint Index = 0; Index < NumSpotLights; ++Index)
        {
            FSpotLightInfo Light = SpotLights[Index];

            float3 ToLight = Light.Position - WorldPosition;
            float Distance = length(ToLight);
            float Radius = Light.AttenuationRadius;

            if (Distance >= Radius)
            {
                continue;
            }

            float3 L = ToLight / Distance;
            float DistanceAttenuation = ComputeDistanceAttenuation(Distance, Radius);

            float CosAngle = dot(-L, Light.Direction);
            float SpotAttenuation = Light.CosInnerCone == Light.CosOuterCone
            ? (CosAngle > Light.CosOuterCone ? 1.0f : 0.0f)
            : saturate((CosAngle - Light.CosOuterCone) / (Light.CosInnerCone - Light.CosOuterCone));

            float3 Il = Light.Color * DistanceAttenuation * SpotAttenuation;

            AccumulateLight(Il, N, L, V, n, DiffuseLight, SpecularLight);
        }
    }
}

FPSInput mainVS(FVSInput Input)
{
    FPSInput Output;

    float4 WorldPosition = mul(float4(Input.Position, 1.0f), World);

    Output.Position = mul(WorldPosition, ViewProjection);
    Output.WorldPosition = WorldPosition.xyz;
    Output.Normal = mul(Input.Normal, (float3x3) NormalMatrix);
    Output.Tangent = mul(Input.Tangent, (float3x3) NormalMatrix);
    Output.UV = Input.UV;

#if GOURAUD
    float3 N = normalize(Output.Normal);
    float3 V = normalize(CameraPosition - Output.WorldPosition);
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
    // 그람 슈미트 직교화
    float3 Tangent = normalize(Input.Tangent - dot(Input.Tangent, Normal) * Normal);
    float3 BiTangent = cross(Normal, Tangent);
    float3x3 TBN = float3x3(Tangent, BiTangent, Normal);
    // Texture Normal 매핑
    float3 TextureNormal;
    if (NormalTexture)
    {
        float3 NormalSample = NormalTexture.Sample(AlbedoSampler, Input.UV + UVScrollOffset).xyz;
        TextureNormal = normalize(NormalSample * 2.0f - 1.0f);
    }
    else
    {
        TextureNormal = float3(0.0f, 0.0f, 1.0f);
    }
    // TBN 행렬을 이용하여 월드 공간으로 변환
    float3 WorldNormal = mul(TextureNormal, TBN);

#if UNLIT               // Unlit
    FinalColor = float4(Albedo, Alpha);
#else                   // Lit
    float3 DiffuseLight;
    float3 SpecularLight;
#if SHADING_MODEL == 0  // Gouraud
    DiffuseLight = Input.DiffuseLight;
    SpecularLight = Input.SpecularLight;
#else                   // Phong
    float3 N = normalize(WorldNormal);
    float3 V = normalize(CameraPosition - Input.WorldPosition);
    ComputeLighting(N, Input.WorldPosition, V, Shininess, DiffuseLight, SpecularLight);
#endif
    FinalColor = float4(Ka * AmbientLight.Color + Kd * DiffuseLight + Ks * SpecularLight, Alpha);
#endif
    return FinalColor;
}
