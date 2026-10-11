#pragma pack_matrix(row_major)
struct PointLightData
{
    float3 Color;
    float3 Position;
    float Intensity;
    float Radius;
    float Radiusfalloff;
};

struct SpotLightData
{
    float3 Color;
    float3 Position;
    float3 Direction;
    float Intensity;
    float Radius;
    float Radiusfalloff;
    float InnerAngle;
    float OuterAngle;
};

struct DirectionalLightData
{
    float3 Color;
    float3 Direction;
};

cbuffer Viewconstants : register(b0)
{
    matrix VP;
};

cbuffer Worldconstants : register(b2)
{
    matrix World;
};

cbuffer LightBuffer : register(b3)
{
    PointLightData PointLights[8];
    int PointLightCount;
    SpotLightData SpotLights[8];
    int SpotLightCount;
    DirectionalLightData DirectionalLights[8];
    int DirectionalLightCount;
    float3 AmbientLightColor;
};

// 물체가 바뀔때마다 세팅하는 상수버퍼
cbuffer MaterialParams : register(b1)
{
    float3 Ka; // k_a  
    float3 Kd; // k_d  
    float3 Ks; // k_s 
    float Shininess; // alpha    
    float2 UVOffset;
    float bOpaque;
};

Texture2D DiffuseTexture : register(t0); // 텍스처로 쓸 때의 k_d, 컬러 텍스처
Texture2D NormalTexture : register(t1); // 노멀 텍스처
SamplerState LinearSampler : register(s0);


struct VS_INPUT
{
    float3 position : POSITION; // Input position from vertex buffer
    float3 normal : NORMAL; // Input normal from vertex buffer
    float3 tangent : TANGENT; // Input tangent from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer
    float2 tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
#if defined(USE_GOURAD)
    float4 color : COLOR0;
#else
    float3 worldPos : POSITION0; // 픽셀의 라이트 값 계산용 월드 포지션
    float3 normal : NORMAL0; // TBN행렬에 쓰일 N
    float3 tangent : TANGENT0; // TBN행렬에 쓰일 T
#endif
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    output.worldPos = mul(float4(input.position, 1.0f), World).xyz;
    output.position = mul(mul(float4(input.position, 1.0f), World), VP);
    
    // w=0으로 이동 성분을 빼고 월드 공간으로 보낸다. 비균등 스케일이면 역전치가 필요하다.
    output.normal = mul(float4(input.normal, 0.0f), World).xyz;
#if defined(USE_GOURAD)
    // Gouraud shading을 위해서 vertex shader에서 라이트 계산을 한다.
#endif
    output.uv = input.tex;
    return output;
}