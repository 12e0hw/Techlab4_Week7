#pragma pack_matrix(row_major)

cbuffer Viewconstants : register(b0)
{
    matrix VP;
    float4 CameraPosition; // xyz: 카메라 월드 위치, w: 패딩
};

cbuffer Worldconstants : register(b2)
{
    matrix World;
};

cbuffer MaterialParams : register(b1)
{
    float4 BaseColor;
    float3 SpecularColor;
    float Shininess;
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
    float3 p : POSITION;
    float3 n : NORMAL;
    float4 c : COLOR;
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

float3 CalcAmbientLight(AmbientLightData ambient, uint count)
{
    if (count > 0)
    {
        return ambient.ColorIntensity.rgb * ambient.ColorIntensity.w;
    }
    return float3(0.1f, 0.1f, 0.1f);
}

float3 CalcDirectionalLight(DirectionalLightData dirLight, float3 N, uint count, float3 V)
{
    float specPower = max(Shininess, 1.0f);

    if (count > 0 && dirLight.Direction.w > 0.5f)
    {
        float3 L = -normalize(dirLight.Direction.xyz);
        float NdotL = saturate(dot(N, L));
        float3 diffuse = dirLight.ColorIntensity.rgb * (dirLight.ColorIntensity.w * NdotL);

#if defined(SHADING_MODEL_BLINN_PHONG)
        if (NdotL > 0.0f)
        {
            float3 H = normalize(L + V);
            float NdotH = saturate(dot(N, H));
            float spec = pow(NdotH, specPower);
            diffuse += SpecularColor * (dirLight.ColorIntensity.rgb * (dirLight.ColorIntensity.w * spec));
        }
#endif
        return diffuse;
    }

    // Directional Light가 없을 때의 디폴트 키 라이트
    float3 defaultDir = normalize(float3(0.5f, 0.5f, -1.0f));
    float defaultNdotL = saturate(dot(N, -defaultDir));
    float3 defaultDiff = float3(0.5f, 0.5f, 0.5f) * defaultNdotL;
#if defined(SHADING_MODEL_BLINN_PHONG)
    if (defaultNdotL > 0.0f)
    {
        float3 H = normalize(-defaultDir + V);
        float NdotH = saturate(dot(N, H));
        defaultDiff += SpecularColor * (float3(0.5f, 0.5f, 0.5f) * pow(NdotH, specPower));
    }
#endif
    return defaultDiff;
}

float3 CalcPointLight(PointLightData pointLight, float3 worldPos, float3 N, float3 V)
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
    float3 diffuse = pointLight.ColorIntensity.rgb * pointNdotL;

#if defined(SHADING_MODEL_BLINN_PHONG)
    if (pointNdotL > 0.0f)
    {
        float3 H = normalize(lightDir + V);
        float NdotH = saturate(dot(N, H));
        float specPower = max(Shininess, 1.0f);
        diffuse += SpecularColor * (pointLight.ColorIntensity.rgb * pow(NdotH, specPower));
    }
#endif

    return diffuse * (falloff * intensity);
}

float3 CalcSpotLight(SpotLightData spotLight, float3 worldPos, float3 N, float3 V)
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
    
    float3 diffuse = spotLight.ColorIntensity.rgb * spotNdotL;

#if defined(SHADING_MODEL_BLINN_PHONG)
    if (spotNdotL > 0.0f)
    {
        float3 H = normalize(lightDir + V);
        float NdotH = saturate(dot(N, H));
        float specPower = max(Shininess, 1.0f);
        diffuse += SpecularColor * (spotLight.ColorIntensity.rgb * pow(NdotH, specPower));
    }
#endif

    return diffuse * (falloff * spotCone * intensity);
}

// 최종 표면 법선 N 결정부 (Normal Mapping 대응 지점)
float3 GetWorldNormal(PS_INPUT input)
{
    // 추후 노멀 매핑 적용 시:
    // 1. 노멀 맵 텍스처에서 탄젠트 공간 노멀 샘플링 (sample * 2 - 1)
    // 2. TBN 행렬 구성 (input.tangent, input.bitangent, input.normal)
    // 3. 월드 노멀 변환: normalize(mul(sample, TBN))
    return normalize(input.normal);
}

//-----------------------------
PS_INPUT mainVS(VS_INPUT input)
//-----------------------------
{
    PS_INPUT output;

    output.worldPos = mul(float4(input.p, 1.0f), World).xyz;
    output.position = mul(mul(float4(input.p, 1.0f), World), VP);
    output.normal = mul(float4(input.n, 0.0f), World).xyz;
    output.uv = input.t;

#if defined(SHADING_MODEL_GOURAUD)
    float3 N = normalize(output.normal);
    float3 ambient = CalcAmbientLight(AmbientLight, AmbientLightCount);
    float3 dir = CalcDirectionalLight(DirectionalLight, N, DirectionalLightCount, float3(0, 0, 0));
    float3 totalPt = float3(0, 0, 0);
    for (uint i = 0; i < PointLightCount; ++i)
    {
        totalPt += CalcPointLight(PointLights[i], output.worldPos, N, float3(0, 0, 0));
    }
    float3 totalSpot = float3(0, 0, 0);
    for (uint j = 0; j < SpotLightCount; ++j)
    {
        totalSpot += CalcSpotLight(SpotLights[j], output.worldPos, N, float3(0, 0, 0));
    }
    output.color = float4(ambient + dir + totalPt + totalSpot, 1.0f);
#else
    output.color = input.c;
#endif

    return output;
}

//---------------------------------------
float4 mainPS(PS_INPUT input) : SV_TARGET
//---------------------------------------
{
    float4 texColor = g_txColor.Sample(g_Sample, input.uv + UVOffset);
    float4 albedo = texColor * BaseColor;

#if defined(SHADING_MODEL_GOURAUD)
    // Gouraud Shading: 보간된 정점 컬러를 알베도와 곱함
    float3 finalRGB = albedo.rgb * input.color.rgb;
#else
    float3 N = GetWorldNormal(input);

    float3 V = float3(0, 0, 0);
#if defined(SHADING_MODEL_BLINN_PHONG)
    V = normalize(CameraPosition.xyz - input.worldPos);
#endif

    float3 ambientLight = CalcAmbientLight(AmbientLight, AmbientLightCount);
    float3 dirLight = CalcDirectionalLight(DirectionalLight, N, DirectionalLightCount, V);

    float3 totalPointLight = float3(0, 0, 0);
    for (uint i = 0; i < PointLightCount; ++i)
    {
        totalPointLight += CalcPointLight(PointLights[i], input.worldPos, N, V);
    }

    float3 totalSpotLight = float3(0, 0, 0);
    for (uint j = 0; j < SpotLightCount; ++j)
    {
        totalSpotLight += CalcSpotLight(SpotLights[j], input.worldPos, N, V);
    }

    float3 lighting = ambientLight + dirLight + totalPointLight + totalSpotLight;
    float3 finalRGB = albedo.rgb * lighting;
#endif

    float alpha = bOpaque > 0.5f ? 1.0f : albedo.a;
    return float4(finalRGB, alpha);
}
