#pragma pack_matrix(row_major)
cbuffer constants : register(b0)
{
    matrix VP;
};

cbuffer Worldconstants : register(b2)
{
    matrix World;
};

cbuffer BillboardParams : register(b1)
{
    float4 TintColor;
};

Texture2D BillboardTexture : register(t0);
SamplerState BillboardSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.position = mul(mul(float4(input.position, 1.0f), World), VP);
    output.uv = input.uv;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 color = BillboardTexture.Sample(BillboardSampler, input.uv);
    clip(color.a - 0.01f);
    color *= TintColor;
    return color;
}
