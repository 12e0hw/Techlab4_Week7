#pragma pack_matrix(row_major)
cbuffer Viewconstants : register(b0)
{
    matrix VP;
};

cbuffer Worldconstants : register(b2)
{
    matrix World;
};

cbuffer MaterialParams : register(b1)
{
    float4 BaseColor;
    float2 UVOffset;
    float bOpaque;
    float Padding;
};

struct DirectionalLightData
{
    float4 Direction;      // xyz: 빛이 향하는 방향, w: 활성화 여부
    float4 ColorIntensity; // rgb: 색상, w: 강도
};

struct AmbientLightData
{
    float4 ColorIntensity; // rgb: 색상, w: 강도
};

struct PointLightData
{
    float4 PositionRadius; // xyz: 월드 위치, w: 감쇄 반경
    float4 ColorIntensity; // rgb: 색상, w: 강도
    float4 FalloffEnabled; // x: Falloff, y: 활성화 여부, zw: 패딩
};

struct SpotLightData
{
    float4 PositionRadius;    // xyz: 월드 위치, w: 감쇄 반경
    float4 DirectionEnabled;  // xyz: 빛이 향하는 방향, w: 활성화 여부
    float4 ColorIntensity;    // rgb: 색상, w: 강도
    float4 ConeAnglesFalloff; // x: cos(Inner), y: cos(Outer), z: Falloff, w: 패딩
};

cbuffer LightConstants : register(b3)
{
    DirectionalLightData DirectionalLight;
    AmbientLightData AmbientLight;
    PointLightData PointLights[16];
    SpotLightData SpotLights[16];
    uint PointLightCount;
    uint SpotLightCount;
    uint DirectionalLightCount;
    uint AmbientLightCount;
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
    
    // w=0으로 이동 성분을 빼고 월드 공간으로 보낸다. 비균등 스케일이면 역전치가 필요하다.
    output.normal = mul(float4(input.n, 0.0f), World).xyz;
    output.color = input.c;
    output.uv = input.t;
    return output;
}

// =========================================================================
// [추상화 1] 최종 표면 법선 N 결정부 (Normal Mapping 대응 지점)
// =========================================================================
float3 GetWorldNormal(PS_INPUT input)
{
    // [추후 노멀 매핑 적용 시]:
    // 1. 노멀 맵 텍스처에서 탄젠트 공간 노멀 샘플링 (sample * 2 - 1)
    // 2. TBN 행렬 구성 (input.tangent, input.bitangent, input.normal)
    // 3. 월드 노멀 변환: normalize(mul(sample, TBN))
    return normalize(input.normal);
}

// =========================================================================
// [추상화 2] 개별 라이트 계산 함수 (Tile-based Light Culling 대비 지점)
// =========================================================================
float3 CalcAmbientLight(AmbientLightData ambient, uint count)
{
    if (count > 0)
    {
        return ambient.ColorIntensity.rgb * ambient.ColorIntensity.w;
    }
    return float3(0.1f, 0.1f, 0.1f);
}

float3 CalcDirectionalLight(DirectionalLightData dirLight, float3 N, uint count)
{
    if (count > 0 && dirLight.Direction.w > 0.5f)
    {
        float3 L = -normalize(dirLight.Direction.xyz);
        float NdotL = saturate(dot(N, L));
        return dirLight.ColorIntensity.rgb * (dirLight.ColorIntensity.w * NdotL);
    }

    // Directional Light가 씬에 없을 때의 기본 디폴트 키 라이트
    float3 defaultDir = normalize(float3(0.5f, 0.5f, -1.0f));
    float defaultNdotL = saturate(dot(N, -defaultDir));
    return float3(0.5f, 0.5f, 0.5f) * defaultNdotL;
}

float3 CalcPointLight(PointLightData pointLight, float3 worldPos, float3 N)
{
    if (pointLight.FalloffEnabled.y < 0.5f)
        return float3(0, 0, 0);

    float distance = length(pointLight.PositionRadius.xyz - worldPos);
    float radius = max(pointLight.PositionRadius.w, 0.001f);
    
    float baseFalloff = saturate(1.0f - distance / radius);
    float falloff = pow(baseFalloff, max(pointLight.FalloffEnabled.x, 0.001f));
    
    float intensity = pointLight.ColorIntensity.w;
    float3 lightDir = normalize(pointLight.PositionRadius.xyz - worldPos);
    
    float pointNdotL = saturate(dot(N, lightDir));
    return pointLight.ColorIntensity.rgb * (pointNdotL * falloff * intensity);
}

float3 CalcSpotLight(SpotLightData spotLight, float3 worldPos, float3 N)
{
    if (spotLight.DirectionEnabled.w < 0.5f)
        return float3(0, 0, 0);

    float distance = length(spotLight.PositionRadius.xyz - worldPos);
    float radius = max(spotLight.PositionRadius.w, 0.001f);
    
    float baseFalloff = saturate(1.0f - distance / radius);
    float falloff = pow(baseFalloff, max(spotLight.ConeAnglesFalloff.z, 0.001f));
    
    float3 lightToPixel = normalize(worldPos - spotLight.PositionRadius.xyz);
    float3 spotForward = normalize(spotLight.DirectionEnabled.xyz);
    
    float cosAngle = dot(spotForward, lightToPixel);
    float cosInner = spotLight.ConeAnglesFalloff.x;
    float cosOuter = spotLight.ConeAnglesFalloff.y;
    
    float spotCone = saturate((cosAngle - cosOuter) / max(cosInner - cosOuter, 0.0001f));
    
    float intensity = spotLight.ColorIntensity.w;
    float3 lightDir = -lightToPixel;
    float spotNdotL = saturate(dot(N, lightDir));
    
    return spotLight.ColorIntensity.rgb * (spotNdotL * falloff * spotCone * intensity);
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Mesh 표면색
    float4 texColor = g_txColor.Sample(g_Sample, input.uv + UVOffset);
    float4 albedo = texColor * BaseColor;
    
    // 1. 최종 표면 법선 N 결정 (Normal Mapping 추상화 지점)
    float3 N = GetWorldNormal(input);

    // 2. 환경광 및 방향광 계산
    float3 ambientLight = CalcAmbientLight(AmbientLight, AmbientLightCount);
    float3 dirLight = CalcDirectionalLight(DirectionalLight, N, DirectionalLightCount);

    // 3. 점광원 누적 계산 (Tile-based Culling 시 인덱스 버퍼 순회로 루프만 교체)
    float3 totalPointLight = float3(0, 0, 0);
    for (uint i = 0; i < PointLightCount; ++i)
    {
        totalPointLight += CalcPointLight(PointLights[i], input.worldPos, N);
    }

    // 4. 스포트라이트 누적 계산 (Tile-based Culling 시 인덱스 버퍼 순회로 루프만 교체)
    float3 totalSpotLight = float3(0, 0, 0);
    for (uint j = 0; j < SpotLightCount; ++j)
    {
        totalSpotLight += CalcSpotLight(SpotLights[j], input.worldPos, N);
    }

    float3 lighting = ambientLight + dirLight + totalPointLight + totalSpotLight;
    
    // Opaque는 알파를 1로 고정한다. 뷰포트 RT를 ImGui가 알파 블렌딩으로 그리므로 알파가 남으면 비쳐 보인다
    float alpha = bOpaque > 0.5f ? 1.0f : albedo.a;
    
    float3 finalRGB = albedo.rgb * lighting;
    return float4(finalRGB, alpha);
}
