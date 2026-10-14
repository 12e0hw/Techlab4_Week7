#pragma pack_matrix(row_major)
// 등장방형(equirectangular) 파노라마를 배경으로 그린다.
// 정점 버퍼 없이 화면을 덮는 삼각형 하나를 SV_VertexID로 만든다.

cbuffer SkyboxConstants : register(b0)
{
    matrix InverseViewProjection;
    float3 CameraPosition;
    float SunRadius;
    float3 SunDirection;
    float SunIntensity;
    float3 SunColor;
    float Padding;
};

Texture2D    PanoramaTexture : register(t0);
SamplerState LinearSampler   : register(s0);

static const float PI = 3.14159265f;

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
};

PSInput mainVS(uint VertexID : SV_VertexID)
{
    PSInput Output;

    // VertexID 0,1,2 -> UV (0,0) (2,0) (0,2)
    // 화면보다 큰 삼각형 하나로 전체를 덮는다. 사각형 두 장보다 대각선 이음매가 없다.
    Output.UV = float2((VertexID << 1) & 2, VertexID & 2);

    // UV [0,2] -> NDC [-1,3]. y는 위아래가 뒤집혀 있다.
    Output.Position = float4(Output.UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);

    return Output;
}

float4 mainPS(PSInput Input) : SV_TARGET
{
    float2 NDC = Input.UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float4 FarPoint = mul(float4(NDC, 1.0f, 1.0f), InverseViewProjection);
    FarPoint /= FarPoint.w;
    float3 Direction = normalize(FarPoint.xyz - CameraPosition);

    float2 PanoUV;
    PanoUV.x = atan2(Direction.y, Direction.x) / (2.0f * PI) + 0.5f;
    PanoUV.y = acos(clamp(Direction.z, -1.0f, 1.0f)) / PI;
    float3 SkyBase = PanoramaTexture.SampleLevel(LinearSampler, PanoUV, 0).rgb;

    // 태양 고도 계수 계산
    float SunElevation = SunDirection.z;

    float DayFactor = saturate(SunElevation * 3.5f);
    float SunsetFactor = saturate(1.0f - abs(SunElevation - 0.06f) * 6.0f) * (1.0f - DayFactor * 0.5f);

    const float3 DayTint = float3(1.0f, 1.0f, 1.0f);
    const float3 SunsetTint = float3(4.0f, 0.55f, 0.22f);
    const float3 NightTint = float3(0.015f, 0.025f, 0.06f);

    // 낮과 밤을 먼저 보간한 뒤, 노을 구간에서 SunsetTint로 전환
    float3 CurrentTint = lerp(NightTint, DayTint, DayFactor);
    CurrentTint = lerp(CurrentTint, SunsetTint, SunsetFactor);

    float3 SkyColor = SkyBase * CurrentTint;
    

    // 노을
    float HorizonMask = pow(1.0f - abs(Direction.z), 4.0f);
    float3 SunsetGlow = float3(1.0f, 0.35f, 0.08f) * (SunsetFactor * HorizonMask * 1.2f);
    SkyColor += SunsetGlow;

    // 태양
    float SunDot = dot(Direction, SunDirection);
    float CosSunRadius = cos(SunRadius);
    float SunDisc = smoothstep(CosSunRadius - 0.0005f, CosSunRadius, SunDot);
    float SunGlow = pow(saturate(SunDot), 48.0f) * 0.6f;

    float3 DynamicSunColor = lerp(float3(1.0f, 0.35f, 0.1f), SunColor, DayFactor);
    float SunVisibility = saturate((SunElevation + 0.05f) * 10.0f);

    float3 SunLight = (SunDisc * SunIntensity + SunGlow) * DynamicSunColor * SunVisibility;

    return float4(SkyColor + SunLight, 1.0f);
}