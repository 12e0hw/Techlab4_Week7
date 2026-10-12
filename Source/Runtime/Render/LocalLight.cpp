#include "EnginePCH.h"
#include "LocalLight.h"
#include "RenderCommand.h"

void FLocalLight::UploadLights(const TArray<FPointLightInfo>& PointLights, const TArray<FSpotLightInfo>& SpotLights)
{
    NumPointLights = PointLights.Num();
    NumSpotLights = SpotLights.Num();

    EnsurePointLightCapacity(NumPointLights);
    EnsureSpotLightCapacity(NumSpotLights);

    ID3D11DeviceContext* Context = RenderCommand::GetContext();
    D3D11_MAPPED_SUBRESOURCE Mapped{};

    if (NumPointLights > 0 && SUCCEEDED(Context->Map(PointLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
    {
        std::memcpy(Mapped.pData, PointLights.GetData(), NumPointLights * sizeof(FPointLightInfo));
        Context->Unmap(PointLightBuffer.Get(), 0);
    }

    if (NumSpotLights > 0 && SUCCEEDED(Context->Map(SpotLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
    {
        std::memcpy(Mapped.pData, SpotLights.GetData(), NumSpotLights * sizeof(FSpotLightInfo));
        Context->Unmap(SpotLightBuffer.Get(), 0);
    }

}

bool FLocalLight::EnsurePointLightCapacity(uint32 RequiredCount)
{
    RequiredCount = std::max(RequiredCount, 1u);

    if (PointLightBuffer && PointLightSRV && RequiredCount <= PointLightCapacity)
    {
        return true;
    }

    uint32 NewCapacity = std::max(PointLightCapacity * 2, 64u);
    while (NewCapacity < RequiredCount)
    {
        NewCapacity *= 2;
    }

    PointLightBuffer.Reset();
    PointLightSRV.Reset();
    PointLightCapacity = 0;

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = NewCapacity * sizeof(FPointLightInfo);
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FPointLightInfo);

    ID3D11Device* Device = RenderCommand::GetDevice();

    if (FAILED(Device->CreateBuffer(&BufferDesc, nullptr, PointLightBuffer.GetAddressOf())))
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.FirstElement = 0;
    SRVDesc.Buffer.NumElements = NewCapacity;

    if (FAILED(Device->CreateShaderResourceView(PointLightBuffer.Get(), &SRVDesc, PointLightSRV.GetAddressOf())))
    {
        PointLightBuffer.Reset();
        return false;
    }

    PointLightCapacity = NewCapacity;
    return true;
}

bool FLocalLight::EnsureSpotLightCapacity(uint32 RequiredCount)
{
    RequiredCount = std::max(RequiredCount, 1u);

    if (SpotLightBuffer && SpotLightSRV && RequiredCount <= SpotLightCapacity)
    {
        return true;
    }

    uint32 NewCapacity = std::max(SpotLightCapacity * 2, 64u);
    while (NewCapacity < RequiredCount)
    {
        NewCapacity *= 2;
    }

    SpotLightBuffer.Reset();
    SpotLightSRV.Reset();
    SpotLightCapacity = 0;

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = NewCapacity * sizeof(FSpotLightInfo);
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FSpotLightInfo);

    ID3D11Device* Device = RenderCommand::GetDevice();

    if (FAILED(Device->CreateBuffer(&BufferDesc, nullptr, SpotLightBuffer.GetAddressOf())))
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.FirstElement = 0;
    SRVDesc.Buffer.NumElements = NewCapacity;

    if (FAILED(Device->CreateShaderResourceView(SpotLightBuffer.Get(), &SRVDesc, SpotLightSRV.GetAddressOf())))
    {
        SpotLightBuffer.Reset();
        return false;
    }

    SpotLightCapacity = NewCapacity;
    return true;
}